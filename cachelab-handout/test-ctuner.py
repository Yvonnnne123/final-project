#!/usr/bin/env python3

import subprocess
import re

def parse_summary(output):
    """
    Extracts hits, misses, evictions from csim output.
    """
    m = re.search(r'hits:(\d+)\s+misses:(\d+)\s+evictions:(\d+)', output)
    if not m:
        return None
    return int(m.group(1)), int(m.group(2)), int(m.group(3))

def calculate_metric(metric, hits, misses, evictions):
    total = hits + misses
    if total == 0:
        return 0.0
    if metric == 'h':
        return (hits / total) * 100.0
    elif metric == 'm':
        return (misses / total) * 100.0
    elif metric == 'e':
        return (evictions / total) * 100.0
    else:
        raise ValueError("Unknown metric")

TRACES = [
    "traces/yi.trace",
    "traces/yi2.trace",
    "traces/trans.trace",
    "traces/long.trace"
]

CTUNER = "./ctuner"
CSIM_REF = "./csim-ref"
CTUNER_REF = "./ctuner-ref"

def run_test(metric, target, cmp, tracefile):
    print(f"Test: {CTUNER} -p {metric} -r {target} -b {CSIM_REF} -t {tracefile}")
    try:
        result = subprocess.run(
            [CTUNER, "-p", metric, "-r", str(target), "-b", CSIM_REF, "-t", tracefile],
            capture_output=True, text=True, timeout=60
        )
        out = result.stdout.strip()

        if out == "No valid configuration found":
            result_ref = subprocess.run([CTUNER_REF, "-p", metric, "-r", str(target), "-b", CSIM_REF, "-t", tracefile],capture_output=True, text=True, timeout=60)
            ref_out = result_ref.stdout.strip()
            if ref_out == "No valid configuration found":
                print(f"  ctuner output: {out}")
                print(f"  ctuner-ref output: {ref_out}")
                print("  PASS")
                return True
            else:
                print(f"  ctuner output: {out}")
                print(f"  ctuner-ref output: {ref_out}")
                print("  FAIL")
                return False
            
        print(f"  ctuner output: {out}")
        args = out.split()
        csim_run = subprocess.run(args, capture_output=True, text=True, timeout=60)
        sim_out = csim_run.stdout
        print(f"  csim output: {sim_out.strip()}")
        parsed = parse_summary(sim_out)
        if not parsed:
            print("  FAIL: Could not parse csim output")
            return False
        hits, misses, evicts = parsed
        actual = calculate_metric(metric, hits, misses, evicts)
        print(f"  Result: {actual:.2f}%")
        passed = (actual > target) if cmp == '>' else (actual < target)
        print("  PASS" if passed else "  FAIL")
        return passed
    except subprocess.TimeoutExpired:
        print("  FAIL: Timed out")
        return False
    except Exception as e:
        print(f"  FAIL: {e}")
        return False

def main():
    total = 0
    passed = 0
    passed_max = 0
    for t in TRACES:
        for metric in ["h","m", "e"]:
            if metric == "h":
                for target in [30.00, 40.00, 80.00, 95.00]:
                    res = run_test(metric, target, ">", t)
                    if res:
                        passed += 1
                    total +=1
            else:
                for target in [10.00, 20.00, 30.00, 40.00]:
                    res =  run_test(metric, target, "<", t)
                    if res:
                        passed += 1
                    total +=1

    print(f"\nPassed {passed}/{total} tests.")

if __name__ == "__main__":
    main()
