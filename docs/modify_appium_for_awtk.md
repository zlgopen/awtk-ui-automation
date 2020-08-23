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