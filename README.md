# Power or Nah ⚡

A small desk device built with an M5StickC Plus 2 that shows real-time electricity prices in a simple, glanceable way.

Instead of asking you to interpret numbers, the device uses color-coded categories to answer a basic question:

**Is electricity cheap right now, or should I wait?**

---

## What it does

- Fetches real-time electricity spot prices from the ENTSO-E API
- Displays the current price on screen (c/kWh)
- Classifies the price visually:
  - Free / Cheap / Okay / High / Don’t use
- Updates automatically every minute
- Button actions:
  - Show the cheapest upcoming time slot
  - Show how today’s price categories were defined

---

## Relative price logic (important)

The device does **not** use fixed price thresholds.

Instead, price categories are calculated **relative to today’s prices**:

- All prices for the current day are collected
- Percentile-based thresholds are calculated from that data
- The current price is classified based on where it sits *within today’s range*

This means:
- Winter prices don’t permanently show “DON’T USE”
- Summer prices don’t falsely look “cheap”
- The classification adapts automatically to market conditions

The screen explicitly notes this with:

> *“Relative to today”*

---

## Region & data source

- Electricity data is fetched from **ENTSO-E**
- The default bidding zone is **Finland**
- The concept works for **any country or bidding zone** supported by ENTSO-E

To adapt this project to another country, you only need to change the bidding zone code.

---

## Hardware

- M5StickC Plus 2
- WiFi connection required
- Can run on battery, but works best when powered continuously

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
