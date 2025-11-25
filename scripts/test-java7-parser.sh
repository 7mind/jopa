#!/usr/bin/env bash
# Run JDK7 parser tests
# Usage: test-java7-parser.sh [--build]

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec "${SCRIPT_DIR}/test-parser.sh" jdk7 "$@"
