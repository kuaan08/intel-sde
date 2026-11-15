#!/usr/bin/env bash
# SDE AVX Lab — non-invasive audit of an Intel SDE distribution.
# This script does not modify, patch, strip, disassemble, or redistribute kit binaries.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
KIT_ROOT="${KIT_ROOT:-$REPO_ROOT/windows-9.58}"
QEMU_BIN="${QEMU_BIN:-qemu-x86_64}"
QEMU_LD_PREFIX="${QEMU_LD_PREFIX:-/usr/x86_64-linux-gnu}"
RUNNER=()

if [[ ! -d "$KIT_ROOT" ]]; then
  echo "ERROR: kit root does not exist: $KIT_ROOT" >&2
  exit 2
fi

host_machine="$(uname -m)"
if [[ "$host_machine" == x86_64 || "$host_machine" == amd64 ]]; then
  RUNNER=()
elif command -v "$QEMU_BIN" >/dev/null 2>&1; then
  RUNNER=("$QEMU_BIN" -L "$QEMU_LD_PREFIX")
else
  echo "ERROR: host is $host_machine and $QEMU_BIN is unavailable" >&2
  exit 2
fi

run_guest() { "${RUNNER[@]}" "$@"; }
require_file() { [[ -f "$KIT_ROOT/$1" ]] || { echo "MISSING $KIT_ROOT/$1" >&2; exit 1; }; }

printf 'SDE kit root: %s\n' "$KIT_ROOT"
printf 'Host architecture: %s\n' "$host_machine"
if [[ ${#RUNNER[@]} -eq 0 ]]; then
  printf 'Execution mode: native x86-64\n'
else
  printf 'Execution mode: qemu-x86_64\n'
fi

if [[ -f "$KIT_ROOT/sde.exe" ]]; then
  ARCH_LABEL=Windows
  SDE_BIN="$KIT_ROOT/sde.exe"
  XED_BIN="$KIT_ROOT/xed.exe"
  PIN_BIN="$KIT_ROOT/intel64/pin.exe"
  MIX_BIN="$KIT_ROOT/intel64/sde-mix-mt.dll"
else
  ARCH_LABEL=Linux
  SDE_BIN="$KIT_ROOT/sde64"
  XED_BIN="$KIT_ROOT/xed64"
  PIN_BIN="$KIT_ROOT/intel64/pinbin"
  MIX_BIN="$KIT_ROOT/intel64/sde-mix-mt.so"
fi

if [[ "$ARCH_LABEL" == Windows ]]; then
  printf 'Kit platform: Windows x64/ia32 distribution (inspection only)\n'
  printf 'Windows execution is not attempted on Linux.\n'
  for f in sde.exe xed.exe intel64/pin.exe intel64/sde-mix-mt.dll Licenses/LICENSE.pdf README.txt; do require_file "$f"; done
  printf 'Essential Windows files: PASS\n'
  printf '\n--- version metadata ---\n'
  strings -a "$SDE_BIN" | grep -E 'Intel\(R\) Software Development Emulator\.  Version:|Copyright' | head -4 || true
  strings -a "$XED_BIN" | grep -E 'XED version:|Copyright' | head -4 || true
  printf '\n--- Windows profiles ---\n'
  find "$KIT_ROOT/misc/cpuid" -mindepth 1 -maxdepth 1 -type d -printf '%f\n' | sort | tr '\n' ' '; printf '\n'
else
  for f in sde64 sde64.sig xed64 xed64.sig intel64/pinbin intel64/pinbin.sig \
           intel64/xed_lib/libxed.so intel64/xed_lib/libxed.so.sig \
           misc/cpuid/snb/cpuid.def misc/cpuid/glm/cpuid.def; do
    require_file "$f"
  done
  printf 'Essential Linux files: PASS\n'
  printf '\n--- version ---\n'
  run_guest "$SDE_BIN" -version
  run_guest "$XED_BIN" -version
  printf '\n--- Linux profiles ---\n'
  find "$KIT_ROOT/misc/cpuid" -mindepth 1 -maxdepth 1 -type d -printf '%f\n' | sort | tr '\n' ' '; printf '\n'
fi

printf '\n--- signed artifacts / certificates ---\n'
for f in $(find "$KIT_ROOT" -maxdepth 3 -type f -name '*.sig' | sort | head -20); do
  certs="$(openssl pkcs7 -inform DER -in "$f" -print_certs -noout 2>/dev/null || true)"
  if grep -q 'Intel Corporation' <<<"$certs"; then
    printf '%-60s Intel certificate chain: PASS\n' "${f#$REPO_ROOT/}"
  fi
done

printf '\n--- principal artifact hashes ---\n'
sha256sum "$SDE_BIN" "$XED_BIN" "$PIN_BIN" "$MIX_BIN" 2>/dev/null || true

printf '\nAUDIT RESULT: PASS (read-only checks only)\n'
printf 'This result does not validate Windows runtime execution, application compatibility, AVX translation, or gameplay.\n'
