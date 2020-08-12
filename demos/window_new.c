#include "awtk.h"
#include "automation_agent/automation_agent.h"

static ret_t on_close(void* ctx, event_t* e) {
  widget_close_window(WIDGET(e->target));
  return RET_OK;
}

ret_t window_new_open(void) {
  widget_t* win = window_open("new");
  widget_child_on(win, "close", EVT_CLICK, on_close, win);

  return RET_OK;
}


