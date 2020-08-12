/**
 * File:   automation_agent.c
 * Author: AWTK Develop Team
 * Brief:  automation_agent
 *
 * Copyright (c) 2020 - 2020  Guangzhou ZHIYUAN Electronics Co.,Ltd.
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
#include "base/window_manager.h"
#include "ui_loader/ui_serializer.h"

#define STR_X "x"
#define STR_Y "y"
#define STR_MS "ms"
#define STR_WIDTH "width"
#define STR_HEIGHT "height"
#define STR_TYPE "type"
#define STR_NAME "name"
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

static widget_t* automation_agent_find_element(const char* name) {
  widget_t* wm = window_manager();
  widget_t* win = window_manager_get_top_window(wm);

  if (tk_str_eq(win->name, name)) {
    return win;
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
  conf_doc_set_str(resp, "value.sessionId", "1234");
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
  conf_doc_t* resp = c->resp;
  widget_t* wm = window_manager();
  widget_t* element = window_manager_get_top_window(wm);
  return_value_if_fail(element != NULL, RET_NOT_FOUND);

  str_init(&str, 100000);
  widget_to_xml(element, &str);
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

#define STBI_WRITE_NO_STDIO
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
  t = widget_take_snapshot(element);
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

static ret_t automation_agent_on_back(http_connection_t* c) {
  conf_doc_t* resp = c->resp;

  window_manager_back(window_manager());
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

  if (tk_str_ieq(using, "id")) {
    conf_doc_set_int(resp, STR_STATUS, 0);
    conf_doc_set_str(resp, "value.ELEMENT", value);
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

static ret_t automation_agent_on_click_element(http_connection_t* c) {
  pointer_event_t evt;
  conf_doc_t* resp = c->resp;
  const char* id = object_get_prop_str(c->args, STR_ELEMENT_ID);
  widget_t* element = automation_agent_find_element(id);
  return_value_if_fail(element != NULL, RET_NOT_FOUND);

  widget_dispatch(element, pointer_event_init(&evt, EVT_CLICK, element, 0, 0));

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
  conf_doc_t* resp = c->resp;
  const char* id = object_get_prop_str(c->args, STR_ELEMENT_ID);
  widget_t* element = automation_agent_find_element(id);
  return_value_if_fail(element != NULL, RET_NOT_FOUND);

  str_init(&str, 0);
  conf_doc_set_int(resp, STR_STATUS, 0);
  str_from_wstr(&str, element->text.str);
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

static ret_t automation_agent_on_element_input(http_connection_t* c) {
  im_commit_event_t evt;
  conf_doc_t* resp = c->resp;
  const char* id = object_get_prop_str(c->args, STR_ELEMENT_ID);
  const char* value = conf_doc_get_str(c->req, "text", "");
  widget_t* element = automation_agent_find_element(id);
  return_value_if_fail(element != NULL, RET_NOT_FOUND);

  widget_dispatch(element, im_commit_event_init(&evt, value, FALSE));
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

    {HTTP_POST, "/wd/hub/session/:session/element", automation_agent_on_get_element},
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

ret_t automation_agent_start(httpd_t* httpd) {
  return_value_if_fail(httpd != NULL, RET_BAD_PARAMS);

  httpd_set_routes(httpd, s_automation_agent_routes, ARRAY_SIZE(s_automation_agent_routes));

  return httpd_start(httpd);
}
