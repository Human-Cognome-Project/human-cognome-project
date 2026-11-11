#!/usr/bin/env bash
# Hosted Ubuntu runners only; optional APT root supports isolated fixture tests.
set -Eeuo pipefail
apt_root=${1:-/etc/apt}
shopt -s nullglob
changed=0
pattern='(https?://)azure\.archive\.ubuntu\.com/ubuntu([/[:space:]]|$)'
for source in "$apt_root/apt-mirrors.txt" "$apt_root/sources.list" "$apt_root"/sources.list.d/*.list "$apt_root"/sources.list.d/*.sources; do
  [[ -f "$source" ]] || continue
  # Change only the known Ubuntu archive URL. Preserve suites, components,
  # Signed-By, security.ubuntu.com and unrelated repositories verbatim.
  if ! grep -Eq "$pattern" "$source"; then continue; fi
  sed -i -E 's#(https?://)azure\.archive\.ubuntu\.com/ubuntu([/[:space:]]|$)#\1archive.ubuntu.com/ubuntu\2#g' "$source"
  changed=$((changed + 1))
  printf 'Replaced Azure Ubuntu archive URL in %s\n' "$source"
done
if (( changed )); then
  printf 'Hosted APT: updated %s file(s) to archive.ubuntu.com\n' "$changed"
else
  printf 'Hosted APT: no matching Azure Ubuntu archive URL; no files changed\n'
fi
