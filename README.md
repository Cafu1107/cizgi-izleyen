# Çizgi İzleyen Robot

Okul projesi: kitteki parçalarla (Arduino UNO, L298N, 4'lü kızılötesi sensör, iki katlı pleksi şasi) siyah çizgiyi izleyen robot.

Robotumuz **Versiyon 2** yerleşiminde (tekerlekler arkada, sensörler ön kenarda); koddaki ayarlar buna göre: `TABAN_HIZ 130`, `KP 30`.

## Dosyalar

| Dosya | Ne işe yarar |
|---|---|
| [`cizgi_izleyen/cizgi_izleyen.ino`](cizgi_izleyen/cizgi_izleyen.ino) | Arduino kodu. Arduino IDE ile açıp karta yükle. |
| [`robot.html`](robot.html) | 3D model, pist simülasyonu, ölçüler, kablolama şeması ve ayar menülü kod üretici (claude.ai sayfasının kaynağı). |
| [`v2.html`](v2.html) | Versiyon 2: yarışma afişindeki yerleşim, farklar ve iki robotun yan yana simülasyonu. |
| [`index.html`](index.html) | GitHub Pages sürümü. `robot.html` değişince `node tools/build-index.js` ile yeniden üret. |

Sayfa: https://cafu1107.github.io/cizgi-izleyen/
claude.ai kopyası: https://claude.ai/artifact/7p15a4Bj83Es5iEGig92bk (sadece paylaşılan kişiler açabilir).

## Kodun modları

Kodun başındaki `MOD` ayarını değiştirip tekrar yükle:

- `MOD = 1` sensör testi: Seri Monitör 9600'de sensörleri gösterir, trimpot ayarı için.
- `MOD = 2` motor testi: tekerlekler havadayken motorları sırayla döndürür.
- `MOD = 0` yarış: robot çizgiyi izler.

## Bağlantılar

| Nereden | Nereye |
|---|---|
| L298N ENA / ENB | D5 / D6 (ENA ve ENB jumper'larını çıkar) |
| L298N IN1 / IN2 / IN3 / IN4 | D7 / D8 / D9 / D10 |
| L298N 12V / GND | Arduino VIN / GND |
| Sensör OUT1…OUT4 (soldan sağa) | A0…A3 |
| Sensör VCC / GND | 5V / GND |
| Pil jakı | Arduino güç jakı |

Henüz gerçek robotta denenmedi.
