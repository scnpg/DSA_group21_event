from state import VisualState
from ui_components import get_status_text


def make_state(event="IDLE", is_idle=True, heap=None, targets=None):
    """ 方便每個測試裡造 state 用 """
    return VisualState(
        heap=heap or [],
        event=event,
        targets=targets or [],
        is_idle=is_idle,
    )


# 1. 正常 idle 要顯示 Ready
def test_idle_should_say_ready():
    s = make_state(event="IDLE", is_idle=True)
    assert "Ready" in get_status_text(s)


# 2. COMPARE 事件要把 targets 的 index 顯示出來
def test_compare_shows_target_indices():
    s = make_state(event="COMPARE", is_idle=False, targets=[0, 1])
    text = get_status_text(s)
    assert "比較" in text
    assert "0" in text and "1" in text


# 3. 亂打的 event 不該讓程式炸掉，應該掉到 fallback
def test_unknown_event_should_not_crash():
    s = make_state(event="WEIRD_EVENT_xyz", is_idle=False)
    text = get_status_text(s)
    assert "WEIRD_EVENT_xyz" in text


# 4. 對應 FE_QA #5：INSERTED 現在有專屬中文文案，不應掉到 fallback
def test_spec_INSERTED_has_dedicated_text():
    s = make_state(event="INSERTED", is_idle=False)
    text = get_status_text(s)
    assert "插入" in text      # 有專屬文案，不是 fallback "進行中"
    assert "in progress" not in text


# 5. VisualState 不傳參數時應該是 IDLE 狀態
def test_visual_state_defaults():
    s = VisualState()
    assert s.heap == []
    assert s.event == "IDLE"
    assert s.targets == []
    assert s.is_idle is True
