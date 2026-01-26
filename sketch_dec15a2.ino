#include <M5StickCPlus2.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>

// =====================================================
// WIFI + API
// =====================================================

const char* ssid = "WRITE YOUR SSID HERE";
const char* password = "WRITE YOUR PASSWORD HERE";

const char* entsoUrl = "https://web-api.tp.entsoe.eu/api";
const char* token = "WRITE YOUR TOKEN HERE";
const char* zone = "10YFI-1--------U"; // Finland

// =====================================================
// STATE
// =====================================================

float currentPrice = 0.0;
unsigned long lastFetch = 0;
const unsigned long fetchInterval = 60000;

float cheapestFuturePrice = 9999.0;
time_t cheapestFutureStart = 0;

// =====================================================
// PRICE CLASSIFICATION
// =====================================================

enum PriceLevel {
  NEGATIVE,
  CHEAP,
  OKAY,
  EXPENSIVE,
  PAIN
};

PriceLevel classifyPrice(float c) {
  if (c < 0.0) return NEGATIVE;
  if (c < 3.0) return CHEAP;
  if (c < 6.0) return OKAY;
  if (c < 10.0) return EXPENSIVE;
  return PAIN;
}

// =====================================================
// TIME UTILS
// =====================================================

time_t timegm_esp(struct tm *tm) {
  char *oldTz = getenv("TZ");
  char tzBuf[64];
  if (oldTz) strcpy(tzBuf, oldTz);

  setenv("TZ", "UTC0", 1);
  tzset();
  time_t t = mktime(tm);

  if (oldTz) setenv("TZ", tzBuf, 1);
  else unsetenv("TZ");
  tzset();

  return t;
}

// =====================================================
// API FETCH
// =====================================================

bool fetchPrice() {
  if (WiFi.status() != WL_CONNECTED) return false;

  time_t now = time(nullptr);

  cheapestFuturePrice = 9999.0;
  cheapestFutureStart = 0;

  struct tm tmNow;
  gmtime_r(&now, &tmNow);
  tmNow.tm_hour = 0;
  tmNow.tm_min = 0;
  tmNow.tm_sec = 0;

  char startStr[16];
  sprintf(startStr, "%04d%02d%02d0000",
          tmNow.tm_year + 1900, tmNow.tm_mon + 1, tmNow.tm_mday);

  time_t end = mktime(&tmNow) + 24 * 3600;
  struct tm tmEnd;
  gmtime_r(&end, &tmEnd);

  char endStr[16];
  sprintf(endStr, "%04d%02d%02d%02d%02d",
          tmEnd.tm_year + 1900, tmEnd.tm_mon + 1, tmEnd.tm_mday,
          tmEnd.tm_hour, tmEnd.tm_min);

  String url = String(entsoUrl) +
               "?securityToken=" + token +
               "&documentType=A44" +
               "&out_Domain=" + zone +
               "&in_Domain=" + zone +
               "&periodStart=" + startStr +
               "&periodEnd=" + endStr +
               "&contract_MarketAgreement.type=A01";

  HTTPClient http;
  http.begin(url);
  http.setTimeout(10000);

  if (http.GET() != 200) {
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  int startIdx = payload.indexOf("<timeInterval>");
  int sStart = payload.indexOf("<start>", startIdx);
  int sEnd = payload.indexOf("</start>", sStart);
  String startStrTime = payload.substring(sStart + 7, sEnd);

  struct tm startTm;
  sscanf(startStrTime.c_str(), "%d-%d-%dT%d:%dZ",
         &startTm.tm_year, &startTm.tm_mon, &startTm.tm_mday,
         &startTm.tm_hour, &startTm.tm_min);

  startTm.tm_year -= 1900;
  startTm.tm_mon -= 1;
  startTm.tm_sec = 0;

  time_t startTime = timegm_esp(&startTm);

  int idx = 0;
  int point = 0;
  float latestPrice = currentPrice;

  while ((idx = payload.indexOf("<price.amount>", idx)) != -1) {
    int pEnd = payload.indexOf("</price.amount>", idx);
    float price = payload.substring(idx + 14, pEnd).toFloat();
    time_t priceTime = startTime + (point * 15 * 60);

    if (now >= priceTime && now < priceTime + 900) {
      latestPrice = price * 0.1;
    }

    if (priceTime >= now && price < cheapestFuturePrice) {
      cheapestFuturePrice = price;
      cheapestFutureStart = priceTime;
    }

    point++;
    idx = pEnd;
  }

  currentPrice = latestPrice;
  return true;
}

// =====================================================
// MAIN UI
// =====================================================

void drawUI() {
  PriceLevel level = classifyPrice(currentPrice);

  uint16_t bg;
  const char* label;
  const char* face;

  switch (level) {
    case NEGATIVE:  bg = TFT_PURPLE; label = "FREE POWER"; face = ":D"; break;
    case CHEAP:     bg = TFT_GREEN;  label = "CHEAP";      face = ":)"; break;
    case OKAY:      bg = TFT_BLUE;   label = "OKAY";       face = ":|"; break;
    case EXPENSIVE: bg = TFT_ORANGE; label = "HIGH";       face = ":/"; break;
    default:        bg = TFT_RED;    label = "DONT USE";   face = ">:["; break;
  }

  M5.Lcd.fillScreen(bg);
  M5.Lcd.setTextColor(TFT_WHITE, bg);

  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(10, 20);
  M5.Lcd.print(currentPrice, 2);
  M5.Lcd.print(" c/kWh");

  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(10, 70);
  M5.Lcd.print(label);

  M5.Lcd.setCursor(200, 70);
  M5.Lcd.print(face);
}

// =====================================================
// SHOULD I USE POWER NOW? (SPICY)
// =====================================================

void showDecisionScreen() {
  PriceLevel level = classifyPrice(currentPrice);

  uint16_t bg;
  const char* verdict;
  const char* advice1;
  const char* advice2;

  switch (level) {
    case NEGATIVE:
    case CHEAP:
      bg = TFT_GREEN;
      verdict = "YES. GO WILD.";
      advice1 = "Laundry, EV, Sauna";
      advice2 = "All systems GO";
      break;

    case OKAY:
      bg = TFT_BLUE;
      verdict = "OKAY IF NEEDED";
      advice1 = "Cooking, TV, PC";
      advice2 = "Avoid big loads";
      break;

    case EXPENSIVE:
      bg = TFT_ORANGE;
      verdict = "BETTER WAIT";
      advice1 = "Lights & phone OK";
      advice2 = "Skip laundry";
      break;

    default:
      bg = TFT_RED;
      verdict = "DO NOT USE";
      advice1 = "Essentials only";
      advice2 = "Wait for cheap";
      break;
  }

  M5.Lcd.fillScreen(bg);
  M5.Lcd.setTextColor(TFT_WHITE, bg);

  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(20, 15);
  M5.Lcd.print("Power or nah?");

  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(20, 45);
  M5.Lcd.print(verdict);

  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(20, 95);
  M5.Lcd.print(advice1);

  M5.Lcd.setCursor(20, 120);
  M5.Lcd.print(advice2);

  delay(2500);
  drawUI();
}

// =====================================================
// CHEAPEST FUTURE PRICE FLASH
// =====================================================

void showCheapestFuturePrice() {
  if (cheapestFutureStart == 0) return;

  struct tm t;
  localtime_r(&cheapestFutureStart, &t);

  char timeBuf[32];
  sprintf(timeBuf, "%02d:%02d - %02d:%02d",
          t.tm_hour, t.tm_min,
          (t.tm_hour + 1) % 24, t.tm_min);

  PriceLevel level = classifyPrice(cheapestFuturePrice * 0.1);

  uint16_t bg;
  switch (level) {
    case NEGATIVE:  bg = TFT_PURPLE; break;
    case CHEAP:     bg = TFT_GREEN;  break;
    case OKAY:      bg = TFT_BLUE;   break;
    case EXPENSIVE: bg = TFT_ORANGE; break;
    default:        bg = TFT_RED;    break;
  }

  M5.Lcd.fillScreen(bg);
  M5.Lcd.setTextColor(TFT_WHITE, bg);

  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(20, 20);
  M5.Lcd.print("CHEAPEST AHEAD");

  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(20, 60);
  M5.Lcd.print(cheapestFuturePrice * 0.1, 2);
  M5.Lcd.print(" c/kWh");

  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(20, 110);
  M5.Lcd.print(timeBuf);

  delay(2500);
  drawUI();
}

// =====================================================
// BUTTONS
// =====================================================

void handleButtons() {
  M5.update();

  if (M5.BtnA.wasPressed()) {
    showDecisionScreen();
  }

  if (M5.BtnB.wasPressed()) {
    showCheapestFuturePrice();
  }
}

// =====================================================
// SETUP / LOOP
// =====================================================

void setup() {
  M5.begin();
  M5.Lcd.setRotation(1);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(300);

  configTime(0, 0, "pool.ntp.org", "time.google.com");
  setenv("TZ", "EET-2EEST,M3.5.0/3,M10.5.0/4", 1);
  tzset();

  fetchPrice();
  drawUI();
}

void loop() {
  if (millis() - lastFetch > fetchInterval) {
    if (fetchPrice()) drawUI();
    lastFetch = millis();
  }

  handleButtons();
}
