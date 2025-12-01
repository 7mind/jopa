#!/usr/bin/env bash
# Wrapper script that runs JOPA and tracks peak memory usage.
# Used by CI to report memory consumption during devjopak builds.
#
# Environment variables:
#   JOPA_REAL_EXECUTABLE - Path to actual JOPA binary (required)
#   JOPA_MEMORY_LOG - File to track peak memory (default: /tmp/jopa-peak-memory.log)
#
# The wrapper writes peak RSS (in KB) to the log file, one line per invocation.
# After build, use: sort -n $JOPA_MEMORY_LOG | tail -1 to get max RSS.

set -euo pipefail

REAL_JOPA="${JOPA_REAL_EXECUTABLE:-}"
MEMLOG="${JOPA_MEMORY_LOG:-/tmp/jopa-peak-memory.log}"

if [ -z "$REAL_JOPA" ]; then
    echo "ERROR: JOPA_REAL_EXECUTABLE not set" >&2
    exit 1
fi

if [ ! -x "$REAL_JOPA" ]; then
    echo "ERROR: JOPA executable not found or not executable: $REAL_JOPA" >&2
    exit 1
fi

# Find GNU time (not the shell builtin)
TIME_CMD=""
if [ -x /usr/bin/time ]; then
    TIME_CMD="/usr/bin/time"
elif command -v gtime >/dev/null 2>&1; then
    TIME_CMD="gtime"
else
    # Fallback: just run JOPA without memory tracking
    exec "$REAL_JOPA" "$@"
fi

# Create temp file for time output
TIME_OUTPUT=$(mktemp)
trap "rm -f '$TIME_OUTPUT'" EXIT

# Run JOPA with GNU time to measure peak RSS
# -f '%M' outputs only the maximum RSS in KB
if "$TIME_CMD" -f '%M' -o "$TIME_OUTPUT" "$REAL_JOPA" "$@"; then
    EXIT_CODE=0
else
    EXIT_CODE=$?
fi

# Read peak RSS and append to log
PEAK_KB=$(cat "$TIME_OUTPUT" 2>/dev/null || echo "0")

if [[ "$PEAK_KB" =~ ^[0-9]+$ ]] && [ "$PEAK_KB" -gt 0 ]; then
    echo "$PEAK_KB" >> "$MEMLOG"
fi

exit $EXIT_CODE
