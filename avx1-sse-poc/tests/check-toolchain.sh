#!/usr/bin/env bash
set -euo pipefail
for tool in x86_64-linux-gnu-g++ x86_64-linux-gnu-objdump qemu-x86_64; do
  command -v "$tool"
  "$tool" --version 2>&1 | head -n 1 || true
done
printf '\nAvailable QEMU x86 CPU models/features:\n'
qemu-x86_64 -cpu help 2>&1 | sed -n '1,100p' || true
printf '\nCross root:\n'
ls -ld /usr/x86_64-linux-gnu
