#!/usr/bin/env python3
"""
Remove Windows-specific code from source files.

Handles these patterns:
1. #ifdef WIN32_FILE_SYSTEM ... #endif (remove block)
2. #ifdef WIN32_FILE_SYSTEM ... #else ... #endif (keep else part)
3. #if defined(WIN32_FILE_SYSTEM) ... #elif ... #endif (keep elif part)
4. #ifdef HAVE_WINDOWS_H ... #endif (remove block)
5. #ifdef HAVE_DIRECT_H ... #endif (remove block)
6. #ifdef HAVE_WIN32_MKDIR ... #endif (remove block)
7. #ifdef HAVE_CYGWIN_* ... #endif (remove block)
8. #ifdef HAVE_VCPP_SET_NEW_HANDLER ... #else ... #endif (keep else part)
"""

import re
import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).parent.parent.resolve()
SRC_DIR = PROJECT_ROOT / "src"

# Patterns to match Windows-specific preprocessor conditions
WIN_PATTERNS = [
    r'WIN32_FILE_SYSTEM',
    r'HAVE_WINDOWS_H',
    r'HAVE_DIRECT_H',
    r'HAVE_WIN32_MKDIR',
    r'HAVE_CYGWIN\w*',
    r'HAVE_VCPP_SET_NEW_HANDLER',
    r'HAVE_PATHNAME_STYLE_DOS',
]

WIN_PATTERN = '|'.join(WIN_PATTERNS)


def remove_windows_blocks(content: str, filename: str) -> tuple[str, int]:
    """Remove Windows-specific preprocessor blocks."""
    lines = content.split('\n')
    result_lines = []
    changes = 0

    i = 0
    while i < len(lines):
        line = lines[i]
        stripped = line.strip()

        # Check for #if/#ifdef WIN32_FILE_SYSTEM or similar
        match = re.match(rf'#\s*if(?:def)?\s+(?:defined\s*\(\s*)?({WIN_PATTERN})\s*\)?', stripped)
        if match:
            # Find the matching #endif, handling nested #if
            start_idx = i
            nesting = 1
            else_idx = None
            elif_idx = None
            i += 1

            while i < len(lines) and nesting > 0:
                s = lines[i].strip()
                if re.match(r'#\s*if(?:def|ndef)?\s', s):
                    nesting += 1
                elif re.match(r'#\s*endif', s):
                    nesting -= 1
                elif nesting == 1 and re.match(r'#\s*else\s*$', s):
                    else_idx = i
                elif nesting == 1 and re.match(r'#\s*elif\s', s):
                    # Check if elif is not also Windows-specific
                    if not re.search(WIN_PATTERN, s):
                        elif_idx = i
                i += 1

            end_idx = i - 1  # Points to #endif

            if else_idx is not None:
                # Keep the else block (lines between #else and #endif)
                result_lines.extend(lines[else_idx + 1:end_idx])
                changes += 1
            elif elif_idx is not None:
                # Keep the elif block, converting it to #if
                elif_line = lines[elif_idx]
                # Convert #elif to #if
                converted = re.sub(r'#\s*elif', '#if', elif_line)
                result_lines.append(converted)
                result_lines.extend(lines[elif_idx + 1:end_idx + 1])  # Include #endif
                changes += 1
            else:
                # Remove the entire block
                changes += 1

            continue

        # Check for #elif WIN32_FILE_SYSTEM (need to remove up to next #else or #endif)
        match = re.match(rf'#\s*elif\s+(?:defined\s*\(\s*)?({WIN_PATTERN})\s*\)?', stripped)
        if match:
            # Skip this elif block
            nesting = 0
            i += 1
            while i < len(lines):
                s = lines[i].strip()
                if re.match(r'#\s*if(?:def|ndef)?\s', s):
                    nesting += 1
                elif re.match(r'#\s*endif', s):
                    if nesting == 0:
                        # Don't skip the endif, it belongs to outer block
                        break
                    nesting -= 1
                elif nesting == 0 and re.match(r'#\s*(?:else|elif)\s*', s):
                    # Found end of this elif block
                    break
                i += 1
            changes += 1
            continue

        result_lines.append(line)
        i += 1

    if changes > 0:
        print(f"  {filename}: removed {changes} Windows block(s)")

    return '\n'.join(result_lines), changes


def remove_win32_find_data(content: str, filename: str) -> tuple[str, int]:
    """Remove WIN32_FIND_DATA variable declarations and usages."""
    # This is for cleaning up any stray WIN32_FIND_DATA references
    lines = content.split('\n')
    new_lines = []
    removed = 0

    for line in lines:
        if 'WIN32_FIND_DATA' in line:
            removed += 1
            continue
        new_lines.append(line)

    if removed > 0:
        print(f"  {filename}: removed {removed} WIN32_FIND_DATA line(s)")

    return '\n'.join(new_lines), removed


def process_file(filepath: Path, dry_run: bool = False) -> bool:
    """Process a single file."""
    content = filepath.read_text()
    original = content

    content, c1 = remove_windows_blocks(content, filepath.name)
    content, c2 = remove_win32_find_data(content, filepath.name)

    if content != original:
        if not dry_run:
            filepath.write_text(content)
        return True
    return False


def main():
    dry_run = '--dry-run' in sys.argv

    if dry_run:
        print("DRY RUN - no files will be modified\n")

    # Process all .h and .cpp files
    files = list(SRC_DIR.rglob("*.h")) + list(SRC_DIR.rglob("*.cpp"))

    print(f"Processing {len(files)} files...\n")

    modified = 0
    for f in sorted(files):
        if process_file(f, dry_run):
            modified += 1

    print(f"\nModified {modified} files")


if __name__ == "__main__":
    main()
