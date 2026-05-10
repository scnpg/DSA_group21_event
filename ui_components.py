import flet as ft
import math

def get_status_text(state):
    if state.is_idle:
        return "Status: Ready (Idle)"
    
    if state.event == "INSERT":
        return f"Action: Inserting value into Heap..."
    elif state.event == "SWAP":
        return f"Action: Swapping indices {state.targets}"
    elif state.event == "COMPARE":
        return f"Action: Comparing indices {state.targets}"
    
    return f"Status: {state.event} in progress..."
def create_heap_view(state, mode="tree"):
    if mode == "array":
        return ft.Container(
            content=ft.Row(
                controls=[
                    ft.Container(
                        content=ft.Text(str(val), size=12, weight="bold"),
                        width=40, height=40,
                        bgcolor=ft.Colors.BLUE_50 if i not in state.targets else ft.Colors.AMBER_100,
                        border=ft.border.all(1, ft.Colors.BLUE_400),
                        alignment=ft.alignment.center
                    ) for i, val in enumerate(state.heap)
                ],
                wrap=True,
                spacing=5,
                run_spacing=5,
                width=450, # Setting width forces wrap after ~10 blocks (40px + spacing)
            ),
            padding=10,
            alignment=ft.alignment.top_center
        )

    else:
        # Tree View using absolute positioning
        if not state.heap:
            return ft.Text("No Data")
            
        nodes = []
        width = 500  # Total width of the drawing area
        
        for i, val in enumerate(state.heap):
            depth = int(math.log2(i + 1))
            pos_in_level = i - (2**depth - 1)
            total_nodes_in_level = 2**depth
            
            # Calculate spacing so nodes don't overlap
            x = (width / (total_nodes_in_level + 1)) * (pos_in_level + 1)
            y = depth * 70 + 20
            
            nodes.append(
                ft.Container(
                    content=ft.Text(str(val), color="white", weight="bold"),
                    bgcolor=ft.Colors.BLUE_700 if i not in state.targets else ft.Colors.ORANGE_800,
                    width=35, height=35,
                    shape=ft.BoxShape.CIRCLE,
                    alignment=ft.alignment.center,
                    left=x - 17.5,
                    top=y
                )
            )
        return ft.Stack(nodes, width=width, height=400)
