#!/bin/bash
# Wrapper for zip that filters out non-existent directories
# This is needed because GNU Classpath's Makefile passes directories like 'sun'
# to zip even when they don't exist. jar silently ignores missing args, zip doesn't.

ZIP_COMMAND="$1"
shift

# Find the output file (first non-flag argument after -r -D)
args=()
output_file=""
skip_next=false
for arg in "$@"; do
    if [ "$skip_next" = true ]; then
        skip_next=false
        args+=("$arg")
        continue
    fi

    case "$arg" in
        -*)
            args+=("$arg")
            ;;
        *)
            if [ -z "$output_file" ]; then
                output_file="$arg"
                args+=("$arg")
            elif [ -e "$arg" ]; then
                args+=("$arg")
            fi
            ;;
    esac
done

exec "$ZIP_COMMAND" "${args[@]}"
