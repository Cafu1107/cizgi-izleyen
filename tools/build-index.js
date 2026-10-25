// robot.html'den GitHub Pages için tam bir index.html üretir.
// Kullanım: node tools/build-index.js
// robot.html claude.ai sayfası için yazıldı (doctype/head/body yok); burada onları ekliyoruz.
const fs = require('fs');
const path = require('path');

const root = path.join(__dirname, '..');
const src = fs.readFileSync(path.join(root, 'robot.html'), 'utf8');
const cut = src.indexOf('<div class="page"');
if (cut < 0) throw new Error('robot.html içinde <div class="page"> bulunamadı');

// <title>, <meta>, <link> ve <style> head'e, sayfa içeriği ve scriptler body'ye
const head = src.slice(0, cut).replace(/^<meta charset="utf-8">\s*/, '');
const body = src.slice(cut);

const out = `<!doctype html>
<html lang="tr">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<meta name="description" content="Okul projesi çizgi izleyen robot: 3D model, pist simülasyonu, ölçüler, kablolama ve ayar menülü Arduino kodu.">
<style>
  :root { color-scheme: light; padding-top: env(safe-area-inset-top, 0px); padding-bottom: env(safe-area-inset-bottom, 0px); }
  body { margin: 0; font: 14px system-ui, sans-serif; }
  img { max-width: 100%; }
  [hidden] { display: none !important; }
</style>
${head.trim()}
</head>
<body>
${body.trim()}
</body>
</html>
`;
fs.writeFileSync(path.join(root, 'index.html'), out);
console.log('index.html yazıldı (' + out.length + ' bayt)');
