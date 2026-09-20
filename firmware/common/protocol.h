/*
 * protocol.h — STM32 传感网自研二进制协议 C 实现
 *
 * 依据：docs/PROTOCOL.md
 * D3 决策：大端字节序
 * D4 决策：CRC16-MODBUS（初值 0xFFFF + 多项式 0xA001）
 *
 * 用途：STM32 节点 + ESP32-S3 网关通用
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

/* === 帧常量 === */
#define PROTOCOL_HEADER_0         0xAA
#define PROTOCOL_HEADER_1         0x55
#define PROTOCOL_MAX_PAYLOAD       26    /* 32 字节总长 - 6 字节开销 */
#define PROTOCOL_MIN_FRAME_LEN     7     /* 最小帧：帧头+LEN+TYPE+DST+CRC */

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
#define PROTOCOL_ADDR_GATEWAY       0xF0   /* ESP32-S3 */
#define PROTOCOL_ADDR_BCAST         0xFF   /* 广播 */

/* === 解析结果（PROTOCOL.md §5.1）=== */
typedef struct {
    uint8_t  type;
    uint8_t  dst_id;
    uint8_t  payload[PROTOCOL_MAX_PAYLOAD];
    uint8_t  payload_len;
    uint16_t crc16;
    uint8_t  valid;      /* 1 = 通过 CRC */
} protocol_frame_t;

/* === CRC16-MODBUS === */
uint16_t crc16_modbus(const uint8_t *data, uint8_t len);

/* === 字节序工具（大端）=== */
void     protocol_put_be16(uint8_t *buf, uint16_t val);
uint16_t protocol_get_be16(const uint8_t *buf);

/* === 协议 API === */

/*
 * protocol_parse — 解析一帧
 * @buf  : 完整帧数据
 * @len  : buf 长度
 * @out  : 输出（valid=1 表示成功）
 * @return: 1 = 成功，0 = 失败（长度不够/帧头错/CRC 错）
 */
uint8_t protocol_parse(const uint8_t *buf, uint8_t len, protocol_frame_t *out);

/*
 * protocol_build — 构造一帧
 * @type, @dst_id: 帧类型和目标地址
 * @payload, @payload_len: 载荷
 * @out_buf, @out_max: 输出缓冲
 * @return: 帧总长（7 + payload_len），0 表示缓冲不够
 */
uint8_t protocol_build(uint8_t type, uint8_t dst_id,
                       const uint8_t *payload, uint8_t payload_len,
                       uint8_t *out_buf, uint8_t out_max);

/* === 调试辅助 === */
const char *protocol_type_str(uint8_t type);
const char *protocol_addr_str(uint8_t addr);

#endif /* PROTOCOL_H */
