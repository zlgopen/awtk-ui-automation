#include "awtk.h"
#include "automation_agent/automation_agent.h"

ret_t window_new_open(void);

static ret_t on_open_new(void* ctx, event_t* e) {
  window_new_open();
  return RET_OK;
}

static ret_t on_close(void* ctx, event_t* e) {
  tk_quit();

  return RET_OK;
}

static ret_t on_start(void* ctx, event_t* e) {
  widget_t* win = WIDGET(ctx);

  widget_set_text_utf8(widget_lookup(win, "info", TRUE), "Start");
  return RET_OK;
}

static ret_t on_stop(void* ctx, event_t* e) {
  widget_t* win = WIDGET(ctx);

  widget_set_text_utf8(widget_lookup(win, "info", TRUE), "Stop");
  return RET_OK;
}

/**
 * 初始化
 */
static ret_t window_main_create(void) {
  widget_t* win = window_open("main");
  widget_child_on(win, "new", EVT_CLICK, on_open_new, win);
  widget_child_on(win, "start", EVT_CLICK, on_start, win);
  widget_child_on(win, "stop", EVT_CLICK, on_stop, win);
  widget_child_on(win, "close", EVT_CLICK, on_close, win);

  return RET_OK;
}


static httpd_t* s_httpd = NULL;
ret_t application_init(void) {
  s_httpd = httpd_create(8000, 1);

  socket_init();
  automation_agent_start(s_httpd);
  window_main_create();

  return RET_OK;
}

/**
 * 退出
 */
ret_t application_exit(void) {
  log_debug("application_exit\n");
  httpd_destroy(s_httpd);
  socket_deinit();
  return RET_OK;
}
