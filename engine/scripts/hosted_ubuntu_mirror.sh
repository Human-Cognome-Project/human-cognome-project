#!/usr/bin/env bash
# Hosted Ubuntu runners only; optional APT root supports isolated fixture tests.
set -Eeuo pipefail
apt_root=${1:-/etc/apt}
shopt -s nullglob
for source in "$apt_root/sources.list" "$apt_root"/sources.list.d/*.list "$apt_root"/sources.list.d/*.sources; do
  [[ -f "$source" ]] || continue
  # Change only the known Ubuntu archive URL. Preserve suites, components,
  # Signed-By, security.ubuntu.com and unrelated repositories verbatim.
  sed -i -E 's#(https?://)azure\.archive\.ubuntu\.com/ubuntu([/[:space:]]|$)#\1archive.ubuntu.com/ubuntu\2#g' "$source"
done
printf 'Hosted APT sources: Ubuntu Azure archive URLs normalized to archive.ubuntu.com\n'
