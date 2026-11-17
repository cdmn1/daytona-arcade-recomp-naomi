#!/usr/bin/env python3
"""Append the Naomi test executable after the main executable's ROMFS."""
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
PIO_LIMIT = 0x10000000
ROMFS_MAGIC = b"ROMFS\0"


def read_segment(header: bytes, offset: int):
    return struct.unpack_from("<III", header, offset)


def main():
    if len(sys.argv) != 4:
        raise SystemExit("usage: combine_naomi_test_rom.py main-with-romfs.bin test.bin output.bin")
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

    main_end = main_seg[0] + main_seg[2]
    if main_seg[0] >= PIO_LIMIT or main_seg[2] > PIO_LIMIT - main_seg[0]:
        raise SystemExit("main executable exceeds the Naomi 256 MiB PIO address range")
    fs_offset = (main_end + 3) & ~3
    if main_rom[fs_offset:fs_offset + len(ROMFS_MAGIC)] != ROMFS_MAGIC:
        raise SystemExit("main ROM does not contain ROMFS immediately after its executable segment")

    payload = test_rom[HEADER_SIZE:HEADER_SIZE + test_seg[2]]
    if len(payload) != test_seg[2]:
        raise SystemExit("test payload is shorter than its declared segment size")

    # Keep the main ROMFS at main-segment-end; place the BIOS test program
    # after the ROMFS and point the Naomi test segment table to this payload.
    offset = (len(main_rom) + ALIGNMENT - 1) & ~(ALIGNMENT - 1)
    if offset >= PIO_LIMIT or test_seg[2] > PIO_LIMIT - offset:
        raise SystemExit("test executable exceeds the Naomi 256 MiB PIO address range")
    main_rom.extend(b"\x00" * (offset - len(main_rom)))
    main_rom.extend(payload)
    final_size = (len(main_rom) + 1023) & ~1023
    if final_size > PIO_LIMIT:
        raise SystemExit("final ROM exceeds the Naomi 256 MiB PIO address range")
    main_rom.extend(b"\x00" * (final_size - len(main_rom)))

    for i in range(SEGMENT_COUNT):
        struct.pack_into("<III", main_rom, TEST_SEGMENTS + i * SEGMENT_SIZE,
                         0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF)
    struct.pack_into("<III", main_rom, TEST_SEGMENTS, offset, test_seg[1], test_seg[2])
    struct.pack_into("<I", main_rom, TEST_ENTRY, test_entry)

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(main_rom)
    print(f"Main entry: 0x{struct.unpack_from('<I', main_rom, ENTRY)[0]:08x}")
    print(f"Test entry: 0x{test_entry:08x}")
    print(f"ROMFS offset: 0x{fs_offset:x}")
    print(f"Test payload: ROM 0x{offset:x}, RAM 0x{test_seg[1]:08x}, {test_seg[2]} bytes")
    print(f"Wrote {output_path} ({len(main_rom)} bytes)")


if __name__ == "__main__":
    main()
