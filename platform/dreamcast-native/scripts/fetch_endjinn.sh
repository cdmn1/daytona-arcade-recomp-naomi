#!/usr/bin/env sh
# Fetch enDjinn (Dreamcast game framework on KOS: base_link.mk, environ.sh, the deferred render queue) at the
# pinned commit into extern/endjinn (git-ignored). MIT. platform/dreamcast-native's Makefile and recipes/build_assets.sh
# use this checkout when ENDJINN_ROOT is unset; an explicit ENDJINN_ROOT still wins.
# 725f8b1 was origin main on 28 October 2026.
set -eu
ENDJINN_COMMIT=725f8b192c18ba56298eca1b5eaf8abc216e67be
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd -P)"
DEST="$ROOT/extern/endjinn"
[ -d "$DEST/.git" ] || git clone https://github.com/dfchil/enDjinn.git "$DEST"
git -C "$DEST" fetch origin 2>/dev/null || true
git -C "$DEST" checkout --detach "$ENDJINN_COMMIT"

# Naomi: expose KOS's MIE/JVS mapped controller state through enDjinn's
# existing normalized controller slots. Keep the patch in this repository.
PATCH="$ROOT/platform/naomi/patches/endjinn-mie-input.patch"
if git -C "$DEST" apply --check "$PATCH" >/dev/null 2>&1; then
    git -C "$DEST" apply "$PATCH"
elif ! git -C "$DEST" apply --reverse --check "$PATCH" >/dev/null 2>&1; then
    echo "Cannot apply or verify the Naomi MIE input patch: $PATCH" >&2
    exit 1
fi
