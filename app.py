import flet as ft
import subprocess
import json
import sys
import threading
import time
import math

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
            if not line:
                break
            try:
                data = json.loads(line.strip())
                state = VisualState(
                    heap=data.get("heap", []),
                    event=data.get("event", "IDLE"),
                    targets=data.get("targets", []),
                    is_idle=data.get("is_idle", True),
                    sort_boundary=data.get("sort_boundary", -1),
                    is_max_heap=data.get("is_max_heap", True),
                    stats=data.get("stats", {"cur_swap": 0, "cur_cmp": 0, "tot_swap": 0, "tot_cmp": 0})
                )
                self.on_state_update(state)
            except Exception as e:
                print(f"IPC 解析錯誤: {e}")
                self.on_state_update(VisualState(event="IPC_ERROR", is_idle=True))
        # 後端程序已結束（stdout EOF）
        self.on_state_update(VisualState(event="BACKEND_DIED", is_idle=True))

    def send_command(self, cmd: str):
        self.proc.stdin.write(cmd + "\n")
        self.proc.stdin.flush()


# ── 小工具：把逗號分隔字串驗證成整數清單，回傳 "1,2,3" 或 None ──
def parse_int_list(raw: str):
    try:
        parts = [str(int(p.strip())) for p in raw.split(",") if p.strip()]
        return ",".join(parts) if parts else None
    except ValueError:
        return None

def parse_int(raw: str):
    try:
        return int(raw.strip())
    except ValueError:
        return None


def main(page: ft.Page):
    page.title = "Heap Visualizer"
    page.horizontal_alignment = ft.CrossAxisAlignment.CENTER
    page.theme_mode = ft.ThemeMode.LIGHT
    page.window_width = 1200
    page.window_height = 820
    page.scroll = ft.ScrollMode.AUTO

    # ── 視覺區塊 ──────────────────────────────────────────────────
    status_text     = ft.Text("初始化中...", size=16, color=ft.Colors.BLUE_700)
    heap_type_badge = ft.Container(
        content=ft.Text("Max Heap", size=13, weight="bold", color="white"),
        bgcolor=ft.Colors.BLUE_700,
        border_radius=8,
        padding=ft.padding.symmetric(horizontal=12, vertical=4),
    )
    complexity_view = ft.Container()   # 由 build_complexity_view() 動態填充

    _last_stats = [{"cur_cmp": 0, "cur_swap": 0, "tot_cmp": 0, "tot_swap": 0}]
    BAR_MAX_W   = 300

    def build_complexity_view(n: int, stats: dict, is_idle: bool):
        if n == 0:
            return ft.Container()

        log_n   = math.log2(n)
        s       = _last_stats[0] if is_idle else stats
        cur_ops = s.get("cur_cmp", 0)          # 只計比較次數（交換是附帶動作）
        cur_swp = s.get("cur_swap", 0)
        op_name, op_complexity, op_color = _current_op[0]

        # 動態縮放基準：比較與交換分開計算各自的 max_ref
        max_ref_cmp = max(n, cur_ops, 1)
        max_ref_swp = max(n, cur_swp, 1)

        def bar_row(label: str, value: float, max_r: float, color, show_val: str):
            filled_w = max(4, int(BAR_MAX_W * value / max_r))
            empty_w  = max(0, BAR_MAX_W - filled_w)
            return ft.Row([
                # 標籤固定寬度，文字過長允許換行，避免擠進 bar
                ft.Container(
                    content=ft.Text(label, size=11, color=ft.Colors.GREY_700),
                    width=185,
                ),
                ft.Container(width=filled_w, height=14, bgcolor=color,
                             border_radius=ft.border_radius.only(
                                 top_left=3, bottom_left=3,
                                 top_right=(3 if empty_w == 0 else 0),
                                 bottom_right=(3 if empty_w == 0 else 0))),
                ft.Container(width=empty_w, height=14,
                             bgcolor=ft.Colors.GREY_200,
                             border_radius=ft.border_radius.only(
                                 top_right=3, bottom_right=3)),
                ft.Text(f"  {show_val}", size=11, color=ft.Colors.GREY_600),
            ], spacing=0, vertical_alignment=ft.CrossAxisAlignment.CENTER)

        # 本次比較標示：超過 n 時顯示倍數
        if cur_ops > 0 and n > 0:
            multiplier = cur_ops / n
            ops_label = (f"{cur_ops} 次  ≈ {multiplier:.1f}n"
                         if multiplier > 1.2 else f"{cur_ops} 次")
        else:
            ops_label = f"{cur_ops} 次"

        swp_label = f"{cur_swp} 次"

        return ft.Container(
            content=ft.Column([
                # ── 操作類型標籤 ──
                ft.Row([
                    ft.Text("本次操作：", size=12, color=ft.Colors.GREY_600),
                    ft.Container(
                        content=ft.Text(op_name, size=12, weight="bold", color="white"),
                        bgcolor=op_color,
                        border_radius=6,
                        padding=ft.padding.symmetric(horizontal=10, vertical=3),
                    ),
                    ft.Container(width=8),
                    ft.Container(
                        content=ft.Text(op_complexity, size=12, weight="bold", color=op_color),
                        border=ft.border.all(1.5, op_color),
                        border_radius=6,
                        padding=ft.padding.symmetric(horizontal=10, vertical=3),
                    ),
                    ft.Container(width=16),
                    ft.Text(f"n = {n}　｜　log₂(n) ≈ {log_n:.1f}",
                            size=12, color=ft.Colors.GREY_500),
                ], spacing=4, vertical_alignment=ft.CrossAxisAlignment.CENTER),

                # ── 四條長條圖 ──
                bar_row("n  （線性基準 O(n)）",
                        n,       max_ref_cmp, ft.Colors.GREY_400,  f"{n} 次"),
                bar_row("log₂n  （對數基準 O(log n)）",
                        log_n,   max_ref_cmp, ft.Colors.BLUE_400,  f"{log_n:.1f} 次"),
                bar_row("比較次數  （決定複雜度）",
                        cur_ops, max_ref_cmp, op_color,            ops_label),
                bar_row("交換次數  （資料移動量）",
                        cur_swp, max_ref_swp, ft.Colors.PINK_400,  swp_label),
            ], spacing=6),
            padding=ft.padding.symmetric(horizontal=16, vertical=10),
            bgcolor=ft.Colors.GREY_50,
            border_radius=8,
            border=ft.border.all(1, ft.Colors.GREY_200),
            width=640,
        )
    tree_view  = ft.Container(
        width=660, height=420,
        border=ft.border.all(1, ft.Colors.GREY_300),
        border_radius=10,
        clip_behavior=ft.ClipBehavior.HARD_EDGE,
    )
    tree_scroll = ft.Column(
        [tree_view],
        scroll=ft.ScrollMode.AUTO,
        width=660,
    )
    array_view = ft.Container(
        width=480, height=420,
        border=ft.border.all(1, ft.Colors.GREY_300),
        border_radius=10,
    )

    # ── 共用輸入框 ────────────────────────────────────────────────
    # INIT / INSERT 共用同一個輸入框（逗號分隔，支援負數）
    bulk_input      = ft.TextField(label="數字（逗號分隔）", width=220, hint_text="例：5,-3,10")

    # UPDATE 需要 index + value 兩個欄位
    update_idx_input = ft.TextField(label="Index", width=80)
    update_val_input = ft.TextField(label="新值",  width=100)

    # DELETE / SEARCH 各自一個欄位
    delete_idx_input = ft.TextField(label="Index", width=80)
    search_val_input = ft.TextField(label="搜尋值", width=100)

    # ── 按鈕定義 ──────────────────────────────────────────────────
    init_btn   = ft.ElevatedButton("INIT",
                                   bgcolor=ft.Colors.TEAL_700, color=ft.Colors.WHITE)
    insert_btn = ft.ElevatedButton("INSERT",
                                   bgcolor=ft.Colors.BLUE_700, color=ft.Colors.WHITE)
    remove_btn = ft.ElevatedButton("REMOVE TOP",
                                   bgcolor=ft.Colors.RED_700,  color=ft.Colors.WHITE, disabled=True)

    update_btn = ft.ElevatedButton("UPDATE",
                                   bgcolor=ft.Colors.INDIGO_600, color=ft.Colors.WHITE, disabled=True)
    delete_btn = ft.ElevatedButton("DELETE",
                                   bgcolor=ft.Colors.DEEP_ORANGE_700, color=ft.Colors.WHITE, disabled=True)
    search_btn = ft.ElevatedButton("SEARCH",
                                   bgcolor=ft.Colors.PURPLE_700, color=ft.Colors.WHITE, disabled=True)

    sort_btn   = ft.ElevatedButton("SORT",
                                   bgcolor=ft.Colors.ORANGE_800, color=ft.Colors.WHITE)
    invert_btn = ft.ElevatedButton("INVERT (Max↔Min)",
                                   bgcolor=ft.Colors.GREEN_800,  color=ft.Colors.WHITE, disabled=True)
    clear_btn  = ft.ElevatedButton("CLEAR",
                                   bgcolor=ft.Colors.GREY_700,   color=ft.Colors.WHITE, disabled=True)
    step_btn   = ft.ElevatedButton("⏭ 下一步 (Step)", disabled=True)

    # ── 呼吸燈：包在 step_btn 外層的螢光圈 ───────────────────────────
    step_glow = ft.Container(
        width=180, height=52,
        border_radius=10,
        border=ft.border.all(3, ft.Colors.AMBER_400),
        shadow=ft.BoxShadow(
            spread_radius=2,
            blur_radius=14,
            color=ft.Colors.with_opacity(0.85, ft.Colors.AMBER_300),
            offset=ft.Offset(0, 0),
        ),
        opacity=0,
        animate_opacity=ft.Animation(750, ft.AnimationCurve.EASE_IN_OUT),
    )

    # Stack：螢光圈在下，按鈕在上
    step_wrapper = ft.Container(
        content=ft.Stack(
            [
                ft.Container(content=step_glow, alignment=ft.alignment.center),
                ft.Container(content=step_btn,  alignment=ft.alignment.center),
            ],
            width=180, height=52,
        ),
        width=180, height=52,
    )

    # ── 所有可以 disable 的輸入欄位清單（操作進行中鎖定）────────────
    all_inputs = [bulk_input, update_idx_input, update_val_input,
                  delete_idx_input, search_val_input]

    # ── 目前操作類型（用於複雜度標籤）───────────────────────────────
    # 格式: [("操作名稱", "O(...)", color)]
    _current_op = [("—", "—", ft.Colors.GREY_500)]

    # ── State Update Callback ─────────────────────────────────────
    _glow_active = [False]   # 用 list 讓 inner function 可以修改

    def _refresh_ui(state: VisualState):
        """實際更新所有 UI 元件（供 handle_state_update 和自動步進共用）"""
        status_text.value = get_status_text(state)
        # 更新 Max / Min Heap 徽章
        if state.is_max_heap:
            heap_type_badge.content = ft.Text("Max Heap", size=13, weight="bold", color="white")
            heap_type_badge.bgcolor = ft.Colors.BLUE_700
        else:
            heap_type_badge.content = ft.Text("Min Heap", size=13, weight="bold", color="white")
            heap_type_badge.bgcolor = ft.Colors.GREEN_700
        s = state.stats
        if not state.is_idle:
            _last_stats[0] = s
        complexity_view.content = build_complexity_view(
            len(state.heap), s, state.is_idle
        )
        tree_view.content  = create_heap_view(state, mode="tree")
        array_view.content = create_heap_view(state, mode="array")

    def handle_state_update(state: VisualState):
        _refresh_ui(state)

        idle     = state.is_idle
        has_data = len(state.heap) > 0

        # 輸入框：操作中全部鎖定
        for inp in all_inputs:
            inp.disabled = not idle

        # 不需要資料的按鈕
        init_btn.disabled   = not idle
        insert_btn.disabled = not idle
        sort_btn.disabled   = not idle
        step_btn.disabled   = idle          # 只有操作進行中才能按

        # 呼吸燈跟著 step_btn 的啟用狀態開關
        _glow_active[0] = not idle
        if idle:
            step_glow.opacity = 0   # 立刻熄滅

        # 需要有資料才能操作的按鈕
        remove_btn.disabled = not (idle and has_data)
        update_btn.disabled = not (idle and has_data)
        delete_btn.disabled = not (idle and has_data)
        search_btn.disabled = not (idle and has_data)
        invert_btn.disabled = not (idle and has_data)
        clear_btn.disabled  = not (idle and has_data)

        page.update()

    backend = BackendController(on_state_update=handle_state_update)

    # ── 呼吸燈動畫執行緒 ──────────────────────────────────────────
    def glow_loop():
        """在 step_btn 啟用時，讓螢光圈 opacity 在 0.15 ↔ 1.0 之間交替，
        搭配 animate_opacity=750ms 達成平滑呼吸效果。"""
        while True:
            if _glow_active[0]:
                step_glow.opacity = 1.0
                page.update()
                time.sleep(0.85)
                if _glow_active[0]:          # 雙重確認，避免 race condition
                    step_glow.opacity = 0.1
                    page.update()
                    time.sleep(0.85)
            else:
                time.sleep(0.05)             # idle 時低耗能等待

    threading.Thread(target=glow_loop, daemon=True).start()

    # ── Event Handlers ────────────────────────────────────────────
    def show_error(field: ft.TextField, msg: str):
        field.error_text = msg
        page.update()

    def clear_error(field: ft.TextField):
        field.error_text = None

    # INIT：批次建堆
    def on_init(e):
        clear_error(bulk_input)
        validated = parse_int_list(bulk_input.value)
        if validated:
            _current_op[0] = ("Build Heap", "O(n)", ft.Colors.TEAL_700)
            backend.send_command(f"INIT {validated}")
            bulk_input.value = ""
            page.update()
        else:
            show_error(bulk_input, "請輸入整數（逗號分隔）")

    # INSERT：逐一插入（支援逗號分隔多個值）
    def on_insert(e):
        clear_error(bulk_input)
        validated = parse_int_list(bulk_input.value)
        if validated:
            _current_op[0] = ("Insert", "O(log n)", ft.Colors.BLUE_700)
            backend.send_command(f"INSERT {validated}")
            bulk_input.value = ""
            page.update()
        else:
            show_error(bulk_input, "請輸入整數（逗號分隔）")

    # REMOVE TOP
    def on_remove(e):
        _current_op[0] = ("Extract Top", "O(log n)", ft.Colors.RED_700)
        backend.send_command("REMOVE")
        page.update()

    # UPDATE
    def on_update(e):
        clear_error(update_idx_input)
        clear_error(update_val_input)
        idx = parse_int(update_idx_input.value)
        val = parse_int(update_val_input.value)
        if idx is None:
            show_error(update_idx_input, "請輸入整數")
            return
        if val is None:
            show_error(update_val_input, "請輸入整數")
            return
        _current_op[0] = ("Update Key", "O(log n)", ft.Colors.INDIGO_600)
        backend.send_command(f"UPDATE {idx} {val}")
        update_idx_input.value = ""
        update_val_input.value = ""
        page.update()

    # DELETE
    def on_delete(e):
        clear_error(delete_idx_input)
        idx = parse_int(delete_idx_input.value)
        if idx is None:
            show_error(delete_idx_input, "請輸入整數 index")
            return
        _current_op[0] = ("Delete Node", "O(log n)", ft.Colors.DEEP_ORANGE_700)
        backend.send_command(f"DELETE {idx}")
        delete_idx_input.value = ""
        page.update()

    # SEARCH
    def on_search(e):
        clear_error(search_val_input)
        val = parse_int(search_val_input.value)
        if val is None:
            show_error(search_val_input, "請輸入整數")
            return
        _current_op[0] = ("Search", "O(n)", ft.Colors.PURPLE_700)
        backend.send_command(f"SEARCH {val}")
        search_val_input.value = ""
        page.update()

    # SORT
    def on_sort(e):
        _current_op[0] = ("Heap Sort", "O(n log n)", ft.Colors.ORANGE_800)
        backend.send_command("SORT")
        page.update()

    # INVERT（切換 Max / Min Heap）
    def on_invert(e):
        _current_op[0] = ("Invert Heap", "O(n)　※ bottom-up heapify", ft.Colors.GREEN_800)
        backend.send_command("INVERT")
        page.update()

    # CLEAR
    def on_clear(e):
        _current_op[0] = ("Clear", "O(1)", ft.Colors.GREY_700)
        backend.send_command("CLEAR")
        page.update()

    # STEP
    def on_step(e):
        backend.send_command("")
        page.update()

    # ── 綁定事件 ──────────────────────────────────────────────────
    init_btn.on_click    = on_init
    insert_btn.on_click  = on_insert
    remove_btn.on_click  = on_remove
    update_btn.on_click  = on_update
    delete_btn.on_click  = on_delete
    search_btn.on_click  = on_search
    sort_btn.on_click    = on_sort
    invert_btn.on_click  = on_invert
    clear_btn.on_click   = on_clear
    step_btn.on_click    = on_step

    # ── 版面配置 ──────────────────────────────────────────────────
    def group(label: str, color, *controls):
        """無外框的橫向群組，只用彩色小標籤區分"""
        return ft.Row(
            [ft.Text(label, size=10, weight="bold", color=color, no_wrap=True)]
            + list(controls),
            spacing=5,
            vertical_alignment=ft.CrossAxisAlignment.CENTER,
        )

    def divider():
        """群組之間的細分隔線"""
        return ft.Container(
            width=1, height=32,
            bgcolor=ft.Colors.GREY_300,
        )

    # 輸入框高度與寬度壓縮
    bulk_input.width        = 160
    update_idx_input.width  = 64
    update_val_input.width  = 72
    delete_idx_input.width  = 64
    search_val_input.width  = 72
    for inp in [bulk_input, update_idx_input, update_val_input,
                delete_idx_input, search_val_input]:
        inp.height          = 34
        inp.text_size       = 13
        inp.content_padding = ft.padding.symmetric(horizontal=8, vertical=0)

    control_panel = ft.Container(
        content=ft.Column([
            # 第一列：所有輸入型操作並排，用細線分隔
            ft.Row([
                group("INIT / INSERT", ft.Colors.BLUE_500,
                      bulk_input, init_btn, insert_btn),
                divider(),
                group("UPDATE", ft.Colors.INDIGO_500,
                      update_idx_input, update_val_input, update_btn),
                divider(),
                group("DELETE", ft.Colors.DEEP_ORANGE_500,
                      delete_idx_input, delete_btn),
                divider(),
                group("SEARCH", ft.Colors.PURPLE_500,
                      search_val_input, search_btn),
            ], spacing=10, vertical_alignment=ft.CrossAxisAlignment.CENTER),

            # 第二列：純按鈕操作
            ft.Row([
                remove_btn, sort_btn, invert_btn, clear_btn,
                ft.Container(width=12),
                step_wrapper,
            ], spacing=8, vertical_alignment=ft.CrossAxisAlignment.CENTER),
        ], spacing=8),
        padding=ft.padding.symmetric(horizontal=16, vertical=10),
        bgcolor=ft.Colors.GREY_50,
        border_radius=12,
        border=ft.border.all(1, ft.Colors.GREY_200),
    )

    page.add(
        # ── 標題列：中央標題 ＋ 右上角 Heap 類型徽章 ──
        ft.Container(height=12),
        ft.Stack([
            ft.Container(
                content=ft.Text("Heap Sort Visualizer", size=32,
                                weight=ft.FontWeight.BOLD),
                alignment=ft.alignment.center,
            ),
            ft.Container(
                content=heap_type_badge,
                alignment=ft.alignment.center_right,
                padding=ft.padding.only(right=24),
            ),
        ], height=50),

        # ── Status ──
        ft.Container(
            content=status_text,
            alignment=ft.alignment.center,
            padding=ft.padding.only(bottom=8),
        ),

        # ── 視覺化區域：Tree ＋ Array 並排，置中 ──
        ft.Row(
            [
                ft.Column([
                    ft.Text("Tree View", weight="bold", size=13),
                    tree_scroll,
                ], spacing=4, horizontal_alignment=ft.CrossAxisAlignment.CENTER),

                ft.Column([
                    ft.Text("Array View", weight="bold", size=13),
                    array_view,
                ], spacing=4, horizontal_alignment=ft.CrossAxisAlignment.CENTER),
            ],
            alignment=ft.MainAxisAlignment.CENTER,
            vertical_alignment=ft.CrossAxisAlignment.START,
            spacing=24,
        ),

        ft.Container(height=20),
        control_panel,
        ft.Container(height=12),
        complexity_view,
    )


ft.app(target=main)