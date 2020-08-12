"use strict";

require("./helpers/setup");

let wd = require("wd"),
      _ = require('underscore'),
      Q = require('q'),
      serverConfigs = require('./helpers/appium-servers');
let startApp = require("./helpers/start-app").startApp;

const appName = '../bin/demo'
describe("awtk simple", function () {
      let driver;
      let allPassed = true;
      this.timeout(300000);

      before(async function () {
            let serverConfig = serverConfigs.local;
            driver = wd.promiseChainRemote(serverConfig);
            require("./helpers/logging").configure(driver);

            await startApp(appName);

            let desired = _.clone(require("./helpers/caps").awtk);
            return driver.init(desired);
      });

      after(function () {
            return driver
                  .quit()
      });

      afterEach(function () {
            allPassed = allPassed && this.currentTest.state === 'passed';
      });
      
      it("text", function () {
            return driver.elementById('info')
                  .text().should.become("Hello");
      });

      it("get x", function () {
            return driver.getAttribute('title', 'x').should.become(10);
      });
      it("get w", function () {
            return driver.getAttribute('title', 'w').should.become(300);
      });
      it("get visible", function () {
            return driver.getAttribute('title', 'visible').should.become(true);
      });

      it("get location", function () {
            return driver.getLocation('title').should.become({x:10, y:10});
      });
      it("get location in view", function () {
            return driver.getLocationInView('title').should.become({x:10, y:10});
      });
      it("get size", function () {
            return driver.getSize('title').should.become({width:300, height:30});
      });

      it("set text", function () {
            return driver.elementById('info')
                  .setText("AWTK").sleep(1000)
                  .elementById('info')
                  .text().should.become("AWTK");
      });
      it("clear", function () {
            return driver.clear('edit').elementById("edit").text().should.become("");
      });
      it("input", function () {
            return driver.type('edit', ['ab','cd']).elementById("edit").text().should.become("abcd");
      });
      it("enable", function () {
            return driver.enabled('remember').should.become(true);
      });
      it("name", function () {
            return driver.getTagName('remember').should.become("remember");
      });
      it("selected", function () {
            return driver.isSelected('remember').should.become(true);
      });
      it("active", function () {
            return driver.active().text().should.become("Main Window");
      });

      it("maximize", function () {
            return driver.maximize("main").getWindowSize().should.become({
                  width: 320,
                  height: 480
            });
      });

      it("set window size", function () {
            return driver.setWindowSize(320, 480).getWindowSize().should.become({
                  width: 320,
                  height: 480
            });
      });

      it("set window position", function () {
            return driver.setWindowPosition(0, 0).getWindowPosition().should.become({
                  x: 0,
                  y: 0
            });
      });


      it("window position", function () {
            return driver.getWindowPosition().should.become({
                  x: 0,
                  y: 0
            });
      });


      it("window size", function () {
            return driver.getWindowSize().should.become({
                  width: 320,
                  height: 480
            });
      });

      it("timeouts", function () {
            return driver.setCommandTimeout(3000)
      });

      it("title", function () {
            return driver.title().should.become("Main Window");
      });

      it("window name", function () {
            return driver.windowHandle().should.become("main");
      });

      it("windows name", function () {
            return driver.windowHandles().should.become(["main"]);
      });

      it("window", function () {
            return driver.window("main").windowHandle().should.become("main");
      });

      it("click start", function () {
            return driver.elementById('start')
                  .click().sleep(1000)
                  .elementById('info')
                  .text().should.become("Start");
      });
      it("click stop", function () {
            return driver.elementById('stop')
                  .click().sleep(1000)
                  .elementById('info')
                  .text().should.become("Stop");
      });
      it("click new", function () {
            return driver.elementById('new')
                  .click().sleep(1000)
                  .elementById('close').click().sleep(1000)
      });
      it("click new back", function () {
            return driver.elementById('new')
                  .click().sleep(1000)
                  .back()
      });
});
