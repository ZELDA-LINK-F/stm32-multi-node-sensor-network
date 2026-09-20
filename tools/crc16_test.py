#!/usr/bin/env python3
"""
crc16_test.py — CRC16-MODBUS 验证工具（PROTOCOL.md §3.2 参考实现）

用法：
  # 计算 CRC
  python3 crc16_test.py --crc "01 02 03 04"
  
  # 验证带 CRC 的整帧
  python3 crc16_test.py --verify "AA 55 08 01 02 01 2E FF AD 5A C8"
  
  # 生成完整测试帧
  python3 crc16_test.py --build --type 01 --dst 02 --payload "01 2E FF AD"
"""

import argparse
import sys


def crc16_modbus(data: bytes) -> int:
    """CRC16-MODBUS（D4 决策）：初值 0xFFFF，多项式 0xA001"""
    crc = 0xFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 0x0001:
                crc = (crc >> 1) ^ 0xA001
            else:
                crc >>= 1
    return crc


def parse_hex(s: str) -> bytes:
    """解析 "AA 55 08" 或 "AA5508" → bytes"""
    s = s.replace(',', ' ').replace('0x', '').replace('-', ' ')
    return bytes.fromhex(''.join(s.split()))


def cmd_crc(args):
    data = parse_hex(args.crc)
    crc = crc16_modbus(data)
    print(f"输入: {data.hex(' ').upper()}")
    print(f"CRC16 = 0x{crc:04X}")
    print(f"CRC_L = 0x{crc & 0xFF:02X}  (先发)")
    print(f"CRC_H = 0x{(crc >> 8) & 0xFF:02X}  (后发)")
    print(f"完整帧尾: {(crc & 0xFF):02X} {(crc >> 8):02X}".upper())


def cmd_verify(args):
    frame = parse_hex(args.verify)
    if len(frame) < 7:
        print(f"错误：帧太短（{len(frame)} 字节），最少 7 字节")
        return 1
    
    # 校验：CRC 覆盖 LEN（offset 2）到 PAYLOAD 末尾（不含 CRC 自身）
    crc_data = frame[2:-2]
    crc_recv_l = frame[-2]
    crc_recv_h = frame[-1]
    crc_recv = (crc_recv_h << 8) | crc_recv_l
    
    crc_calc = crc16_modbus(crc_data)
    
    print(f"完整帧: {frame.hex(' ').upper()}")
    print(f"校验范围（不含帧头和 CRC）: {crc_data.hex(' ').upper()}")
    print(f"接收 CRC: 0x{crc_recv:04X}")
    print(f"计算 CRC: 0x{crc_calc:04X}")
    
    if crc_recv == crc_calc:
        print("✅ CRC 校验通过")
        return 0
    else:
        print("❌ CRC 校验失败")
        return 1


def cmd_build(args):
    payload = parse_hex(args.payload) if args.payload else b''
    type_b = int(args.type, 16)
    dst_b = int(args.dst, 16)
    
    # LEN = 5 + payload_len (type + dst + payload + 2 字节 CRC)
    length = 5 + len(payload)
    if length > 0x1F:
        print(f"错误：载荷太长（{length} 字节），最大 0x1F")
        return 1
    
    # 构造 LEN 起所有数据（不含帧头和 CRC）
    body = bytes([length, type_b, dst_b]) + payload
    
    # 计算 CRC
    crc = crc16_modbus(body)
    crc_l = crc & 0xFF
    crc_h = (crc >> 8) & 0xFF
    
    frame = b'\xAA\x55' + body + bytes([crc_l, crc_h])
    print(f"完整帧: {frame.hex(' ').upper()}")
    print(f"长度: {len(frame)} 字节")
    return 0


def main():
    p = argparse.ArgumentParser(description='STM32 传感网 CRC16 工具')
    g = p.add_mutually_exclusive_group(required=True)
    g.add_argument('--crc', help='计算 CRC，输入 HEX 字符串')
    g.add_argument('--verify', help='验证完整帧 CRC')
    g.add_argument('--build', action='store_true', help='构建测试帧')
    p.add_argument('--type', help='消息类型（hex）')
    p.add_argument('--dst', help='目标地址（hex）')
    p.add_argument('--payload', help='载荷 HEX 字符串')
    
    args = p.parse_args()
    
    if args.crc:
        return cmd_crc(args)
    elif args.verify:
        return cmd_verify(args)
    elif args.build:
        if not args.type or not args.dst:
            print("--build 需要 --type 和 --dst")
            return 1
        return cmd_build(args)


if __name__ == '__main__':
    sys.exit(main() or 0)
