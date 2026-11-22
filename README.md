# MeshForest — forest-fire early-warning mesh (application repo)

Low-cost, solar/battery-powered wireless sensor nodes that watch for early
fire indicators and relay authenticated alerts over multiple hops to a
gateway with a display and siren.

**Status: bench prototype — NOT a tested or certified fire-detection system.**
Two boards have exchanged authenticated packets over the air on a desk
(2026-11-06, 18/18 clean). Range, field packet-error rate, multi-hop relay
behavior, sensor calibration, and power autonomy are **not yet tested**. Do
not deploy this where a life depends on it. See
[docs/test-log.md](docs/test-log.md) for exactly what has and has not been
verified.

## What it is

This repository is the **application side** of the project: the ESP8266
firmware sketches, board configuration, hardware notes, and project-level
documentation. The custom protocol implementation and its reference model live
in a separate `yanki-protocol` repository, consumed here as a git submodule at
[firmware/lib/yanki-protocol](firmware/lib/yanki-protocol).

| Part | Description |
|---|---|
| Field node | NodeMCU (ESP8266) + CC1101 433 MHz radio + DHT22 + flame sensor (+ MQ-2, not yet wired). Sends authenticated sensor reports and flame alerts, relays other nodes' traffic. |
| Gateway | Same platform + SSD1306 OLED + siren. Dedups alerts, ACKs them end-to-end, fires the siren once per unique alert. |
| Protocol library | Custom compact binary mesh protocol (60-byte packets): managed flooding, TTL + duplicate suppression, AES-CMAC authentication, replay windows, end-to-end gateway ACKs with bounded retries. Specified in [firmware/lib/yanki-protocol/docs/protocol-spec.md](firmware/lib/yanki-protocol/docs/protocol-spec.md). |
| Board library | [firmware/lib/MeshForestBoard](firmware/lib/MeshForestBoard) — app-local Arduino library with pins, radio parameters, and demo timing. |
| Protocol tests | Python reference model and discrete-event flood simulator live with the protocol repository, not here. The C++ firmware cross-validates against its golden vectors at boot. |

## Repository layout

```
docs/                       project requirements, hardware notes, decisions, test log
  forest_fire_mesh_node_project_spec.md   project requirements (source of truth)
  hardware.md               verified pin map + wiring status
  decision-log.md           every accepted design decision (D1–D18) with rationale
  test-log.md               every test run with results AND its limits
  protocol-spec.md          retained for now; authoritative copy moves to the submodule
firmware/
  lib/MeshForestBoard/      app-local board library (pins, radio, timing)
  lib/yanki-protocol/       protocol git submodule (spec, C++ codec, Python model)
  demo_node/ demo_gateway/  the working bench demo (Arduino sketches)
  bringup/                  staged bring-up sketches (blink → SPI probe → ping)
tools/proto/                retained for now; protocol tests move to the submodule
```

## Hardware (bench-verified parts)

- NodeMCU V3 ESP8266 ESP-12E (**non-standard clone** — its D-labels are raw
  GPIO numbers; see [docs/hardware.md](docs/hardware.md) §4)
- CC1101 433 MHz module (RadioLib 7.8.1), wired on D14/D13/D12/D15 + GDO0 on D4
- DHT22 on GPIO0, flame sensor on GPIO16, MQ-2 planned on GPIO5 via divider
- Full verified pin table and bootstrap traps (GPIO0/GPIO2/GPIO15) in
  [docs/hardware.md](docs/hardware.md)

## Build & flash

Toolchain: arduino-cli 1.5.1, ESP8266 core 3.1.2, FQBN `esp8266:esp8266:nodemcuv2`.

First initialise the protocol submodule:

```sh
git submodule update --init --recursive
```

Then compile the sketches with both the board library and the protocol library
on the include path:

```sh
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 --libraries firmware/lib firmware/demo_node
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 --libraries firmware/lib firmware/demo_gateway
arduino-cli upload -p <PORT> --fqbn esp8266:esp8266:nodemcuv2 --libraries firmware/lib firmware/demo_node
```

The sketches include `<YankiProtocol.h>` from the protocol library and
`<MeshForestBoard.h>` from the app-local board library. See
[firmware/lib/README.md](firmware/lib/README.md) for details.

## Security model (honest version)

- Every packet is authenticated with AES-128-CMAC (RFC 4493) under one
  pre-shared 128-bit network key; replay protection is a per-source window.
- **No encryption** — payloads are readable by anyone with a receiver.
- A captured node reveals the network key and can forge anything, including
  ACKs. No secure key storage on ESP8266. Rotation = reflash everything.
- The demo key in the protocol library's `YankiConfig.h` is a public
  placeholder. For any real deployment, provision a fresh random key that is
  never committed.

Full threat model and limitations: the protocol spec's security and threat-model
sections in the `yanki-protocol` repository.

## Regulatory note

Configured for the 433 MHz SRD band (Türkiye, general-purpose rules: 10 mW
e.r.p., ≤10 % duty cycle) at 0 dBm with the stock antenna. e.r.p. including
antenna gain and the occupied-bandwidth mask are still unverified — check your
local rules before transmitting outdoors.

## License

MIT — see [LICENSE](LICENSE).
