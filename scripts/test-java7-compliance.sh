#!/usr/bin/env bash
# Run JDK7 compliance tests
# Usage: test-java7-compliance.sh [--build]

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec "${SCRIPT_DIR}/test-compliance.sh" jdk7 "$@"
