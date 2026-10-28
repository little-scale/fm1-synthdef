#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
archive="${1:-dependencies/jieli-linux-toolchain.tar.xz}"
expected=a1545cddbc451a06bac43eb5cd96d831470c42a97c5c0b1f98879a20fc7647bd
mkdir -p dependencies/jieli
if [[ ! -f "$archive" ]]; then
  if [[ $# -gt 0 ]]; then echo "Archive not found: $archive" >&2; exit 1; fi
  curl --fail --location --retry 3 https://pkgman.jieliapp.com/s/linux-toolchain -o "$archive"
fi
actual="$(shasum -a 256 "$archive" | cut -d ' ' -f 1)"
if [[ "$actual" != "$expected" ]]; then
  echo 'Compiler archive does not match the pinned SHA-256; nothing extracted.' >&2; exit 1
fi
if [[ -e dependencies/jieli/jieli-linux-toolchains-20260730.1 ]]; then
  echo 'Pinned compiler directory already exists; leaving it unchanged.'; exit 0
fi
tar -xJf "$archive" -C dependencies/jieli
[[ -d dependencies/jieli/jieli-linux-toolchains-20260730.1/pi32v2 ]]
echo 'Pinned compiler extracted.'
