#!/bin/sh
# Checks the HTTP endpoints of a running epaper board.
# Usage: ./test_endpoints.sh [host]   (default host: epaper.local)
# Triggers three panel refreshes of about 10 s each and leaves slot 1 empty
# and the notes cleared.
host="${1:-epaper.local}"
base="http://$host"
work="$(mktemp -d)"
failures=0

# Compares an actual value to the expected one and reports the result
check() {
  if [ "$2" = "$3" ]; then
    echo "ok   $1"
  else
    echo "FAIL $1: expected '$3', got '$2'"
    failures=$((failures + 1))
  fi
}

# Prints only the HTTP status of a request
status() {
  curl -s -o /dev/null -w '%{http_code}' --max-time 40 "$@"
}

# Test image: top half solid black, bottom half solid red
python3 -c "import sys; sys.stdout.buffer.write(b'\xff'*7500 + b'\x00'*7500 + b'\x00'*7500 + b'\xff'*7500)" > "$work/image.bin"
head -c 100 "$work/image.bin" > "$work/short.bin"
head -c 40000 /dev/zero > "$work/long.bin"

check "page is served" "$(status "$base/")" 200
check "short image is refused" "$(status -F "image=@$work/short.bin" "$base/slot/1")" 400
check "long image is refused" "$(status -F "image=@$work/long.bin" "$base/slot/1")" 400
check "unknown slot is refused" "$(status "$base/slot/4")" 404
check "too long notes are refused" "$(status --data-urlencode "text=$(head -c 500 /dev/zero | tr '\0' 'x')" "$base/notes")" 400

check "notes are saved" "$(status --data-urlencode 'text=Test note one
Test note two' "$base/notes")" 200
check "notes read back" "$(curl -s --max-time 40 "$base/notes" | head -1)" "Test note one"

check "image is saved" "$(status -F "image=@$work/image.bin" "$base/slot/1")" 200
curl -s --max-time 40 -o "$work/back.bin" "$base/slot/1"
check "image reads back identical" "$(cmp -s "$work/image.bin" "$work/back.bin" && echo same)" same

check "image is removed" "$(status -X DELETE "$base/slot/1")" 200
check "removed image is gone" "$(status "$base/slot/1")" 404
check "notes are cleared" "$(status --data-urlencode 'text=' "$base/notes")" 200

# Settings: change both, read back, then restore what was there
before="$(curl -s --max-time 40 "$base/settings")"
check "zero minutes are refused" "$(status -d 'minutes=0&screens=2' "$base/settings")" 400
check "five screens are refused" "$(status -d 'minutes=5&screens=5' "$base/settings")" 400
check "settings are saved" "$(status -d 'minutes=7&screens=2' "$base/settings")" 200
check "settings read back" "$(curl -s --max-time 40 "$base/settings")" '{"minutes":7,"screens":2}'
restore="$(echo "$before" | sed -E 's/.*"minutes":([0-9]+),"screens":([0-9]+).*/minutes=\1\&screens=\2/')"
check "settings are restored" "$(status -d "$restore" "$base/settings")" 200
check "restored settings read back" "$(curl -s --max-time 40 "$base/settings")" "$before"

rm -rf "$work"
echo "$failures failed"
[ "$failures" -eq 0 ]
