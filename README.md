# Power-or-Nah (M5StickC Plus 2)

A small desk device that shows real-time electricity prices and gives a simple verdict:
**is it a good idea to use power right now, or should you wait?**

Built for an M5StickC Plus 2 using the ENTSO-E electricity market API.

---

## What it does

- Continuously fetches electricity prices (automatic refresh)
- Shows the **current price** with clear color coding
- Answers a simple question:
  > *Should I use power now?*
- Shows the **cheapest upcoming time window**
- Designed to be glanceable, opinionated, and useful

This is meant to live on a desk, not be a full dashboard.

---

## Buttons

- **Button A**  
  Shows a short decision screen with guidance  
  (e.g. *OK to use*, *better wait*, *do not use*)

- **Button B**  
  Shows the cheapest upcoming price and its time range

- **Power button**  
  Default system behavior (screen on/off, restart)

---

## Price logic

Prices are classified into simple levels:

- FREE / CHEAP
- OKAY
- EXPENSIVE
- DO NOT USE

The device does not try to be precise — it tries to be useful.

---

## Region & data source

This implementation is configured for **Finland**, using the ENTSO-E API.

However, the **idea is fully reusable**:
- Any country or bidding zone supported by ENTSO-E
- Or any other electricity pricing API

Only the zone code and data source need to change.

---

## Hardware

- M5StickC Plus 2
- Wi-Fi connection
- USB power recommended for stability

---

## Notes

- API keys and Wi-Fi credentials must be provided by the user
- This project is intentionally simple and opinionated
- Use, adapt, fork, or modify as needed

---

## License

MIT
