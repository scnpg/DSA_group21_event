import flet as ft
import subprocess
import json
import sys
import threading
import os

from state import VisualState
from ui_components import create_heap_view, get_status_text

class BackendController:
    def __init__(self, on_state_update):
        exe_name = "backend.exe" if sys.platform == "win32" else "./backend"    
        subprocess.run(["gcc", "main.c", "heap_logic.c", "-o", exe_name.replace("./", "")], check=True)
        
        self.proc = subprocess.Popen(
            [exe_name], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True, bufsize=1
        )
        self.on_state_update = on_state_update
        threading.Thread(target=self._read_loop, daemon=True).start()

    def _read_loop(self):
        while True:
            line = self.proc.stdout.readline()
            if not line: break
            try:
                data = json.loads(line.strip())
                state = VisualState(
                    heap=data.get("heap", []),
                    event=data.get("event", "IDLE"),
                    targets=data.get("targets", []),
                    is_idle=data.get("is_idle", True),
                    sort_boundary=data.get("sort_boundary", -1)
                )
                self.on_state_update(state)
            except Exception as e:
                print(f"IPC 解析錯誤: {e}")

    def send_command(self, cmd: str):
        self.proc.stdin.write(cmd + "\n")
        self.proc.stdin.flush()

def main(page: ft.Page):
    page.title = "Heap Visualizer"
    page.horizontal_alignment = ft.CrossAxisAlignment.CENTER
    page.theme_mode = ft.ThemeMode.LIGHT
    page.window_width = 1100 

    # 1. Define UI Components FIRST
    status_text = ft.Text("初始化中...", size=20, color=ft.Colors.BLUE_700)
    tree_view = ft.Container(expand=True, height=400, border=ft.border.all(1, ft.Colors.GREY_300), border_radius=10)
    array_view = ft.Container(expand=True, height=400, border=ft.border.all(1, ft.Colors.GREY_300), border_radius=10)
    
    val_input = ft.TextField(label="輸入數字", width=100)
    insert_btn = ft.ElevatedButton("Insert")
    sort_btn = ft.ElevatedButton("Sort Heap", bgcolor=ft.Colors.ORANGE_800, color=ft.Colors.WHITE)
    step_btn = ft.ElevatedButton("下一步 (Step)", disabled=True)
    remove_btn = ft.ElevatedButton(
        "Remove Top", 
        bgcolor=ft.Colors.RED_700, 
        color=ft.Colors.WHITE,
        disabled=True
    )
    
    # 2. Define Callback
    def handle_state_update(state: VisualState):
        status_text.value = get_status_text(state)
        tree_view.content = create_heap_view(state, mode="tree")
        array_view.content = create_heap_view(state, mode="array")

        is_idle = state.is_idle
        has_data = len(state.heap) > 0
        
        # Enable/Disable logic
        val_input.disabled = not state.is_idle
        insert_btn.disabled = not state.is_idle

        sort_btn.disabled = not state.is_idle
        remove_btn.disabled = not (is_idle and has_data)

        step_btn.disabled = state.is_idle

        page.update()

    backend = BackendController(on_state_update=handle_state_update)

    # 3. Define Event Handlers
    def on_insert(e):
        if val_input.value.isdigit():
            backend.send_command(f"INSERT {val_input.value}")
            val_input.value = ""
            page.update()

    def on_sort(e):
        backend.send_command("SORT")
        page.update()

    def on_step(e):
        backend.send_command("") # Send empty line to C
        page.update()

    def on_remove(e):
        backend.send_command("REMOVE")
        page.update()

    # 4. Bind Events to Buttons
    insert_btn.on_click = on_insert
    sort_btn.on_click = on_sort
    step_btn.on_click = on_step
    remove_btn.on_click = on_remove

    # 5. Build Layout
    page.add(
        ft.Container(height=20),
        ft.Text("Heap Sort Visualizer", size=32, weight=ft.FontWeight.BOLD),
        status_text,
        ft.Row(
            [
                ft.Column([ft.Text("Tree View", weight="bold"), tree_view], expand=1),
                ft.Column([ft.Text("Array View (10 per row)", weight="bold"), array_view], expand=1),
            ],
            alignment=ft.MainAxisAlignment.CENTER,
            spacing=20,
        ),
        ft.Row([val_input, insert_btn, sort_btn, remove_btn, step_btn], alignment=ft.MainAxisAlignment.CENTER)
    )

ft.app(target=main)
