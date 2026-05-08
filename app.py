import flet as ft
import subprocess
import json
import sys
import threading
import os

from state import VisualState
from ui_components import create_heap_view, get_status_text


class BackendController:
    """管理 C 語言子程序的生命週期與 Threading"""
    def __init__(self, on_state_update):
        # 自動編譯 C 專案
        exe_name = "backend.exe" if sys.platform == "win32" else "./backend"    
        subprocess.run(["gcc", "main.c", "heap_logic.c", "-o", exe_name.replace("./", "")], check=True)
        
        self.proc = subprocess.Popen(
            [exe_name], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True, bufsize=1
        )
        self.on_state_update = on_state_update
        
        # 啟動背景讀取執行緒，保證 UI 絕對不卡死
        threading.Thread(target=self._read_loop, daemon=True).start()

    def _read_loop(self):
        while True:
            line = self.proc.stdout.readline()
            if not line: break
            try:
                data = json.loads(line.strip())
                # 手動字典解包至 Dataclass，確保型別安全且無外部依賴
                state = VisualState(
                    heap=data.get("heap", []),
                    event=data.get("event", "IDLE"),
                    targets=data.get("targets", []),
                    is_idle=data.get("is_idle", True)
                )
                self.on_state_update(state) # 觸發 UI 更新
            except Exception as e:
                print(f"IPC 解析錯誤: {e}")

    def send_command(self, cmd: str):
        self.proc.stdin.write(cmd + "\n")
        self.proc.stdin.flush()

def main(page: ft.Page):
    page.title = "Heap Visualizer"
    page.horizontal_alignment = ft.CrossAxisAlignment.CENTER
    page.theme_mode = ft.ThemeMode.LIGHT

    # --- UI 元件初始化 ---
    status_text = ft.Text("初始化中...", size=20, color=ft.Colors.BLUE_700)
    heap_container = ft.Container(height=100)
    
    val_input = ft.TextField(label="輸入數字", width=100)
    insert_btn = ft.ElevatedButton("Insert")
    step_btn = ft.ElevatedButton("下一步 (Step)", disabled=True)
    
    # --- UI 更新回呼函數 (給背景執行緒呼叫) ---
    def handle_state_update(state: VisualState):
        status_text.value = get_status_text(state)
        heap_container.content = create_heap_view(state)
        
        # UI 防呆控制：演算法進行中鎖定輸入，閒置時解鎖
        val_input.disabled = not state.is_idle
        insert_btn.disabled = not state.is_idle
        step_btn.disabled = state.is_idle
        page.update()

    # 啟動後端控制器
    backend = BackendController(on_state_update=handle_state_update)

    # --- 事件綁定 ---
    def on_insert(e):
        if val_input.value.isdigit():
            backend.send_command(f"INSERT {val_input.value}")
            val_input.value = ""

    def on_step(e):
        step_btn.disabled = True # 防止連點過快導致管線崩潰
        page.update()
        backend.send_command("") # 送出空字串 + \n 解鎖 C 語言

    insert_btn.on_click = on_insert
    step_btn.on_click = on_step

    # --- 排版與掛載 ---
    controls_row = ft.Row([val_input, insert_btn, step_btn], alignment=ft.MainAxisAlignment.CENTER)
    page.add(
        ft.Container(height=30),
        ft.Text("Demo", size=32, weight=ft.FontWeight.BOLD),
        status_text,
        ft.Container(height=20),
        heap_container,
        ft.Container(height=20),
        controls_row
    )

ft.app(target=main)