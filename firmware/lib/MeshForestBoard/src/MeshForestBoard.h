#pragma once
/*
 * MeshForestBoard.h — board configuration for the MeshForest application
 * (pins, radio parameters, demo timing).
 *
 * This is the application-local counterpart to the protocol library's
 * <YankiConfig.h>. Network identity and keys live in the protocol library;
 * this file only describes this application's hardware.
 *
 * Pin values are PROPOSED defaults. The owner's actual wiring wins
 * (docs/hardware.md §4 is the authoritative pin table); fix this file to
 * match it before flashing.
 */

#include <Arduino.h>

// ---- node identity (application demo IDs) -------------------------------
#define ID_GATEWAY 0x01
#define ID_NODE1   0x02
#define ID_NODE2   0x03   // stretch goal (relay node)

// ---- radio pins (CONFIRMED by the 2026-10-24 probe session; see
// docs/hardware.md §4). This clone's D-labels are RAW GPIO numbers.
#define PIN_RADIO_CS   15   // "CSO" label = GPIO15 (onboard pull-down, boot-safe)
#define PIN_RADIO_GDO0 4    // "D4" label = GPIO4 — GDO0 lives here, NOT on D2
                            // (D2 = GPIO2, a boot-strap pin: GDO0 idles LOW
                            // and held GPIO2 down -> boot loop, empirically)
// SPI is fixed on this board: SCLK = GPIO14, MOSI = GPIO13, MISO = GPIO12.
// Use the D14/D13/D12/D15 labels. The pins printed MOSI/SCLK/MISO/CSO are
// NOT the same nets: the radio only initialised after the four wires were
// moved off them (measured 2026-11-06, docs/test-log.md).

// ---- node sensor pins (labels CONFIRMED; see docs/hardware.md §4) -------
#define PIN_DHT22  0        // "D0" = GPIO0. BOOT-CRITICAL: the DHT's 8.2 kOhm
                            // pull-up holds GPIO0 HIGH at boot. If the DHT is
                            // ever removed, GPIO0 must be pulled high or the
                            // board will not start normally.
#define PIN_FLAME  16       // "D16" = GPIO16: polled input, no boot constraint
#define PIN_MQ2    5        // "D5" = GPIO5: MQ-2 divided DO (15k/22k divider)
// Flame module polarity is UNVERIFIED for the exact board: most of these
// modules pull DO LOW when a flame is detected. If the boot self-test works
// but the node never triggers, flip this to LOW.
#define FLAME_ACTIVE_LEVEL HIGH

// ---- gateway pins ---------------------------------------------------------
#define PIN_SIREN  16       // "D16" = GPIO16, ESP drives HIGH to sound
#define PIN_OLED_SDA 2       // "D2" = GPIO2 (I2C data; idles HIGH = boot-safe)
#define PIN_OLED_SCL 5       // "D5" = GPIO5 (I2C clock; free on gateway)
#define OLED_I2C_ADDR 0x3C  // standard SSD1306; fall back to 0x3D/0x3F if needed
#define OLED_WIDTH  128
#define OLED_HEIGHT 64

// ---- radio parameters (both boards MUST match; protocol-spec §14) --------
#define RF_FREQ_MHZ   434.0      // inside 433.05-434.79 MHz SRD band
#define RF_POWER_DBM  0          // 1 mW + ~2 dBi antenna = safely under the
                                 // 10 mW e.r.p. legal limit (decision D10)
#define RF_SYNC0 0x12           // demo keeps RadioLib default sync word;
#define RF_SYNC1 0xAD           // a real deployment sets its own (§3.3)
// Frequency deviation, kHz. RadioLib's CC1101 default is 5 kHz, measured
// too narrow for this module pair: 17-byte packets corrupted at bench
// range while 4-byte pings passed (2026-11-06, docs/test-log.md). Two
// ±10 ppm crystals at 434 MHz sit up to ~8.7 kHz apart — wider than the
// default deviation itself. 25 kHz gives ~5x margin; the 135 kHz default
// RX bandwidth still fits (Carson ≈ 55 kHz). MUST match on all boards
// (protocol-spec §14).
#define RF_FREQ_DEV_KHZ 25.0f

// ---- demo timing profile (bench profile, decision D13) -------------------
#define REPORT_INTERVAL_MS  10000UL   // demo/bench cadence (2026-11-06): one
                                       // 17-byte report ≈ 43 ms airtime per
                                       // 10 s ≈ 0.43% duty — inside the 1%
                                       // engineering margin (60 s ≈ 0.07% for
                                       // deployment; ALERTs add short bursts)
#define FLAME_DEBOUNCE_MS      500    // persistence before ALERT
#define ALERT_REARM_MS        5000    // min time between alerts per node
#define SIREN_MS              3000    // gateway siren blast
