from dataclasses import dataclass, field
from typing import List

@dataclass
class VisualState:
    heap: List[int] = field(default_factory=list)
    event: str = "IDLE"          # 預期值: "IDLE", "COMPARE", "SWAP", "INSERTED", "DONE"
    targets: List[int] = field(default_factory=list)
    is_idle: bool = True