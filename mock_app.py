import subprocess
import json
import argparse
import sys

# 🧪 測資定義區 (C 小組可自行新增 Edge Cases)
TEST_CASES = {
    "1": {
        "name": "測試 Max-Heap: INSERT 極大值與 Sift Up",
        "commands": ["INIT 10,20,30", "INSERT 99"],
        "expected_heap": [99, 30, 20, 10] 
    },
    "2": {
        "name": "測試連續 Extract",
        "commands": ["INIT 50,40,30,20,10", "EXTRACT", "EXTRACT"],
        "expected_heap": [30, 20, 10]
    }
}

def run_test(case_id, executable_path):
    if case_id not in TEST_CASES:
        print(f"❌ 找不到測資 Case {case_id}"); sys.exit(1)

    case = TEST_CASES[case_id]
    print(f"========== 啟動測試: Case {case_id} [{case['name']}] ==========")

    try:
        # text=True 自動處理編解碼
        proc = subprocess.Popen(
            executable_path, stdin=subprocess.PIPE, stdout=subprocess.PIPE, 
            stderr=sys.stderr, text=True 
        )
    except FileNotFoundError:
        print("❌ 找不到執行檔，請先編譯 C 程式碼！"); sys.exit(1)

    final_heap = []
    
    # 消耗掉 C 程式剛啟動時吐出的初始 IDLE
    proc.stdout.readline() 
    
    for cmd in case["commands"]:
        print(f"\n[Python 前端] 送出指令 ➡️  {cmd}")
        proc.stdin.write(cmd + "\n")
        proc.stdin.flush()

        while True:
            line = proc.stdout.readline()
            if not line: break
                
            line = line.strip()
            if not line: continue

            try:
                state = json.loads(line)
            except json.JSONDecodeError:
                print(f"❌ JSON 解析失敗: {line}"); continue

            event = state.get("event", "UNKNOWN")
            final_heap = state.get("heap", [])
            is_idle = state.get("is_idle", True)
            
            print(f"  ⬅️ [C 後端] 事件: {event.ljust(10)} | 陣列: {final_heap}")

            if is_idle:
                break
            else:
                # 模擬點擊「下一步」，解除阻塞
                proc.stdin.write("\n")
                proc.stdin.flush()

    proc.stdin.close()
    proc.wait(timeout=2)

    print("\n========== 測試結果驗證 ==========")
    expected = case["expected_heap"]
    if final_heap == expected:
        print(f"✅ Pass! 最終 Heap 正確: {final_heap}")
    else:
        print(f"❌ Fail! 預期: {expected}, 實際: {final_heap}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--case", type=str, required=True)
    parser.add_argument("--exe", type=str, default="./main")
    args = parser.parse_args()
    run_test(args.case, args.exe)