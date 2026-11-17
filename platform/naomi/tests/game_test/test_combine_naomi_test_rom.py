#!/usr/bin/env python3
"""Host test for Naomi ROMFS-before-test dual-entry ROM packaging."""
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

TOOL = Path(__file__).with_name("combine_naomi_test_rom.py")
HEADER = 0x1000
MAIN_SEG = 0x360
TEST_SEG = 0x3C0
ENTRY = 0x420
TEST_ENTRY = 0x424


def make_rom(path: Path, load: int, entry: int, payload: bytes) -> None:
    image = bytearray(HEADER + len(payload))
    image[:6] = b"NAOMI "
    struct.pack_into("<III", image, MAIN_SEG, HEADER, load, len(payload))
    struct.pack_into("<I", image, ENTRY, entry)
    image[HEADER:] = payload
    path.write_bytes(image)


def main() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        main_payload = bytes(range(128))
        test_payload = b"DAYTONA-TEST-ELF-PAYLOAD"
        main_rom, test_rom, output = root / "main.bin", root / "test.bin", root / "out.bin"
        make_rom(main_rom, 0x8c020000, 0x8c020000, main_payload)
        main_image = bytearray(main_rom.read_bytes())
        fake_romfs = b"ROMFS\\0" + bytes(14)
        main_image.extend(fake_romfs)
        main_rom.write_bytes(main_image)
        make_rom(test_rom, 0x8c100000, 0x8c100000, test_payload)

        subprocess.run([sys.executable, str(TOOL), str(main_rom), str(test_rom), str(output)], check=True)
        image = output.read_bytes()
        assert image[:6] == b"NAOMI "
        assert struct.unpack_from("<I", image, ENTRY)[0] == 0x8c020000
        offset, load, size = struct.unpack_from("<III", image, TEST_SEG)
        assert struct.unpack_from("<I", image, TEST_ENTRY)[0] == 0x8c100000
        assert (load, size) == (0x8c100000, len(test_payload))
        assert offset % 0x1000 == 0
        assert image[HEADER:HEADER + len(main_payload)] == main_payload
        fs_offset = HEADER + len(main_payload)
        assert image[fs_offset:fs_offset + len(fake_romfs)] == fake_romfs
        assert offset >= len(main_image)
        assert image[offset:offset + size] == test_payload
        print("Naomi dual-entry ROM packing (ROMFS before test payload): OK")


if __name__ == "__main__":
    main()
