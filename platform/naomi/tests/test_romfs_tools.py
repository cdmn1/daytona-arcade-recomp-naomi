import importlib.util
import struct
import sys
import tempfile
import unittest
from pathlib import Path

TOOLS = Path(__file__).resolve().parents[1] / "tools"


def load_tool(name):
    spec = importlib.util.spec_from_file_location(name, TOOLS / f"{name}.py")
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


romfsgen = load_tool("romfsgen")
append_romfs = load_tool("append_romfs")


def find_entry(image, directory_offset, entry_count, wanted):
    for index in range(entry_count):
        at = directory_offset + index * 256
        offset, size, kind = struct.unpack_from("<iII", image, at)
        name = image[at + 12:at + 256].split(b"\0", 1)[0].decode("utf-8")
        if name == wanted:
            return offset, size, kind
    raise AssertionError(f"ROMFS entry not found: {wanted}")


class NaomiRomfsToolsTests(unittest.TestCase):
    def test_nested_files_and_duplicate_payloads(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary) / "assets"
            music = root / "music"
            music.mkdir(parents=True)
            payload = b"naomi-resident-image-data"
            (music / "t01.adx").write_bytes(payload)
            (music / "t02.adx").write_bytes(payload)
            (root / "readme.dat").write_bytes(b"hello")

            image = romfsgen.build_romfs(root)
            root_count = struct.unpack_from("<I", image, 12)[0]
            music_offset, music_count, music_type = find_entry(image, 16, root_count, "music")
            self.assertEqual(music_type, romfsgen.TYPE_DIR)

            music_table = 16 + music_offset
            first_offset, first_size, first_type = find_entry(
                image, music_table, music_count, "t01.adx"
            )
            second_offset, second_size, second_type = find_entry(
                image, music_table, music_count, "t02.adx"
            )
            self.assertEqual(first_type, romfsgen.TYPE_FILE)
            self.assertEqual(second_type, romfsgen.TYPE_FILE)
            self.assertEqual(first_size, len(payload))
            self.assertEqual(second_size, len(payload))
            self.assertEqual(first_offset, second_offset)
            self.assertEqual(
                image[music_table + first_offset:music_table + first_offset + first_size],
                payload,
            )

    def test_rejects_empty_asset_tree(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary) / "empty"
            root.mkdir()
            with self.assertRaisesRegex(ValueError, "no regular files"):
                romfsgen.build_romfs(root)

    def test_output_is_deterministic(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary) / "assets"
            root.mkdir()
            (root / "z.bin").write_bytes(b"z")
            (root / "a.bin").write_bytes(b"a")
            self.assertEqual(romfsgen.build_romfs(root), romfsgen.build_romfs(root))

    def test_appends_after_naomi_main_segment(self):
        with tempfile.TemporaryDirectory() as temporary:
            folder = Path(temporary)
            base_path = folder / "base.bin"
            fs_path = folder / "assets.romfs"
            output_path = folder / "final.bin"

            base = bytearray(0x1020)
            base[:16] = b"NAOMI           "
            struct.pack_into("<III", base, 0x360, 0x1000, 0x8C020000, 0x20)
            struct.pack_into("<I", base, 0x36C, 0xFFFFFFFF)
            base_path.write_bytes(base)

            source = folder / "assets"
            source.mkdir()
            (source / "file.bin").write_bytes(b"resident")
            fs = romfsgen.build_romfs(source)
            fs_path.write_bytes(fs)

            size = append_romfs.append_romfs(base_path, fs_path, output_path)
            result = output_path.read_bytes()
            self.assertEqual(size, len(result))
            self.assertEqual(result[0x1020:], fs)

    def test_rejects_image_past_pio_address_limit(self):
        with tempfile.TemporaryDirectory() as temporary:
            folder = Path(temporary)
            base_path = folder / "base.bin"
            fs_path = folder / "assets.romfs"
            output_path = folder / "final.bin"

            base = bytearray(0x500)
            base[:16] = b"NAOMI           "
            struct.pack_into(
                "<III", base, 0x360, 0x1000, 0x8C020000,
                append_romfs.PIO_LIMIT - 0x1000,
            )
            struct.pack_into("<I", base, 0x36C, 0xFFFFFFFF)
            base_path.write_bytes(base)

            source = folder / "assets"
            source.mkdir()
            (source / "file.bin").write_bytes(b"x")
            fs_path.write_bytes(romfsgen.build_romfs(source))

            with self.assertRaisesRegex(ValueError, "256 MiB"):
                append_romfs.append_romfs(base_path, fs_path, output_path)


if __name__ == "__main__":
    unittest.main()
