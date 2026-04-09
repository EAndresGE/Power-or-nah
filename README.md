# Power or Nah ⚡

A small desk device built with an M5StickC Plus 2 that shows real-time electricity prices in a simple, glanceable way.

Instead of asking you to interpret numbers, the device uses color-coded categories to answer a basic question:

**Is electricity cheap right now, or should I wait?**

---

## What it does

- Fetches real-time electricity spot prices from the ENTSO-E API
- Displays the current price on screen (c/kWh)
- Classifies the price visually:
  - Free / Cheap / Okay / High / Don't use
- Updates automatically every minute
- Button actions:
  - Show the cheapest upcoming time slot
  - Show how today's price categories were defined

---

## Relative price logic (important)

The device does **not** use fixed price thresholds.

Instead, price categories are calculated **relative to today's prices**:

- All prices for the current day are collected
- Percentile-based thresholds are calculated from that data
- The current price is classified based on where it sits *within today's range*

This means:
- Winter prices don't permanently show "DON'T USE"
- Summer prices don't falsely look "cheap"
- The classification adapts automatically to market conditions

The screen explicitly notes this with:

> *"Relative to today"*

---

## Region & data source

- Electricity data is fetched from **ENTSO-E**
- The default bidding zone is **Finland** (`10YFI-1--------U`)
- The concept works for **any country or bidding zone** supported by ENTSO-E

To adapt this project to another country, you only need to change the bidding zone code in the sketch.

---

## Getting an ENTSO-E API token

Access to the ENTSO-E API requires registration — the process takes a few days:

1. Create an account at [transparency.entsoe.eu](https://transparency.entsoe.eu)
2. **Email them to request API access** — registration alone does not grant it. Send a request to `transparency@entsoe.eu` stating you want API access for your account.
3. Once access is granted (typically within a few business days), log in and go to your profile page to find your API security token.
4. Paste the token into the sketch where indicated (`YOUR_ENTSOE_API_TOKEN`).

---

## Hardware

- M5StickC Plus 2
- WiFi connection required
- Can run on battery, but works best when powered continuously

---

## Setting up Arduino IDE

**Requires Arduino IDE 2.0 or higher.** IDE 1.8.x is not recommended and may cause compilation failures with the M5Stack libraries.

Download Arduino IDE 2.x from [arduino.cc/en/software](https://www.arduino.cc/en/software).

### 1. Add the M5Stack board package

Open Arduino IDE and go to **File > Preferences**. Add the following URL to *Additional Boards Manager URLs*:

```
https://static-cdn.m5stack.com/resource/arduino/package_m5stack_index.json
```

Then go to **Tools > Board > Boards Manager**, search for `M5Stack`, and install the **M5Stack** package.

### 2. Select the correct board

Go to **Tools > Board > M5Stack Arduino** and select **M5StickCPlus2**.

### 3. Install the required library

Go to **Tools > Manage Libraries**, search for `M5StickCPlus2`, and install it. When prompted to install dependencies, accept all — this will also install **M5Unified** and **M5GFX**.

The libraries for WiFi, HTTP requests, and NTP time sync (`WiFi.h`, `HTTPClient.h`, `time.h`) are all built into the ESP32 core — no separate install needed.

---

## Opening the sketch

Arduino IDE requires the sketch folder name to **exactly match** the `.ino` filename.

When you clone or download this repo, rename the folder containing the `.ino` file to match it exactly:

```
sketch_dec15a2/
└── sketch_dec15a2.ino
```

If the folder name doesn't match, Arduino IDE will show a "main file missing" error and refuse to open it.

---

## Configuration

Before uploading, open `sketch_dec15a2.ino` and fill in your credentials:

```cpp
const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* token    = "YOUR_ENTSOE_API_TOKEN";
const char* zone     = "10YFI-1--------U";  // Change for other countries
```

---

## Why this exists

This project is intentionally simple.

No charts.  
No predictions.  
No notifications.

Just:
- real prices
- honest context
- fast decisions

Think of it as a small ambient instrument for energy awareness.

---

## Disclaimer

This project is for informational purposes only.  
It does not provide financial or energy usage advice.

Electricity prices can change rapidly — always use your own judgment.

---

## License

MIT License.  
Use it, modify it, build on it.
