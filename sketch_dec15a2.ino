#include <M5StickCPlus2.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>

// =====================================================
// WIFI + API
// =====================================================

const char* ssid = "DNA-WIFI-CFC8";
const char* password = "aKAB6J6c";

const char* entsoUrl = "https://web-api.tp.entsoe.eu/api";
const char* token = "c4c7b7bf-633e-4e61-972e-1ecd0cb878ef";
const char* zone = "10YFI-1--------U";

// =====================================================
// STATE
// =====================================================

float currentPrice = 0.0;
unsigned long lastFetch = 0;
const unsigned long fetchInterval = 60000;

float cheapestFuturePrice = 9999.0;
time_t cheapestFutureStart = 0;

// Store today's prices (max 96 x 15min)
float todayPrices[100];
int todayPriceCount = 0;

// Dynamic thresholds (relative to today)
float tCheap, tOkay, tExpensive, tPain;

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
  if (c <= tCheap) return CHEAP;
  if (c <= tOkay) return OKAY;
  if (c <= tExpensive) return EXPENSIVE;
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
// HELPERS
// =====================================================

int cmpFloat(const void* a, const void* b) {
  float fa = *(float*)a;
  float fb = *(float*)b;
  return (fa > fb) - (fa < fb);
}

float percentile(float* arr, int n, float p) {
  int idx = (int)((n - 1) * p);
  return arr[idx];
}

// =====================================================
// API FETCH
// =====================================================

bool fetchPrice() {
  if (WiFi.status() != WL_CONNECTED) return false;

  time_t now = time(nullptr);

  cheapestFuturePrice = 9999.0;
  cheapestFutureStart = 0;
  todayPriceCount = 0;

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
    float price = payload.substring(idx + 14, pEnd).toFloat() * 0.1;
    time_t priceTime = startTime + (point * 15 * 60);

    if (todayPriceCount < 100) {
      todayPrices[todayPriceCount++] = price;
    }

    if (now >= priceTime && now < priceTime + 900) {
      latestPrice = price;
    }

    if (priceTime >= now && price < cheapestFuturePrice) {
      cheapestFuturePrice = price;
      cheapestFutureStart = priceTime;
    }

    point++;
    idx = pEnd;
  }

  qsort(todayPrices, todayPriceCount, sizeof(float), cmpFloat);

  tCheap     = percentile(todayPrices, todayPriceCount, 0.30);
  tOkay      = percentile(todayPrices, todayPriceCount, 0.55);
  tExpensive = percentile(todayPrices, todayPriceCount, 0.80);
  tPain      = percentile(todayPrices, todayPriceCount, 0.95);

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

  M5.Lcd.setCursor(10, 110);
  M5.Lcd.print("Relative to today");
}

// =====================================================
// DECISION SCREEN → NOW EXPLANATION SCREEN
// =====================================================

void showDecisionScreen() {
  if (todayPriceCount == 0) return;

  float minP = todayPrices[0];
  float maxP = todayPrices[todayPriceCount - 1];

  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);

  // Title
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(10, 8);
  M5.Lcd.print("PRICE CONTEXT");

  // Range
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(10, 35);
  M5.Lcd.print("Range:");
  M5.Lcd.print(" ");
  M5.Lcd.print(minP, 1);
  M5.Lcd.print(" - ");
  M5.Lcd.print(maxP, 1);
  M5.Lcd.print(" c");

  // Thresholds (smaller font)
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(10, 65);
  M5.Lcd.print("CHEAP  <= ");
  M5.Lcd.print(tCheap, 1);

  M5.Lcd.setCursor(10, 80);
  M5.Lcd.print("OKAY   <= ");
  M5.Lcd.print(tOkay, 1);

  M5.Lcd.setCursor(10, 95);
  M5.Lcd.print("HIGH   <= ");
  M5.Lcd.print(tExpensive, 1);

  // Footer
  M5.Lcd.setCursor(10, 120);
  M5.Lcd.print("Categories are relative to today");

  delay(6000);   // ⏱ stays longer
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

  M5.Lcd.fillScreen(TFT_GREEN);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_GREEN);

  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(20, 20);
  M5.Lcd.print("CHEAPEST AHEAD");

  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(20, 60);
  M5.Lcd.print(cheapestFuturePrice, 2);
  M5.Lcd.print(" c/kWh");

  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(20, 110);
  M5.Lcd.print(timeBuf);

  delay(2500);
  drawUI();
}

// =====================================================
// BUTTONS (UNCHANGED)
// =====================================================

void handleButtons() {
  M5.update();

  if (M5.BtnA.wasPressed()) {
    showCheapestFuturePrice();
  }

  if (M5.BtnB.wasPressed()) {
    showDecisionScreen();
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
