/**
 * @file    protocol.h
 * @brief   STM32 传感网自研二进制通信协议
 * @details 帧格式: AA 55 | LEN | TYPE | DST_ID | PAYLOAD | CRC16
 *          - 大端字节序（D3 决策）
 *          - CRC16-MODBUS（0xFFFF 初值 + 0xA001 多项式，D4 决策）
 *          - 载荷上限 26 字节（NRF24 32B - 6B 帧头）
 * @author  ZELDA-LINK-F
 * @date    2026-09-22
 * @version 1.0
 * 
 * @section 协议设计
 * - 5 个工程共用（STM32 节点 + ESP32 网关）
 * - 帧结构详见 docs/PROTOCOL.md
 * - 34 个 PC 单元测试全过（tools/test_protocol/）
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

/* === 帧常量 === */
#define PROTOCOL_HEADER_0         0xAA   /* 帧头字节 0 */
#define PROTOCOL_HEADER_1         0x55   /* 帧头字节 1 */
#define PROTOCOL_MAX_PAYLOAD       26     /* 载荷上限 */
#define PROTOCOL_MIN_FRAME_LEN     7      /* 最小帧长 */

/* === 消息类型（PROTOCOL.md §1.3）=== */
#define PROTOCOL_TYPE_DATA_REPORT   0x01   /* 节点 → 网关：传感器数据 */
#define PROTOCOL_TYPE_DATA_ACK      0x02   /* 网关 → 节点：应答 */
#define PROTOCOL_TYPE_CMD_SET_RATE  0x10   /* 网关 → 节点：设采样率 */
#define PROTOCOL_TYPE_CMD_GET_VER   0x11   /* 查询固件版本 */
#define PROTOCOL_TYPE_CMD_RESP_VER  0x12   /* 版本应答 */
#define PROTOCOL_TYPE_HEARTBEAT     0x80   /* 节点 → 网关：心跳 */

/* === 节点地址（PROTOCOL.md §1.2）=== */
#define PROTOCOL_ADDR_NODE1         0x01   /* C8T6 + DHT11 */
#define PROTOCOL_ADDR_NODE2         0x02   /* C8T6 + BNO055 */
#define PROTOCOL_ADDR_NODE3         0x03   /* VET6 主节点 */
#define PROTOCOL_ADDR_GATEWAY       0xF0   /* ESP32-S3 网关 */
#define PROTOCOL_ADDR_BCAST         0xFF   /* 广播 */

/* === 解析结果 === */
typedef struct {
    uint8_t  type;                    /* 消息类型 */
    uint8_t  dst_id;                  /* 目标地址 */
    uint8_t  payload[PROTOCOL_MAX_PAYLOAD];
    uint8_t  payload_len;
    uint16_t crc16;
    uint8_t  valid;                   /* 1 = CRC 通过 */
} protocol_frame_t;

/**
 * @brief  CRC16-MODBUS 计算（D4 决策）
 * @param  data 数据指针
 * @param  len  数据长度
 * @return CRC16 值
 */
uint16_t crc16_modbus(const uint8_t *data, uint8_t len);

/**
 * @brief  大端字节序：uint16_t → 2 字节（高字节在前）
 * @param  buf  目标缓冲（至少 2 字节）
 * @param  val  要写入的值
 */
void     protocol_put_be16(uint8_t *buf, uint16_t val);

/**
 * @brief  大端字节序：2 字节 → uint16_t
 * @param  buf  源缓冲（至少 2 字节）
 * @return 解析后的值
 */
uint16_t protocol_get_be16(const uint8_t *buf);

/**
 * @brief  解析一帧
 * @param  buf  完整帧数据
 * @param  len  长度
 * @param  out  输出结构体
 * @return 1 = 成功，0 = 失败（帧头错/CRC 错/长度错）
 */
uint8_t protocol_parse(const uint8_t *buf, uint8_t len, protocol_frame_t *out);

/**
 * @brief  构造一帧
 * @param  type         帧类型
 * @param  dst_id       目标地址
 * @param  payload      载荷数据
 * @param  payload_len  载荷长度（≤ 26）
 * @param  out_buf      输出缓冲（至少 7 + payload_len 字节）
 * @param  out_max      缓冲大小
 * @return 帧总长，0 = 缓冲不够或 payload 过长
 */
uint8_t protocol_build(uint8_t type, uint8_t dst_id,
                       const uint8_t *payload, uint8_t payload_len,
                       uint8_t *out_buf, uint8_t out_max);

/** 消息类型 → 字符串（调试用） */
const char *protocol_type_str(uint8_t type);

/** 地址 → 字符串（调试用） */
const char *protocol_addr_str(uint8_t addr);

#endif
