/*
 * B.5 网关主程序（ESP32-S3）
 *
 * 架构（5 任务）：
 *   - Task NRF24_RX (prio 10):  从 NRF24 FIFO 读 → CRC 校验 → 入队
 *   - Task Parser  (prio 8):   阻塞读 queue_frames → 解析字段
 *   - Task MQTT    (prio 5):   阻塞读 queue_data → 构造 JSON → 发布
 *   - Task WiFiMgr (prio 3):   管理 Wi-Fi 连接 + 重连
 *   - Task Heartbeat (prio 1): 每 30s 打印状态
 *
 * 协议：复用 firmware/common/protocol.c
 * 依据：docs/GATEWAY_DESIGN.md
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "mqtt_client.h"
#include "protocol.h"

static const char *TAG = "GATEWAY";

/* === 消息结构（与 STM32 端保持一致） === */
typedef struct {
    uint8_t  frame[32];
    uint8_t  frame_len;
    uint32_t timestamp;
} rx_msg_t;

typedef struct {
    uint8_t  type;
    uint8_t  src_id;
    uint8_t  payload[26];
    uint8_t  payload_len;
    uint32_t timestamp;
} parsed_msg_t;

/* === 全局队列 === */
static QueueHandle_t queue_frames = NULL;
static QueueHandle_t queue_data = NULL;

/* === MQTT 客户端 === */
static esp_mqtt_client_handle_t mqtt_client = NULL;

/* === WiFi 事件组 === */
static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0

/* ============================================================
 * WiFi 事件处理
 * ============================================================ */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                                int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "WiFi disconnected, retry...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

/* ============================================================
 * Task WiFiMgr — Wi-Fi 管理
 * ============================================================ */
static void Task_WiFiMgr(void *pvParameters) {
    /* 等待 WiFi 配置（实际应该用 menuconfig）*/
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "YOUR_WIFI_SSID",          /* TODO: 改你的 WiFi */
            .password = "YOUR_WIFI_PASSWORD",  /* TODO: 改你的密码 */
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                &wifi_event_handler, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFiMgr started, connecting to %s...", wifi_config.sta.ssid);

    /* 等连接成功 */
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT,
                         pdFALSE, pdTRUE, portMAX_DELAY);
    ESP_LOGI(TAG, "WiFi connected!");

    /* 此任务完成，删自己 */
    vTaskDelete(NULL);
}

/* ============================================================
 * MQTT 事件处理
 * ============================================================ */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                                 int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT connected");
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT disconnected");
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT published, msg_id=%d", event->msg_id);
            break;
        default:
            break;
    }
}

/* ============================================================
 * Task MQTT — MQTT 客户端初始化 + 上报
 * ============================================================ */
static void Task_MQTT(void *pvParameters) {
    /* 等 WiFi 连上 */
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT,
                         pdFALSE, pdTRUE, portMAX_DELAY);

    /* 配置 MQTT 客户端（OneNet）*/
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://mqtt.heclouds.com:1883",
        .broker.address.port = 1883,
        .credentials.username = "YOUR_DEVICE_ID",     /* TODO: OneNet device_id */
        .credentials.authentication.password = "YOUR_API_KEY",  /* TODO: api_key 的 MD5 */
        .session.keepalive = 60,
        .session.last_will.topic = "stm32/sys/status",
        .session.last_will.msg = "offline",
        .session.last_will.qos = 1,
        .session.last_will.retain = 1,
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID,
                                     mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
    ESP_LOGI(TAG, "MQTT client started");

    /* 处理解析后的数据 */
    parsed_msg_t msg;
    char json_buf[128];
    char topic_buf[64];

    while (1) {
        if (xQueueReceive(queue_data, &msg, portMAX_DELAY) == pdTRUE) {
            /* 根据节点 ID 选择 MQTT 主题 */
            switch (msg.src_id) {
                case PROTOCOL_ADDR_NODE1:
                    snprintf(topic_buf, sizeof(topic_buf), "stm32/01/dht11");
                    snprintf(json_buf, sizeof(json_buf),
                             "{\"temp\":%d.%d,\"humi\":%d}",
                             msg.payload[0], msg.payload[1], msg.payload[2]);
                    break;
                case PROTOCOL_ADDR_NODE2:
                    snprintf(topic_buf, sizeof(topic_buf), "stm32/02/bno055");
                    snprintf(json_buf, sizeof(json_buf),
                             "{\"yaw\":%.2f,\"roll\":%.2f,\"pitch\":%.2f}",
                             (int16_t)((msg.payload[0] << 8) | msg.payload[1]) / 16.0,
                             (int16_t)((msg.payload[2] << 8) | msg.payload[3]) / 16.0,
                             (int16_t)((msg.payload[4] << 8) | msg.payload[5]) / 16.0);
                    break;
                case PROTOCOL_ADDR_NODE3:
                    snprintf(topic_buf, sizeof(topic_buf), "stm32/03/vet6");
                    snprintf(json_buf, sizeof(json_buf),
                             "{\"temp\":%.2f,\"light\":%u,\"status\":%d}",
                             (int16_t)((msg.payload[0] << 8) | msg.payload[1]) / 100.0,
                             (msg.payload[2] << 8) | msg.payload[3],
                             msg.payload[4]);
                    break;
                default:
                    continue;
            }

            esp_mqtt_client_publish(mqtt_client, topic_buf, json_buf, 0, 1, 0);
            ESP_LOGI(TAG, "→ %s: %s", topic_buf, json_buf);
        }
    }
}

/* ============================================================
 * Task Parser — 协议解析
 * ============================================================ */
static void Task_Parser(void *pvParameters) {
    rx_msg_t rx_msg;
    parsed_msg_t parsed;

    while (1) {
        if (xQueueReceive(queue_frames, &rx_msg, portMAX_DELAY) == pdTRUE) {
            protocol_frame_t frame;
            if (protocol_parse(rx_msg.frame, rx_msg.frame_len, &frame)) {
                parsed.type = frame.type;
                parsed.src_id = frame.dst_id;  /* 实际应该是 src_id */
                parsed.payload_len = frame.payload_len;
                memcpy(parsed.payload, frame.payload, frame.payload_len);
                parsed.timestamp = rx_msg.timestamp;

                xQueueSend(queue_data, &parsed, 0);
                ESP_LOGI(TAG, "Parsed: type=%s, id=0x%02X, len=%d",
                         protocol_type_str(frame.type),
                         frame.dst_id, frame.payload_len);
            } else {
                ESP_LOGE(TAG, "Invalid frame (%d bytes)", rx_msg.frame_len);
            }
        }
    }
}

/* ============================================================
 * Task NRF24_RX — NRF24 接收（占位）
 * ============================================================ */
static void Task_NRF24_RX(void *pvParameters) {
    ESP_LOGW(TAG, "NRF24_RX not implemented yet — using loopback");

    /* TODO: 用 ESP-IDF SPI 驱动实现 NRF24L01+ 接收
     *       复用 firmware/04_nrf24/Src/bsp_nrf24.c 的算法
     *       （firmware/common/protocol.c 也可以 #include）
     */

    /* 暂时：每 5 秒造一个假帧（仿真测试链路）*/
    rx_msg_t fake;
    while (1) {
        /* 模拟节点 2 (BNO055) 的 DATA_REPORT 帧 */
        uint8_t payload[] = {0x01, 0x2E, 0xFF, 0xAD, 0x00, 0xA0};
        fake.frame_len = protocol_build(PROTOCOL_TYPE_DATA_REPORT,
                                          PROTOCOL_ADDR_GATEWAY,
                                          payload, 6,
                                          fake.frame, sizeof(fake.frame));
        fake.timestamp = xTaskGetTickCount();
        xQueueSend(queue_frames, &fake, 0);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

/* ============================================================
 * Task Heartbeat — 心跳
 * ============================================================ */
static void Task_Heartbeat(void *pvParameters) {
    while (1) {
        ESP_LOGI(TAG, "[HB] queue_frames=%u queue_data=%u uptime=%lu ms",
                 (unsigned)uxQueueMessagesWaiting(queue_frames),
                 (unsigned)uxQueueMessagesWaiting(queue_data),
                 xTaskGetTickCount());
        vTaskDelay(pdMS_TO_TICKS(30000));  /* 30s */
    }
}

/* ============================================================
 * app_main
 * ============================================================ */
void app_main(void) {
    ESP_LOGI(TAG, "=== B.5 Gateway (ESP32-S3) starting ===");

    /* 创建队列 */
    queue_frames = xQueueCreate(8, sizeof(rx_msg_t));
    queue_data   = xQueueCreate(4, sizeof(parsed_msg_t));
    if (!queue_frames || !queue_data) {
        ESP_LOGE(TAG, "Queue creation failed!");
        return;
    }

    /* 创建 5 个任务（依据 GATEWAY_DESIGN.md §3.2）*/
    xTaskCreate(Task_NRF24_RX,  "NRF24_RX",  4096, NULL, 10, NULL);
    xTaskCreate(Task_Parser,    "Parser",    2048, NULL,  8, NULL);
    xTaskCreate(Task_MQTT,      "MQTT",      4096, NULL,  5, NULL);
    xTaskCreate(Task_WiFiMgr,   "WiFiMgr",   2048, NULL,  3, NULL);
    xTaskCreate(Task_Heartbeat, "Heartbeat", 1024, NULL,  1, NULL);

    ESP_LOGI(TAG, "All tasks created, scheduler running");
}
