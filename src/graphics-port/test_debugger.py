from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries
import hashlib
import json
import subprocess
import time

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'analysis/graphics-port'
debugger = OUT / 'fm5_debugger.exe'
fixture = OUT / 'debug_fixture.exe'

def run_fixture(name, arguments, timeout, debugger_arguments=()):
    path = OUT / (name + '.jsonl')
    started = time.monotonic()
    result = subprocess.run([str(debugger), '--log', str(path), '--timeout',
                             str(timeout), *debugger_arguments, '--fixture', '--', str(fixture), *arguments],
                            text=True, capture_output=True, timeout=timeout+15)
    rows = [json.loads(line) for line in path.read_text().splitlines()]
    return result, rows, time.monotonic()-started

crash, rows, elapsed = run_fixture('debugger-fixture-crash', [], 10)
exceptions = [x for x in rows if x['event'] == 'exception']
assert any(x['code'] == '0xc0000005' and x['first_chance'] for x in exceptions)
assert any(x['code'] == '0xc0000005' and not x['first_chance'] for x in exceptions)
assert any(x['event'] == 'crash_context_amd64' and int(x['rip'], 16) for x in rows)
assert any(x['event'] == 'crash_stack_bytes' and x['read'] == 128 for x in rows)
assert any(x['event'] == 'crash_stack_bytes' and len(x['qwords']) == 16 for x in rows)
assert any(x['event'] == 'crash_instruction_bytes' and x['read'] == 64 for x in rows)
assert any(x['event'] == 'process_exit' and x['exit_code'] == '0xc0000005' for x in rows)
assert not any(x['event'].startswith('timeout_') for x in rows)
assert len({x['pid'] for x in rows}) == 1

timed, timed_rows, timed_elapsed = run_fixture('debugger-fixture-timeout', ['--wait'], 1)
assert any(x['event'] == 'timeout_close_requested' for x in timed_rows)
assert any(x['event'] == 'timeout_terminate_child' for x in timed_rows)
assert any(x['event'] == 'process_exit' and x['exit_code'] == '0xe0000001' for x in timed_rows)
assert timed_elapsed < 10

refused = subprocess.run([str(debugger), '--log', str(OUT/'should-not-exist.jsonl'),
                          '--', r'C:\Windows\System32\notepad.exe'],
                         capture_output=True, text=True, timeout=5)
assert refused.returncode == 2
assert not (OUT/'should-not-exist.jsonl').exists()

add_python_libraries(__file__)
import pefile
fixture_pe = pefile.PE(str(fixture))
marker = next(x.address for x in fixture_pe.DIRECTORY_ENTRY_EXPORT.symbols
              if x.name == b'FM5FixtureBreakTarget')
break_test, break_rows, break_elapsed = run_fixture('debugger-fixture-breakpoint', [], 10,
                                                   ['--break-rva', hex(marker)])
assert len([x for x in break_rows if x['event'] == 'breakpoint_hit']) == 1
assert any(x['event'] == 'breakpoint_hit' and x['original_instruction_restored'] for x in break_rows)
assert any(x['event'] == 'breakpoint_stack_bytes' and x['read'] == 1024 and len(x['qwords']) == 128
           for x in break_rows)
assert any(x['event'] == 'process_exit' and x['exit_code'] == '0xc0000005' for x in break_rows)
break_snapshot = next(x for x in break_rows if x['event'] == 'breakpoint_context_amd64')
image = next(x for x in break_rows if x['event'] == 'process_image')
assert int(break_snapshot['rip'], 16) - int(image['base'], 16) == marker

report = {
    'passed': True,
    'debugger_sha256': hashlib.sha256(debugger.read_bytes()).hexdigest(),
    'fixture_sha256': hashlib.sha256(fixture.read_bytes()).hexdigest(),
    'crash_exit_code': hex(crash.returncode & 0xffffffff),
    'crash_elapsed_seconds': elapsed,
    'timeout_elapsed_seconds': timed_elapsed,
    'checks': [
        'Own fixture access violation observed at first and second chance',
        'AMD64 register context and 128 stack bytes / 64 instruction bytes captured',
        'Module bases and image paths recorded',
        'Timeout requests WM_CLOSE before terminating only the created child',
        'An unrelated Windows executable is refused before process creation',
        'One-shot software breakpoint restores instruction and RIP; fixture then reaches original crash',
        'Breakpoint snapshot includes 1024 stack bytes and 128 decoded QWORDs',
        'Game executable was not launched by this verification',
    ],
    'limitations': [
        'Debugging changes timing and IsDebuggerPresent behavior.',
        'First-chance Microsoft C++ exceptions are counted, not logged individually.',
        'Raw stack bytes are not a symbolized stack trace or a full memory dump.',
        'Only this child is debugged; processes launched by it are outside coverage.',
    ],
}
(OUT/'debugger-verification.json').write_text(json.dumps(report, indent=2)+'\n')
print(json.dumps(report, indent=2))
