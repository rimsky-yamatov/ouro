#!/usr/bin/env bash
# Generate Markdown release notes for a tag from the commit history.
#
#   scripts/release-notes.sh v0.1.0
#
# Commits are grouped by their Conventional Commits prefix.
set -eu

tag=${1:?usage: release-notes.sh <tag>}
repo=${GITHUB_REPOSITORY:-rimsky-yamatov/ouro}
server=${GITHUB_SERVER_URL:-https://github.com}

prev=$(git describe --tags --abbrev=0 --match 'v*' "${tag}^" 2>/dev/null || true)
if [ -n "$prev" ]; then
  range="$prev..$tag"
else
  range="$tag"
fi

log=$(git log --no-merges --reverse --pretty='format:%s (%h)' "$range")

section() {
  local title=$1 pattern=$2 lines
  lines=$(printf '%s\n' "$log" | grep -E "$pattern" || true)
  [ -n "$lines" ] || return 0
  printf '### %s\n\n' "$title"
  printf '%s\n' "$lines" | sed 's/^/- /'
  printf '\n'
}

other() {
  local lines
  lines=$(printf '%s\n' "$log" | grep -Ev '^(feat|fix|perf|refactor|docs|test|build|ci|chore)(\([^)]*\))?!?: ' || true)
  [ -n "$lines" ] || return 0
  printf '### Other\n\n'
  printf '%s\n' "$lines" | sed 's/^/- /'
  printf '\n'
}

cat <<NOTES
# Ouro ${tag}

\`ouroc0\` is the bootstrap compiler for the Ouro language, written in C.
It reads an Ouro program on standard input and writes a static x86-64 Linux
ELF executable to standard output. It is a cross compiler: every build below
emits the same Linux executables, whatever system it runs on.

\`\`\`
ouroc0 < prog.ou > prog && chmod +x prog && ./prog
\`\`\`

## Downloads

| OS | CPU | File |
|---|---|---|
| Linux (static) | x86-64 | \`ouroc0-${tag}-linux-x86_64.tar.gz\` |
| Linux (static) | arm64 | \`ouroc0-${tag}-linux-aarch64.tar.gz\` |
| macOS | Intel | \`ouroc0-${tag}-macos-x86_64.tar.gz\` |
| macOS | Apple silicon | \`ouroc0-${tag}-macos-arm64.tar.gz\` |
| Windows | x86-64 | \`ouroc0-${tag}-windows-x86_64.zip\` |
| Windows | arm64 | \`ouroc0-${tag}-windows-arm64.zip\` |

Verify downloads with \`SHA256SUMS.txt\`. Each archive also contains the license,
the README and the language specification (\`docs/spec.md\`).

## Changes

NOTES

section "Features" '^feat(\([^)]*\))?!?: '
section "Fixes" '^fix(\([^)]*\))?!?: '
section "Performance and refactoring" '^(perf|refactor)(\([^)]*\))?!?: '
section "Documentation" '^docs(\([^)]*\))?!?: '
section "Tests" '^test(\([^)]*\))?!?: '
section "Build and CI" '^(build|ci|chore)(\([^)]*\))?!?: '
other

if [ -n "$prev" ]; then
  echo "**Full changelog**: ${server}/${repo}/compare/${prev}...${tag}"
else
  echo "**Full changelog**: ${server}/${repo}/commits/${tag}"
fi
