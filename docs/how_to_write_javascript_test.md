# 如何用 javascript 编写测试脚本

在理论上 Appium 支持使用任何编程语言编写测试，而且提供了常见编程语言客户端库。

不过我个人觉得 javascript(nodejs) 是最好的选择，这里介绍一下使用 javascript 编写 AWTK 自动测试程序的过程和方法：

## 1 创建测试项目

### 1.1 在当前项目中创建 uitests 目录，然后进入 uitests 目录：

```
mkdir uitests
cd uitests
```

### 1.2 新建 package.json 文件，可根据以下内容进行修改：

```json
{
  "name": "uitests",
  "version": "1.0.0",
  "description": "awtk ui tests",
  "main": "index.js",
  "scripts": {
    "test": "mocha index.js"
  },
  "devDependencies": {
    "chai": "^4.2.0",
    "chai-as-promised": "^7.1.1",
    "colors": "^1.4.0",
    "mocha": "^8.1.1",
    "q": "^1.5.1",
    "underscore": "^1.10.2",
    "wd": "^1.12.1"
  },
  "dependencies": {
    "awtk-appium-js-helpers": "git+https://github.com/zlgopen/awtk-appium-js-helpers.git"
  },
  "keywords": [
    "awtk",
    "appium"
  ],
  "author": "xianjimli@hotmail.com",
  "license": "LGPL-2.0"
}
```

### 1.3 创建 index.js

index.js 以下面的内容为模板：

```js
"use strict";

let Q = require('q');
let wd = require("wd");
let _ = require('underscore');
require("awtk-appium-js-helpers/setup.js");
let serverConfigs = require('awtk-appium-js-helpers/appium-servers');
let startApp = require("awtk-appium-js-helpers/start-app").startApp;

const appName = '../bin/demo'

describe("awtk", function () {
      let driver;
      let allPassed = true;
      this.timeout(300000);

      before(async function () {
            let serverConfig = serverConfigs.local;
            driver = wd.promiseChainRemote(serverConfig);
            require("awtk-appium-js-helpers/logging").configure(driver);

            await startApp(appName);

            let desired = _.clone(require("awtk-appium-js-helpers/caps").awtk);
            return driver.init(desired);
      });

      after(function () {
            return driver.quit()
      });

      afterEach(function () {
            allPassed = allPassed && this.currentTest.state === 'passed';
      });

      it("demo", function () {
      });        
});      
```

> appName 需要根据实际情况进行调整

```
const appName = '../bin/demo'
```

### 1.4 安装依赖包

```
npm install
```

### 1.5 运行测试

```
npm run test
```

## 2 编写测试程序

### 2.1 基本用法

这里使用了流行 js 测试框架 [mochajs](https://mochajs.org/)，最好看看相关文档，其实很简单，几分钟时间就熟悉了。

每个 it 是一个测试用例，把测试代码放在其中：

```
      it("demo", function () {
      });        
```      

如：

```js
      it("set text", function () {
            return driver.elementById('info').setText("AWTK")
            .elementById('info').text().should.become("AWTK");
      })
```

driver 对象是测试程序客户端的对象，通过它调用测试函数。

* 示例：获取控件文本

```js
      it("text", function () {
            return driver.elementById('info').text().should.become("Hello");
      });
```

* 示例：获取控件属性

```js
      it("get x", function () {
            return driver.getAttribute('title', 'x').should.become(10);
      });
```

* 示例：修改控件的文本

```js
      it("set text", function () {
            return driver.elementById('info').setText("AWTK")
                  .elementById('info').text().should.become("AWTK");
      });
```

* 示例：修改控件的文本

```js
      it("set text", function () {
            return driver.elementById('info').setText("AWTK")
                  .elementById('info').text().should.become("AWTK");
      });
```

* 示例：点击按钮

```js
      it("click start", function () {
            return driver.elementById('start').click().sleep(1000)
                  .elementById('info').text().should.become("Start");
      });
```

* 示例：关闭窗口

```js
      it("click new back", function () {
            return driver.elementById('new').click().sleep(1000).back().sleep(1000)
      });;
```

> 更多示例请参考：uitests/index.js

### 2.2 注意事项

* 需要操作的控件都提供窗口内唯一的名称。不要使用 XPath 选择元素，XPath 写死了文档结构，不便于维护，所以 AWTK 测试引擎只提供了按名称选择控件。

* 为方便阅读代码，对每一个控件的测试，放到一行内，以 elementById 打头：

```js
    return driver.elementById('info').setText("AWTK")
                 .elementById('info').text().should.become("AWTK");
```      

* 打开/关闭窗口后，调用sleep(1000)，确保窗口动画完成。

```js
      it("click new back", function () {
            return driver.elementById('new').click().sleep(1000).back().sleep(1000)
      });
```      