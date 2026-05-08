import flet as ft
import flet.canvas as cv

import math

def main(page: ft.Page):
    depth = 3            # 樹的總層數 (1-based, 3 層代表 1+2+4 = 7 個節點)
    height = 50          # Y 軸：每一層往下增加的固定高度
    base_offset = 40     # X 軸：最底層相鄰節點的「水平偏移基準量」
    
    origin = (200, 200)  # Root 節點的起始座標

    def construct_complete_binary_tree(origin_pos, total_depth):
        nodes = []
        
        # 使用 Queue 來達成 BFS (層序遍歷)
        # 佇列內的資料結構: (陣列 Index, X 座標, Y 座標, 當前層數)
        queue = [(0, origin_pos[0], origin_pos[1], 1)]
        
        while queue:
            # 取出佇列最前方的節點
            i, x, y, curr_depth = queue.pop(0)
            
            # 依序加入 nodes 陣列，確保 index 完美對應 [Root, L, R, LL, LR, RL, RR]
            nodes.append((x, y))

            if curr_depth < total_depth:
                # 【關鍵】動態間距計算：利用 2 的次方讓偏移量逐層減半
                # 以 depth=3, base=40 為例：
                # 第一層 (curr=1) offset = 40 * 2^1 = 80
                # 第二層 (curr=2) offset = 40 * 2^0 = 40
                current_offset = base_offset * (2 ** (total_depth - curr_depth - 1))
                
                left_x = x - current_offset
                right_x = x + current_offset
                next_y = y + height
                
                # 將左子節點推入佇列 (左子節點的 Index 必定為 2i + 1)
                queue.append((2 * i + 1, left_x, next_y, curr_depth + 1))
                
                # 將右子節點推入佇列 (右子節點的 Index 必定為 2i + 2)
                queue.append((2 * i + 2, right_x, next_y, curr_depth + 1))
                
        return nodes

    # 取得依照 Level-order 排序的座標陣列
    nodes = construct_complete_binary_tree(origin, depth)

    cp = cv.Canvas(
        [
            cv.Circle(
                x=node[0],      # 圓心 X 座標
                y=node[1],      # 圓心 Y 座標
                radius=20,  # 半徑
                paint=ft.Paint(
                    color=ft.Colors.BLUE, 
                    style=ft.PaintingStyle.FILL
                ),
            )
            for node in nodes
        ]
        + 
        [
            cv.Line(
                x1=nodes[(i - 1) // 2][0], y1=nodes[(i - 1) // 2][1],  # 父節點的座標
                x2=node[0], y2=node[1],  # 當前節點的座標
                paint=ft.Paint(
                    color=ft.Colors.BLACK,
                    style=ft.PaintingStyle.STROKE,
                    stroke_width=2
                )
            )
            for i, node in enumerate(nodes[1:], start=1)
        ],
        width=1000,
        height=1000,
    )

    page.add(cp)

    # txt_input = ft.TextField(label="請輸入內容")
    
    # page.add(txt_input)

    # def update_canvas(e):
    #     cp.shapes[0].radius = int(txt_input.value)  # 更新圓的半徑
    #     for i in range(1, int(txt_input.value)):
    #         cp.shapes.append(
    #             cv.Circle(
    #                 x=100,      # 圓心 X 座標
    #                 y=100,      # 圓心 Y 座標
    #                 radius=i*10,  # 半徑
    #                 paint=ft.Paint(
    #                     color=ft.Colors.BLUE.with_opacity(1 - i*0.1, color=ft.Colors.BLUE), 
    #                     style=ft.PaintingStyle.FILL
    #                 ),
    #             )
    #         )
    #     cp.update()

    # btn = ft.ElevatedButton(
    #     text="按我",
    #     on_click=update_canvas
    # )

    # page.add(btn)


ft.app(target=main)
