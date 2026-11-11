# Daytona USA — Naomi port

This directory contains Naomi-specific integration and bring-up work for the
native SH-4 game in `platform/dreamcast-native`.

## Target and build environment

- Naomi 1, SH-4, KallistiOS Naomi sub-architecture.
- Set `KOS_SUBARCH=naomi` **before** sourcing the Naomi KOS environment.
- Use a KOS build that includes Naomi/NetDIMM support (KallistiOS 2.1.x or a
  later compatible revision).
- Keep the 256 MB NetDIMM as the minimum supported deployment target; the
  512 MB DIMM may be used for optional content.

Example environment setup (adjust the KOS path to the installed Naomi tree):

```sh
export KOS_SUBARCH=naomi
source /opt/toolchains/dc/kos-naomi/environ.sh
```

The exact KOS path is installation-specific. Do not source a Dreamcast-only
KOS tree and assume that changing the variable afterwards retargets its
already-built libraries. The standalone input probe does not need enDjinn,
game assets, or AICAflow.

## First bring-up test

Build the standalone KOS input probe:

```sh
make -C platform/naomi/tests/input_probe clean all
```

The Makefile refuses to build unless the active environment explicitly has
`KOS_SUBARCH=naomi` and `KOS_BASE` set. The output is
`platform/naomi/tests/input_probe/naomi_input_probe.elf`. Package it as a
Naomi ROM image with:

```sh
make -C platform/naomi/tests/input_probe rom
```

This creates `naomi_input_probe.bin` using KOS's `naomibintool`, ready for
the existing NetDIMM uploader. If the tool has not been built yet, the
Makefile builds it with the host compiler; Linux/WSL may need `libelf-dev`.

It reports:
- the Naomi MIE/JVS bridge and its mapped controller state;
- native JVS wheel, accelerator, brake and switch values;
- ordinary Maple controllers, independently of JVS.

The probe is deliberately independent of game assets, enDjinn and AICAflow.
It is a ROM-free test of KOS startup and input discovery. Its successful
compilation does not prove Flycast or physical-cabinet operation; those need
separate runs.

## ROM-board / NetDIMM read probe

Build and package the second diagnostic:

```sh
make -C platform/naomi/tests/cart_probe clean all
make -C platform/naomi/tests/cart_probe rom
```

This produces `naomi_cart_probe.bin`. It reads the Naomi ROM header through
the ROM-board PIO registers, mounts the appended test ROMFS at `/naomi/`,
and opens `/naomi/probe.txt`. A successful run prints the known payload,
verifying both NetDIMM PIO reads and the read-only KOS VFS end to end.

## Input integration

KOS exposes Naomi JVS through `MAPLE_FUNC_MIE`, not as an ordinary
`MAPLE_FUNC_CONTROLLER` device. enDjinn's pinned controller mapper originally
enumerated only ordinary controller devices. The patch in
`patches/endjinn-mie-input.patch` makes the MIE device's already-mapped
`mie_state_t.cont` visible through enDjinn's existing normalized controller
state slot. Standard Maple gamepads remain supported.

The patch is applied by
`platform/dreamcast-native/scripts/fetch_endjinn.sh` after checking out the
pinned enDjinn revision. It is kept in this repository so the dependency
modification is reviewable and repeatable.

The game still needs a Daytona-specific JVS mapping decision for wheel,
accelerator, brake, start, service and coin inputs. The KOS default MIE mapper
already converts wheel and pedals to `cont_state_t`; this is a compatibility
starting point, not yet a claim of arcade-correct calibration.

## NetDIMM and game assets

A NetDIMM is treated as the resident disc-image source: the game image is
uploaded to DIMM RAM and the Naomi executes from that image. Do not treat DIMM
RAM as Naomi main RAM, and do not introduce a PC share or a separate asset
server as a runtime dependency.

KOS's documented `/cd` ISO9660 filesystem is Dreamcast-only. Therefore the
Dreamcast port's `NATIVE_DISC=1` and `/cd/` path is **not** assumed to work on
Naomi. The Naomi ROM-board PIO access path is now identified and has an initial
reader. A read-only KOS VFS now mounts the appended ROMFS at `/naomi/` before
the game starts, so existing `fopen`/read/seek-based asset loaders can read
files directly from the resident image.

KOS also supplies `naomibintool`, which wraps a Naomi-targeted SH-4 ELF in
the Naomi ROM header format to produce the uploadable `.bin`. Its current
implementation packs one ELF image as one load segment; it is a ROM-image
builder, not a general-purpose asset filesystem or a way to map a large game
archive into Naomi main RAM.

The PIO reader in `code/naomi_cart.c` reads bounded 1 KiB blocks without
copying the whole image into main RAM. `code/naomi_romfs.c` implements a
read-only KOS VFS at `/naomi/`; it supports file open/read/seek/stat and
keeps a small per-file cache. Directory enumeration and writes are not
implemented.

The ROM image build is now wired into `platform/dreamcast-native/Makefile`.
With the Naomi KOS environment and the existing game assets configured, run:

```sh
make -C platform/dreamcast-native naomi-bin
```

This builds the ELF, wraps it with KOS `naomibintool`, creates a ROMFS from
`ROMBASEPATH`, and appends it after the executable segment. The output is
`platform/dreamcast-native/bin/*.naomi.bin`, ready for the existing NetDIMM
uploader.

This initial PIO implementation addresses the low 256 MiB only (the register
address field is 28 bits). A 512 MB image needs a separate DMA-based reader if
assets extend beyond that range. The VFS, PIO reads, and final image still
need real Naomi KOS compilation plus Flycast and hardware validation.

Do not silently fall back to
`/pc/`, `/cd/`, network filesystems, or a host-mounted directory.

## Current status

- [x] Naomi KOS target confirmed in KOS documentation/source.
- [x] KOS MIE/JVS API and standard Maple controller API identified and checked against current KOS headers.
- [x] Input probe Makefile rejects non-Naomi KOS environments.
- [x] enDjinn MIE-to-controller compatibility patch prepared.
- [x] ROM-free input probe source and Makefile added.
- [x] Naomi ROM-board PIO read primitive and ROM-header diagnostic added.
- [x] Read-only ROMFS VFS source added and mounted at `/naomi/` in Naomi builds.
- [x] ROMFS generator, Naomi image appender, and host-side format tests added.
- [x] Naomi product build target added (`make naomi-bin`).
- [x] Host C syntax checks passed for the PIO reader and cart diagnostic using minimal KOS stubs; the VFS has not yet been compiled.
- [ ] Run ROMFS tooling tests in GitHub Actions.
- [ ] Build both probes and the full game with a real Naomi KOS toolchain (not yet compiled in this environment).
- [ ] Verify PIO reads in Flycast and on Naomi 1 with a NetDIMM.
- [ ] Run probe in Flycast.
- [ ] Run probe on Naomi 1 with JVS cabinet and with a standard Maple controller.
- [ ] Add Naomi product build and validate PVR/AICA startup.
- [ ] Integrate fixed 57.52 Hz simulation cadence and verify on hardware.
- [ ] Implement Naomi save/settings backend (or explicitly disable saving initially).

No Naomi ELF, BIN, DAT or NetDIMM upload has been produced by this commit.
