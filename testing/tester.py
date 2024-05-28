import unittest
import subprocess
import os
import json
from dataclasses import dataclass
from typing import TypedDict, List

__unittest = True # removes tracebacks, somehow lol

class ProgramTestCase(TypedDict):
    name: str
    file: str
    should_compile: bool
    expected_return_code: int

@dataclass
class RunResult:
    compile_success: bool
    exit_code: int
    stdout: str
    stderr: str

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
TEST_CASES_FILE = os.path.join(SCRIPT_DIR, "test_cases.json")
PROGRAMS_DIR = os.path.join(SCRIPT_DIR, "programs")


class TestCompiler(unittest.TestCase):

    @staticmethod
    def run_program(source_file: str) -> RunResult:
        # Compile the program
        compile_process = subprocess.run(
            [f"{SCRIPT_DIR}/../compiler", "compile", source_file],
            capture_output=True, text=True
        )
        if compile_process.returncode != 0:
            return RunResult(False, -1, compile_process.stdout, compile_process.stderr)

        # Run the compiled program
        run_process = subprocess.run(['./output'], capture_output=True, text=True)
        return RunResult(True, run_process.returncode, run_process.stdout, run_process.stderr)

    def check_program(self, source_file: str, case_info: ProgramTestCase):
        test_name = case_info["name"]
        result = self.run_program(source_file)
        self.assertEqual(result.compile_success, case_info["should_compile"], 
                         f"Unexpected compilation status for test '{test_name}' -\n{result.stderr}")
        if not result.compile_success:
            return
        self.assertEqual(result.exit_code, case_info["expected_return_code"], 
                         f"Status code mismatch for test '{test_name}'.")

def load_test_cases():
    with open(TEST_CASES_FILE, "r") as f:
        test_cases: List[ProgramTestCase] = json.load(f)

    for idx, program in enumerate(test_cases):
        source_file = os.path.join(PROGRAMS_DIR, program["file"])
        test_method = create_test_method(source_file, program)
        test_method.__name__ = f"test '{program['name']}'"
        setattr(TestCompiler, test_method.__name__, test_method)

def create_test_method(source_file: str, case_info: ProgramTestCase):
    def test(self: TestCompiler):
        self.check_program(source_file, case_info)
    return test


if __name__ == "__main__":
    load_test_cases()
    unittest.main(verbosity=2)
