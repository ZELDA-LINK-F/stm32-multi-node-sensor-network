/*
 * test_protocol.c — 协议层 PC 单元测试
 *
 * 测试向量来源：
 *   - tools/crc16_test.py 已经验证过的标准向量
 *   - docs/PROTOCOL.md §6.1 离线验证方案
 *
 * 编译（在 tools/test_protocol/ 下）：
 *   make
 *
 * 运行：
 *   ./test_protocol
 */

#include <stdio.h>
#include <string.h>
#include "protocol.h"

static int total = 0;
static int passed = 0;

#define ASSERT_EQ(actual, expected, name) do { \
    total++; \
    if ((actual) == (expected)) { \
        passed++; \
        printf("  ✅ %s\n", name); \
    } else { \
        printf("  ❌ %s (expected 0x%X, got 0x%X)\n", name, (unsigned)(expected), (unsigned)(actual)); \
    } \
} while (0)

#define ASSERT_TRUE(cond, name) do { \
    total++; \
    if (cond) { \
        passed++; \
        printf("  ✅ %s\n", name); \
    } else { \
        printf("  ❌ %s\n", name); \
    } \
} while (0)

/* ============================================================
 * 测试 1: CRC16 标准向量
 * ============================================================ */
static void test_crc16_standard(void) {
    printf("\n[1] CRC16-MODBUS 标准测试向量\n");

    /* ASCII "123456789" → 0x4B37 */
    uint8_t data1[] = "123456789";
    uint16_t crc1 = crc16_modbus((const uint8_t*)data1, 9);
    ASSERT_EQ(crc1, 0x4B37, "CRC16('123456789') = 0x4B37");

    /* Modbus 01 03 00 00 00 0A → 0xCDC5 */
    uint8_t data2[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x0A};
    uint16_t crc2 = crc16_modbus(data2, 6);
    ASSERT_EQ(crc2, 0xCDC5, "CRC16(01 03 00 00 00 0A) = 0xCDC5");

    /* Modbus 01 04 02 FF FF → 0x80B8 */
    uint8_t data3[] = {0x01, 0x04, 0x02, 0xFF, 0xFF};
    uint16_t crc3 = crc16_modbus(data3, 5);
    ASSERT_EQ(crc3, 0x80B8, "CRC16(01 04 02 FF FF) = 0xB880");
}

/* ============================================================
 * 测试 2: 大端字节序
 * ============================================================ */
static void test_byte_order(void) {
    printf("\n[2] 大端字节序\n");

    uint8_t buf[2];
    protocol_put_be16(buf, 0x1234);
    ASSERT_EQ(buf[0], 0x12, "put_be16(0x1234) buf[0] = 0x12");
    ASSERT_EQ(buf[1], 0x34, "put_be16(0x1234) buf[1] = 0x34");

    uint16_t val = protocol_get_be16(buf);
    ASSERT_EQ(val, 0x1234, "get_be16([0x12,0x34]) = 0x1234");
}

/* ============================================================
 * 测试 3: Build + Parse 往返（BNO055 数据）
 * ============================================================ */
static void test_roundtrip_bno055(void) {
    printf("\n[3] Build + Parse 往返 (BNO055 DATA_REPORT)\n");

    /* yaw=302(=30.5°×16) roll=-83 pitch=160 → 6 字节大端 int16 */
    uint8_t payload[] = {0x01, 0x2E, 0xFF, 0xAD, 0x00, 0xA0};

    /* 构建 */
    uint8_t frame[32];
    uint8_t len = protocol_build(PROTOCOL_TYPE_DATA_REPORT, PROTOCOL_ADDR_GATEWAY,
                                  payload, 6, frame, sizeof(frame));
    ASSERT_EQ(len, 13, "Build 帧长度 = 13 字节 (7 + 6)");
    ASSERT_EQ(frame[0], 0xAA, "帧头 0xAA");
    ASSERT_EQ(frame[1], 0x55, "帧头 0x55");
    ASSERT_EQ(frame[2], 11, "LEN = 11 (5 + 6)");
    ASSERT_EQ(frame[3], PROTOCOL_TYPE_DATA_REPORT, "TYPE = 0x01");
    ASSERT_EQ(frame[4], PROTOCOL_ADDR_GATEWAY, "DST_ID = 0xF0");

    /* 解析 */
    protocol_frame_t out;
    memset(&out, 0, sizeof(out));
    ASSERT_TRUE(protocol_parse(frame, len, &out), "Parse 成功");
    ASSERT_TRUE(out.valid, "out.valid = 1");
    ASSERT_EQ(out.type, PROTOCOL_TYPE_DATA_REPORT, "out.type = DATA_REPORT");
    ASSERT_EQ(out.dst_id, PROTOCOL_ADDR_GATEWAY, "out.dst_id = GATEWAY");
    ASSERT_EQ(out.payload_len, 6, "out.payload_len = 6");
    ASSERT_EQ(memcmp(out.payload, payload, 6), 0, "out.payload == 原 payload");
}

/* ============================================================
 * 测试 4: Build + Parse 往返（HEARTBEAT）
 * ============================================================ */
static void test_roundtrip_heartbeat(void) {
    printf("\n[4] Build + Parse 往返 (HEARTBEAT)\n");

    uint8_t payload[] = {0x01};  /* 1 字节状态 */
    uint8_t frame[32];

    uint8_t len = protocol_build(PROTOCOL_TYPE_HEARTBEAT, PROTOCOL_ADDR_GATEWAY,
                                  payload, 1, frame, sizeof(frame));
    ASSERT_EQ(len, 8, "HEARTBEAT 帧 = 8 字节 (7 + 1)");

    protocol_frame_t out;
    memset(&out, 0, sizeof(out));
    ASSERT_TRUE(protocol_parse(frame, len, &out), "Parse HEARTBEAT 成功");
    ASSERT_EQ(out.type, PROTOCOL_TYPE_HEARTBEAT, "type = HEARTBEAT");
    ASSERT_EQ(out.payload[0], 0x01, "payload[0] = 0x01 (status)");
}

/* ============================================================
 * 测试 5: 错误 CRC 检测
 * ============================================================ */
static void test_invalid_crc(void) {
    printf("\n[5] 无效 CRC 检测\n");

    uint8_t payload[] = {0x01, 0x02, 0x03};
    uint8_t frame[32];
    uint8_t len = protocol_build(PROTOCOL_TYPE_HEARTBEAT, PROTOCOL_ADDR_GATEWAY,
                                  payload, 3, frame, sizeof(frame));

    /* 损坏 CRC */
    frame[len - 1] ^= 0xFF;

    protocol_frame_t out;
    memset(&out, 0, sizeof(out));
    ASSERT_TRUE(!protocol_parse(frame, len, &out), "损坏 CRC 应该被拒绝");
}

/* ============================================================
 * 测试 6: 截断帧检测
 * ============================================================ */
static void test_truncated(void) {
    printf("\n[6] 截断帧检测\n");

    /* 只有 3 字节 */
    uint8_t frame1[] = {0xAA, 0x55, 0x05};
    protocol_frame_t out;
    memset(&out, 0, sizeof(out));
    ASSERT_TRUE(!protocol_parse(frame1, 3, &out), "3 字节截断帧被拒绝");

    /* 帧头正确但长度不足（仅 6 字节） */
    uint8_t frame2[] = {0xAA, 0x55, 0x05, 0x01, 0xF0, 0xAB};
    memset(&out, 0, sizeof(out));
    ASSERT_TRUE(!protocol_parse(frame2, 6, &out), "6 字节无 CRC 帧被拒绝");
}

/* ============================================================
 * 测试 7: 错误帧头检测
 * ============================================================ */
static void test_bad_header(void) {
    printf("\n[7] 错误帧头检测\n");

    uint8_t frame[] = {0xFF, 0xFF, 0x05, 0x01, 0xF0, 0x00, 0xAA, 0xBB};
    protocol_frame_t out;
    memset(&out, 0, sizeof(out));
    ASSERT_TRUE(!protocol_parse(frame, 8, &out), "帧头 0xFFFF 应该被拒绝");
}

/* ============================================================
 * 测试 8: LEN 不匹配
 * ============================================================ */
static void test_len_mismatch(void) {
    printf("\n[8] LEN 字段不匹配\n");

    /* 帧头正确 + LEN=10 但实际帧长 7 → 不匹配 */
    uint8_t frame[] = {0xAA, 0x55, 0x0A, 0x01, 0xF0, 0x00, 0x00};
    protocol_frame_t out;
    memset(&out, 0, sizeof(out));
    ASSERT_TRUE(!protocol_parse(frame, 7, &out), "LEN 不匹配被拒绝");
}

/* ============================================================
 * 测试 9: payload 过大
 * ============================================================ */
static void test_oversized_payload(void) {
    printf("\n[9] payload 过大\n");

    /* 27 字节 payload > 26 字节上限 */
    uint8_t payload[27];
    memset(payload, 0xAA, sizeof(payload));
    uint8_t frame[64];
    uint8_t len = protocol_build(PROTOCOL_TYPE_DATA_REPORT, PROTOCOL_ADDR_GATEWAY,
                                  payload, 27, frame, sizeof(frame));
    ASSERT_EQ(len, 0, "27 字节 payload → build 返回 0");
}

/* ============================================================
 * 测试 10: 边界 - 空 payload
 * ============================================================ */
static void test_empty_payload(void) {
    printf("\n[10] 空 payload (ACK 等命令帧)\n");

    uint8_t frame[32];
    uint8_t len = protocol_build(PROTOCOL_TYPE_DATA_ACK, PROTOCOL_ADDR_NODE1,
                                  NULL, 0, frame, sizeof(frame));
    ASSERT_EQ(len, 7, "空 payload 帧 = 7 字节");

    protocol_frame_t out;
    memset(&out, 0, sizeof(out));
    ASSERT_TRUE(protocol_parse(frame, len, &out), "空 payload 帧解析成功");
    ASSERT_EQ(out.payload_len, 0, "payload_len = 0");
}

/* ============================================================
 * 测试 11: 字符串辅助函数
 * ============================================================ */
static void test_string_helpers(void) {
    printf("\n[11] 字符串辅助函数\n");

    ASSERT_TRUE(strcmp(protocol_type_str(PROTOCOL_TYPE_DATA_REPORT), "DATA_REPORT") == 0,
                "type_str(0x01) = DATA_REPORT");
    ASSERT_TRUE(strcmp(protocol_addr_str(PROTOCOL_ADDR_GATEWAY), "Gateway(ESP32)") == 0,
                "addr_str(0xF0) = Gateway(ESP32)");
    ASSERT_TRUE(strcmp(protocol_addr_str(0xFF), "Unknown") != 0 ||
                strcmp(protocol_addr_str(0xFF), "Broadcast") == 0,
                "addr_str(0xFF) = Broadcast");
}

/* ============================================================
 * main
 * ============================================================ */
int main(void) {
    printf("========================================\n");
    printf("  STM32 传感网协议层 - 单元测试\n");
    printf("========================================\n");

    test_crc16_standard();
    test_byte_order();
    test_roundtrip_bno055();
    test_roundtrip_heartbeat();
    test_invalid_crc();
    test_truncated();
    test_bad_header();
    test_len_mismatch();
    test_oversized_payload();
    test_empty_payload();
    test_string_helpers();

    printf("\n========================================\n");
    printf("  结果: %d/%d 测试通过\n", passed, total);
    printf("========================================\n");

    return (passed == total) ? 0 : 1;
}
