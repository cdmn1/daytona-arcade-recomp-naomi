#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)
STUBS="$ROOT/platform/naomi/tests/host_stubs"
INCLUDES="$ROOT/platform/naomi/include"

cc -std=gnu11 -Wall -Wextra -Wformat=2 -Werror \
  -I"$STUBS" -I"$INCLUDES" -fsyntax-only \
  "$ROOT/platform/naomi/code/naomi_cart.c" \
  "$ROOT/platform/naomi/code/naomi_romfs.c"

cc -std=gnu11 -Wall -Wextra -Wformat=2 -Werror -D_arch_sub_naomi \
  -I"$STUBS" -I"$INCLUDES" -fsyntax-only \
  "$ROOT/platform/naomi/tests/input_probe/main.c"

cc -std=gnu11 -Wall -Wextra -Wformat=2 -Werror -D_arch_sub_naomi \
  -I"$STUBS" -I"$INCLUDES" -fsyntax-only \
  "$ROOT/platform/naomi/tests/cart_probe/main.c"

cc -std=gnu11 -Wall -Wextra -Wformat=2 -Werror \
  -I"$STUBS" -I"$INCLUDES" \
  "$ROOT/platform/naomi/tests/test_romfs_vfs.c" \
  "$ROOT/platform/naomi/code/naomi_romfs.c" \
  -o /tmp/naomi-romfs-vfs-test
/tmp/naomi-romfs-vfs-test

echo "Naomi storage source syntax and ROMFS VFS host tests passed (not a KOS target build)."
