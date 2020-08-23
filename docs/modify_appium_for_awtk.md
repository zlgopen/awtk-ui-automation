# 让 appium 支持 awtk 的笔记

## 1. 实现 appium-awtk-driver

研究了几种驱动的实现之后，发现 awtk 最适合用 [appium-mac-driver](https://github.com/appium/appium-mac-driver) 类似机制。所以决定以 [appium-mac-driver](https://github.com/appium/appium-mac-driver) 为蓝本进行改造。主要改动如下：

* 名称修改。

```
mac => awtk
MAC => AWTK
a4m => a4a
```

* 应用程序的启动移动到客户端实现。

> 这个驱动就是一个 http 代理，把请求转发给实际的测试程序。

## 2. 修改 appium

下载 appium 稳定版本 [appium-1.18.1.zip](https://github.com/appium/appium/archive/v1.18.1.zip)

* 解压之后，进入 appium-1.18.1 目录：

```
unzip appium-1.18.1.zip
cd appium-1.18.1
```

* 安装 appium-awtk-driver

```
npm install --save git+https://github.com/zlgopen/appium-awtk-driver.git
```

* 修改 lib/appium.js，增加 awtk 支持。

修改很简单，把 MAC 对应的代码拷贝一份，然后改成 AWTK 即可。

* 编译

```
npm install
npm run build
```

## 3. 其它

当时有个问题折腾了很久：appium-desktop 的 inspector 无法显示源码，后来发现 inspector 取得 DOM tree 中第一个节点来显示。

后来只好用一个 view 把窗口包起来：

```c
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

  conf_doc_set_int(resp, STR_STATUS, 0);
  conf_doc_set_str(resp, STR_VALUE, str.str);
  str_reset(&str);

  return RET_OK;
}
```
