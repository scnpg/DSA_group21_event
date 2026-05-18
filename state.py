from dataclasses import dataclass, field
from typing import List, Dict

@dataclass
class VisualState:
    heap: List[int] = field(default_factory=list)
    event: str = "IDLE"
    targets: List[int] = field(default_factory=list)
    is_idle: bool = True
    sort_boundary: int = -1      # heap sort 用：index >= sort_boundary 表示已排序、固定。-1 表示沒有已排序區
    is_max_heap: bool = True
    stats: Dict = field(default_factory=lambda: {"cur_swap": 0, "cur_cmp": 0, "tot_swap": 0, "tot_cmp": 0})