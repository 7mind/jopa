#!/usr/bin/env python3
"""
Refactor headers:
1. Replace include guards with #pragma once
"""

import re
import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).parent.parent.resolve()
SRC_DIR = PROJECT_ROOT / "src"


def migrate_to_pragma_once(content: str, filename: str) -> tuple[str, bool]:
    """Replace include guards with #pragma once."""

    lines = content.split('\n')
    modified = False

    # Find the guard pattern at the start (may have leading blank lines or comments)
    ifndef_idx = None
    define_idx = None
    guard_name = None

    for i, line in enumerate(lines):
        stripped = line.strip()
        if ifndef_idx is None:
            match = re.match(r'#\s*ifndef\s+(\w+)', stripped)
            if match:
                ifndef_idx = i
                guard_name = match.group(1)
                continue
        elif define_idx is None and guard_name:
            match = re.match(rf'#\s*define\s+{re.escape(guard_name)}\b', stripped)
            if match:
                define_idx = i
                break

    if ifndef_idx is None or define_idx is None or guard_name is None:
        # Check if already has pragma once
        if any('#pragma once' in line for line in lines[:20]):
            return content, False
        print(f"  {filename}: no standard include guard found")
        return content, False

    # Find closing endif (should be at the end of the file)
    endif_idx = None
    for i in range(len(lines) - 1, -1, -1):
        stripped = lines[i].strip()
        if re.match(r'#\s*endif', stripped):
            endif_idx = i
            break

    if endif_idx is None:
        print(f"  {filename}: no closing #endif found")
        return content, False

    # Build new content
    # Everything before ifndef
    new_lines = lines[:ifndef_idx]

    # Add pragma once (if there was content before the guard, add a blank line)
    if new_lines and new_lines[-1].strip():
        new_lines.append('')
    new_lines.append('#pragma once')

    # Everything between define and endif
    new_lines.extend(lines[define_idx + 1:endif_idx])

    # Everything after endif (usually just empty lines)
    remaining = lines[endif_idx + 1:]
    # Only add if there's something meaningful after endif
    if any(line.strip() for line in remaining):
        new_lines.extend(remaining)
    else:
        # Just add a trailing newline
        new_lines.append('')

    print(f"  Migrated {filename}: removed guard {guard_name}")
    return '\n'.join(new_lines), True


def process_file(filepath: Path, dry_run: bool = False) -> bool:
    """Process a single header file."""

    content = filepath.read_text()
    original = content

    content, modified = migrate_to_pragma_once(content, filepath.name)

    if modified and content != original:
        if not dry_run:
            filepath.write_text(content)
        return True
    return False


def main():
    dry_run = '--dry-run' in sys.argv

    if dry_run:
        print("DRY RUN - no files will be modified\n")

    # Find all header files
    headers = list(SRC_DIR.rglob("*.h"))

    print(f"Processing {len(headers)} header files...\n")

    modified = 0
    for header in sorted(headers):
        if process_file(header, dry_run):
            modified += 1

    print(f"\nModified {modified} files")


if __name__ == "__main__":
    main()
