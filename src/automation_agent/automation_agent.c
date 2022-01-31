/**
 * File:   automation_agent.c
 * Author: AWTK Develop Team
 * Brief:  automation_agent
 *
 * Copyright (c) 2020 - 2022  Guangzhou ZHIYUAN Electronics Co.,Ltd.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * License file for more details.
 *
 */

/**
 * History:
 * ================================================================
 * 2020-08-02 Li XianJing <xianjimli@hotmail.com> created
 *
 */

#include "base64.h"
#include "tkc/utils.h"
#include "base/input_method.h"
#include "httpd/http_route.h"
#include "httpd/http_parser.h"
#include "automation_agent.h"
#include "awtk_global.h"
#include "base/window.h"
#include "widgets/view.h"
#include "base/window_manager.h"
#include "ui_loader/ui_serializer.h"

#define STR_X "x"
#define STR_Y "y"
#define STR_MS "ms"
#define STR_ID "id"
#define STR_TAP "tap"
#define STR_PRESS "press"
#define STR_WAIT "wait"
#define STR_MOVE_TO "moveTo"
#define STR_RELEASE "release"
#define STR_TYPE "type"
#define STR_NAME "name"
#define STR_WIDTH "width"
#define STR_HEIGHT "height"
#define STR_USING "using"
#define STR_HANDLE "handle"
#define STR_STATUS "status"
#define STR_VALUE "value"
#define STR_WINDOW "window"
#define STR_CURRENT "current"
#define STR_PROPERTY "property"
#define STR_ATTRIBUTE "attribute"
#define STR_ELEMENT_ID "element"
#define STR_SESSION_ID "session"
#define STR_ACCESS_ID "accessibility id"

#define STR_WM "wm"
#define STR_CURRENT_WINDOW "current_window"

static widget_t* automation_agent_find_element(const char* name) {
  widget_t* wm = window_manager();
  const char* widget_name = strchr(name, '.');
  widget_t* win = window_manager_get_top_window(wm);

  if (tk_str_eq(STR_WM, name)) {
    return wm;
  }

  if (tk_str_eq(win->name, name) || tk_str_eq(STR_CURRENT_WINDOW, name)) {
    return win;
  }

  if (widget_name != NULL) {
    str_t win_name;
    str_init(&win_name, 0);
    str_set_with_len(&win_name, name, widget_name - name);

    if (win_name.size > 0) {
      win = widget_child(wm, win_name.str);
    }
    str_reset(&win_name);
    name = widget_name + 1;
  }

  return widget_lookup(win, name, TRUE);
}

static widget_t* automation_agent_find_window(const char* name) {
  widget_t* win = NULL;
  widget_t* wm = window_manager();
  if (tk_str_eq(name, STR_CURRENT)) {
    win = window_manager_get_top_window(wm);
  } else {
    win = widget_lookup(wm, name, FALSE);
  }

  return win;
}

static ret_t automation_agent_on_status(http_connection_t* c) {
  conf_doc_t* resp = c->resp;
  conf_doc_set_int(resp, STR_STATUS, 0);

  return RET_OK;
}

static ret_t automation_agent_on_new_session(http_connection_t* c) {
  conf_doc_t* resp = c->resp;

  conf_doc_set_int(resp, STR_STATUS, 0);
  conf_doc_set_str(resp, "value.sessionId", "8888");
  conf_doc_set_str(resp, "value.capabilities.platformName", "awtk");

  return RET_OK;
}

static ret_t automation_agent_on_set_url(http_connection_t* c) {
  conf_doc_t* resp = c->resp;

  conf_doc_set_int(resp, STR_STATUS, 0);

  return RET_OK;
}

static ret_t automation_agent_on_get_source(http_connection_t* c) {
  str_t str;
  widget_t* view = NULL;
  conf_doc_t* resp = c->resp;
  widget_t* wm = window_manager();
  widget_t* element = window_manager_get_top_window(wm);
  return_value_if_fail(element != NULL, RET_NOT_FOUND);

  str_init(&str, 100000);
  view = view_create(NULL, 0, 0, wm->w, wm->h);
  view->children = darray_create(1, NULL, NULL);
  /*appium get the first child to show*/
  darray_push(view->children, element);
  widget_to_xml(view, &str);
  view->children->size = 0;
  widget_destroy(view);
  log_debug("source:%s\n", str.str);
  conf_doc_set_int(resp, STR_STATUS, 0);
  conf_doc_set_str(resp, STR_VALUE, str.str);
  str_reset(&str);

  return RET_OK;
}

static ret_t automation_agent_on_get_contexts(http_connection_t* c) {
  conf_doc_t* resp = c->resp;

  conf_doc_set_int(resp, STR_STATUS, 0);
  conf_doc_set_str(resp, "value.[0]", "NATIVE_APP");

  return RET_OK;
}

static ret_t automation_agent_on_set_context(http_connection_t* c) {
  conf_doc_t* resp = c->resp;

  conf_doc_set_int(resp, STR_STATUS, 0);

  return RET_OK;
}

static ret_t automation_agent_on_get_context(http_connection_t* c) {
  conf_doc_t* resp = c->resp;

  conf_doc_set_int(resp, STR_STATUS, 0);
  conf_doc_set_str(resp, "value", "NATIVE_APP");

  return RET_OK;
}

#define STB_IMAGE_STATIC 1
#define STBI_WRITE_NO_STDIO 1
#define STB_IMAGE_WRITE_STATIC 1
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_FREE TKMEM_FREE
#define STBI_MALLOC TKMEM_ALLOC
#define STBI_REALLOC(p, s) TKMEM_REALLOC(p, s)

#include "stb/stb_image_write.h"

static ret_t automation_agent_on_get_screenshot(http_connection_t* c) {
  int32_t len = 0;
  bitmap_t* t = NULL;
  char* result = NULL;
  uint8_t* tdata = NULL;
  unsigned char* png_data = NULL;
  conf_doc_t* resp = c->resp;
  widget_t* wm = window_manager();
  widget_t* element = window_manager_get_top_window(wm);

  return_value_if_fail(element != NULL, RET_NOT_FOUND);
  t = widget_take_snapshot(wm);
  return_value_if_fail(t != NULL, RET_BAD_PARAMS);

  tdata = bitmap_lock_buffer_for_write(t);
  png_data = stbi_write_png_to_mem(tdata, t->w * 4, t->w, t->h, 4, &len);
  bitmap_unlock_buffer(t);
  bitmap_destroy(t);
  return_value_if_fail(png_data != NULL, RET_BAD_PARAMS);

  conf_doc_set_int(resp, STR_STATUS, 0);
  result = TKMEM_ALLOC(BASE64_ENCODE_OUT_SIZE(len));
  base64_encode(png_data, len, result);
  conf_doc_set_str(resp, STR_VALUE, result);
  TKMEM_FREE(result);
  STBIW_FREE(png_data);

  return RET_OK;
}

static ret_t automation_agent_on_get_session(http_connection_t* c) {
  conf_doc_t* resp = c->resp;

  conf_doc_set_int(resp, STR_STATUS, 0);
  conf_doc_set_str(resp, "value.platform", "awtk");
  conf_doc_set_str(resp, "value.version", "1.0");
  conf_doc_set_str(resp, "value.browserName", "chrome");
  conf_doc_set_bool(resp, "value.javascriptEnabled", FALSE);
  conf_doc_set_bool(resp, "value.takesScreenshot", TRUE);

  return RET_OK;
}

static ret_t automation_agent_on_remove_session(http_connection_t* c) {
  conf_doc_t* resp = c->resp;

  conf_doc_set_int(resp, STR_STATUS, 0);
  tk_quit();

  return RET_OK;
}

static ret_t automation_agent_on_get_timeouts(http_connection_t* c) {
  conf_doc_t* resp = c->resp;

  conf_doc_set_int(resp, STR_STATUS, 0);
  conf_doc_set_int(resp, "value.script", 30000);
  conf_doc_set_int(resp, "value.pageLoad", 30000);
  conf_doc_set_int(resp, "value.implicit", 30000);

  return RET_OK;
}

static ret_t automation_agent_on_set_timeouts(http_connection_t* c) {
  conf_doc_t* req = c->req;
  conf_doc_t* resp = c->resp;
  int ms = conf_doc_get_int(req, STR_MS, 0);
  const char* type = conf_doc_get_str(req, STR_TYPE, NULL);

  /*TODO*/
  log_debug("set_timeout: type=%s ms=%d\n", type, ms);
  conf_doc_set_int(resp, STR_STATUS, 0);

  return RET_OK;
}

static ret_t idle_back(const idle_info_t* info) {
  window_manager_back(window_manager());
  return RET_REMOVE;
}

static ret_t automation_agent_on_back(http_connection_t* c) {
  conf_doc_t* resp = c->resp;

  idle_add(idle_back, NULL);
  conf_doc_set_int(resp, STR_STATUS, 0);

  return RET_OK;
}

static ret_t automation_agent_on_get_title(http_connection_t* c) {
  conf_doc_t* resp = c->resp;
  widget_t* wm = window_manager();
  widget_t* element = window_manager_get_top_window(wm);
  return_value_if_fail(element != NULL, RET_NOT_FOUND);
  conf_doc_set_int(resp, STR_STATUS, 0);

  if (element->text.size > 0) {
    str_t str;
    str_init(&str, 0);
    str_from_wstr(&str, element->text.str);
    conf_doc_set_str(resp, STR_VALUE, str.str);
    str_reset(&str);
  } else {
    conf_doc_set_str(resp, STR_VALUE, element->name);
  }

  return RET_OK;
}

static ret_t automation_agent_on_get_window_name(http_connection_t* c) {
  conf_doc_t* resp = c->resp;
  widget_t* wm = window_manager();
  widget_t* element = window_manager_get_top_window(wm);
  return_value_if_fail(element != NULL, RET_NOT_FOUND);

  conf_doc_set_int(resp, STR_STATUS, 0);
  conf_doc_set_str(resp, STR_VALUE, element->name);

  return RET_OK;
}

static ret_t automation_agent_on_get_windows_name(http_connection_t* c) {
  uint32_t i = 0;
  char path[32];
  conf_doc_t* resp = c->resp;
  widget_t* wm = window_manager();
  uint32_t nr = widget_count_children(wm);

  conf_doc_set_int(resp, STR_STATUS, 0);
  for (i = 0; i < nr; i++) {
    widget_t* iter = widget_get_child(wm, i);
    tk_snprintf(path, sizeof(path), "value.[%u]", i);
    conf_doc_set_str(resp, path, iter->name);
  }

  return RET_OK;
}

static ret_t automation_agent_on_get_window_rect(http_connection_t* c) {
  conf_doc_t* resp = c->resp;
  const char* name = object_get_prop_str(c->args, STR_WINDOW);
  widget_t* element = automation_agent_find_window(name);
  return_value_if_fail(element != NULL, RET_NOT_FOUND);

  conf_doc_set_int(resp, STR_STATUS, 0);
  conf_doc_set_int(resp, "value.x", element->x);
  conf_doc_set_int(resp, "value.y", element->y);
  conf_doc_set_int(resp, "value.width", element->w);
  conf_doc_set_int(resp, "value.height", element->h);

  return RET_OK;
}

static ret_t automation_agent_on_get_window_position(http_connection_t* c) {
  conf_doc_t* resp = c->resp;
  const char* name = object_get_prop_str(c->args, STR_WINDOW);
  widget_t* element = automation_agent_find_window(name);
  return_value_if_fail(element != NULL, RET_NOT_FOUND);

  conf_doc_set_int(resp, STR_STATUS, 0);
  conf_doc_set_int(resp, "value.x", element->x);
  conf_doc_set_int(resp, "value.y", element->y);

  return RET_OK;
}

static ret_t automation_agent_on_get_window_size(http_connection_t* c) {
  conf_doc_t* resp = c->resp;
  const char* name = object_get_prop_str(c->args, STR_WINDOW);
  widget_t* element = automation_agent_find_window(name);
  return_value_if_fail(element != NULL, RET_NOT_FOUND);

  conf_doc_set_int(resp, STR_STATUS, 0);
  conf_doc_set_int(resp, "value.width", element->w);
  conf_doc_set_int(resp, "value.height", element->h);

  return RET_OK;
}

static ret_t automation_agent_on_set_window_position(http_connection_t* c) {
  conf_doc_t* resp = c->resp;
  int x = conf_doc_get_int(c->req, STR_X, 0);
  int y = conf_doc_get_int(c->req, STR_Y, 0);
  const char* name = object_get_prop_str(c->args, STR_WINDOW);
  widget_t* element = automation_agent_find_window(name);
  return_value_if_fail(element != NULL, RET_NOT_FOUND);

  widget_move(element, x, y);
  conf_doc_set_int(resp, STR_STATUS, 0);

  return RET_OK;
}

static ret_t automation_agent_on_set_window_size(http_connection_t* c) {
  conf_doc_t* resp = c->resp;
  int w = conf_doc_get_int(c->req, STR_WIDTH, 0);
  int h = conf_doc_get_int(c->req, STR_HEIGHT, 0);
  const char* name = object_get_prop_str(c->args, STR_WINDOW);
  widget_t* element = automation_agent_find_window(name);
  return_value_if_fail(element != NULL, RET_NOT_FOUND);

  widget_resize(element, w, h);
  conf_doc_set_int(resp, STR_STATUS, 0);

  return RET_OK;
}

static ret_t automation_agent_on_maximize_window(http_connection_t* c) {
  conf_doc_t* resp = c->resp;
  const char* name = object_get_prop_str(c->args, STR_WINDOW);
  widget_t* element = automation_agent_find_window(name);
  return_value_if_fail(element != NULL, RET_NOT_FOUND);

  window_set_fullscreen(element, TRUE);
  conf_doc_set_int(resp, STR_STATUS, 0);

  return RET_OK;
}

static ret_t automation_agent_on_not_impl(http_connection_t* c) {
  conf_doc_t* resp = c->resp;
  conf_doc_set_int(resp, STR_STATUS, 0);
  log_debug("not impl:%s\n", c->url);

  return RET_OK;
}

static ret_t automation_agent_on_switch_to_window(http_connection_t* c) {
  conf_doc_t* req = c->req;
  conf_doc_t* resp = c->resp;
  widget_t* wm = window_manager();
  const char* name = conf_doc_get_str(req, STR_NAME, NULL);
  widget_t* element = widget_child(wm, name);
  return_value_if_fail(element != NULL, RET_NOT_FOUND);

  widget_restack(element, widget_count_children(element->parent));
  conf_doc_set_int(resp, STR_STATUS, 0);

  return RET_OK;
}

static ret_t automation_agent_on_close_window(http_connection_t* c) {
  conf_doc_t* resp = c->resp;
  widget_t* wm = window_manager();
  widget_t* element = window_manager_get_top_window(wm);
  return_value_if_fail(element != NULL, RET_NOT_FOUND);

  window_close(element);
  conf_doc_set_int(resp, STR_STATUS, 0);

  return RET_OK;
}

static ret_t automation_agent_on_get_element(http_connection_t* c) {
  conf_doc_t* req = c->req;
  conf_doc_t* resp = c->resp;
  const char* using = conf_doc_get_str(req, STR_USING, NULL);
  const char* value = conf_doc_get_str(req, STR_VALUE, NULL);
  return_value_if_fail(using != NULL && value != NULL, RET_BAD_PARAMS);

  if (tk_str_ieq(using, STR_ID) || tk_str_ieq(using, STR_ACCESS_ID)) {
    conf_doc_set_int(resp, STR_STATUS, 0);
    conf_doc_set_str(resp, "value.ELEMENT", value);
  }

  return RET_OK;
}

static ret_t automation_agent_on_get_elements(http_connection_t* c) {
  conf_doc_t* req = c->req;
  conf_doc_t* resp = c->resp;
  const char* using = conf_doc_get_str(req, STR_USING, NULL);
  const char* value = conf_doc_get_str(req, STR_VALUE, NULL);
  return_value_if_fail(using != NULL && value != NULL, RET_BAD_PARAMS);

  if (tk_str_ieq(using, STR_ID) || tk_str_ieq(using, STR_ACCESS_ID)) {
    conf_doc_set_int(resp, STR_STATUS, 0);
    conf_doc_set_str(resp, "value.[0].ELEMENT", value);
  }

  return RET_OK;
}

static ret_t automation_agent_on_get_focus_element(http_connection_t* c) {
  conf_doc_t* resp = c->resp;
  widget_t* wm = window_manager();
  widget_t* element = window_manager_get_top_window(wm);
  return_value_if_fail(element != NULL, RET_BAD_PARAMS);

  while (element->target != NULL) {
    element = element->target;
  }

  conf_doc_set_int(resp, STR_STATUS, 0);
  conf_doc_set_str(resp, "value.ELEMENT", element->name);

  return RET_OK;
}

static ret_t idle_click(const idle_info_t* info) {
  pointer_event_t evt;
  widget_t* widget = WIDGET(info->ctx);

  widget_set_focused(widget, TRUE);
  widget_dispatch(widget, pointer_event_init(&evt, EVT_CLICK, widget, 0, 0));

  return RET_REMOVE;
}

static ret_t automation_agent_on_click_element(http_connection_t* c) {
  conf_doc_t* resp = c->resp;
  const char* id = object_get_prop_str(c->args, STR_ELEMENT_ID);
  widget_t* element = automation_agent_find_element(id);
  return_value_if_fail(element != NULL, RET_NOT_FOUND);
  idle_add(idle_click, element);
  conf_doc_set_int(resp, STR_STATUS, 0);

  return RET_OK;
}

static ret_t automation_agent_on_touch_perform(http_connection_t* c) {
  int i = 0;
  int x = 0;
  int y = 0;
  char path[256];
  pointer_event_t evt;
  const char* action = NULL;
  conf_doc_t* resp = c->resp;
  widget_t* wm = window_manager();
  int nr = conf_doc_get_int(c->req, "actions.#size", 0);
  return_value_if_fail(wm != NULL, RET_NOT_FOUND);

  memset(path, 0x00, sizeof(path));
  for (i = 0; i < nr; i++) {
    tk_snprintf(path, sizeof(path), "actions.[%d].action", i);
    action = conf_doc_get_str(c->req, path, NULL);

    if (tk_str_eq(action, STR_TAP)) {
      tk_snprintf(path, sizeof(path), "actions.[%d].options.x", i);
      x = conf_doc_get_int(c->req, path, 0);
      tk_snprintf(path, sizeof(path), "actions.[%d].options.y", i);
      y = conf_doc_get_int(c->req, path, 0);
      window_manager_dispatch_input_event(wm, pointer_event_init(&evt, EVT_POINTER_DOWN, wm, x, y));
      window_manager_dispatch_input_event(wm, pointer_event_init(&evt, EVT_POINTER_UP, wm, x, y));
      log_debug("tap: %d %d\n", x, y);
    } else if (tk_str_eq(action, STR_PRESS)) {
      tk_snprintf(path, sizeof(path), "actions.[%d].options.x", i);
      x = conf_doc_get_int(c->req, path, 0);
      tk_snprintf(path, sizeof(path), "actions.[%d].options.y", i);
      y = conf_doc_get_int(c->req, path, 0);
      window_manager_dispatch_input_event(wm, pointer_event_init(&evt, EVT_POINTER_DOWN, wm, x, y));
    } else if (tk_str_eq(action, STR_MOVE_TO)) {
      tk_snprintf(path, sizeof(path), "actions.[%d].options.x", i);
      x = conf_doc_get_int(c->req, path, 0);
      tk_snprintf(path, sizeof(path), "actions.[%d].options.y", i);
      y = conf_doc_get_int(c->req, path, 0);

      pointer_event_init(&evt, EVT_POINTER_MOVE, wm, x, y);
      evt.pressed = TRUE;
      window_manager_dispatch_input_event(wm, (event_t*)(&evt));
    } else if (tk_str_eq(action, STR_RELEASE)) {
      x = window_manager_get_pointer_x(wm);
      y = window_manager_get_pointer_y(wm);
      window_manager_dispatch_input_event(wm, pointer_event_init(&evt, EVT_POINTER_UP, wm, x, y));
    } else {
      log_debug("not suported:%s\n", action);
    }
  }

  conf_doc_set_int(resp, STR_STATUS, 0);

  return RET_OK;
}

static ret_t automation_agent_on_clear_element(http_connection_t* c) {
  conf_doc_t* resp = c->resp;
  const char* id = object_get_prop_str(c->args, STR_ELEMENT_ID);
  widget_t* element = automation_agent_find_element(id);
  return_value_if_fail(element != NULL, RET_NOT_FOUND);

  widget_set_text_utf8(element, "");
  conf_doc_set_int(resp, STR_STATUS, 0);

  return RET_OK;
}

static ret_t automation_agent_on_get_element_text(http_connection_t* c) {
  str_t str;
  value_t v;
  conf_doc_t* resp = c->resp;
  const char* id = object_get_prop_str(c->args, STR_ELEMENT_ID);
  widget_t* element = automation_agent_find_element(id);
  return_value_if_fail(element != NULL, RET_NOT_FOUND);

  str_init(&str, 0);
  if(widget_get_prop(element, WIDGET_PROP_TEXT, &v) == RET_OK) {
    str_from_value(&str, &v);
  }
  conf_doc_set_int(resp, STR_STATUS, 0);
  conf_doc_set_str(resp, STR_VALUE, str.str);
  str_reset(&str);

  return RET_OK;
}

static ret_t automation_agent_on_get_element_name(http_connection_t* c) {
  conf_doc_t* resp = c->resp;
  const char* id = object_get_prop_str(c->args, STR_ELEMENT_ID);
  widget_t* element = automation_agent_find_element(id);
  return_value_if_fail(element != NULL, RET_NOT_FOUND);

  conf_doc_set_int(resp, STR_STATUS, 0);
  conf_doc_set_str(resp, STR_VALUE, element->name);

  return RET_OK;
}

static ret_t automation_agent_on_get_element_enable(http_connection_t* c) {
  conf_doc_t* resp = c->resp;
  const char* id = object_get_prop_str(c->args, STR_ELEMENT_ID);
  widget_t* element = automation_agent_find_element(id);
  return_value_if_fail(element != NULL, RET_NOT_FOUND);

  conf_doc_set_int(resp, STR_STATUS, 0);
  conf_doc_set_bool(resp, STR_VALUE, element->enable);

  return RET_OK;
}

static ret_t automation_agent_on_get_element_selected(http_connection_t* c) {
  conf_doc_t* resp = c->resp;
  const char* id = object_get_prop_str(c->args, STR_ELEMENT_ID);
  widget_t* element = automation_agent_find_element(id);
  return_value_if_fail(element != NULL, RET_NOT_FOUND);

  conf_doc_set_int(resp, STR_STATUS, 0);
  conf_doc_set_bool(resp, STR_VALUE, widget_get_value(element) == TRUE);

  return RET_OK;
}

static ret_t automation_agent_on_get_element_location(http_connection_t* c) {
  conf_doc_t* resp = c->resp;
  const char* id = object_get_prop_str(c->args, STR_ELEMENT_ID);
  widget_t* element = automation_agent_find_element(id);
  return_value_if_fail(element != NULL, RET_NOT_FOUND);

  conf_doc_set_int(resp, STR_STATUS, 0);
  conf_doc_set_int(resp, "value.x", element->x);
  conf_doc_set_int(resp, "value.y", element->y);

  return RET_OK;
}

static ret_t automation_agent_on_get_element_location_in_view(http_connection_t* c) {
  point_t p = {0, 0};
  conf_doc_t* resp = c->resp;
  const char* id = object_get_prop_str(c->args, STR_ELEMENT_ID);
  widget_t* element = automation_agent_find_element(id);
  return_value_if_fail(element != NULL, RET_NOT_FOUND);

  widget_to_screen(element, &p);
  conf_doc_set_int(resp, STR_STATUS, 0);
  conf_doc_set_int(resp, "value.x", p.x);
  conf_doc_set_int(resp, "value.y", p.y);

  return RET_OK;
}

static ret_t automation_agent_on_get_element_size(http_connection_t* c) {
  conf_doc_t* resp = c->resp;
  const char* id = object_get_prop_str(c->args, STR_ELEMENT_ID);
  widget_t* element = automation_agent_find_element(id);
  return_value_if_fail(element != NULL, RET_NOT_FOUND);

  conf_doc_set_int(resp, STR_STATUS, 0);
  conf_doc_set_int(resp, "value.width", element->w);
  conf_doc_set_int(resp, "value.height", element->h);

  return RET_OK;
}

static ret_t automation_agent_on_get_element_prop(http_connection_t* c) {
  value_t v;
  conf_doc_t* resp = c->resp;
  const char* id = object_get_prop_str(c->args, STR_ELEMENT_ID);
  const char* prop = object_get_prop_str(c->args, STR_PROPERTY);
  widget_t* element = automation_agent_find_element(id);
  return_value_if_fail(element != NULL, RET_NOT_FOUND);
  return_value_if_fail(widget_get_prop(element, prop, &v) == RET_OK, RET_NOT_FOUND);

  conf_doc_set_int(resp, STR_STATUS, 0);
  conf_doc_set(resp, STR_VALUE, &v);

  return RET_OK;
}

static ret_t automation_agent_on_set_element_value(http_connection_t* c) {
  conf_doc_t* resp = c->resp;
  const char* id = object_get_prop_str(c->args, STR_ELEMENT_ID);
  const char* value = conf_doc_get_str(c->req, "value.[0]", "");
  widget_t* element = automation_agent_find_element(id);
  return_value_if_fail(element != NULL, RET_NOT_FOUND);

  widget_set_prop_str(element, WIDGET_PROP_VALUE, value);
  conf_doc_set_int(resp, STR_STATUS, 0);

  return RET_OK;
}

static ret_t automation_agent_on_set_element_prop(http_connection_t* c) {
  conf_doc_t* resp = c->resp;
  const char* id = object_get_prop_str(c->args, STR_ELEMENT_ID);
  const char* prop = object_get_prop_str(c->args, STR_PROPERTY);
  const char* value = conf_doc_get_str(c->req, "value", "");
  widget_t* element = automation_agent_find_element(id);
  return_value_if_fail(element != NULL, RET_NOT_FOUND);

  widget_set_prop_str(element, prop, value);
  conf_doc_set_int(resp, STR_STATUS, 0);

  return RET_OK;
}

/*https://github.com/admc/wd/blob/master/lib/special-keys.js*/
static int32_t map_key(wchar_t c) {
  /*TODO*/
  switch (c) {
    case 0xE003: {
      return TK_KEY_BACKSPACE;
    }
    case 0xE004: {
      return TK_KEY_TAB;
    }
    case 0xE006: {
      return TK_KEY_RETURN;
    }
    case 0xE012: {
      return TK_KEY_LEFT;
    }
    case 0xE013: {
      return TK_KEY_UP;
    }
    case 0xE014: {
      return TK_KEY_RIGHT;
    }
    case 0xE015: {
      return TK_KEY_DOWN;
    }
    case 0xE010: {
      return TK_KEY_END;
    }
    case 0xE011: {
      return TK_KEY_HOME;
    }
    case 0xE00D: {
      return TK_KEY_SPACE;
    }
    case 0xE00E: {
      return TK_KEY_PAGEUP;
    }
    case 0xE00F: {
      return TK_KEY_PAGEDOWN;
    }
    default: {
      if (c >= 0xE031 && c <= 0xE03C) {
        return TK_KEY_F1 + (c - 0xE031);
      } else if (c >= 0xE000 && c < 0xE0ff) {
        log_debug("not supported key:%x\n", c);
        return TK_KEY_SPACE;
      } else {
        return -1;
      }
    }
  }
}

static ret_t idle_send_key(const idle_info_t* info) {
  key_event_t evt;
  int32_t key = tk_pointer_to_int(info->ctx);
  widget_t* widget = WIDGET(info->extra_ctx);

  if (widget == NULL) {
    widget = window_manager();
    window_manager_dispatch_input_event(widget, key_event_init(&evt, EVT_KEY_DOWN, widget, key));
    window_manager_dispatch_input_event(widget, key_event_init(&evt, EVT_KEY_UP, widget, key));
  } else {
    widget_on_keydown(widget, (key_event_t*)key_event_init(&evt, EVT_KEY_DOWN, widget, key));
    widget_on_keyup(widget, (key_event_t*)key_event_init(&evt, EVT_KEY_UP, widget, key));
  }

  return RET_REMOVE;
}

#define KEY_CMD_OR_CTRL "cmd_or_ctrl+"
#define KEY_SHIFT "shift+"
#define SHORTCUT_COPY "<cmd_or_ctrl+c>"
#define SHORTCUT_CUT "<cmd_or_ctrl+x>"
#define SHORTCUT_PASTE "<cmd_or_ctrl+v>"
#define SHORTCUT_UNDO "<cmd_or_ctrl+z>"
#define SHORTCUT_REDO "<cmd_or_ctrl+y>"
#define SHORTCUT_SELECT_ALL "<cmd_or_ctrl+a>"
#define SHORTCUT_SELECT_LEFT "<shift+left>"
#define SHORTCUT_SELECT_RIGHT "<shift+right>"
#define SHORTCUT_SELECT_DOWN "<shift+down>"
#define SHORTCUT_SELECT_UP "<shift+up>"

static ret_t widget_send_special_key(widget_t* widget, const char* value) {
  key_event_t evt;
  int32_t key = 0;
  bool_t shift = FALSE;
  bool_t cmd = FALSE;
  bool_t ctrl = FALSE;
  return_value_if_fail(value != NULL, RET_BAD_PARAMS);

  if (strstr(value, KEY_CMD_OR_CTRL) != NULL) {
#ifdef MACOS
    cmd = TRUE;
#else
    ctrl = TRUE;
#endif /*MACOS*/
  }
  if (strstr(value, KEY_SHIFT) != NULL) {
    shift = TRUE;
  }

  if (strstr(value, "+c") != NULL) {
    key = TK_KEY_c;
  } else if (strstr(value, "+x") != NULL) {
    key = TK_KEY_x;
  } else if (strstr(value, "+v") != NULL) {
    key = TK_KEY_v;
  } else if (strstr(value, "+z") != NULL) {
    key = TK_KEY_z;
  } else if (strstr(value, "+y") != NULL) {
    key = TK_KEY_y;
  } else if (strstr(value, "+a") != NULL) {
    key = TK_KEY_a;
  } else if (strstr(value, "+left") != NULL) {
    key = TK_KEY_LEFT;
  } else if (strstr(value, "+right") != NULL) {
    key = TK_KEY_RIGHT;
  } else if (strstr(value, "+up") != NULL) {
    key = TK_KEY_UP;
  } else if (strstr(value, "+down") != NULL) {
    key = TK_KEY_DOWN;
  } else {
    return RET_CONTINUE;
  }

  key_event_init(&evt, EVT_KEY_DOWN, widget, key);
  evt.shift = shift;
  evt.cmd = cmd;
  evt.ctrl = ctrl;
  widget_on_keydown(widget, &evt);

  key_event_init(&evt, EVT_KEY_UP, widget, key);
  evt.shift = shift;
  evt.cmd = cmd;
  evt.ctrl = ctrl;
  widget_on_keydown(widget, &evt);

  return RET_OK;
}

static ret_t automation_agent_on_element_input(http_connection_t* c) {
  conf_doc_t* resp = c->resp;
  const char* id = object_get_prop_str(c->args, STR_ELEMENT_ID);
  const char* value = conf_doc_get_str(c->req, "text", "");
  widget_t* element = automation_agent_find_element(id);
  return_value_if_fail(element != NULL, RET_NOT_FOUND);

  if (widget_send_special_key(element, value) == RET_OK) {
    log_debug("special key:%s\n", value);
  } else if (value != NULL) {
    char tstr[32];
    uint32_t i = 0;
    wstr_t str;
    wstr_init(&str, 0);
    wstr_set_utf8(&str, value);

    memset(tstr, 0x00, sizeof(tstr));
    for (i = 0; i < str.size; i++) {
      wchar_t c = str.str[i];
      int32_t key = map_key(c);
      if (key < 0) {
        im_commit_event_t evt;
        tk_utf8_from_utf16_ex(&c, 1, tstr, sizeof(tstr));
        if (tk_str_eq(id, STR_WM)) {
          input_method_commit_text(input_method(), tstr);
        } else {
          widget_dispatch(element, im_commit_event_init(&evt, tstr, FALSE));
        }
        log_debug("commit text: %s\n", tstr);
      } else if (key > 0) {
        if (tk_str_eq(id, STR_WM)) {
          idle_add(idle_send_key, tk_pointer_from_int(key));
        } else {
          uint32_t id = idle_add(idle_send_key, tk_pointer_from_int(key));
          idle_info_t* info = (idle_info_t*)idle_find(id);
          assert(info != NULL);
          info->extra_ctx = element;
        }
        log_debug("send key event: %d\n", key);
      }
    }
    wstr_reset(&str);
  }

  conf_doc_set_int(resp, STR_STATUS, 0);

  return RET_OK;
}

/*https://www.w3.org/TR/webdriver*/
static const http_route_entry_t s_automation_agent_routes[] = {
    {HTTP_POST, "/wd/hub/session", automation_agent_on_new_session},
    {HTTP_DELETE, "/wd/hub/session/:session", automation_agent_on_remove_session},
    {HTTP_GET, "/wd/hub/session/:session", automation_agent_on_get_session},
    {HTTP_POST, "/wd/hub/session/:session/url", automation_agent_on_set_url},
    {HTTP_GET, "/wd/hub/status", automation_agent_on_status},
    {HTTP_GET, "/wd/hub/session/:session/source", automation_agent_on_get_source},
    {HTTP_GET, "/wd/hub/session/:session/contexts", automation_agent_on_get_contexts},
    {HTTP_GET, "/wd/hub/session/:session/context", automation_agent_on_get_context},
    {HTTP_POST, "/wd/hub/session/:session/context", automation_agent_on_set_context},
    {HTTP_GET, "/wd/hub/session/:session/screenshot", automation_agent_on_get_screenshot},
    {HTTP_GET, "/wd/hub/session/:session/timeouts", automation_agent_on_get_timeouts},
    {HTTP_POST, "/wd/hub/session/:session/timeouts", automation_agent_on_set_timeouts},
    {HTTP_POST, "/wd/hub/session/:session/back", automation_agent_on_back},
    {HTTP_GET, "/wd/hub/session/:session/title", automation_agent_on_get_title},

    {HTTP_GET, "/wd/hub/session/:session/window", automation_agent_on_get_window_name},
    {HTTP_GET, "/wd/hub/session/:session/window_handle", automation_agent_on_get_window_name},
    {HTTP_DELETE, "/wd/hub/session/:session/window", automation_agent_on_close_window},
    {HTTP_POST, "/wd/hub/session/:session/window", automation_agent_on_switch_to_window},
    {HTTP_GET, "/wd/hub/session/:session/window_handles", automation_agent_on_get_windows_name},
    {HTTP_GET, "/wd/hub/session/:session/window/:window/rect", automation_agent_on_get_window_rect},
    {HTTP_GET, "/wd/hub/session/:session/window/:window/position",
     automation_agent_on_get_window_position},
    {HTTP_GET, "/wd/hub/session/:session/window/:window/size", automation_agent_on_get_window_size},

    {HTTP_POST, "/wd/hub/session/:session/window/:window/position",
     automation_agent_on_set_window_position},
    {HTTP_POST, "/wd/hub/session/:session/window/:window/size",
     automation_agent_on_set_window_size},
    {HTTP_POST, "/wd/hub/session/:session/window/:window/maximize",
     automation_agent_on_maximize_window},

    {HTTP_POST, "/wd/hub/session/:session/window/new", automation_agent_on_not_impl},
    {HTTP_POST, "/wd/hub/session/:session/window/frame", automation_agent_on_not_impl},
    {HTTP_POST, "/wd/hub/session/:session/window/frame/parent", automation_agent_on_not_impl},
    {HTTP_POST, "/wd/hub/session/:session/touch/perform", automation_agent_on_touch_perform},

    {HTTP_POST, "/wd/hub/session/:session/element", automation_agent_on_get_element},
    {HTTP_POST, "/wd/hub/session/:session/elements", automation_agent_on_get_elements},
    {HTTP_GET, "/wd/hub/session/:session/element/active", automation_agent_on_get_focus_element},
    {HTTP_POST, "/wd/hub/session/:session/element/active", automation_agent_on_get_focus_element},

    {HTTP_GET, "/wd/hub/session/:session/element/:element/text",
     automation_agent_on_get_element_text},
    {HTTP_GET, "/wd/hub/session/:session/element/:element/name",
     automation_agent_on_get_element_name},
    {HTTP_GET, "/wd/hub/session/:session/element/:element/enabled",
     automation_agent_on_get_element_enable},

    {HTTP_GET, "/wd/hub/session/:session/element/:element/selected",
     automation_agent_on_get_element_selected},

    {HTTP_POST, "/wd/hub/session/:session/element/:element/value",
     automation_agent_on_element_input},
    {HTTP_POST, "/wd/hub/session/:session/element/:element/click",
     automation_agent_on_click_element},
    {HTTP_POST, "/wd/hub/session/:session/element/:element/clear",
     automation_agent_on_clear_element},
    {HTTP_GET, "/wd/hub/session/:session/element/:element/location",
     automation_agent_on_get_element_location},
    {HTTP_GET, "/wd/hub/session/:session/element/:element/location_in_view",
     automation_agent_on_get_element_location_in_view},
    {HTTP_GET, "/wd/hub/session/:session/element/:element/size",
     automation_agent_on_get_element_size},

    {HTTP_GET, "/wd/hub/session/:session/element/:element/attribute/:property",
     automation_agent_on_get_element_prop},
    {HTTP_GET, "/wd/hub/session/:session/element/:element/property/:property",
     automation_agent_on_get_element_prop},
    {HTTP_POST, "/wd/hub/session/:session/element/:element/property/:property",
     automation_agent_on_set_element_prop},
    {HTTP_POST, "/wd/hub/session/:session/appium/element/:element/replace_value",
     automation_agent_on_set_element_value},
};

static httpd_t* s_httpd;
ret_t automation_agent_start(int port) {
  httpd_t* httpd = httpd_create(8000, 1);
  return_value_if_fail(httpd != NULL, RET_BAD_PARAMS);

  httpd_set_routes(httpd, s_automation_agent_routes, ARRAY_SIZE(s_automation_agent_routes));

  s_httpd = httpd;
  return httpd_start(httpd);
}

ret_t automation_agent_stop(void) {
  return_value_if_fail(s_httpd != NULL, RET_BAD_PARAMS);

  httpd_destroy(s_httpd);
  s_httpd = NULL;

  return RET_OK;
}
