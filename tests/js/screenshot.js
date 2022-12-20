// Copyright (C) 2022 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions
// and limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

const puppeteer = require('puppeteer-core');

const args = process.argv.slice(2).reduce((acc, arg) => {
    let [k, v = true] = arg.split('=');
    acc[k] = v;
    return acc;
}, {})

if (args['uri'] === undefined) {
    console.log('error: no server specified');
    console.log('usage: node screenshot.js uri=<ip> [ref=<ref.png>|json=<sequence.json>] [psnr=true|false] ');
    process.exit(1);
}

const uri = args['uri'];
const ref = (args['ref'] === undefined)? undefined: args['ref'];
const json = (args['json'] === undefined)? undefined: args['json']
const calc_psnr = (args['psnr'] === undefined)? true: args['psnr'];

if (args['ref'] != undefined && args['json'] != undefined) {
    console.log('error: can not use ref and json at the same time');
    process.exit(1);
}

const psnr_threshold = 25;

function timeout(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
};

async function check_psnr(_shot, _ref) {
  try {
    var fs = require("fs");
    var PngCompare = require('png-quality');

    const psnr = await PngCompare.psnr(_shot, _ref);
    console.log('psnr: ' + psnr);
    if (psnr < psnr_threshold) {
      console.log('failed: psnr too low (< ' + psnr_threshold + ')');
      return false;
    }
  } catch(err) {
    console.log('failed: ' + err.message);
    return false;
  }
  return true;
}

(async() => {
  try {
    const browser = await puppeteer.launch({ executablePath: '/usr/bin/google-chrome',
      ignoreDefaultArgs: true, args: [
      '--user-data-dir=/home/user',
      '--disable-gpu',
      '--enable-logging',
      '--v=1',
      '--allow-file-access-from-files',
      '--no-proxy-server',
      '--no-sandbox',
      '--use-fake-ui-for-media-stream',
      '--disable-user-media-security',
      '--no-default-browser-check',
      '--no-first-run',
      '--disable-default-apps',
      '--disable-popup-blocking',
      '--use-fake-device-for-media-stream',
      '--headless',
      '--autoplay-policy=no-user-gesture-required',
      '--unsafely-treat-insecure-origin-as-secure="' + uri + '"',
      '--remote-debugging-port=9222']});
    const page = await browser.newPage();
    await page.setViewport({width: 1920, height: 1080});
    await page.goto(uri + '/?sId=0');
    var res = true;
    if (json != undefined) {
      const fj = require(json);
      const steps = fj['steps'];

      console.log('json execution mode...');
      for (const s in steps) {
        const step=steps[s];
        if ('timeout' in step) {
          console.log('timeout: ' + step['timeout']);
          await timeout(step['timeout']);
        } else if ('ref' in step) {
          const _shot='/tmp/screenshot.' + step['ref'] + '.png';
          console.log('screenshot: ' + _shot);
          console.log('  ref: ' + step['ref']);
          console.log('  calc_psnr: ' + calc_psnr);;
          await page.screenshot({path: _shot});
          if (calc_psnr == true) {
            const r = await check_psnr(_shot, step['ref']);
            if (r == false) {
              res = false;
            }
          }
        } else if ('mouse.click' in step) {
          console.log('mouse.click: ' + step['mouse.click']);
          await page.mouse.click(step['mouse.click'][0], step['mouse.click'][1]);
        } else {
          console.log('warning: ignored unknown step at ' + s);
        }
      }
    } else {
      console.log('secreenshot mode...');
      console.log('timeout: 10000');
      const _shot='/tmp/screenshot.png';
      await timeout(10000);
      console.log('screenshot: ' + _shot);
      console.log('  ref: ' + ref);
      console.log('  calc_psnr: ' + calc_psnr);
      await page.screenshot({path: _shot});
      if (calc_psnr == true) {
        if (check_psnr(_shot, ref) == false) {
          res=false;
        }
      }
    }
    browser.close();

    if (res == false) {
      process.exit(1);
    }
  } catch(err) {
    console.log('failed: ' + err.message);
    process.exit(1);
  }
})();
