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
  "$ROOT/platform/naomi/tests/cart_probe/main.c"

echo "Naomi storage source syntax checks passed (host stubs; not a KOS target build)."
