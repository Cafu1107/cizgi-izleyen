/*
  ============================================================
   ÇİZGİ İZLEYEN ROBOT
  ============================================================
   Kart    : Arduino UNO R3 (CH340)
   Sürücü  : L298N çift motor sürücü
   Sensör  : 4'lü kızılötesi çizgi sensörü (dijital çıkışlı)
   Motor   : 2 x 6V 250 RPM redüktörlü motor
   Pil     : 6 x AA (barrel jak Arduino'nun jakına takılır)

   KABLOLAMA
   ---------
   L298N ENA  -> D5   (sol motor hızı, PWM)  * ENA jumper'ını çıkar!
   L298N IN1  -> D7
   L298N IN2  -> D8
   L298N IN3  -> D9
   L298N IN4  -> D10
   L298N ENB  -> D6   (sağ motor hızı, PWM)  * ENB jumper'ını çıkar!
   L298N OUT1/OUT2 -> SOL motor
   L298N OUT3/OUT4 -> SAĞ motor
   L298N 12V  -> Arduino VIN
   L298N GND  -> Arduino GND   (ortak toprak, şart!)
   L298N 5V   -> BOŞ BIRAK (5V jumper'ı takılı kalsın)

   Sensör kartı OUT1 (en sol) -> A0
   Sensör kartı OUT2          -> A1
   Sensör kartı OUT3          -> A2
   Sensör kartı OUT4 (en sağ) -> A3
   Sensör kartı VCC -> 5V,  GND -> GND

   (İsteğe bağlı) Başlat butonu: D2 ile GND arasına

   "Sol" ve "sağ": robotun arkasından, gideceği yöne bakarken.

   MODLAR (aşağıdaki MOD ayarını değiştir, tekrar yükle)
   0 = YARIŞ        : robot çizgiyi izler
   1 = SENSÖR TESTİ : Seri Monitör (9600) sensörleri yazar, motorlar dönmez
   2 = MOTOR TESTİ  : tekerlekleri havaya kaldır, motorları sırayla dener
                      (pil takılı olmalı, USB motorları döndüremez)
  ============================================================
*/

// ===================== AYARLAR =====================

const byte MOD = 0;  // 0 = yarış, 1 = sensör testi, 2 = motor testi

// Sensör siyah çizginin üstündeyken pin ne okuyor?
// Çoğu modülde siyah = HIGH. Sensör testinde ters görürsen LOW yap.
const bool CIZGI_SEVIYESI = HIGH;

// Hızlar (0-255 arası PWM değeri)
// Versiyon 2 (afişteki robot) için. Versiyon 1: TABAN_HIZ 140, KP 45.
const int TABAN_HIZ   = 130;  // çizgi tam ortadayken iki motorun hızı
const int MAKS_HIZ    = 255;  // hiçbir motor bundan hızlı dönmez
const int ARAMA_HIZI  = 150;  // çizgi kaybolunca yerinde dönme hızı
const float KP        = 30;   // dönüş sertliği: hata başına hız farkı
const int VIRAJ_FRENI = 10;   // çizgi ortadan kaydıkça taban hızdan düşülen

// Bir motor ters dönüyorsa true yap (ya da o motorun iki kablosunu değiştir)
const bool SOL_TERS = false;
const bool SAG_TERS = false;

// Süreler (milisaniye)
const unsigned long BASLAMA_BEKLEME = 3000;  // güç verince bu kadar bekler
const unsigned long TUM_SIYAH_DUR   = 400;   // 4 sensör bu kadar süre siyah görürse durur
const unsigned long KAYIP_DUR       = 3000;  // çizgi bu kadar süre bulunamazsa durur

const bool BUTON_KULLAN = false;  // true: D2'deki butona basınca başlar

// ===================== PİNLER =====================

const byte ENA = 5;
const byte IN1 = 7;
const byte IN2 = 8;
const byte IN3 = 9;
const byte IN4 = 10;
const byte ENB = 6;
const byte BUTON_PIN = 2;

// Soldan sağa. Robot çizgiden kaçıyorsa sıra ters: {A3, A2, A1, A0} yap.
const byte SENSOR_PIN[4] = {A0, A1, A2, A3};

// Her sensörün konumu: en sol -3, en sağ +3.
// Çizgiyi gören sensörlerin ortalaması = çizginin robota göre yeri (hata).
const int AGIRLIK[4] = {-3, -1, 1, 3};

// ===================== DEĞİŞKENLER =====================

float sonHata = 0;                  // çizgi en son nerede görüldü
unsigned long sonGorme = 0;         // çizgi en son ne zaman görüldü
bool tumSiyah = false;              // 4 sensör de siyah mı görüyor
unsigned long tumSiyahBaslangic = 0;

// ===================== KURULUM =====================

void setup() {
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BUTON_PIN, INPUT_PULLUP);
  for (byte i = 0; i < 4; i++) {
    pinMode(SENSOR_PIN[i], INPUT);
  }
  motorSur(0, 0);

  Serial.begin(9600);
  if (MOD == 1) {
    Serial.println(F("SENSOR TESTI  (X = cizgi goruyor, . = zemin)"));
    return;
  }
  if (MOD == 2) {
    Serial.println(F("MOTOR TESTI  - tekerlekleri havaya kaldir!"));
    delay(2000);
    return;
  }

  Serial.println(F("YARIS MODU"));
  baslamayiBekle();
  sonGorme = millis();
}

void loop() {
  if (MOD == 1) {
    sensorTesti();
  } else if (MOD == 2) {
    motorTesti();
  } else {
    cizgiIzle();
  }
}

// ===================== ÇİZGİ İZLEME =====================

void cizgiIzle() {
  bool s[4];
  byte adet = sensorleriOku(s);
  digitalWrite(LED_BUILTIN, adet > 0);  // kart LED'i: çizgi görülüyor mu

  // 1) Hiçbir sensör çizgiyi görmüyor: çizgi kaçtı.
  //    Çizgiyi en son hangi tarafta gördüysek o tarafa yerinde dön.
  if (adet == 0) {
    if (millis() - sonGorme > KAYIP_DUR) {
      motorSur(0, 0);  // çok uzun süre bulamadı, güvenlik için dur
    } else if (sonHata < 0) {
      motorSur(-ARAMA_HIZI, ARAMA_HIZI);  // sola dön
    } else if (sonHata > 0) {
      motorSur(ARAMA_HIZI, -ARAMA_HIZI);  // sağa dön
    } else {
      motorSur(TABAN_HIZ, TABAN_HIZ);     // ortadaydı: kesik çizgi, düz devam
    }
    return;
  }
  sonGorme = millis();

  // 2) Dört sensör de siyah: kavşak, bitiş bandı ya da robot havaya kalktı.
  //    Kısa sürerse kavşaktır, düz geç. Uzun sürerse dur.
  if (adet == 4) {
    if (!tumSiyah) {
      tumSiyah = true;
      tumSiyahBaslangic = millis();
    }
    if (millis() - tumSiyahBaslangic > TUM_SIYAH_DUR) {
      motorSur(0, 0);
    } else {
      motorSur(TABAN_HIZ, TABAN_HIZ);
    }
    return;
  }
  tumSiyah = false;

  // 3) Normal durum: çizginin yerini hesapla.
  //    hata < 0: çizgi solda, hata > 0: çizgi sağda, 0: tam ortada.
  int toplam = 0;
  for (byte i = 0; i < 4; i++) {
    if (s[i]) toplam += AGIRLIK[i];
  }
  float hata = (float)toplam / adet;
  sonHata = hata;

  // Çizgi soldaysa sol motor yavaşlar, sağ motor hızlanır (robot sola döner).
  // Çizgi ortadan uzaklaştıkça iki motor da biraz yavaşlar (virajda fren).
  int taban = TABAN_HIZ - VIRAJ_FRENI * fabs(hata);
  int duzeltme = KP * hata;
  motorSur(taban + duzeltme, taban - duzeltme);
}

// Sensörleri okur, s[] dizisine yazar (true = çizgi var).
// Kaç sensörün çizgi gördüğünü döndürür.
byte sensorleriOku(bool s[4]) {
  byte adet = 0;
  for (byte i = 0; i < 4; i++) {
    s[i] = (digitalRead(SENSOR_PIN[i]) == CIZGI_SEVIYESI);
    if (s[i]) adet++;
  }
  return adet;
}

// ===================== MOTORLAR =====================

// sol, sag: -255 (tam geri) ... 0 (dur) ... 255 (tam ileri)
void motorSur(int sol, int sag) {
  if (SOL_TERS) sol = -sol;
  if (SAG_TERS) sag = -sag;
  tekMotor(ENA, IN1, IN2, sol);
  tekMotor(ENB, IN3, IN4, sag);
}

void tekMotor(byte en, byte inA, byte inB, int hiz) {
  hiz = constrain(hiz, -MAKS_HIZ, MAKS_HIZ);
  if (hiz >= 0) {
    digitalWrite(inA, HIGH);
    digitalWrite(inB, LOW);
  } else {
    digitalWrite(inA, LOW);
    digitalWrite(inB, HIGH);
    hiz = -hiz;
  }
  analogWrite(en, hiz);
}

// ===================== BAŞLANGIÇ =====================

void baslamayiBekle() {
  if (BUTON_KULLAN) {
    Serial.println(F("Baslamak icin butona bas (D2)"));
    while (digitalRead(BUTON_PIN) == HIGH) {
      digitalWrite(LED_BUILTIN, (millis() / 100) % 2);  // hızlı yanıp söner
    }
    delay(500);  // elini robottan çekmen için
  } else {
    Serial.print(BASLAMA_BEKLEME / 1000);
    Serial.println(F(" saniye sonra basliyor..."));
    unsigned long bas = millis();
    while (millis() - bas < BASLAMA_BEKLEME) {
      digitalWrite(LED_BUILTIN, (millis() / 250) % 2);  // yavaş yanıp söner
    }
  }
  Serial.println(F("BASLADI"));
}

// ===================== TEST MODLARI =====================

void sensorTesti() {
  bool s[4];
  sensorleriOku(s);

  Serial.print(F("S1 S2 S3 S4:  "));
  for (byte i = 0; i < 4; i++) {
    Serial.print(s[i] ? F("X  ") : F(".  "));
  }
  Serial.print(F("   ham pin: "));
  for (byte i = 0; i < 4; i++) {
    Serial.print(digitalRead(SENSOR_PIN[i]));
    Serial.print(' ');
  }
  Serial.println();
  delay(200);
}

void motorTesti() {
  motorAdim(F("SOL motor ileri  (sadece sol teker, ileri)"), 160, 0);
  motorAdim(F("SAG motor ileri  (sadece sag teker, ileri)"), 0, 160);
  motorAdim(F("Ikisi ileri"), 160, 160);
  motorAdim(F("Ikisi geri"), -160, -160);
  motorAdim(F("Yerinde SOLA donus (sol geri, sag ileri)"), -160, 160);
  Serial.println(F("--- tekrar ---"));
  delay(2000);
}

void motorAdim(const __FlashStringHelper *yazi, int sol, int sag) {
  Serial.println(yazi);
  motorSur(sol, sag);
  delay(1500);
  motorSur(0, 0);
  delay(800);
}
