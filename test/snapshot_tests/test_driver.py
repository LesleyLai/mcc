import os
import subprocess
from typing import List, NamedTuple

def test_c_file(path: str, base: str):
    res = subprocess.run(["./mcc", f"{base}.c"], cwd=path)
    if res.returncode != 0:
        print(f'Failed to compile ${base}.c')
        return
    
    res = subprocess.run([os.path.join(path, "file")], cwd=path)

    received_filename = f"{base}.received.txt"
    with open(os.path.join(path, received_filename), mode="w") as file:
        file.write("Return code of main:\n")
        file.write(f"{res.returncode}\n")

    approved_filename = f"{base}.approaved.txt"
    # approved_exist = os.path.exists(os.path.join(path, approved_filename))

    #print(approved_exist)
    
class TestFile(NamedTuple):
    path: str
    base: str

def main():
    test_cases: List[TestFile] = []
    for root, subdirs, files in os.walk("."):
        for filename in files:
            base, extension = os.path.splitext(filename)
            if extension == '.c':
                test_cases.append(TestFile(root, base))

    case_count = len(test_cases)
    print(f"Total {case_count} test cases")
    for i, test_case in enumerate(test_cases):
        path, base = test_case
        src_file = f"{os.path.join(path, base)}.c"
        print(f"[{i + 1}/{case_count}] Testing {src_file}")

        test_c_file(path, base)
        print("Pass!")


if __name__ == "__main__":
    main()