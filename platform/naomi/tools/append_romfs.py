#!/usr/bin/env python3
"""Append a Naomi ROMFS immediately after a naomibintool executable segment."""
from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

HEADER_SIZE = 0x500
MAIN_SEGMENTS = 0x360
SEGMENT_COUNT = 8
ROMFS_MAGIC = b"ROMFS\0"
ROMFS_CHECKSUM = 0x11291985
PIO_LIMIT = 0x10000000


def u32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def append_romfs(base_path: Path, fs_path: Path, output_path: Path) -> int:
    image = base_path.read_bytes()
    romfs = fs_path.read_bytes()

    if len(image) < HEADER_SIZE:
        raise ValueError("input is too small to contain a Naomi ROM header")
    if image[:6] != b"NAOMI "[:6] and image[:6] != b"Naomi2":
        raise ValueError(f"unexpected Naomi ROM signature: {image[:16]!r}")
    if len(romfs) < 16 or romfs[:6] != ROMFS_MAGIC:
        raise ValueError("filesystem input is not a Naomi ROMFS image")
    if u32(romfs, 8) != ROMFS_CHECKSUM:
        raise ValueError("ROMFS checksum/magic field is invalid")

    sections = []
    for index in range(SEGMENT_COUNT):
        entry = MAIN_SEGMENTS + index * 12
        offset = u32(image, entry)
        size = u32(image, entry + 8)
        if offset == 0xFFFFFFFF:
            break
        if size:
            sections.append((offset, size))

    if len(sections) != 1:
        raise ValueError(
            f"expected one main executable segment from naomibintool, found {len(sections)}"
        )

    segment_offset, segment_size = sections[0]
    if segment_offset >= PIO_LIMIT or segment_size > PIO_LIMIT - segment_offset:
        raise ValueError("executable segment exceeds the 256 MiB PIO address range")

    fs_offset = (segment_offset + segment_size + 3) & ~3
    image_end = fs_offset + len(romfs)
    final_size = (image_end + 1023) & ~1023
    if final_size > PIO_LIMIT:
        raise ValueError(
            "final ROM exceeds the 256 MiB PIO address range; a DMA reader is required"
        )

    if len(image) > fs_offset:
        if any(image[fs_offset:]):
            raise ValueError(
                "input ROM already contains data after the executable segment; refusing to overwrite it"
            )
        image = image[:fs_offset]
    elif len(image) < fs_offset:
        image += bytes(fs_offset - len(image))

    result = image + romfs + bytes(final_size - image_end)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(result)
    return len(result)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("base_rom", type=Path, help="naomibintool output")
    parser.add_argument("romfs", type=Path, help="ROMFS image to append")
    parser.add_argument("output", type=Path, help="final NetDIMM .bin output")
    args = parser.parse_args()

    try:
        size = append_romfs(args.base_rom, args.romfs, args.output)
    except (OSError, ValueError, struct.error) as exc:
        print(f"append_romfs: {exc}", file=sys.stderr)
        return 1

    print(f"Wrote {args.output} ({size} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
