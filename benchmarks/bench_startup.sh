#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/.."

SANDBOX="./bin/sandbox"
ITERATIONS="${ITERATIONS:-1000}"

if [ ! -x "$SANDBOX" ]; then
    echo "Sandbox binary $SANDBOX not found. Please run 'make' first."
    exit 1
fi

TIMER_SRC="benchmarks/bench_timer.c"
TIMER_BIN="benchmarks/bench_timer"

# Compile precision C benchmark timer if compiler is available
if command -v gcc >/dev/null 2>&1; then
    gcc -O2 "$TIMER_SRC" -o "$TIMER_BIN" -lm 2>/dev/null || true
fi

if [ -x "$TIMER_BIN" ]; then
    exec "$TIMER_BIN" "$ITERATIONS"
fi

# Fallback POSIX shell timing if C timer could not be compiled
echo "C timer binary unavailable; running shell fallback benchmark..."

start_native=$(date +%s%N 2>/dev/null || python3 -c 'import time; print(int(time.time()*1e9))')
for ((i=1; i<=100; i++)); do
    /bin/true
done
end_native=$(date +%s%N 2>/dev/null || python3 -c 'import time; print(int(time.time()*1e9))')
native_total_ns=$((end_native - start_native))
native_avg_us=$((native_total_ns / 100 / 1000))

start_sandbox=$(date +%s%N 2>/dev/null || python3 -c 'import time; print(int(time.time()*1e9))')
for ((i=1; i<=100; i++)); do
    $SANDBOX -- /bin/true
done
end_sandbox=$(date +%s%N 2>/dev/null || python3 -c 'import time; print(int(time.time()*1e9))')
sandbox_total_ns=$((end_sandbox - start_sandbox))
sandbox_avg_us=$((sandbox_total_ns / 100 / 1000))

overhead_us=$((sandbox_avg_us - native_avg_us))
if [ "$overhead_us" -lt 0 ]; then overhead_us=0; fi

echo -e "\nResults (100 iterations):"
echo -e "  - Native /bin/true average latency:    ${native_avg_us} µs"
echo -e "  - Sandbox /bin/true average latency:   ${sandbox_avg_us} µs"
echo -e "  - Sandbox initialization overhead:     ${overhead_us} µs"
