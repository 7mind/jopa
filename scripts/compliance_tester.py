#!/usr/bin/env python3
import os
import sys
import re
import shutil
import subprocess
import argparse
import tempfile
import concurrent.futures
import glob
from pathlib import Path
import time

# Configuration
DEFAULT_COMPILER = "jopa"
DEFAULT_JVM = "jamvm"
STUB_RT_PATH = "build/runtime/jopa-stub-rt.jar"
GNUCP_PATH = "build-devjopak/vendor-install/classpath/share/classpath/glibj.zip"
JOPA_PATH = "build/src/jopa"
JAMVM_PATH = "build-devjopak/vendor-install/jamvm/bin/jamvm"

class Test:
    def __init__(self, file_path, instructions, reason):
        self.file_path = file_path
        self.instructions = instructions
        self.reason = reason
        self.name = os.path.basename(file_path)

    def __repr__(self):
        return f"<Test {self.name} ({self.reason})>"

def resolve_tool(arg, name, default_path, system_name):
    target = arg
    if arg == name:
        target = default_path
    elif arg == 'system':
        target = system_name
    
    # Check existence (unless it is a system command we assume is in PATH, though we could check shutil.which)
    if arg != 'system':
        if not os.path.exists(target):
             # If default was requested but not found
             if arg == name:
                 print(f"Error: Default {name.upper()} not found at {target}. Build it or use --{name.lower()} system or path.")
             else:
                 print(f"Error: {name} binary '{arg}' not found at {target}")
             sys.exit(1)
    
    # If system, resolve full path for logging
    if arg == 'system':
        resolved = shutil.which(target)
        if not resolved:
            print(f"Error: System command '{target}' not found in PATH")
            sys.exit(1)
        return resolved

    return os.path.abspath(target)

def resolve_classpath(cp_arg):
    """
    Resolves the classpath argument to a concrete path.
    Handles 'stub', 'gnucp', None (default), and direct paths.
    Returns absolute path or exits on error.
    """
    target_path = None
    if cp_arg == 'stub':
        target_path = STUB_RT_PATH
    elif cp_arg == 'gnucp' or cp_arg is None:
        target_path = GNUCP_PATH
    else:
        target_path = cp_arg

    if not os.path.exists(target_path):
        if cp_arg is None:
             print(f"Error: Default GNU Classpath not found at {target_path}. Build it or use --classpath.")
        else:
             print(f"Error: Classpath target '{cp_arg}' not found at {target_path}")
        sys.exit(1)
        
    return os.path.abspath(target_path)

def parse_test_file(file_path, blacklist=None, verbose_prepare=False):
    """
    Parses a Java test file to find test instructions.
    Returns a list of instructions or None if not a valid positive test.
    """
    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
    except Exception as e:
        if verbose_prepare:
            print(f"Error reading {file_path}: {e}")
        return None

    if blacklist:
        for item in blacklist:
            if item in content:
                if verbose_prepare:
                    print(f"Excluded '{file_path}' due to blacklist item '{item}' found in content")
                return None

    # Find /* ... */ block containing @test
    # We search for the first block containing @test
    match = re.search(r'/\*.*?\s@test\s.*?\*/', content, re.DOTALL)
    if not match:
        if verbose_prepare:
            print(f"Excluded '{file_path}': No '@test' annotation found")
        return None
    
    comment_block = match.group(0)
    
    # Check if it's a positive test intended to be run
    
    # Directives to collect
    run_directives = []
    compile_directives = []
    library_directives = []
    # Track ordered steps for multi-step tests (compile, clean, compile, etc.)
    ordered_steps = []

    lines = comment_block.split('\n')
    is_positive = False
    is_negative = False
    expect_failure = False  # For @run main/fail tests
    reason = None

    for line in lines:
        line = line.strip()
        # Remove leading *
        line = re.sub(r'^\*\s*', '', line)

        # Standard jtreg - handle @run main, @run main/fail, @run main/othervm
        if line.startswith('@run main'):
            parts = line.split()
            if len(parts) >= 2:
                run_type = parts[1]  # main, main/fail, main/othervm, etc.
                args = parts[2:] if len(parts) > 2 else []

                # Filter out JVM options (starting with -) before the class name
                # JVM options like -Xfuture, -verify, etc. are not supported
                filtered_args = []
                for arg in args:
                    if arg.startswith('-'):
                        continue  # Skip JVM options
                    filtered_args.append(arg)

                if filtered_args:
                    run_directives.append(filtered_args)

                is_positive = True
                if '/fail' in run_type:
                    expect_failure = True
                    reason = "@run main/fail"
                else:
                    reason = "@run main"

        elif line.startswith('@compile'):
            if line.startswith('@compile/fail'):
                is_negative = True
            elif line.startswith('@compile/ref'):
                # @compile/ref=output.txt - ignore ref output checks
                parts = line.split()
                if len(parts) > 1:
                    # Skip the /ref=xxx part, get the actual files
                    files = [p for p in parts[1:] if not p.startswith('-')]
                    if files:
                        compile_directives.append(files)
                        ordered_steps.append(('compile', files))
                if not is_positive:
                    is_positive = True
                    reason = "@compile"
            else:
                parts = line.split()
                if len(parts) > 1:
                    # Filter out compiler options and their arguments
                    # e.g., -classpath . or -d outdir or -XDrawDiagnostics
                    files = []
                    skip_next = False
                    for p in parts[1:]:
                        if skip_next:
                            skip_next = False
                            continue
                        if p.startswith('-'):
                            # Options that take arguments
                            if p in ('-classpath', '-cp', '-d', '-sourcepath', '-bootclasspath', '-extdirs', '-encoding', '-source', '-target'):
                                skip_next = True
                            continue
                        files.append(p)
                    if files:
                        compile_directives.append(files)
                        ordered_steps.append(('compile', files))
                if not is_positive:
                    is_positive = True
                    reason = "@compile"

        elif line.startswith('@clean'):
            # @clean fully.qualified.ClassName [ClassName2 ...]
            parts = line.split()
            if len(parts) > 1:
                classes_to_clean = parts[1:]
                ordered_steps.append(('clean', classes_to_clean))

        elif line.startswith('@library') or line.startswith('@ library'):
            # Handle @library path1 path2 ...
            # Normalize spaces
            parts = line.split()
            # parts[0] is @library or @, parts[1] could be library if spaced
            start_idx = 1
            if parts[0] == '@':
                start_idx = 2
            
            if len(parts) > start_idx:
                for lib_path in parts[start_idx:]:
                    library_directives.append(lib_path)
                
        # Prompt specific format support
        elif 'main' in line and 'RuntimeAnnotations_attribute.java' in line:
            parts = line.split()
            try:
                idx = parts.index('main')
                if idx + 1 < len(parts):
                    run_directives.append(parts[idx+1:])
                    is_positive = True
                    reason = "RuntimeAnnotations main"
            except ValueError:
                pass

        elif 'compile' in line and 'RuntimeAnnotations_attribute.java' in line:
             parts = line.split()
             try:
                 idx = parts.index('compile')
                 if idx + 1 < len(parts):
                     compile_directives.append(parts[idx+1:])
             except ValueError:
                 pass

        elif 'compile' in line and 'CompileProperties.java' in line:
             parts = line.split()
             try:
                 idx = parts.index('compile')
                 if idx + 1 < len(parts):
                     compile_directives.append(parts[idx+1:])
                     is_positive = True
                     reason = "CompileProperties"
             except ValueError:
                 pass
                 
    if is_positive and not is_negative:
        return {
            'run': run_directives,
            'compile': compile_directives,
            'library': library_directives,
            'steps': ordered_steps,  # Ordered list of ('compile', files) or ('clean', classes)
            'reason': reason,
            'expect_failure': expect_failure
        }

    if verbose_prepare:
        print(f"Excluded '{file_path}': Not a positive test or marked as negative (@compile/fail)")
    return None

def scan_tests(roots, blacklist=None, exclude_set=None, verbose_prepare=False):
    tests = []
    for root in roots:
        if not os.path.exists(root):
            continue
        for dirpath, _, filenames in os.walk(root):
            for filename in filenames:
                if filename.endswith('.java'):
                    full_path = os.path.join(dirpath, filename)
                    
                    # Check exclude set (substring match)
                    if exclude_set:
                        excluded = False
                        matched_pattern = None
                        for pattern in exclude_set:
                            if pattern in full_path:
                                excluded = True
                                matched_pattern = pattern
                                break
                        if excluded:
                            if verbose_prepare:
                                print(f"Excluded '{full_path}' due to exclude pattern '{matched_pattern}'")
                            continue

                    instructions = parse_test_file(full_path, blacklist, verbose_prepare)
                    if instructions:
                        tests.append(Test(full_path, instructions, instructions['reason']))
    return tests

def copy_recursive(src, dst):
    """
    Recursively copies src to dst.
    If src is a file, copies it to dst.
    If src is a directory, copies its content to dst, merging if dst exists.
    """
    if os.path.isdir(src):
        if not os.path.exists(dst):
            os.makedirs(dst)
        for item in os.listdir(src):
            s = os.path.join(src, item)
            d = os.path.join(dst, item)
            copy_recursive(s, d)
    else:
        shutil.copy2(src, dst)

def get_clean_env():
    """
    Returns a clean environment for running compiler and JVM.
    Excludes CLASSPATH and other potentially polluting variables.
    """
    # Start with minimal required variables
    clean = {
        'PATH': os.environ.get('PATH', '/usr/bin:/bin'),
        'HOME': os.environ.get('HOME', '/tmp'),
        'USER': os.environ.get('USER', 'nobody'),
        'LANG': os.environ.get('LANG', 'C'),
        'LC_ALL': os.environ.get('LC_ALL', 'C'),
        'TMPDIR': os.environ.get('TMPDIR', '/tmp'),
    }
    # Explicitly do NOT include: CLASSPATH, JIKESPATH, JAVA_HOME, etc.
    return clean

def run_single_test(test, compiler, jvm, compiler_args, timeout, mode, extra_cp=None, verbose=False):
    """
    Executes a single test.
    Returns: (outcome, reason, stdout, stderr, temp_file_path)
    Outcome: 'SUCCESS', 'FAILURE', 'TIMEOUT', 'CRASH', 'SKIP'
    """
    tmpdir = tempfile.mkdtemp(prefix="test_run_")
    dest_test_file = os.path.join(tmpdir, os.path.basename(test.file_path))
    outcome = 'CRASH'
    reason = 'unknown'
    stdout_str = ''
    stderr_str = ''

    try:
        test_dir = os.path.dirname(test.file_path)

        # Check if @compile directive specifies explicit files to compile
        explicit_compile_files = []
        for files in test.instructions['compile']:
            for f in files:
                if f.startswith('-'): continue
                if f.endswith('.java'):
                    explicit_compile_files.append(f)

        # Check if test file name matches any explicit compile file
        test_basename = os.path.basename(test.file_path)
        test_file_explicitly_excluded = (explicit_compile_files and
                                         test_basename not in explicit_compile_files and
                                         not any(f.endswith('/' + test_basename) for f in explicit_compile_files))

        # 1. Copy files to compile
        if explicit_compile_files:
            # @compile specifies explicit files - copy those
            for f in explicit_compile_files:
                src = os.path.join(test_dir, f)
                if os.path.exists(src):
                    dest = os.path.join(tmpdir, f)
                    dest_dir = os.path.dirname(dest)
                    if dest_dir and not os.path.exists(dest_dir):
                        os.makedirs(dest_dir)
                    shutil.copy2(src, dest)
            # Also copy all subdirectories from test_dir - they may contain
            # dependencies (e.g., pkg/B.java when compiling pkg/A.java)
            # Use copy_recursive which handles existing directories by merging
            for item in os.listdir(test_dir):
                src = os.path.join(test_dir, item)
                if os.path.isdir(src):
                    dest = os.path.join(tmpdir, item)
                    copy_recursive(src, dest)
            # If @run exists and test file wasn't in compile list, also copy test file
            # (the test file contains the main method to run)
            if test.instructions['run'] and test_file_explicitly_excluded:
                shutil.copy2(test.file_path, dest_test_file)
                explicit_compile_files.append(test_basename)
        else:
            # No explicit files - copy main test file
            shutil.copy2(test.file_path, dest_test_file)

            # Also copy any extra files from compile directives (non-.java args like directories)
            for files in test.instructions['compile']:
                for f in files:
                    if f.startswith('-'): continue
                    src = os.path.join(test_dir, f)
                    if os.path.exists(src):
                        dest = os.path.join(tmpdir, f)
                        dest_dir = os.path.dirname(dest)
                        if dest_dir and not os.path.exists(dest_dir):
                            os.makedirs(dest_dir)
                        shutil.copy2(src, dest)

        # 2. Handle library directives
        for lib in test.instructions.get('library', []):
            src = os.path.join(test_dir, lib)
            if os.path.exists(src):
                copy_recursive(src, tmpdir)

        # 3. Compile - check if we have multi-step with @clean
        ordered_steps = test.instructions.get('steps', [])
        has_clean = any(step[0] == 'clean' for step in ordered_steps)

        javac_out_path = os.path.join(tmpdir, 'javac.out')
        javac_err_path = os.path.join(tmpdir, 'javac.err')

        if has_clean and ordered_steps:
            # Multi-step compilation with @clean directives
            for step_type, step_args in ordered_steps:
                if step_type == 'compile':
                    # Compile specified files
                    java_files = [os.path.join(tmpdir, f) for f in step_args if os.path.exists(os.path.join(tmpdir, f))]
                    if not java_files:
                        continue  # Skip empty compile steps

                    compile_cmd = [compiler] + (compiler_args if compiler_args else [])
                    if extra_cp:
                        compile_cmd += ['-bootclasspath', extra_cp]
                    compile_cmd += ['-d', '.'] + java_files

                    if verbose:
                        print(f"Compiling step: {' '.join(compile_cmd)}")

                    try:
                        with open(javac_out_path, 'w') as f_out, open(javac_err_path, 'w') as f_err:
                            proc = subprocess.run(
                                compile_cmd,
                                cwd=tmpdir,
                                stdout=f_out,
                                stderr=f_err,
                                timeout=timeout,
                                env=get_clean_env()
                            )

                        with open(javac_out_path, 'r', errors='replace') as f: stdout_str = f.read()
                        with open(javac_err_path, 'r', errors='replace') as f: stderr_str = f.read()

                    except subprocess.TimeoutExpired:
                        outcome = 'TIMEOUT'
                        reason = 'compiler timed out'
                        return outcome, reason, stdout_str, stderr_str, dest_test_file
                    except Exception as e:
                        outcome = 'CRASH'
                        reason = f'compiler crashed: {e}'
                        return outcome, reason, stdout_str, stderr_str, dest_test_file

                    if proc.returncode != 0:
                        if verbose:
                            print(f"Compilation failed for {test.name}:\n{stderr_str}")
                        outcome = 'FAILURE'
                        reason = 'compiler failed (exit code not 0)'
                        return outcome, reason, stdout_str, stderr_str, dest_test_file

                elif step_type == 'clean':
                    # Delete specified class files
                    # @clean takes fully qualified class names like pkg.ClassName
                    for class_name in step_args:
                        # Convert pkg.ClassName to pkg/ClassName.class
                        class_file = class_name.replace('.', '/') + '.class'
                        class_path = os.path.join(tmpdir, class_file)
                        if os.path.exists(class_path):
                            os.remove(class_path)
                            if verbose:
                                print(f"Cleaned: {class_path}")
                        # Also try with $ for inner classes (pkg.Outer$Inner)
                        # The @clean might specify pkg.Outer.Inner but file is Outer$Inner.class
                        parts = class_name.rsplit('.', 1)
                        if len(parts) == 2:
                            alt_file = parts[0].replace('.', '/') + '$' + parts[1] + '.class'
                            alt_path = os.path.join(tmpdir, alt_file)
                            if os.path.exists(alt_path):
                                os.remove(alt_path)
                                if verbose:
                                    print(f"Cleaned: {alt_path}")
        else:
            # Single compilation - use existing logic
            if explicit_compile_files:
                # Compile only the explicitly specified files
                java_files = [os.path.join(tmpdir, f) for f in explicit_compile_files if os.path.exists(os.path.join(tmpdir, f))]
            else:
                # Compile all .java files in tmpdir
                java_files = glob.glob(os.path.join(tmpdir, "*.java"))
            if not java_files:
                outcome = 'FAILURE'
                reason = 'nothing to compile'
                return outcome, reason, stdout_str, stderr_str, dest_test_file

            compile_cmd = [compiler] + (compiler_args if compiler_args else [])
            if extra_cp:
                compile_cmd += ['-bootclasspath', extra_cp]
            compile_cmd += ['-d', '.'] + java_files

            if verbose:
                print(f"Compiling: {' '.join(compile_cmd)}")

            try:
                with open(javac_out_path, 'w') as f_out, open(javac_err_path, 'w') as f_err:
                    proc = subprocess.run(
                        compile_cmd,
                        cwd=tmpdir,
                        stdout=f_out,
                        stderr=f_err,
                        timeout=timeout,
                        env=get_clean_env()
                    )

                # Read back for reporting
                with open(javac_out_path, 'r', errors='replace') as f: stdout_str = f.read()
                with open(javac_err_path, 'r', errors='replace') as f: stderr_str = f.read()

            except subprocess.TimeoutExpired:
                outcome = 'TIMEOUT'
                reason = 'compiler timed out'
                return outcome, reason, stdout_str, stderr_str, dest_test_file
            except Exception as e:
                outcome = 'CRASH'
                reason = f'compiler crashed: {e}'
                return outcome, reason, stdout_str, stderr_str, dest_test_file

            if proc.returncode != 0:
                if verbose:
                    print(f"Compilation failed for {test.name}:\n{stderr_str}")
                outcome = 'FAILURE'
                reason = 'compiler failed (exit code not 0)'
                return outcome, reason, stdout_str, stderr_str, dest_test_file

        # 5. Run
        if not test.instructions['run'] or mode == 'compile':
            outcome = 'SUCCESS'
            reason = 'compiled'
            return outcome, reason, stdout_str, stderr_str, dest_test_file

        # Reuse output strings for test execution
        last_stdout = stdout_str
        last_stderr = stderr_str

        test_out_path = os.path.join(tmpdir, 'test.out')
        test_err_path = os.path.join(tmpdir, 'test.err')

        for run_args in test.instructions['run']:
            main_class = run_args[0]
            app_args = run_args[1:]
            
            cp = '.'
            if extra_cp:
                cp = f'.{os.pathsep}{extra_cp}'
            
            jvm_cmd = [jvm] + ['-cp', cp, f'-Dtest.src={tmpdir}'] + [main_class] + app_args
            
            if verbose:
                print(f"Running: {' '.join(jvm_cmd)}")

            try:
                with open(test_out_path, 'w') as f_out, open(test_err_path, 'w') as f_err:
                    proc = subprocess.run(
                        jvm_cmd,
                        cwd=tmpdir,
                        stdout=f_out,
                        stderr=f_err,
                        timeout=timeout,
                        env=get_clean_env()
                    )
                
                with open(test_out_path, 'r', errors='replace') as f: last_stdout = f.read()
                with open(test_err_path, 'r', errors='replace') as f: last_stderr = f.read()

            except subprocess.TimeoutExpired:
                outcome = 'TIMEOUT'
                reason = 'test timed out'
                return outcome, reason, last_stdout, last_stderr, dest_test_file
            except Exception as e:
                outcome = 'CRASH'
                reason = f'test crashed: {e}'
                return outcome, reason, last_stdout, last_stderr, dest_test_file
            
            expect_failure = test.instructions.get('expect_failure', False)

            if proc.returncode != 0:
                if expect_failure:
                    # Expected failure - this is success
                    pass
                else:
                    if verbose:
                        print(f"Execution failed for {test.name} ({main_class}):\n{last_stderr}")
                    outcome = 'FAILURE'
                    reason = 'exit code not 0'
                    return outcome, reason, last_stdout, last_stderr, dest_test_file
            elif expect_failure:
                # Expected failure but got success - that's a failure
                outcome = 'FAILURE'
                reason = 'expected failure but succeeded'
                return outcome, reason, last_stdout, last_stderr, dest_test_file

        outcome = 'SUCCESS'
        reason = 'exit code 0' if not test.instructions.get('expect_failure', False) else 'expected failure'
        return outcome, reason, last_stdout, last_stderr, dest_test_file

    except Exception as e:
        if verbose:
            print(f"Crash in {test.name}: {e}")
        outcome = 'CRASH'
        reason = f'harness crashed: {e}'
        return outcome, reason, '', '', dest_test_file
    
    finally:
        if outcome == 'SUCCESS' and os.path.exists(tmpdir):
            try:
                shutil.rmtree(tmpdir)
            except OSError as e:
                print(f"Error removing {tmpdir}: {e}")

def main():
    parser = argparse.ArgumentParser(description="Compliance Test Tool")
    parser.add_argument('--prepare', action='store_true', help="Prepare and list tests")
    parser.add_argument('--test', action='store_true', help="Run tests")
    parser.add_argument('--jdk', choices=['7', '8'], help="JDK version to test (7 or 8)")
    parser.add_argument('--testlist', help="File to read/write test list")
    parser.add_argument('--limit', type=int, help="Limit number of tests")
    parser.add_argument('--timeout', type=int, default=5, help="Timeout in seconds (default 5)")
    parser.add_argument('--compiler', default=DEFAULT_COMPILER, help="Path to javac")
    parser.add_argument('--jvm', default=DEFAULT_JVM, help="Path to java")
    parser.add_argument('--arg', action='append', help="Compiler arguments", dest='compiler_args')
    parser.add_argument('--no-success', action='store_true', help="Do not log successful tests")
    parser.add_argument('--verbose-failures', action='store_true', help="Print output for failed tests")
    parser.add_argument('--verbose', action='store_true', help="Verbose output")
    parser.add_argument('--verbose-prepare', action='store_true', help="Print explanation for excluded tests during prepare phase")
    parser.add_argument('--blacklist', action='append', help="Blacklist file (can be used multiple times)")
    parser.add_argument('--exclude', help="Exclude file")
    parser.add_argument('--classpath', help="Classpath to use: 'stub', 'gnucp', or path. Default: gnucp")
    parser.add_argument('--mode', choices=['run', 'compile'], default='run', help="Test mode: 'run' (default) or 'compile' (only compile)")
    
    args = parser.parse_args()

    roots = []
    if args.jdk == '7':
        roots.append("assets/jdk7u-langtools/test/tools/javac/")
    elif args.jdk == '8':
        roots.append("assets/jdk8u_langtools/test/tools/javac/")
    
    if args.prepare:
        if not roots:
             print("Error: --jdk {7,8} is required for --prepare")
             sys.exit(1)

        blacklist_files = args.blacklist if args.blacklist else ['./scripts/test-blacklist.txt']
        blacklist = []
        
        for bl_file in blacklist_files:
            if os.path.exists(bl_file):
                try:
                    with open(bl_file, 'r') as f:
                        items = [line.strip() for line in f if line.strip()]
                        blacklist.extend(items)
                    print(f"Loaded {len(items)} blacklist items from {bl_file}")
                except Exception as e:
                    print(f"Error reading blacklist {bl_file}: {e}")
            elif args.blacklist: # Only warn if explicitly provided
                 print(f"Warning: Blacklist file {bl_file} not found")

        exclude_set = set()
        exclude_file = args.exclude if args.exclude else f"./scripts/jdk{args.jdk}-exclude.txt"
        if os.path.exists(exclude_file):
            try:
                with open(exclude_file, 'r') as f:
                    for line in f:
                        # Strip comments and whitespace
                        clean_line = line.split('#')[0].strip()
                        if not clean_line:
                            continue
                        # Strip optional leading @
                        if clean_line.startswith('@'):
                            clean_line = clean_line[1:].strip()
                        exclude_set.add(clean_line)
                print(f"Loaded {len(exclude_set)} exclude items from {exclude_file}")
            except Exception as e:
                print(f"Error reading exclude file {exclude_file}: {e}")

        print("Scanning for tests...")
        tests = scan_tests(roots, blacklist, exclude_set, args.verbose_prepare)
        
        # Determine output file for prepare mode
        outfile = args.testlist
        if not outfile:
            if args.jdk:
                outfile = f"./scripts/jdk{args.jdk}-test-whitelist.txt"
            else:
                outfile = 'test_whitelist.txt' # Fallback if --jdk is not provided (should be caught by roots check)
        
        run_count = 0
        compile_count = 0
        
        with open(outfile, 'w') as f:
            for t in tests:
                print(f"{t.file_path} [{t.reason}]")
                f.write(t.file_path + '\n')
                if t.instructions['run']:
                    run_count += 1
                else:
                    compile_count += 1
        
        print(f"Found {len(tests)} positive tests.")
        print(f"  Tests to run: {run_count}")
        print(f"  Tests to compile: {compile_count}")
        print(f"Whitelist written to {outfile}")
        return

    if args.test:
        resolved_cp = resolve_classpath(args.classpath)
        compiler_path = resolve_tool(args.compiler, 'jopa', JOPA_PATH, 'javac')
        jvm_path = resolve_tool(args.jvm, 'jamvm', JAMVM_PATH, 'java')

        print(f"Using classpath: {resolved_cp}")
        print(f"Using compiler: {compiler_path}")
        print(f"Using JVM: {jvm_path}")

        # Load exclusions for test mode
        exclude_set = set()
        blacklist_files = args.blacklist if args.blacklist else []
        for bl_file in blacklist_files:
            if os.path.exists(bl_file):
                try:
                    with open(bl_file, 'r') as f:
                        for line in f:
                            clean_line = line.split('#')[0].strip()
                            if clean_line:
                                if clean_line.startswith('@'):
                                    clean_line = clean_line[1:].strip()
                                exclude_set.add(clean_line)
                except Exception as e:
                    print(f"Warning: Error reading blacklist {bl_file}: {e}")

        tests = []
        # Determine input file for test mode
        input_testlist = args.testlist
        if not input_testlist:
            if args.jdk:
                input_testlist = f"./scripts/jdk{args.jdk}-test-whitelist.txt"
            else:
                print("Error: --jdk {7,8} is required for --test if --testlist is not provided")
                sys.exit(1)

        if input_testlist:
             print(f"Reading tests from {input_testlist}...")
             if not os.path.exists(input_testlist):
                 print(f"Error: Test list {input_testlist} not found.")
                 sys.exit(1)
             with open(input_testlist, 'r') as f:
                 for line in f:
                     path = line.strip()
                     if not path or not os.path.exists(path):
                         continue
                     # Check against exclude set (substring match)
                     should_skip = False
                     for pattern in exclude_set:
                         if pattern in path:
                             should_skip = True
                             break
                     if should_skip:
                         continue
                     instructions = parse_test_file(path)
                     if instructions:
                         tests.append(Test(path, instructions, instructions['reason']))
        else: # This block is now effectively unreachable due to input_testlist determination above
            if not roots:
                print("Error: --jdk {7,8} is required for --test if --testlist is not provided")
                sys.exit(1)
            print("Scanning for tests to run...")
            tests = scan_tests(roots)
        
        if args.limit:
            import random
            random.shuffle(tests)
            tests = tests[:args.limit]
            print(f"Limiting to {len(tests)} tests.")
        
        # Add default -source for JDK-specific tests
        compiler_args = args.compiler_args or []
        if args.jdk == '7' and '-source' not in ' '.join(compiler_args):
            compiler_args = ['-source', '1.7'] + compiler_args
        elif args.jdk == '8' and '-source' not in ' '.join(compiler_args):
            compiler_args = ['-source', '1.8'] + compiler_args

        print(f"Running {len(tests)} tests using {compiler_path} and {jvm_path}...")

        results = {
            'SUCCESS': 0,
            'FAILURE': 0,
            'TIMEOUT': 0,
            'CRASH': 0
        }
        failure_reasons = {}
        
        start_time = time.time()
        
        with concurrent.futures.ThreadPoolExecutor(max_workers=os.cpu_count()) as executor:
            future_to_test = {
                executor.submit(run_single_test, test, compiler_path, jvm_path, compiler_args, args.timeout, args.mode, resolved_cp, args.verbose): test
                for test in tests
            }
            
            for i, future in enumerate(concurrent.futures.as_completed(future_to_test)):
                test = future_to_test[future]
                try:
                    outcome, reason, out, err, tmp_path = future.result()
                    results[outcome] += 1
                    
                    if outcome != 'SUCCESS':
                        failure_reasons[reason] = failure_reasons.get(reason, 0) + 1
                    
                    if outcome == 'SUCCESS' and args.no_success:
                        continue
                    
                    log_msg = f"[{i+1}/{len(tests)}] {test.file_path} ({tmp_path}): {outcome} ({reason})"
                    if outcome != 'SUCCESS':
                        log_dir = os.path.dirname(tmp_path)
                        if 'compiler' in reason:
                            log_msg += f" javac log: {os.path.join(log_dir, 'javac.err')}"
                        else:
                            log_msg += f" test log: {os.path.join(log_dir, 'test.err')}"
                    print(log_msg)
                    
                    if outcome != 'SUCCESS' and args.verbose_failures:
                        if out:
                            print("--- STDOUT ---")
                            print(out)
                        if err:
                            print("--- STDERR ---")
                            print(err)
                        if out or err:
                            print("----------------")

                except Exception as exc:
                    print(f"{test.name} generated an exception: {exc}")
                    results['CRASH'] += 1

        duration = time.time() - start_time
        print("\n--- Test Summary ---")
        print(f"Total: {len(tests)}")
        print(f"Success: {results['SUCCESS']}")
        print(f"Failure: {results['FAILURE']}")
        print(f"Timeout: {results['TIMEOUT']}")
        print(f"Crash: {results['CRASH']}")
        
        if failure_reasons:
            print("\nFailure Breakdown:")
            for reason, count in sorted(failure_reasons.items(), key=lambda x: x[1], reverse=True):
                print(f"  {reason}: {count}")
                
        print(f"Duration: {duration:.2f}s")

if __name__ == "__main__":
    main()
