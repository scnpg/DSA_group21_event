import flet as ft
from state import VisualState


def create_heap_view(state: VisualState) -> ft.Row:
    """純 UI 函數：只負責根據 State 吐出 Flet 元件"""
    row = ft.Row(alignment=ft.MainAxisAlignment.CENTER, spacing=15)
    
    for i, val in enumerate(state.heap):
        is_target = i in state.targets
        
        # 根據事件決定節點顏色 (視覺化核心)
        bgcolor = ft.Colors.BLUE_GREY_100
        if is_target:
            if state.event == "COMPARE":
                bgcolor = ft.Colors.AMBER_400
            elif state.event == "SWAP":
                bgcolor = ft.Colors.RED_400
            elif state.event == "INSERTED":
                bgcolor = ft.Colors.GREEN_400
                
        node = ft.Container(
            content=ft.Text(str(val), size=24, weight=ft.FontWeight.BOLD, 
                            color=ft.Colors.WHITE if is_target else ft.Colors.BLACK87),
            width=70, height=70,
            alignment=ft.alignment.center,
            bgcolor=bgcolor,
            border_radius=35,
            animate=ft.Animation(400, "easeInOut"), # Flet 隱式動畫
        )
        row.controls.append(node)
        
    return row

def get_status_text(state: VisualState) -> str:
    """純邏輯函數：將後端 Event 轉成給人類看的文字"""
    translations = {
        "IDLE": "等待輸入指令...",
        "INSERTED": "將新元素放入陣列尾端",
        "COMPARE": "正在比較節點大小...",
        "SWAP": "發生交換！",
        "DONE": "操作完成，Heap 結構已更新。"
    }
    return translations.get(state.event, state.event)