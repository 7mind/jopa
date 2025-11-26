#!/usr/bin/env python3
"""
Prepare JDK parser test whitelists by running real javac on OpenJDK test files.

This script enumerates all .java files in the OpenJDK test directories and runs
javac on each to determine which files are valid for parser testing. Files that
compile successfully (or fail only due to missing dependencies) are whitelisted.

Usage:
    python scripts/prepare_jdk_parser_tests.py [--jdk7] [--jdk8] [--all]
"""

import argparse
import os
import subprocess
import sys
from concurrent.futures import ProcessPoolExecutor, as_completed
from dataclasses import dataclass
from pathlib import Path


PROJECT_ROOT = Path(__file__).parent.parent.resolve()

JDK_CONFIGS = {
    "jdk7": {
        "assets_dir": PROJECT_ROOT / "assets" / "jdk7u-langtools" / "test",
        "source_version": "1.7",
        "whitelist_file": PROJECT_ROOT / "test" / "jdk7_parser_whitelist.txt",
    },
    "jdk8": {
        "assets_dir": PROJECT_ROOT / "assets" / "jdk8u_langtools" / "test",
        "source_version": "1.8",
        "whitelist_file": PROJECT_ROOT / "test" / "jdk8_parser_whitelist.txt",
    },
}


@dataclass
class TestResult:
    file_path: Path
    success: bool
    reason: str


def find_java_files(assets_dir: Path) -> list[Path]:
    """Find all .java files in the assets directory."""
    return sorted(assets_dir.rglob("*.java"))


def check_file_with_javac(java_file: Path, source_version: str) -> TestResult:
    """
    Check if a Java file can be parsed by javac.

    We use -proc:none to skip annotation processing and -d with a temp dir
    to avoid writing class files. We only care about parse/syntax errors.
    """
    try:
        result = subprocess.run(
            [
                "javac",
                "-source", source_version,
                "-proc:none",
                "-implicit:none",
                "-XDshouldStopPolicyIfNoError=FLOW",
                "-Xlint:none",
                str(java_file),
            ],
            capture_output=True,
            text=True,
            timeout=30,
        )

        stderr = result.stderr

        # Parse errors indicate invalid syntax
        if "error:" in stderr:
            # Check if it's a parse/syntax error vs other errors
            error_lines = [line for line in stderr.split("\n") if "error:" in line]

            parse_error_indicators = [
                "illegal start",
                "expected",
                "unclosed",
                "reached end of file",
                "not a statement",
                "';' expected",
                "')' expected",
                "class, interface, or enum expected",
                "invalid method declaration",
                "<identifier> expected",
                "illegal character",
                "malformed",
                "unclosed string literal",
                "unclosed character literal",
            ]

            has_parse_error = any(
                indicator in line.lower()
                for line in error_lines
                for indicator in parse_error_indicators
            )

            if has_parse_error:
                return TestResult(java_file, False, "parse_error")

            # Symbol not found, package not found, etc. are OK for parser tests
            return TestResult(java_file, True, "dependency_error_ok")

        return TestResult(java_file, True, "compiles")

    except subprocess.TimeoutExpired:
        return TestResult(java_file, False, "timeout")
    except Exception as e:
        return TestResult(java_file, False, f"error: {e}")


def process_jdk_version(jdk_version: str, parallel: int) -> tuple[int, int]:
    """Process all Java files for a JDK version and create whitelist."""
    config = JDK_CONFIGS[jdk_version]
    assets_dir = config["assets_dir"]
    source_version = config["source_version"]
    whitelist_file = config["whitelist_file"]

    if not assets_dir.exists():
        print(f"Error: Assets directory not found: {assets_dir}", file=sys.stderr)
        return 0, 0

    print(f"Processing {jdk_version.upper()} tests from {assets_dir}")

    java_files = find_java_files(assets_dir)
    total_count = len(java_files)

    print(f"Found {total_count} .java files")
    print(f"Checking with javac -source {source_version}...")

    whitelisted_files: list[Path] = []
    failed_count = 0

    with ProcessPoolExecutor(max_workers=parallel) as executor:
        futures = {
            executor.submit(check_file_with_javac, f, source_version): f
            for f in java_files
        }

        completed = 0
        for future in as_completed(futures):
            completed += 1
            if completed % 500 == 0 or completed == total_count:
                print(f"  Progress: {completed}/{total_count}", flush=True)

            result = future.result()
            if result.success:
                whitelisted_files.append(result.file_path)
            else:
                failed_count += 1

    # Sort for deterministic output
    whitelisted_files.sort()

    # Write whitelist file with paths relative to assets directory
    with open(whitelist_file, "w") as f:
        for java_file in whitelisted_files:
            rel_path = java_file.relative_to(assets_dir)
            f.write(f"{rel_path}\n")

    whitelist_count = len(whitelisted_files)
    print(f"Created {whitelist_file}")
    print(f"  Total: {total_count}, Whitelisted: {whitelist_count}")
    print()

    return total_count, whitelist_count


def main():
    parser = argparse.ArgumentParser(
        description="Prepare JDK parser test whitelists by validating with javac"
    )
    parser.add_argument("--jdk7", action="store_true", help="Process JDK7 tests")
    parser.add_argument("--jdk8", action="store_true", help="Process JDK8 tests")
    parser.add_argument("--all", action="store_true", help="Process all JDK versions")
    parser.add_argument(
        "-j", "--parallel",
        type=int,
        default=os.cpu_count() or 4,
        help="Number of parallel processes (default: number of CPUs)"
    )

    args = parser.parse_args()

    # Default to --all if nothing specified
    if not (args.jdk7 or args.jdk8 or args.all):
        args.all = True

    versions_to_process = []
    if args.all:
        versions_to_process = ["jdk7", "jdk8"]
    else:
        if args.jdk7:
            versions_to_process.append("jdk7")
        if args.jdk8:
            versions_to_process.append("jdk8")

    # Check javac is available
    try:
        result = subprocess.run(["javac", "-version"], capture_output=True, text=True)
        print(f"Using: {result.stderr.strip() or result.stdout.strip()}")
        print()
    except FileNotFoundError:
        print("Error: javac not found in PATH", file=sys.stderr)
        sys.exit(1)

    for version in versions_to_process:
        process_jdk_version(version, args.parallel)

    print("Done!")


if __name__ == "__main__":
    main()
