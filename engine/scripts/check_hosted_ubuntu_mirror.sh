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
bash "$scripts/hosted_ubuntu_mirror.sh" "$scratch" > "$scratch/diagnostic.log"
cmp "$scratch/expected.list" "$scratch/sources.list"
cmp "$scratch/expected.sources" "$scratch/sources.list.d/ubuntu.sources"
cmp "$scratch/expected.third-party" "$scratch/sources.list.d/third-party.list"
bash "$scripts/hosted_ubuntu_mirror.sh" "$scratch" > "$scratch/diagnostic.log"
cmp "$scratch/expected.list" "$scratch/sources.list"
cmp "$scratch/expected.sources" "$scratch/sources.list.d/ubuntu.sources"
# The hosted runner resolves mirror+file sources through this known file.
mkdir -p "$scratch/indirect/sources.list.d"
cat > "$scratch/indirect/sources.list.d/ubuntu.sources" <<'SOURCES'
Types: deb
URIs: mirror+file:/etc/apt/apt-mirrors.txt
Suites: noble noble-updates noble-backports
Components: main restricted universe multiverse
Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg
SOURCES
printf '%s\t%s\n' 'http://azure.archive.ubuntu.com/ubuntu/' 'priority:1 arch:amd64' > "$scratch/indirect/apt-mirrors.txt"
printf '%s\t%s\n' 'https://security.ubuntu.com/ubuntu' 'suite:noble-security' >> "$scratch/indirect/apt-mirrors.txt"
printf '%s\t%s\n' 'https://mirror.example.invalid/ubuntu' 'priority:2' >> "$scratch/indirect/apt-mirrors.txt"
printf '%s\n' '# mirror metadata comment' >> "$scratch/indirect/apt-mirrors.txt"
cp "$scratch/indirect/sources.list.d/ubuntu.sources" "$scratch/indirect.expected.source"
printf '%s\t%s\n' 'http://archive.ubuntu.com/ubuntu/' 'priority:1 arch:amd64' > "$scratch/indirect.expected.mirrors"
printf '%s\t%s\n' 'https://security.ubuntu.com/ubuntu' 'suite:noble-security' >> "$scratch/indirect.expected.mirrors"
printf '%s\t%s\n' 'https://mirror.example.invalid/ubuntu' 'priority:2' >> "$scratch/indirect.expected.mirrors"
printf '%s\n' '# mirror metadata comment' >> "$scratch/indirect.expected.mirrors"
bash "$scripts/hosted_ubuntu_mirror.sh" "$scratch/indirect" > "$scratch/indirect.log"
grep -Fq 'updated 1 file(s)' "$scratch/indirect.log"
grep -Fq '/apt-mirrors.txt' "$scratch/indirect.log"
cmp "$scratch/indirect.expected.source" "$scratch/indirect/sources.list.d/ubuntu.sources"
cmp "$scratch/indirect.expected.mirrors" "$scratch/indirect/apt-mirrors.txt"
bash "$scripts/hosted_ubuntu_mirror.sh" "$scratch/indirect" > "$scratch/indirect.log"
grep -Fq 'no matching Azure Ubuntu archive URL; no files changed' "$scratch/indirect.log"
cmp "$scratch/indirect.expected.source" "$scratch/indirect/sources.list.d/ubuntu.sources"
cmp "$scratch/indirect.expected.mirrors" "$scratch/indirect/apt-mirrors.txt"
mkdir "$scratch/empty"
bash "$scripts/hosted_ubuntu_mirror.sh" "$scratch/empty" > "$scratch/empty.log"
grep -Fq 'no matching Azure Ubuntu archive URL; no files changed' "$scratch/empty.log"
printf 'PASS: legacy/deb822 and mirror+file exact URL rewrite with metadata/diagnostics, signature/suite/security preservation, unrelated sources, idempotence and absent sources\n'
