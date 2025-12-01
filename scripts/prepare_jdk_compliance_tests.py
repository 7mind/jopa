#!/usr/bin/env python3
"""
Prepare JDK compliance test whitelists by running real javac on OpenJDK test files.

This script creates whitelists for compliance testing by:
1. Finding all .java files with @compile or @run annotations (NOT @compile/fail)
2. Running javac to verify they compile successfully
3. Creating whitelists of files suitable for compliance testing

Usage:
    python scripts/prepare_jdk_compliance_tests.py [--jdk7] [--jdk8] [--all]
"""

import argparse
import os
import re
import subprocess
import sys
import tempfile
from concurrent.futures import ProcessPoolExecutor, as_completed
from dataclasses import dataclass
from pathlib import Path


PROJECT_ROOT = Path(__file__).parent.parent.resolve()

JDK_CONFIGS = {
    "jdk7": {
        "assets_dir": PROJECT_ROOT / "assets" / "jdk7u-langtools" / "test",
        "source_version": "1.7",
        "whitelist_file": PROJECT_ROOT / "test" / "jdk7_compliance_whitelist.txt",
    },
    "jdk8": {
        "assets_dir": PROJECT_ROOT / "assets" / "jdk8u_langtools" / "test",
        "source_version": "1.8",
        "whitelist_file": PROJECT_ROOT / "test" / "jdk8_compliance_whitelist.txt",
    },
}

# Annotations that indicate the test should compile successfully
POSITIVE_TEST_PATTERNS = [
    r"@compile\s",           # @compile (without /fail)
    r"@compile$",            # @compile at end of line
    r"@run\s",               # @run tests should compile
]

# Annotations that indicate the test should fail (exclude these)
# Keep these lowercased and compare against lowercased source content.
NEGATIVE_TEST_PATTERNS = [
    "compile/fail",          # Expected to fail compilation
]


@dataclass
class TestResult:
    file_path: Path
    success: bool
    reason: str


def has_positive_annotation(file_path: Path) -> bool:
    """Check if file has @compile or @run annotation (but not @compile/fail)."""
    try:
        content = file_path.read_text(encoding="utf-8", errors="ignore")
        lower_content = content.lower()

        # First check for negative patterns - if found, exclude
        for pattern in NEGATIVE_TEST_PATTERNS:
            if pattern in lower_content or re.search(pattern, content, re.IGNORECASE):
                return False

        # Then check for positive patterns
        for pattern in POSITIVE_TEST_PATTERNS:
            if re.search(pattern, content, re.IGNORECASE):
                return True

        return False
    except Exception:
        return False


def find_java_files(assets_dir: Path) -> list[Path]:
    """Find all .java files with positive test annotations."""
    all_files = sorted(assets_dir.rglob("*.java"))
    return [f for f in all_files if has_positive_annotation(f)]


def compile_with_javac(java_file: Path, source_version: str) -> TestResult:
    """
    Attempt full compilation with javac.

    Returns success only if the file compiles without errors.
    """
    with tempfile.TemporaryDirectory() as tmpdir:
        try:
            result = subprocess.run(
                [
                    "javac",
                    "-source", source_version,
                    "-target", source_version,
                    "-proc:none",
                    "-implicit:none",
                    "-Xlint:none",
                    "-d", tmpdir,
                    str(java_file),
                ],
                capture_output=True,
                text=True,
                timeout=60,
            )

            if result.returncode == 0:
                return TestResult(java_file, True, "compiles")

            stderr = result.stderr

            # Check if the only errors are dependency-related
            # These files might still work if we had the right classpath
            if "error:" in stderr:
                error_lines = [line for line in stderr.split("\n") if "error:" in line]

                # Dependency errors that don't indicate a problem with the file itself
                dependency_indicators = [
                    "cannot find symbol",
                    "package .* does not exist",
                    "cannot access",
                    "class file for .* not found",
                    "bad class file",
                ]

                # Errors that indicate actual problems we should exclude
                real_error_indicators = [
                    "illegal start",
                    "expected",
                    "unclosed",
                    "reached end of file",
                    "not a statement",
                    "class, interface, or enum expected",
                    "invalid method declaration",
                    "illegal character",
                    "malformed",
                    "duplicate",
                    "is already defined",
                    "incompatible types",
                    "cannot be applied",
                    "unreported exception",
                    "might not have been initialized",
                ]

                has_real_error = any(
                    indicator in line.lower()
                    for line in error_lines
                    for indicator in real_error_indicators
                )

                if has_real_error:
                    return TestResult(java_file, False, "compile_error")

                # Only dependency errors - file is OK
                all_dependency_errors = all(
                    any(re.search(ind, line, re.IGNORECASE)
                        for ind in dependency_indicators)
                    for line in error_lines
                )

                if all_dependency_errors:
                    return TestResult(java_file, True, "dependency_only")

                return TestResult(java_file, False, "mixed_errors")

            return TestResult(java_file, False, "unknown_error")

        except subprocess.TimeoutExpired:
            return TestResult(java_file, False, "timeout")
        except Exception as e:
            return TestResult(java_file, False, f"error: {e}")


def process_jdk_version(jdk_version: str, parallel: int) -> tuple[int, int, int]:
    """Process all Java files for a JDK version and create whitelist."""
    config = JDK_CONFIGS[jdk_version]
    assets_dir = config["assets_dir"]
    source_version = config["source_version"]
    whitelist_file = config["whitelist_file"]

    if not assets_dir.exists():
        print(f"Error: Assets directory not found: {assets_dir}", file=sys.stderr)
        return 0, 0, 0

    print(f"Processing {jdk_version.upper()} compliance tests from {assets_dir}")

    # Count all .java files
    all_java_files = list(assets_dir.rglob("*.java"))
    total_count = len(all_java_files)

    # Find files with positive annotations
    positive_files = find_java_files(assets_dir)
    positive_count = len(positive_files)

    print(f"Found {total_count} total .java files")
    print(f"Found {positive_count} files with @compile or @run annotations")
    print(f"Checking compilation with javac -source {source_version}...")

    whitelisted_files: list[Path] = []
    failed_count = 0
    reasons: dict[str, int] = {}

    with ProcessPoolExecutor(max_workers=parallel) as executor:
        futures = {
            executor.submit(compile_with_javac, f, source_version): f
            for f in positive_files
        }

        completed = 0
        for future in as_completed(futures):
            completed += 1
            if completed % 200 == 0 or completed == positive_count:
                print(f"  Progress: {completed}/{positive_count}", flush=True)

            result = future.result()
            reasons[result.reason] = reasons.get(result.reason, 0) + 1

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

    print(f"\nResults by reason:")
    for reason, count in sorted(reasons.items()):
        print(f"  {reason}: {count}")

    print(f"\nCreated {whitelist_file}")
    print(f"  Total files: {total_count}")
    print(f"  With positive annotations: {positive_count}")
    print(f"  Whitelisted for compliance: {whitelist_count}")
    print()

    return total_count, positive_count, whitelist_count


def main():
    parser = argparse.ArgumentParser(
        description="Prepare JDK compliance test whitelists by validating with javac"
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
