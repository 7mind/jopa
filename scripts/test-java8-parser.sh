#!/usr/bin/env bash
# Run JDK8 parser tests
# Usage: test-java8-parser.sh [--build]

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec "${SCRIPT_DIR}/test-parser.sh" jdk8 "$@"
