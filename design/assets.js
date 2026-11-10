const { chromium } = require('playwright');
const S = process.argv[2], OUT = process.argv[3];
(async () => {
  const b = await chromium.launch();
  for (const [name, file] of [["phosphor", "phosphor.html"], ["matrix", "matrix.html"], ["kawaii", "kawaii.html"]]) {
    // 1. lit background only (no controls)
    let p = await b.newPage({ viewport: { width: 1000, height: 600 }, deviceScaleFactor: 2 });
    await p.goto(`file://${S}/${file}`); await p.waitForTimeout(500);
    await p.addStyleTag({ content: ".over{display:none!important}" });
    await p.waitForTimeout(200);
    await p.screenshot({ path: `${OUT}/bg_${name}.png` });
    await p.close();
    // 2. knob bodies (no pointer / label / readout), transparent
    for (const size of [46, 56, 76]) {
      p = await b.newPage({ viewport: { width: 400, height: 400 }, deviceScaleFactor: 4 });
      await p.goto(`file://${S}/${file}`); await p.waitForTimeout(400);
      const w = await p.evaluate((size) => {
        document.body.style.background = "transparent";
        const ui = document.getElementById("ui"); ui.style.background = "none";
        const host = document.getElementById("knobs"); host.innerHTML = "";
        for (const c of Array.from(ui.children)) if (!(c.tagName === "svg" && c.getAttribute("width") === "0")) c.style.display = "none";
        ui.appendChild(host); host.style.display = "block"; host.style.position = "absolute"; host.style.left = "0"; host.style.top = "0";
        const w = size + 46;
        knob(w / 2, w / 2, size, 0, "x", "x");
        const svg = host.querySelector("svg");
        for (let i = 0; i < 4; i++) svg.removeChild(svg.lastChild);
        return w;
      }, size);
      await p.screenshot({ path: `${OUT}/knob_${name}_${size}.png`, clip: { x: 0, y: 0, width: w, height: w }, omitBackground: true });
      await p.close();
    }
  }
  await b.close();
})();
