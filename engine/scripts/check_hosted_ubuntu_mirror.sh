#!/usr/bin/env bash
set -Eeuo pipefail
scripts=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
scratch=$(mktemp -d)
trap 'rm -rf "$scratch"' EXIT
mkdir -p "$scratch/sources.list.d"
cat > "$scratch/sources.list" <<'SOURCES'
deb [signed-by=/usr/share/keyrings/ubuntu-archive-keyring.gpg] http://azure.archive.ubuntu.com/ubuntu noble main universe
deb https://security.ubuntu.com/ubuntu noble-security main
deb https://azure.archive.ubuntu.com.example/ubuntu noble main
SOURCES
cat > "$scratch/sources.list.d/ubuntu.sources" <<'SOURCES'
Types: deb
URIs: https://azure.archive.ubuntu.com/ubuntu/
Suites: noble noble-updates noble-backports
Components: main restricted universe multiverse
Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg
SOURCES
cp "$scratch/sources.list" "$scratch/expected.list"
cp "$scratch/sources.list.d/ubuntu.sources" "$scratch/expected.sources"
sed -i 's#http://azure.archive.ubuntu.com/ubuntu noble#http://archive.ubuntu.com/ubuntu noble#' "$scratch/expected.list"
sed -i 's#https://azure.archive.ubuntu.com/ubuntu/#https://archive.ubuntu.com/ubuntu/#' "$scratch/expected.sources"
printf 'deb https://packages.example.invalid/repo stable main\n' > "$scratch/sources.list.d/third-party.list"
cp "$scratch/sources.list.d/third-party.list" "$scratch/expected.third-party"
bash "$scripts/hosted_ubuntu_mirror.sh" "$scratch"
cmp "$scratch/expected.list" "$scratch/sources.list"
cmp "$scratch/expected.sources" "$scratch/sources.list.d/ubuntu.sources"
cmp "$scratch/expected.third-party" "$scratch/sources.list.d/third-party.list"
bash "$scripts/hosted_ubuntu_mirror.sh" "$scratch"
cmp "$scratch/expected.list" "$scratch/sources.list"
cmp "$scratch/expected.sources" "$scratch/sources.list.d/ubuntu.sources"
mkdir "$scratch/empty"
bash "$scripts/hosted_ubuntu_mirror.sh" "$scratch/empty"
printf 'PASS: legacy/deb822 exact URL rewrite, signature/suite/security preservation, unrelated sources, idempotence and absent sources\n'
