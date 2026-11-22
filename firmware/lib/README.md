# firmware/lib

This directory holds Arduino libraries used by the MeshForest demo sketches.

## `yanki-protocol/` — git submodule

The protocol implementation, its specification, and its Python reference model
live in a separate `yanki-protocol` repository — **YANKI** (*Yerel Ağlarda
Noktalararası Katmanlı İletişim*, "Layered Inter-Node Communication in Local
Networks"). It is consumed here as a git submodule at
`firmware/lib/yanki-protocol`.

Initialise it after cloning:

```sh
git submodule update --init --recursive
```

The submodule provides `<YankiProtocol.h>` (C++ codec and protocol logic) and
`<YankiConfig.h>` (network identity and keys, including the demo PSK and
network ID). It is a committed gitlink; do not add it to `.gitignore`.

## `MeshForestBoard/` — app-local board library

`MeshForestBoard/` is an ordinary Arduino library owned by this application. It
provides `<MeshForestBoard.h>` with pin assignments, radio parameters, and demo
timing. It deliberately contains no protocol code, keys, or network identity —
those come from the protocol library.

## Build flags

Point arduino-cli at this directory so both libraries are on the include path:

```sh
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 --libraries firmware/lib firmware/demo_node
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 --libraries firmware/lib firmware/demo_gateway
```

`--libraries firmware/lib` makes `<YankiProtocol.h>` and `<MeshForestBoard.h>`
resolvable without copying sources into each sketch folder.
