import subprocess
import json
import argparse
import sys

# 測資定義區
# 規則：值都不重複、不送純 INIT、不超過 100 個
# expected_heap 按目前後端實作（bottom-up heapify + 嚴格大於）算
TEST_CASES = {
    # --- 每個 spec 指令各一條 ---
    "1": {
        "name": "INIT 多值建 heap (bottom-up)",
        "commands": ["INIT 10,20,30"],
        "expected_heap": [30, 20, 10]
    },
    "2": {
        "name": "INSERT 一路 sift 到 root",
        "commands": ["INIT 10,20,30", "INSERT 99"],
        "expected_heap": [99, 30, 10, 20]
    },
    "3": {
        "name": "EXTRACT 連續多次",
        "commands": ["INIT 50,40,30,20,10", "EXTRACT", "EXTRACT"],
        "expected_heap": [30, 20, 10]
    },
    "4": {
        "name": "UPDATE 改大要 sift up",
        "commands": ["INIT 50,40,30,20", "UPDATE 3 99"],
        "expected_heap": [99, 50, 30, 40]
    },
    "5": {
        "name": "UPDATE 改小要 sift down",
        "commands": ["INIT 50,40,30,20", "UPDATE 0 5"],
        "expected_heap": [40, 20, 30, 5]
    },
    "6": {
        "name": "DELETE 中間節點",
        "commands": ["INIT 50,40,30,20,10", "DELETE 1"],
        "expected_heap": [50, 20, 30, 10]
    },
    "7": {
        "name": "SORT 排序結果",
        "commands": ["INIT 30,10,50,20,40", "SORT"],
        "expected_heap": [50, 40, 30, 20, 10]
    },
    # --- Edge case ---
    "8": {
        "name": "邊界：DELETE 最後一個 index（不需 sift）",
        "commands": ["INIT 50,40,30,20", "DELETE 3"],
        "expected_heap": [50, 40, 30]
    },
    "9": {
        "name": "邊界：從空 heap INSERT 再 EXTRACT 變回空",
        "commands": ["INSERT 7", "EXTRACT"],
        "expected_heap": []
    },
    # --- 混合操作 ---
    "10": {
        "name": "混合：INIT + INSERT + EXTRACT + UPDATE",
        "commands": ["INIT 10,20,30", "INSERT 50", "EXTRACT", "UPDATE 0 5"],
        "expected_heap": [20, 5, 10]
    },
    "11": {
        "name": "邊界：INIT 只放一個值",
        "commands": ["INIT 7"],
        "expected_heap": [7]
    },
    "12": {
        "name": "邊界：DELETE root 應等同 EXTRACT",
        "commands": ["INIT 50,40,30,20", "DELETE 0"],
        "expected_heap": [40, 20, 30]
    },
    "13": {
        "name": "混合：連續 INSERT 多次後連續 EXTRACT",
        "commands": ["INSERT 10", "INSERT 20", "INSERT 30", "INSERT 5", "EXTRACT", "EXTRACT"],
        "expected_heap": [10, 5]
    },
    "14": {
        "name": "混合：INIT 後兩個不同方向的 UPDATE",
        "commands": ["INIT 50,40,30,20", "UPDATE 2 99", "UPDATE 1 5"],
        "expected_heap": [99, 20, 50, 5]
    },
    "15": {
        "name": "混合：SORT 後再INSERT",
        "commands": ["INIT 30,10,50,20,40", "SORT", "INSERT 60"],
        "expected_heap": [60, 40, 50, 20, 10, 30]
    },
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
    passed = final_heap == expected
    if passed:
        print(f"✅ Pass! 最終 Heap 正確: {final_heap}")
    else:
        print(f"❌ Fail! 預期: {expected}, 實際: {final_heap}")
    return passed

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--case", type=str, help="跑單一 case 編號，例如 1")
    parser.add_argument("--all", action="store_true", help="一次跑完全部 case")
    parser.add_argument("--exe", type=str, default="./main")
    args = parser.parse_args()

    if args.all:
        results = {}
        for cid in TEST_CASES:
            results[cid] = run_test(cid, args.exe)
        print("\n========== 全部測試結果總結 ==========")
        for cid, ok in results.items():
            mark = "✅" if ok else "❌"
            print(f"  {mark} Case {cid}: {TEST_CASES[cid]['name']}")
        passed_count = sum(results.values())
        total = len(results)
        print(f"\n共 {total} 條，通過 {passed_count}，失敗 {total - passed_count}")
        sys.exit(0 if passed_count == total else 1)
    elif args.case:
        run_test(args.case, args.exe)
    else:
        print("❌ 請指定 --case <編號> 或 --all"); sys.exit(1)