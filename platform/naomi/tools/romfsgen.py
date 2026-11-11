#!/usr/bin/env python3
"""Build the read-only ROMFS format used by Naomi homebrew cartridge images.

The on-image format is compatible with DragonMinded/libnaomi's public-domain
ROMFS implementation. Directory entries are 256 bytes and file payloads are
4-byte aligned.
"""
from __future__ import annotations

import argparse
import hashlib
import os
import struct
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Optional, Tuple

HEADER_SIZE = 16
ENTRY_SIZE = 256
NAME_SIZE = ENTRY_SIZE - 12
ROMFS_MAJOR = 1
ROMFS_MINOR = 0
ROMFS_CHECKSUM = 0x11291985
TYPE_DIR = 1
TYPE_FILE = 2


@dataclass
class Entry:
    offset: int
    size: int
    kind: int


def encode_name(name: str) -> bytes:
    encoded = name.encode("utf-8")
    if len(encoded) > NAME_SIZE - 1:
        raise ValueError(f"filename is too long for Naomi ROMFS ({len(encoded)} bytes): {name}")
    return encoded + bytes(NAME_SIZE - len(encoded))


def pack_entry(name: str, entry: Entry) -> bytes:
    return struct.pack("<iII", entry.offset, entry.size, entry.kind) + encode_name(name)


def build_directory(
    directory: Path,
    parent_offset: Optional[int],
    parent_entries: Optional[int],
) -> Tuple[int, bytes]:
    children = sorted(directory.iterdir(), key=lambda p: p.name)
    header_entries = len(children) + 2
    header_length = header_entries * ENTRY_SIZE
    entries: Dict[str, Entry] = {
        ".": Entry(0, header_entries, TYPE_DIR),
        "..": Entry(parent_offset if parent_offset is not None else 0,
                    parent_entries if parent_entries is not None else header_entries,
                    TYPE_DIR),
    }
    payload = bytearray()
    deduplicated: Dict[Tuple[int, str], Tuple[int, str]] = {}

    for child in children:
        if child.is_symlink():
            print(f"Skipping symbolic link: {child}", file=sys.stderr)
            continue

        name = child.name
        if name in entries:
            raise ValueError(f"reserved or duplicate ROMFS name: {name}")

        if child.is_file():
            data = child.read_bytes()
            digest = hashlib.sha256(data).hexdigest()
            key = (len(data), digest)
            if key in deduplicated:
                original_offset, original_name = deduplicated[key]
                print(f"Adding {child} (shared payload with {original_name})")
                entries[name] = Entry(header_length + original_offset, len(data), TYPE_FILE)
                continue

            while len(payload) & 3:
                payload.append(0)
            data_offset = len(payload)
            payload.extend(data)
            while len(payload) & 3:
                payload.append(0)
            deduplicated[key] = (data_offset, name)
            entries[name] = Entry(header_length + data_offset, len(data), TYPE_FILE)
            print(f"Adding {child} ({len(data)} bytes)")
        elif child.is_dir():
            child_offset = header_length + len(payload)
            child_count, child_data = build_directory(
                child,
                -child_offset,
                header_entries,
            )
            payload.extend(child_data)
            entries[name] = Entry(child_offset, child_count, TYPE_DIR)
        else:
            print(f"Skipping unsupported filesystem object: {child}", file=sys.stderr)

    directory_header = b"".join(pack_entry(name, entry) for name, entry in entries.items())
    if len(directory_header) != header_length:
        raise RuntimeError("internal ROMFS directory size mismatch")

    return len(entries), directory_header + bytes(payload)


def build_romfs(source: Path) -> bytes:
    if not source.is_dir():
        raise ValueError(f"asset root is not a directory: {source}")
    root_entries, root_data = build_directory(source, None, None)
    header = struct.pack(
        "<6sBBII",
        b"ROMFS\0",
        ROMFS_MAJOR,
        ROMFS_MINOR,
        ROMFS_CHECKSUM,
        root_entries,
    )
    if len(header) != HEADER_SIZE:
        raise RuntimeError("internal ROMFS header size mismatch")
    return header + root_data


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path, help="ROMFS output file")
    parser.add_argument("directory", type=Path, help="directory to package")
    args = parser.parse_args()

    try:
        image = build_romfs(args.directory)
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_bytes(image)
    except (OSError, ValueError, RuntimeError) as exc:
        print(f"romfsgen: {exc}", file=sys.stderr)
        return 1

    print(f"Wrote {args.output} ({len(image)} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
