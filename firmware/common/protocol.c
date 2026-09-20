/*
 * protocol.c — STM32 传感网协议实现
 * 帧格式：帧头(2) + LEN(1) + TYPE(1) + DST(1) + PAYLOAD(N) + CRC16(2)
 *        AA 55       LEN     TYPE     DST     ...
 *
 * 注意：本文件用纯 C99 写，不依赖任何硬件/HAL/ESP-IDF
 *      PC 上可直接 gcc 编译运行（单元测试）
 *      STM32/ESP32 工程只需 include protocol.h 即可使用
 */

#include "protocol.h"
#include <stddef.h>     /* NULL */

/* ============================================================
 * CRC16-MODBUS（D4 决策）
 * 多项式 0xA001（反向 0x8005），初值 0xFFFF
 * ============================================================ */
uint16_t crc16_modbus(const uint8_t *data, uint8_t len) {
    uint16_t crc = 0xFFFF;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x0001) crc = (crc >> 1) ^ 0xA001;
            else              crc >>= 1;
        }
    }
    return crc;
}

/* ============================================================
 * 字节序（大端 D3 决策）
 * ============================================================ */
void protocol_put_be16(uint8_t *buf, uint16_t val) {
    buf[0] = (uint8_t)(val >> 8);
    buf[1] = (uint8_t)(val & 0xFF);
}

uint16_t protocol_get_be16(const uint8_t *buf) {
    return ((uint16_t)buf[0] << 8) | (uint16_t)buf[1];
}

/* ============================================================
 * protocol_parse — 解析一帧
 * 返回 1 = 成功，0 = 失败
 * ============================================================ */
uint8_t protocol_parse(const uint8_t *buf, uint8_t len, protocol_frame_t *out) {
    /* 1. 最小长度检查 */
    if (len < PROTOCOL_MIN_FRAME_LEN || out == NULL) return 0;

    /* 2. 帧头检查 */
    if (buf[0] != PROTOCOL_HEADER_0 || buf[1] != PROTOCOL_HEADER_1) return 0;

    /* 3. LEN 一致性：LEN = 5 + payload_len，总长 = 2 + LEN */
    uint8_t payload_len = buf[2] - 5;
    if ((buf[2] + 2) != len) return 0;
    if (payload_len > PROTOCOL_MAX_PAYLOAD) return 0;

    /* 4. CRC16 校验（覆盖 LEN 起所有字节，不含 CRC 自身） */
    uint16_t calc_crc = crc16_modbus(&buf[2], (uint8_t)(len - 4));
    uint16_t recv_crc = protocol_get_be16(&buf[len - 2]);
    if (calc_crc != recv_crc) return 0;

    /* 5. 提取字段 */
    out->type        = buf[3];
    out->dst_id      = buf[4];
    out->payload_len = payload_len;
    for (uint8_t i = 0; i < payload_len; i++) {
        out->payload[i] = buf[5 + i];
    }
    out->crc16 = recv_crc;
    out->valid = 1;

    return 1;
}

/* ============================================================
 * protocol_build — 构造一帧
 * 返回帧总长（0 = 缓冲不够）
 * ============================================================ */
uint8_t protocol_build(uint8_t type, uint8_t dst_id,
                       const uint8_t *payload, uint8_t payload_len,
                       uint8_t *out_buf, uint8_t out_max) {
    /* 长度检查：payload 不能超过协议上限 */
    if (payload_len > PROTOCOL_MAX_PAYLOAD) return 0;

    uint8_t total = PROTOCOL_MIN_FRAME_LEN + payload_len;   /* 7 + payload_len */
    if (out_max < total) return 0;

    /* 帧头 */
    out_buf[0] = PROTOCOL_HEADER_0;
    out_buf[1] = PROTOCOL_HEADER_1;
    out_buf[2] = 5 + payload_len;                            /* LEN */
    out_buf[3] = type;
    out_buf[4] = dst_id;

    /* 载荷 */
    if (payload != NULL && payload_len > 0) {
        for (uint8_t i = 0; i < payload_len; i++) {
            out_buf[5 + i] = payload[i];
        }
    }

    /* CRC16：覆盖 LEN 起所有字节（不含 CRC 自身） */
    uint16_t crc = crc16_modbus(&out_buf[2], (uint8_t)(3 + payload_len));
    protocol_put_be16(&out_buf[5 + payload_len], crc);

    return total;
}

/* ============================================================
 * 调试辅助
 * ============================================================ */
const char *protocol_type_str(uint8_t type) {
    switch (type) {
        case PROTOCOL_TYPE_DATA_REPORT:  return "DATA_REPORT";
        case PROTOCOL_TYPE_DATA_ACK:     return "DATA_ACK";
        case PROTOCOL_TYPE_CMD_SET_RATE: return "CMD_SET_RATE";
        case PROTOCOL_TYPE_CMD_GET_VER:  return "CMD_GET_VER";
        case PROTOCOL_TYPE_CMD_RESP_VER: return "CMD_RESP_VER";
        case PROTOCOL_TYPE_HEARTBEAT:    return "HEARTBEAT";
        default: return "UNKNOWN";
    }
}

const char *protocol_addr_str(uint8_t addr) {
    switch (addr) {
        case PROTOCOL_ADDR_NODE1:   return "Node1(DHT11)";
        case PROTOCOL_ADDR_NODE2:   return "Node2(BNO055)";
        case PROTOCOL_ADDR_NODE3:   return "Node3(VET6)";
        case PROTOCOL_ADDR_GATEWAY: return "Gateway(ESP32)";
        case PROTOCOL_ADDR_BCAST:   return "Broadcast";
        default:                    return "Unknown";
    }
}
