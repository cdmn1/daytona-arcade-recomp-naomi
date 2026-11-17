#!/usr/bin/env python3
"""Insert a separately linked Naomi test ELF into a main Naomi ROM image."""
import struct
import sys
from pathlib import Path

HEADER_SIZE = 0x1000
MAIN_SEGMENTS = 0x360
TEST_SEGMENTS = 0x3C0
ENTRY = 0x420
TEST_ENTRY = 0x424
SEGMENT_SIZE = 12
SEGMENT_COUNT = 8
ALIGNMENT = 0x1000


def read_segment(header: bytes, offset: int):
    return struct.unpack_from("<III", header, offset)


def main():
    if len(sys.argv) != 4:
        raise SystemExit("usage: combine_naomi_test_rom.py main.bin test.bin output.bin")
    main_path, test_path, output_path = map(Path, sys.argv[1:])
    main_rom = bytearray(main_path.read_bytes())
    test_rom = test_path.read_bytes()
    if len(main_rom) < HEADER_SIZE or len(test_rom) < HEADER_SIZE:
        raise SystemExit("input is not a complete Naomi ROM image")
    main_seg = read_segment(main_rom, MAIN_SEGMENTS)
    test_seg = read_segment(test_rom, MAIN_SEGMENTS)
    test_entry = struct.unpack_from("<I", test_rom, ENTRY)[0]
    if main_seg[0] == 0xFFFFFFFF or test_seg[0] == 0xFFFFFFFF:
        raise SystemExit("main/test ROM is missing its first load segment")
    if test_entry in (0, 0xFFFFFFFF) or test_seg[2] == 0:
        raise SystemExit("test ROM has an invalid entry point or segment")
    payload = test_rom[HEADER_SIZE:HEADER_SIZE + test_seg[2]]
    if len(payload) != test_seg[2]:
        raise SystemExit("test payload is shorter than its declared segment size")
    offset = (len(main_rom) + ALIGNMENT - 1) & ~(ALIGNMENT - 1)
    main_rom.extend(b"\x00" * (offset - len(main_rom)))
    main_rom.extend(payload)
    for i in range(SEGMENT_COUNT):
        struct.pack_into("<III", main_rom, TEST_SEGMENTS + i * SEGMENT_SIZE,
                         0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF)
    struct.pack_into("<III", main_rom, TEST_SEGMENTS, offset, test_seg[1], test_seg[2])
    struct.pack_into("<I", main_rom, TEST_ENTRY, test_entry)
    output_path.write_bytes(main_rom)
    print(f"Main entry: 0x{struct.unpack_from('<I', main_rom, ENTRY)[0]:08x}")
    print(f"Test entry: 0x{test_entry:08x}")
    print(f"Test payload: ROM 0x{offset:x}, RAM 0x{test_seg[1]:08x}, {test_seg[2]} bytes")
    print(f"Wrote {output_path} ({len(main_rom)} bytes)")


if __name__ == "__main__":
    main()
