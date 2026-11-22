# Hardware Specification

This document describes the physical hardware used by the forest-fire
detection mesh. It keeps **confirmed** connections and experimentally
**verified** behavior separate from **proposed** or **untested** designs.
Nothing here overrides the working specification
(`forest_fire_mesh_node_project_spec.md`), which remains the requirements
authority for hardware safety rules.

- **Status:** wiring map verified on the bench (probe session 2026-10-24,
  over-the-air confirmation 2026-11-06). §4 is the authoritative pin table.
  The power chain (§5) and the sensor electrical details (MQ-2 D0 level,
  flame-sensor polarity) remain open.
- **Label legend:** CONFIRMED (owner-stated), VENDOR-PAGE (read from the
  vendor product page — not a manufacturer datasheet), VERIFIED (checked
  against an authoritative source), UNVERIFIED (still requires measurement or
  a datasheet check), ESTIMATE (plausible figure pending measurement).

---

## 1. Module inventory

### 1.1 Radio — CC1101 module (Robotistan)

Source: https://www.robotistan.com/cc1101-wireless-modul (VENDOR-PAGE, plus
TI CC1101 datasheet SWRS061 for chip-level facts).

| Property | Value | Status |
|---|---|---|
| Band | 433 MHz (module marking confirmed by owner) | CONFIRMED |
| Supply voltage | 1.8–3.6 V → power from the 3.3 V rail; **never 5 V** | VENDOR-PAGE |
| Sensitivity | −110 dBm @ 1.2 kBaud, 433 MHz, 1% PER | VENDOR-PAGE |
| RX current | 15.6 mA @ 2.4 kBaud (datasheet: ~14–16 mA across rates) | VENDOR-PAGE / VERIFIED |
| Output power | programmable, up to +10 dBm (10 mW) | VENDOR-PAGE |
| Antenna | included with the module (type/gain unknown → e.r.p. unverified) | VENDOR-PAGE |
| Interface | SPI (CS, SCK, MOSI, MISO) + GDO0/GDO2 pins | chip datasheet (VERIFIED) |

⚠ **Regulatory caution (decision D10):** the legal limit at 433.05–434.79 MHz
is 10 mW **e.r.p.**, and e.r.p. includes antenna gain. +10 dBm conducted plus
even ~2 dBi of antenna gain would exceed it. The configured TX power MUST be
set below the module maximum and the actual e.r.p. verified (measurement or
calculation with the real antenna) before any outdoor transmission.

### 1.2 MCU — ESP8266 board sold as "NodeMCU V3 ESP-12E" (32 pins)

- **This board is a NON-STANDARD clone.** Owner-documented silkscreen
  (docs/esp_pinout.md, 2026-10-24):
  right side (top→bottom): 3V3, D10, MOSI, SCLK, EN, TX, RX, GND, D12, D13,
  D14, ADC, GND, GND, 3V3, RESET;
  left side (top→bottom): 5V, CH_PD, VBUS, GND, 5V, 3.3V, D15, D0, D16, D2,
  D5, D4, CSO, MISO, D9, GND.
- No standard NodeMCU prints D9–D16/CSO/VBUS. **The label→GPIO mapping is
  NOT the usual NodeMCU table** and is being verified empirically
  (probe sketch `firmware/bringup/03_gpio_probe`).
- The printed MOSI/SCLK/MISO/CSO labels are **not** the HSPI nets on this
  clone: the radio only initialised after the wires were moved to
  D14/D13/D12/D15 (measured 2026-11-06). The label→GPIO table is filled from
  probe results, not assumptions.
- ESP8266 facts (VERIFIED, platform-level): 3.3 V logic, **not 5 V tolerant**;
  bootstrap pins (GPIO0/GPIO2/GPIO15) constrain wiring; ~40–50 KB free heap.

### 1.3 Gas/smoke — MQ-2 module (Robotistan)

Source: https://www.robotistan.com/yanici-gaz-ve-sigara-dumani-sensor-karti-mq-2
(VENDOR-PAGE).

| Property | Value | Status |
|---|---|---|
| Detection range | 300–10,000 ppm combustible gas / smoke | VENDOR-PAGE |
| Supply | **5 V** (heater requires it) | VENDOR-PAGE + working spec |
| Outputs | digital (comparator, "TTL level") **and** analog | VENDOR-PAGE |
| D0 high-level voltage | unknown — may be pulled to the 5 V rail | **UNVERIFIED — must be measured before connecting to any GPIO** |
| Heater current | not stated on the page; typical MQ-2 heater ≈ 150 mA @ 5 V per the sensor family datasheet | **ESTIMATE — must be measured for this board (the earlier ~900 mA figure is an unverified worst-case rumor, not a measurement)** |
| Warm-up time | not stated; readings during warm-up are not trustworthy | UNVERIFIED — measure |

Consequences recorded in the protocol: firmware only sets `mq2_valid` after
the measured warm-up time (protocol-spec §5.1), and the MQ-2 heater decision
(continuous vs MOSFET-switched) remains open pending real current
measurements.

### 1.4 Flame sensor module (Robotistan)

Source: https://www.robotistan.com/ates-algilayici-sensor-karti-flame-sensor
(VENDOR-PAGE).

| Property | Value | Status |
|---|---|---|
| Detection | IR flame detection, 760–1100 nm | VENDOR-PAGE |
| Outputs | **analog and digital**, sensitivity trimpot on board | VENDOR-PAGE |
| Supply voltage | not captured from the page (commonly 3.3–5 V on this board family) | **UNVERIFIED — verify before wiring** |
| Digital output polarity / voltage | unknown | **UNVERIFIED — must be measured** |
| Detection range / angle | not stated | UNVERIFIED — characterize with safe controlled tests |

Protocol usage: digital output only, debounced in firmware before any ALERT.
Known false-trigger sources to characterize: direct sunlight, other IR
sources. Do **not** perform hazardous flame tests indoors.

### 1.5 Temperature/humidity — DHT22 / AM2302 (Robotistan)

Source: https://www.robotistan.com/dht22-sicaklik-ve-nem-sensoru-am2302
(VENDOR-PAGE; links the SparkFun DHT22 datasheet).

| Property | Value | Status |
|---|---|---|
| Temperature | −40…+80 °C, ±1 °C | VENDOR-PAGE |
| Humidity | 0–100 %RH, ±5 %RH | VENDOR-PAGE |
| Sampling | 2-second minimum reading period | VENDOR-PAGE (no constraint at our 300 s report cadence) |
| Pull-up | **requires external 4.7–10 kΩ pull-up on the data line** (bare sensor, no breakout) | VENDOR-PAGE |
| Form | calibrated digital output; 3.3–5.5 V supply per typical AM2302 datasheet | VENDOR-PAGE / verify with linked datasheet |

⚠ **Parts note:** the on-hand resistor set (15 kΩ, 22 kΩ, 27 kΩ, 30 kΩ,
8.2 kΩ) includes **8.2 kΩ, which fits the recommended 4.7–10 kΩ pull-up
window** — no new purchase strictly required.

---

## 2. Radio driver / library — RadioLib (decision D12)

- Library: **RadioLib by jgromes 7.8.1** — VALIDATED on the bench 2026-11-06
  (CC1101 init, continuous RX, 18/18 authenticated packets received clean).
- Verified capabilities relevant to us (details + links in decision log D12
  and protocol-spec §18): CC1101 support, ESP8266 examples, CS+GDO0(+GDO2)
  pin model, variable-length packets (interrupt-driven < 64 bytes — our ≤60 B
  packets stay in this regime), configurable sync word, CC1101 CRC16,
  appended RSSI/LQI status, `getRSSI()/getLQI()`.
- RadioLib settings actually applied (all in `firmware/lib/MeshForestBoard/src/MeshForestBoard.h`):
  carrier 434.0 MHz, output power 0 dBm, **deviation 25 kHz** (D15 — the
  5 kHz default was measured too narrow for this module pair), sync word
  `0x12AD` **for the demo only** (a deployment MUST set its own, §3.3).
- Not yet applied in firmware (protocol-spec §16.1): the bit rate is
  RadioLib's **4.8 kbps** default (D17; 38.4 kBaud is a v1.1 candidate),
  `variablePacketLengthMode(60)`, and CCA before transmit.
- To verify at bring-up (from protocol-spec §15.3): bad-CRC packets are
  actually rejected (RadioLib checks CRC from status bytes with auto-flush
  off by default); GDO0 interrupt wiring; measured airtime vs. duty counter.

## 3. Gateway hardware (decision D11)

- Same platform as a node (NodeMCU + CC1101) plus:
  - OLED display — **SSD1306 128×64 over I²C**, address 0x3C, SDA = D2
    (GPIO2), SCL = D5 (GPIO5); detected on the bench 2026-10-24
    (`[oled] SSD1306 found at 0x3C`). Exact part/vendor still unrecorded.
  - Speaker module — **part UNVERIFIED** (driver circuit, volume, and whether
    active buzzer or amplified speaker TBD).
- Power: mains at a forest lookout station + battery UPS. UPS design open.
- Role: displays alerts for **authorized personnel** who decide on escalation.

## 4. Wiring map — the authoritative pin table

**Label→GPIO rule (empirically confirmed 2026-10-24, see §4.1): this clone's
D-labels are raw GPIO numbers.** The pins printed `MOSI`/`SCLK`/`MISO`/`CSO`
are **not** the same nets as D14/D13/D12/D15: the CC1101 failed to initialise
(RadioLib -2, chip not found) while wired to them, and only worked after all
four SPI wires were moved to the D-labelled pins (2026-11-06). Do not wire
the radio to the SPI-labelled pins.

| Signal | Board label | GPIO | Status |
|---|---|---|---|
| CC1101 SCK | D14 | 14 | **CONFIRMED** — radio initialises and receives |
| CC1101 MOSI/SI | D13 | 13 | **CONFIRMED** — radio initialises and receives |
| CC1101 MISO/SO | D12 | 12 | **CONFIRMED** — radio initialises and receives |
| CC1101 CSN | D15 | 15 | **CONFIRMED** — radio initialises and receives |
| CC1101 GDO0 | D4 (moved from D2) | 4 | D4=GPIO4 probe-CONFIRMED; **D2=GPIO2 is FORBIDDEN for GDO0** (it caused the boot loop — GDO0 idles LOW) |
| CC1101 GDO2 | — | — | not connected (RadioLib optional) |
| CC1101 VCC / GND | 3V3 / GND | — | requirement VERIFIED (1.8–3.6 V module); module-side voltage to be measured |
| DHT22 DATA (+8.2 kΩ to 3V3) | D0 | 0 | probe-consistent (D0=GPIO0); ⚠ the pull-up is BOOT-CRITICAL for GPIO0 |
| Multimeter | — | — | AVAILABLE (owner-confirmed) |
| Flame sensor DO | D16 (planned) | 16 | wire pending; D16=GPIO16 (polled input, no interrupt needed) |
| MQ-2 DO (via 15 kΩ/22 kΩ divider ≈ 3.0 V) | D5 | 5 | probe-CONFIRMED (D5=GPIO5); divider output voltage still to be measured |
| MQ-2 VCC / GND | 5V / GND | — | 5 V heater rail; never power from 3V3 |
| Siren (gateway) | D16 (planned) | 16 | wire pending (gateway build) |

**Board identification — results (2026-10-24):**
1. `03_gpio_probe` confirmed by touch: D12=GPIO12, D13=GPIO13, D14=GPIO14,
   D4=GPIO4, D5=GPIO5, D2=GPIO2. Boot evidence makes D0=GPIO0, D15=GPIO15,
   D16=GPIO16 consistent (final touch confirmation pending, not blocking).
2. Boot-loop root cause: GDO0 (idles LOW) on D2=GPIO2 violated the bootstrap
   requirement (boot mode (2,7), endless `Fatal exception (0)`). Removing
   that one wire fixed it. GDO0's new home is D4=GPIO4.
3. Radio-init test: `02_radio_ping` first failed with RadioLib code -2
   (CHIP_NOT_FOUND) while wired to the SPI-labelled pins. After moving all
   four SPI wires to D14/D13/D12/D15 the CC1101 initialised, entered
   continuous RX, and later exchanged packets (2026-11-06). **The
   SPI-labelled pins are not usable for the radio on this clone.**

### 4.1 Board label → GPIO map (this clone, empirically confirmed)

| Label | GPIO | Evidence |
|---|---|---|
| D0 | 0 | consistent (rule + DHT pull-up boot behavior); touch-confirm pending |
| D2 | 2 | **CONFIRMED** (GDO0 boot-loop incident) |
| D4 | 4 | **CONFIRMED** (probe touch) |
| D5 | 5 | **CONFIRMED** (probe touch, with MQ-2 divider attached) |
| D9 | 3? (RX) | presumed — serial pins, do not use |
| D10 | 1? (TX) | presumed — serial pins, do not use |
| D12 | 12 | **CONFIRMED** (probe touch) |
| D13 | 13 | **CONFIRMED** (probe touch) |
| D14 | 14 | **CONFIRMED** (probe touch) |
| D15 | 15 | consistent (reads LOW via onboard pull-down); touch-confirm pending |
| D16 | 16 | consistent (reads LOW with pulldown); touch-confirm pending |
| MOSI/SCLK/MISO/CSO | — | **not usable for the radio** — different nets from D13/D14/D12/D15 (radio init failed -2 while wired here; 2026-11-06) |
| ADC | A0 | untouched this session |

Bootstrap rules that bit us once: **GPIO0 must be HIGH at boot** (DHT22's
8.2 kΩ pull-up provides it), **GPIO2 must be HIGH at boot** (GDO0 violated
this), GPIO15 must be LOW at boot (its onboard pull-down handles it).

## 5. Power chain — OPEN (tracked in the working spec §6)

Solar panel → CN3791 (config unverified) → protected 18650 (BMS spec
unverified) → 3.3 V rail (ESP8266 + CC1101) and 5 V rail (MQ-2). All
regulator parts, current capability, and the energy budget table remain open
until the MQ-2 heater current is measured and the continuous vs duty-cycled
decision is made.

## 6. Bench verification checklist (before any firmware claims)

1. Measure MQ-2 D0 high/low voltages and current draw (steady + warm-up).
2. Measure flame-sensor DO voltage/polarity and supply requirement.
3. Verify NodeMCU clone pinout; record silkscreen → GPIO table.
4. Verify CC1101 init over SPI (RadioLib), set `PKTLEN=60`, confirm CRC
   rejection of corrupted packets.
5. Measure CC1101 TX current at the chosen power and validate the airtime
   accounting used by the duty budget.
6. Only after 1–5: two-node PER vs distance measurements (protocol-spec §15.3).
