#!/usr/bin/env bash
# Test runner for ouroc0.
#
#   tests/run.sh path/to/ouroc0
#
# tests/cases/NAME.ou   compiled and (on Linux x86-64) executed; stdout must
#                       match NAME.out, stdin comes from NAME.in if present and
#                       the exit status must match NAME.code (default 0).
# tests/errors/NAME.ou  must fail to compile with status 1; the first line of
#                       stderr must equal the first line of NAME.err.
#
# On other hosts the generated ELF files are only checked, not run.
set -u

OUROC=${1:?usage: run.sh path/to/ouroc0}
DIR=$(cd "$(dirname "$0")" && pwd)
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

can_run=0
if [ "$(uname -s)" = Linux ] && [ "$(uname -m)" = x86_64 ]; then
  can_run=1
fi

pass=0
fail=0
failed() { echo "FAIL: $1"; fail=$((fail + 1)); }

for src in "$DIR"/cases/*.ou; do
  [ -e "$src" ] || continue
  name=$(basename "$src" .ou)
  exe=$TMP/$name

  if ! "$OUROC" < "$src" > "$exe" 2> "$TMP/$name.err"; then
    failed "$name: compile error: $(head -n 1 "$TMP/$name.err")"
    continue
  fi
  magic=$(head -c 4 "$exe" | od -An -tx1 | tr -d ' \n')
  if [ "$magic" != 7f454c46 ]; then
    failed "$name: output is not an ELF file"
    continue
  fi

  if [ "$can_run" = 1 ]; then
    chmod +x "$exe"
    input=/dev/null
    [ -f "$DIR/cases/$name.in" ] && input=$DIR/cases/$name.in
    want_code=0
    [ -f "$DIR/cases/$name.code" ] && want_code=$(cat "$DIR/cases/$name.code")

    "$exe" < "$input" > "$TMP/$name.actual"
    code=$?
    if ! cmp -s "$TMP/$name.actual" "$DIR/cases/$name.out"; then
      failed "$name: unexpected output"
      diff "$DIR/cases/$name.out" "$TMP/$name.actual" | head -n 10
      continue
    fi
    if [ "$code" != "$want_code" ]; then
      failed "$name: exit status $code, expected $want_code"
      continue
    fi
  fi
  pass=$((pass + 1))
done

for src in "$DIR"/errors/*.ou; do
  [ -e "$src" ] || continue
  name=$(basename "$src" .ou)

  "$OUROC" < "$src" > /dev/null 2> "$TMP/$name.err"
  code=$?
  got=$(head -n 1 "$TMP/$name.err" | tr -d '\r')
  want=$(head -n 1 "$DIR/errors/$name.err" | tr -d '\r')
  if [ "$code" != 1 ]; then
    failed "$name: exit status $code, expected 1"
  elif [ "$got" != "$want" ]; then
    failed "$name: message mismatch"
    echo "  expected: $want"
    echo "  actual:   $got"
  else
    pass=$((pass + 1))
  fi
done

echo "passed: $pass, failed: $fail"
[ "$fail" = 0 ]
