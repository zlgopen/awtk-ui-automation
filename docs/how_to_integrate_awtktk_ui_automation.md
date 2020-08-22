# 集成 AWTK 自动测试引擎

## 1 添加依赖包

自动测试引擎依赖 awtk-restful-httpd 和 awtk-ui-automation，需要自行下载并编译。

### 1.1 下载并编译

* 获取 awtk-restful-httpd 并编译
```
git clone https://github.com/zlgopen/awtk-restful-httpd.git
cd awtk-restful-httpd; scons; cd -
```

* 获取 awtk-ui-automation 并编译
```
git clone https://github.com/zlgopen/awtk-ui-automation.git
cd awtk-ui-automation; scons
```
### 1.2 添加依赖

修改自己项目的 SConstruct 文件，增加依赖关系：

```py

DEPENDS_LIBS = [
  {
    "root" : '../awtk-restful-httpd',
    'shared_libs': ['httpd'],
    'static_libs': []
  },
  {
    "root" : '../awtk-ui-automation',
    'shared_libs': ['ui_automation'],
    'static_libs': []
  }
]

helper.set_deps(DEPENDS_LIBS).add_cpppath(APP_CPPPATH).call(DefaultEnvironment)
```

> 详情请参考 [awtk-mvvm-c-hello](https://github.com/zlgopen/awtk-mvvm-c-hello/blob/master/SConstruct)

## 2 调用

### 2.1 包含头文件

```c
#include "automation_agent/automation_agent.h"
```

### 2.2 调用启动函数

在 application_init 调用初始化函数：
```c
  socket_init();
  automation_agent_start(8000);
```

### 2.3 调用停止函数

在 application_exit 中调试停止函数：

```c
  automation_agent_stop();
  socket_deinit();
```  