# -*- coding: utf-8 -*- 
"""
AUTHOR: Tina Burns & Alborz Jelvani
LOCATION: Rutgers University
CLASS: CS211 Computer Architecture
SESSION: Summer 2025

DESCRIPTION:
This script tests the `csim` code for the final project. It compares the results
of the student's cache simulator against the reference implementation.

NOTES:
- Parts of this code were adapted from course materials with the instructor's permission.
- Some sections were generated with the assistance of AI and/or Large Language Models.

SOURCES:
Randal E. Bryant and David R. O'Hallaron, "Computer Systems: A Programmer's Perspective," 3rd Edition (CS:APP3e).
"""

import os
import subprocess
import signal
import sys
from typing import List, Tuple
import re
# Constants
MAX_STR = 1024
N = 8  # Number of tests


def usage(argv):
    print(f"Usage: {argv[0]} [-h]")
    print("Options:")
    print("  -h    Print this help message.")


def sigalrm_handler(signum, frame):
    print("Error: Program timed out.")
    print("TEST_CSIM_RESULTS=0,0,0,0,0,0")
    sys.exit(1)


def runtrace(s, E, b, trace) -> Tuple[bool, List[int], List[int]]:
    """Runs the reference and test simulators and collects results."""
    ref_results = [-1, -1, -1]
    test_results = [-1, -1, -1]

    # Run the reference simulator
    cmd = f"./csim-ref -s {s} -E {E} -b {b} -t {trace} > /dev/null"
    os.system("rm -rf .csim_results")
    status = os.system(cmd)
    if status != 0:
        print(f"Error running reference simulator: Status {status}")
        return False, ref_results, test_results

    # Get the results from the reference simulator
    try:
        with open(".csim_results", "r") as f:
            ref_results = list(map(int, f.readline().split()))
    except FileNotFoundError:
        print("Error: Results for reference simulator not found.")
        return False, ref_results, test_results

    # Run the test simulator
    os.system("rm -rf .csim_results")
    cmd = f"./csim -s {s} -E {E} -b {b} -t {trace} > .csim_results"
    

    status = os.system(cmd)
    if status != 0:
        print(f"Error running test simulator: Status {status}")
        return False, ref_results, test_results

    # Get the results from the test simulator
    try:
        with open(".csim_results", "r") as f:
            line = f.read()
            if "hits:" in line: 
                matches = re.findall(r'\b(?:hits|misses|evictions):(\d+)', line)
                if matches:
                    test_results = list(map(int, matches))
    except FileNotFoundError:
        print("Error: Results for test simulator not found.")
        return False, ref_results, test_results

    return True, ref_results, test_results


def test_csim():
    """Tests the cache simulator for correctness."""
    s = [1, 4, 2, 1, 2, 3, 5, 5]
    E = [1, 2, 1, 2, 2, 2, 1, 1]
    b = [1, 4, 4, 2, 3, 4, 5, 5]

    trace = [
        "traces/yi2.trace", "traces/yi.trace", "traces/dave.trace",
        "traces/trans.trace", "traces/trans.trace",  "traces/trans.trace",
        "traces/trans.trace", "traces/long.trace"
    ]


    total_points = 0
    points = [0] * N
    results = []

    # Run each test
    for i in range(N):
        status, ref_results, test_results = runtrace(s[i], E[i], b[i], trace[i])
        if not status:
            ref_results = [-1, -1, -1]
            test_results = [-1, -1, -1]
        results.append((ref_results, test_results))

    # Compute points
    for i in range(N):
        ref, test = results[i]
        if all(v != -1 for v in ref + test):
            points[i] += (ref[0] == test[0]) * 2
            points[i] += (ref[1] == test[1]) * 2
            points[i] += (ref[2] == test[2]) * 2
        total_points += points[i]



    # Display results
    print(f"\n{'':>17}{'Your simulator':>17}{'Reference simulator':>35}")
    print(f"{'Points':<6}{'(s,E,b)':<10}{'Hits':>8}{'Misses':>8}{'Evicts':>8}"
          f"{'Hits':>8}{'Misses':>8}{'Evicts':>8}")
    for i in range(N):
        ref, test = results[i]
        print(f"{points[i]:<6}({s[i]},{E[i]},{b[i]}): {test[0]:>8}{test[1]:>8}{test[2]:>8}"
              f"{ref[0]:>8}{ref[1]:>8}{ref[2]:>8} {trace[i]}")

    # Print compact summary string
    print(f"\nSCORE for Part A: {total_points:.2f}/48.00\n");  
    
    print("\nThe program will be evaluted for additional test and criteria not included in these test cases.\n");
    return total_points;
    
def main():
    import argparse

    parser = argparse.ArgumentParser(description="Test the cache simulator.")
    #parser.add_argument("-h", action="help", help="Print this help message.")
    args = parser.parse_args()

    # Install SIGALRM handler
    signal.signal(signal.SIGALRM, sigalrm_handler)
    signal.alarm(20)  # Timeout after 20 seconds

    test_csim()


if __name__ == "__main__":
    main()
