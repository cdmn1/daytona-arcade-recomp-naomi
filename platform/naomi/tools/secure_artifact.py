#!/usr/bin/env python3
"""Encrypt/decrypt a Naomi NetDIMM build artifact with a user-held passphrase.

Format: DUSAENC1 || 16-byte scrypt salt || 12-byte AES-GCM nonce || ciphertext+tag.
The passphrase is read from NAOMI_ARTIFACT_PASSWORD when encrypting and is
prompted securely when decrypting. Never pass it as a command-line argument.
"""
from __future__ import annotations

import argparse
import getpass
import hashlib
import os
import secrets
import sys
from pathlib import Path

MAGIC = b"DUSAENC1"
SALT_BYTES = 16
NONCE_BYTES = 12
SCRYPT_N = 1 << 15
SCRYPT_R = 8
SCRYPT_P = 1


def key_for(password: str, salt: bytes) -> bytes:
    return hashlib.scrypt(
        password.encode("utf-8"), salt=salt, n=SCRYPT_N,
        r=SCRYPT_R, p=SCRYPT_P, dklen=32, maxmem=128 * 1024 * 1024,
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--encrypt", action="store_true")
    mode.add_argument("--decrypt", action="store_true")
    parser.add_argument("input", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    try:
        from cryptography.hazmat.primitives.ciphers.aead import AESGCM
    except ImportError:
        print("Install the Python cryptography package: python -m pip install cryptography", file=sys.stderr)
        return 2

    if not args.input.is_file():
        print(f"Input file not found: {args.input}", file=sys.stderr)
        return 2

    try:
        if args.encrypt:
            password = os.environ.get("NAOMI_ARTIFACT_PASSWORD", "")
            if len(password) < 16:
                raise ValueError("NAOMI_ARTIFACT_PASSWORD must contain at least 16 characters")
            salt = secrets.token_bytes(SALT_BYTES)
            nonce = secrets.token_bytes(NONCE_BYTES)
            data = args.input.read_bytes()
            encrypted = AESGCM(key_for(password, salt)).encrypt(nonce, data, MAGIC)
            output = MAGIC + salt + nonce + encrypted
        else:
            password = getpass.getpass("Artifact passphrase: ")
            raw = args.input.read_bytes()
            header = len(MAGIC) + SALT_BYTES + NONCE_BYTES
            if len(raw) < header + 16 or raw[:len(MAGIC)] != MAGIC:
                raise ValueError("not a supported Daytona Naomi encrypted artifact")
            salt = raw[len(MAGIC):len(MAGIC) + SALT_BYTES]
            nonce = raw[len(MAGIC) + SALT_BYTES:header]
            output = AESGCM(key_for(password, salt)).decrypt(nonce, raw[header:], MAGIC)

        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_bytes(output)
        print(f"Wrote {args.output} ({len(output)} bytes)")
    except Exception as exc:
        print(f"secure_artifact: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
