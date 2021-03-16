# API

## 0.比较

* 原型

```
should.become(value)
```

* 示例

```
driver.elementById('info').text().should.become("Stop");
```

## 1.选择控件

* 原型

```
elementById(id) => element
```

* 示例：

```
driver.elementById('info').text().should.become("Stop");
```

## 2.获取控件属性

* 原型

```
getAttribute(attr) => value
```

* 示例：

```
driver.elementById('title').getAttribute('x').should.become(10);
或
driver.getAttribute('title', 'x').should.become(10);
```

## 3.点击控件

* 原型

```
click()
```

* 示例：

```
driver.elementById('start').click()
```

> 一般修改控件的属性或界面时，可用代码放到按钮中，通过click来触发。

## 4.关闭当前窗口

* 原型

```
back()
```

* 示例：

```
driver.back().sleep(1000)
```

## 5.获取窗口标题

* 原型

```
title()
```

* 示例：

```
driver.title().should.become("Main Window");
或
driver.elementById('current_window').text().should.become("Main Window")
```

## 6.获取窗口大小

* 原型

```
getWindowSize() => size
```

* 示例：

```
driver.getWindowSize().should.become({
                  width: 320,
                  height: 480
            });
```

## 7.获取控件位置

* 原型

```
getLocation() => point
```

* 示例：

```
driver.getLocation('title').should.become({x:10, y:10});
```

## 8.获取控件大小

* 原型

```
getSize() => size
```

* 示例：

```
driver.getSize('title').should.become({width:300, height:30});
```

## 9.设置控件文本

* 原型

```
setText()
```

* 示例：

```
driver.elementById('info').setText("AWTK")
                  .elementById('info').text().should.become("AWTK");
```

## 10.获取控件文本

* 原型

```
text() => text
```

* 示例：

```
driver.elementById('info').setText("AWTK")
                  .elementById('info').text().should.become("AWTK");
```

