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
DEFAULT_COMPILER = "javac"
DEFAULT_JVM = "java"
STUB_RT_PATH = "build/runtime/jopa-stub-rt.jar"
GNUCP_PATH = "build-devjopak/vendor-install/classpath/share/classpath/glibj.zip"

class Test:
    def __init__(self, file_path, instructions, reason):
        self.file_path = file_path
        self.instructions = instructions
        self.reason = reason
        self.name = os.path.basename(file_path)

    def __repr__(self):
        return f"<Test {self.name} ({self.reason})>"

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

def parse_test_file(file_path, blacklist=None):
    """
    Parses a Java test file to find test instructions.
    Returns a list of instructions or None if not a valid positive test.
    """
    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
    except Exception as e:
        print(f"Error reading {file_path}: {e}")
        return None

    if blacklist:
        for item in blacklist:
            if item in content:
                return None

    # Find /* ... */ block containing @test
    # We search for the first block containing @test
    match = re.search(r'/\*.*?\s@test\s.*?\*/', content, re.DOTALL)
    if not match:
        return None
    
    comment_block = match.group(0)
    
    # Check if it's a positive test intended to be run
    
    # Directives to collect
    run_directives = []
    compile_directives = []
    
    lines = comment_block.split('\n')
    is_positive = False
    is_negative = False
    reason = None
    
    for line in lines:
        line = line.strip()
        # Remove leading *
        line = re.sub(r'^\*\s*', '', line)
        
        # Standard jtreg
        if line.startswith('@run main'):
            # Extract class name and args
            parts = line.split()
            if len(parts) >= 3:
                run_directives.append(parts[2:]) # [Class, arg1, arg2...]
            else:
                # If just @run main, implies running the class of the file usually, 
                # but simplest is to assume the main class matches filename if not specified?
                # Actually usually it is @run main ClassName
                pass
            is_positive = True
            reason = "@run main"
        
        elif line.startswith('@compile'):
            if line.startswith('@compile/fail'):
                is_negative = True
            else:
                parts = line.split()
                if len(parts) > 1:
                    compile_directives.append(parts[1:])
                if not is_positive:
                    is_positive = True
                    reason = "@compile"
                
        # Prompt specific format support
        # Example: @assets/.../RuntimeAnnotations_attribute.java main ConstValInlining
        elif 'main' in line and 'RuntimeAnnotations_attribute.java' in line:
            # Heuristic to support the user's specific requested format
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
            'reason': reason
        }
    
    return None

def scan_tests(roots, blacklist=None):
    tests = []
    for root in roots:
        if not os.path.exists(root):
            continue
        for dirpath, _, filenames in os.walk(root):
            for filename in filenames:
                if filename.endswith('.java'):
                    full_path = os.path.join(dirpath, filename)
                    instructions = parse_test_file(full_path, blacklist)
                    if instructions:
                        tests.append(Test(full_path, instructions, instructions['reason']))
    return tests

def run_single_test(test, compiler, jvm, compiler_args, timeout, extra_cp=None, verbose=False):
    """
    Executes a single test.
    Returns: (outcome, reason, stdout, stderr, temp_file_path)
    Outcome: 'SUCCESS', 'FAILURE', 'TIMEOUT', 'CRASH', 'SKIP'
    """
    with tempfile.TemporaryDirectory(prefix="test_run_") as tmpdir:
        dest_test_file = os.path.join(tmpdir, os.path.basename(test.file_path))
        try:
            # 1. Copy main test file
            shutil.copy2(test.file_path, dest_test_file)
            
            # 2. Handle compile directives (copy extra files)
            test_dir = os.path.dirname(test.file_path)
            
            for files in test.instructions['compile']:
                for f in files:
                    if f.startswith('-'): continue
                    src = os.path.join(test_dir, f)
                    if os.path.exists(src):
                        shutil.copy2(src, os.path.join(tmpdir, os.path.basename(f)))
                    else:
                        pass

            # 3. Compile
            java_files = glob.glob(os.path.join(tmpdir, "*.java"))
            if not java_files:
                return 'FAILURE', 'nothing to compile', '', '', dest_test_file

            compile_cmd = [compiler] + (compiler_args if compiler_args else [])
            if extra_cp:
                compile_cmd += ['-bootclasspath', extra_cp]
            compile_cmd += ['-d', '.'] + java_files
            
            if verbose:
                print(f"Compiling: {' '.join(compile_cmd)}")
            
            stdout_str = ""
            stderr_str = ""
            
            try:
                proc = subprocess.run(
                    compile_cmd,
                    cwd=tmpdir,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    timeout=timeout
                )
                stdout_str = proc.stdout.decode('utf-8', errors='replace')
                stderr_str = proc.stderr.decode('utf-8', errors='replace')
            except subprocess.TimeoutExpired:
                return 'TIMEOUT', 'compiler timed out', '', '', dest_test_file
            except Exception as e:
                return 'CRASH', f'compiler crashed: {e}', '', '', dest_test_file
            
            if proc.returncode != 0:
                if verbose:
                    print(f"Compilation failed for {test.name}:\n{stderr_str}")
                return 'FAILURE', 'compiler failed (exit code not 0)', stdout_str, stderr_str, dest_test_file

            # 4. Run
            if not test.instructions['run']:
                return 'SUCCESS', 'compiled', stdout_str, stderr_str, dest_test_file

            last_stdout = stdout_str
            last_stderr = stderr_str

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
                    proc = subprocess.run(
                        jvm_cmd,
                        cwd=tmpdir,
                        stdout=subprocess.PIPE,
                        stderr=subprocess.PIPE,
                        timeout=timeout
                    )
                    last_stdout = proc.stdout.decode('utf-8', errors='replace')
                    last_stderr = proc.stderr.decode('utf-8', errors='replace')
                except subprocess.TimeoutExpired:
                    return 'TIMEOUT', 'test timed out', last_stdout, last_stderr, dest_test_file
                except Exception as e:
                    return 'CRASH', f'test crashed: {e}', last_stdout, last_stderr, dest_test_file
                
                if proc.returncode != 0:
                    if verbose:
                        print(f"Execution failed for {test.name} ({main_class}):\n{last_stderr}")
                    return 'FAILURE', 'exit code not 0', last_stdout, last_stderr, dest_test_file
            
            return 'SUCCESS', 'exit code 0', last_stdout, last_stderr, dest_test_file

        except Exception as e:
            if verbose:
                print(f"Crash in {test.name}: {e}")
            return 'CRASH', f'harness crashed: {e}', '', '', dest_test_file

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
    parser.add_argument('--blacklist', default='./scripts/test-blacklist.txt', help="Blacklist file")
    parser.add_argument('--classpath', help="Classpath to use: 'stub', 'gnucp', or path. Default: gnucp")
    
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

        blacklist = []
        if os.path.exists(args.blacklist):
            try:
                with open(args.blacklist, 'r') as f:
                    blacklist = [line.strip() for line in f if line.strip()]
                print(f"Loaded {len(blacklist)} blacklist items from {args.blacklist}")
            except Exception as e:
                print(f"Error reading blacklist: {e}")

        print("Scanning for tests...")
        tests = scan_tests(roots, blacklist)
        
        outfile = args.testlist if args.testlist else 'test_whitelist.txt'
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
        print(f"Using classpath: {resolved_cp}")
        
        tests = []
        if args.testlist:
             print(f"Reading tests from {args.testlist}...")
             if not os.path.exists(args.testlist):
                 print(f"Error: Test list {args.testlist} not found.")
                 sys.exit(1)
             with open(args.testlist, 'r') as f:
                 for line in f:
                     path = line.strip()
                     if path and os.path.exists(path):
                         instructions = parse_test_file(path)
                         if instructions:
                             tests.append(Test(path, instructions, instructions['reason']))
        else:
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
        
        print(f"Running {len(tests)} tests using {args.compiler} and {args.jvm}...")
        
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
                executor.submit(run_single_test, test, args.compiler, args.jvm, args.compiler_args, args.timeout, resolved_cp, args.verbose): test 
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
                        
                    print(f"[{i+1}/{len(tests)}] {test.file_path} ({tmp_path}): {outcome} ({reason})")
                    
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
