#!/usr/bin/env python3
"""Guard the conservative NAOMI SFX phase inventory against accidental omissions.

This is intentionally a source/manifest audit, not a claim that every runtime-generated
sound command has been classified. Dynamic race and results schedules remain explicitly
unresolved until checked against complete original-ROM traces.
"""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
RECIPE = ROOT / "recipes" / "sfx-banks.txt"
PRACTICE = ROOT / "code" / "practice.c"
RESULTS = ROOT / "code" / "gameplay" / "results.c"


def expand(spec: str) -> list[str]:
    if "-" not in spec:
        return [spec.lower()]
    first, last = (part.lower() for part in spec.split("-", 1))
    common = 0
    while common < min(len(first), len(last)) and first[common] == last[common]:
        common += 1
    if common == 0 or len(first) != len(last):
        raise ValueError(f"unsupported range: {spec}")
    lo, hi = int(first[common:], 16), int(last[common:], 16)
    if hi < lo:
        raise ValueError(f"descending range: {spec}")
    return [f"{first[:common]}{n:0{len(first) - common}x}" for n in range(lo, hi + 1)]


def main() -> int:
    banks: dict[str, set[str]] = {}
    owners: dict[str, set[str]] = {}
    shared = {*(f"e{n:x}" for n in range(10)), "9e11"}
    for number, raw in enumerate(RECIPE.read_text().splitlines(), 1):
        line = raw.split("#", 1)[0].strip()
        if not line:
            continue
        fields = line.split()
        if len(fields) < 3:
            raise SystemExit(f"{RECIPE}:{number}: expected BANK CODING KEY...")
        bank, coding, *specs = fields
        if coding not in {"pcm8", "adpcm"}:
            raise SystemExit(f"{RECIPE}:{number}: unsupported coding {coding}")
        target = banks.setdefault(bank, set())
        for spec in specs:
            for key in expand(spec):
                assigned = owners.setdefault(key, set())
                if assigned and key not in shared:
                    raise SystemExit(f"duplicate SFX key {key}: {', '.join(sorted(assigned))} and {bank}")
                assigned.add(bank)
                target.add(key)

    required_kit = {f"9e{n:02x}" for n in range(1, 0x2e)}
    missing = sorted(required_kit - owners.keys())
    if missing:
        raise SystemExit("9E effect kit has unassigned commands: " + ", ".join(missing))

    for key in (f"e{n:x}" for n in range(10)):
        if owners.get(key) != {"race", "select"}:
            raise SystemExit(f"engine layer {key} must be present in both select and race banks")
    if owners.get("9e12") != {"race"}:
        raise SystemExit("9E12 credit sound must remain assigned exactly once")
    if owners.get("9e11") != {"race", "select"}:
        raise SystemExit("9E11 crowd cheer must be available in both race and selection banks")
    if owners.get("ae2001") != {"select"}:
        raise SystemExit("AE2001 selection start cue must be in the select bank")
    if owners.get("ae2158") != {"select"}:
        raise SystemExit("AE2158 selection-to-race transition cue must be in the select bank")

    practice = PRACTICE.read_text().lower()
    results = RESULTS.read_text().lower()
    for literal in ("0x009e127f", "0x9e2d4f"):
        if literal not in practice:
            raise SystemExit(f"expected direct practice callsite disappeared: {literal}")
    for token in ("nm_result_entry_command", "nm_result_announcement_step", "0xae1002",
                  "out.command[i]"):
        if token not in results:
            raise SystemExit(f"expected dynamic results command path disappeared: {token}")

    print(f"PASS SFX inventory guard: {len(owners)} unique commands across {len(banks)} banks")
    print(f"  9E kit: {len(required_kit)} commands assigned; 9E12 currently in {owners['9e12']}")
    print(f"  9E11 crowd cheer: shared by {', '.join(sorted(owners['9e11']))}")
    print(f"  AE2001 selection start cue: isolated in {owners['ae2001']} bank")
    print(f"  AE2158 transition cue: isolated in {owners['ae2158']} bank")
    print("  E0-E9 engine controllers: present in select and race")
    print("  podium 9E2D and dynamic results entry/announcement paths: source callsites present")
    print("  NOTE: dynamic 9E/race/results command membership is not yet fully classified")
    return 0


if __name__ == "__main__":
    sys.exit(main())
