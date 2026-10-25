// Screenshot of the <pre> element of an HTML page (terminal.py), for the
// figures of the manual.
//
// Usage: node screenshot.js PAGE.html OUT.png   (requires Playwright)
const { chromium } = require('playwright');
const path = require('path');

(async () => {
  const [page_path, out] = process.argv.slice(2);
  const browser = await chromium.launch();
  const page = await browser.newPage({ deviceScaleFactor: 2 });
  await page.goto('file://' + path.resolve(page_path));
  await (await page.$('pre')).screenshot({ path: out });
  await browser.close();
})();
