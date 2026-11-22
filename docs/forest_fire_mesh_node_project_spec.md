FOREST FIRE DETECTION MESH NODE PROJECT — WORKING SPECIFICATION
================================================================

Document purpose
----------------
This file is the current project context for AI coding assistants (for example, GitHub Copilot) and future project discussions. It describes the intended system, known hardware, decisions made so far, unresolved details, and development priorities. It is deliberately explicit about uncertainty: do not turn an unverified assumption into a design fact.

Status: evolving working specification
Last updated: 2026-11-22 (pre-publication contradiction audit: header byte order corrected to little-endian, implemented data rate documented as 4.8 kbps — see decisions D16/D17/D18)
Decision identifiers (D1–D18) refer to the project's internal decision log, which is not published with this repository; the accepted outcomes are stated inline where they matter.
Firmware target: Arduino C++ / Arduino Core for ESP8266
Primary node MCU: NodeMCU V3, ESP8266 ESP-12E, 32-pin board
Radio: CC1101 sub-GHz transceiver modules
Primary purpose: low-cost, solar/battery-powered, mesh-connected early warning nodes for forest-fire indicators

IMPORTANT INSTRUCTIONS FOR AI CODING ASSISTANTS
------------------------------------------------
1. Read this entire file before proposing changes.
2. Do not rewrite the whole project in one pass. Work in small, reviewable increments.
3. Before writing or changing firmware, inspect the current repository and report its files, current behavior, and missing pieces.
4. Do not invent GPIO pin assignments, CC1101 module wiring, radio frequency, modulation settings, sensor thresholds, packet field sizes, or charger settings. Mark unknowns clearly and ask for confirmation when the answer affects hardware or safety.
5. Keep hardware-specific configuration in one documented place (for example, a configuration header), rather than scattering pin numbers and settings through the firmware.
6. Use Arduino C++ and the Arduino Core for ESP8266, not MicroPython or Python firmware.
7. Compile after each meaningful firmware change. If the toolchain is not available, say so and provide the exact command that should be run; do not claim compilation succeeded without evidence.
8. Avoid unnecessary dependencies. Before choosing a CC1101 library, check that it supports the intended hardware, Arduino Core, and required radio settings. Record the library and version in the project documentation.
9. Never silently change the radio protocol or packet format. Document compatibility changes and keep a protocol version field.
10. Do not treat a compile success, a single bench test, or an MQ-2 reading as proof that this is a reliable or certified fire detector. The prototype is experimental and must be tested and calibrated.
11. Do not add cryptography or describe the system as secure unless the design, key provisioning, authentication, replay protection, and implementation are reviewed. A hash alone is not encryption or authentication.
12. Prefer descriptive functions and comments that explain hardware constraints. Keep secrets and keys out of source control if authentication is later added.
13. Preserve unrelated user changes. Before large edits, summarize the intended change and list which files would change. Prefer Git commits or small diffs.

1. PROJECT GOAL AND OPERATING CONCEPT
-------------------------------------
The project is a cost-conscious, distributed forest-fire early-warning prototype. Small sensor nodes measure local environmental indicators, communicate using CC1101 radio modules, and relay relevant alerts so a remote gateway can announce a potential fire condition. Nodes are intended to run on solar-charged lithium-ion batteries and operate with limited energy and a limited radio packet size.

Target concept:
- Multiple small field nodes deployed across an area, potentially arranged in a hexagonal/geometric layout if that proves useful for coverage planning.
- Each node samples one or more fire-related indicators and reports measurements or an alert over the radio.
- Nodes can forward messages for other nodes (multi-hop mesh behavior is a design goal, not yet a proven implementation).
- A gateway receives the reports and provides a human-visible / audible warning, using a display and siren as the intended gateway features.
- The system should tolerate lost or repeated radio packets and avoid endlessly forwarding duplicates.
- The link should keep messages compact: the ~50–60 byte target is implemented as a **total packet** cap (60-byte FIFO region including the 8-byte header and 4-byte MIC, i.e. a 48-byte payload ceiling — docs/protocol-spec.md §4.2, D6), not 50–60 bytes of application payload.
- The system should prioritize low cost, understandable firmware, serviceability, and power efficiency.

Intended use is experimental early warning and educational/competition demonstration. It is not a substitute for certified fire detection, official forestry monitoring, or emergency services.

2. KNOWN HARDWARE AND PARTS LIST
--------------------------------
The following parts have been mentioned in the current design/parts list. It is not confirmed that every item has already been purchased, assembled, or tested. Confirm actual stock before treating it as physically available.

Main controller:
- NodeMCU V3, ESP8266 ESP-12E, 32-pin development board.
- User wants Arduino Core / Arduino C++ firmware.
- The specific board has silkscreen/label differences: the user reported not seeing the familiar D6/D8 labels, while MISO, MOSI, and SCLK markings are present. Do not assume all NodeMCU clones use the same labels. Identify the actual board revision and map printed labels to GPIO numbers before wiring.

Radio:
- CC1101 transceiver module(s), intended for node-to-node packet communication.
- GDO2 was specifically discussed; final pin assignment and whether it is used are not yet confirmed.
- Operating band CONFIRMED 2026-10-20: 433 MHz marking (Robotistan module; antenna included; 1.8-3.6 V supply). Library SELECTED 2026-10-20 and VALIDATED on the bench 2026-11-06: RadioLib 7.8.1 — see docs/hardware.md.
- Implemented radio settings (2026-11-06): 434.0 MHz, 4.8 kbps (RadioLib default, D17), 25 kHz deviation (D15), 0 dBm, sync word 0x12AD (demo only). Still to document: channel choice, e.r.p. incl. antenna gain, occupied-bandwidth mask, regulatory verification.

Sensors:
- MQ-2 gas/smoke sensor modules, listed quantity: 3.
- DHT22 temperature/humidity sensors, listed quantity: 2.
- Flame-sensor module(s) (IR-based flame detector with digital output), quantity and exact part not recorded. Confirmed part of the active sensor scope on 2026-10-20.
- LDR (light-dependent resistor): RETIRED from the sensor scope on 2026-10-20 (decision D1). Do not reintroduce it without an approved decision.
- MQ-2 modules appear to be the common small PCB type with analog and digital outputs / comparator trimmer; verify the exact board and pin labels before connecting it.

Power / solar parts:
- 3 × 3.7 V / 3 A BMS boards are in the parts list. Exact cell count, protection topology, and board specifications must be verified; the phrase “3.7 V” suggests single-cell lithium-ion use but is not enough to prove compatibility.
- 2 × BH-18650 cell holders are in the parts list.
- CN3791 solar MPPT/charging modules were considered for charging lithium-ion cells from solar panels.
- LM2596 buck converter modules were considered for power conversion.
- A MOSFET-controlled power rail was explored as a way to switch the MQ-2 heater/sensor off between readings to reduce average power consumption.
- Exact solar panel(s), output voltage/current rating, charge-controller board configuration, battery cell type/capacity, converter variants, and wiring are not yet fixed.

Construction:
- 4 × perfboard, approximately 5 cm × 10 cm, are in the parts list.
- Jumper wire, connectors, fuses, enclosure, weatherproof cable glands, mounting, antenna mounting, and other consumables are not fully specified.

Gateway concept:
- A central gateway with a screen/display, siren/buzzer, and a battery backup/UPS was discussed.
- Exact gateway MCU, display type, alarm part, power source, and interface are not finalized. Do not assume it uses the same board as the nodes unless confirmed.

Resistors reported as available:
- 15 kΩ, 22 kΩ, 27 kΩ, 30 kΩ, and 8.2 kΩ.
- These were considered for voltage dividers/analog interfaces. Do not pick a divider solely from this list until the source output voltage, destination pin limits, and the exact NodeMCU A0 circuit are verified.

Other relevant quantities/notes:
- Three MQ-2 modules and three BMS boards were listed; two DHT22 sensors and two 18650 holders were listed. This does not necessarily mean there will be exactly three fully equipped nodes. Final node count and per-node allocation remain undecided.

3. KNOWN DESIGN CHANGES AND DECISIONS SO FAR
-------------------------------------------
These are the important decisions and changes mentioned in project discussions. Treat them as context, and keep them updated if the design changes.

- Firmware will be written in Arduino C++ using the Arduino Core for ESP8266, not Python/MicroPython.
- Main MCU selected / currently intended: ESP8266 NodeMCU V3 ESP-12E 32-pin board.
- A CC1101 radio is part of the planned communications system. The GDO2 pin was discussed, but the final mapping is not yet confirmed.
- MQ-2 modules are expected to use a 5 V supply because the sensor heater requires the appropriate module supply. Power draw is a concern; approximately 900 mA was raised as a worst-case design concern in earlier discussion, but this is NOT verified as the measured current for the actual module. Measure actual current and consult the exact module documentation before sizing the regulator, switch, battery, and solar panel.
- The original thought of connecting MQ-2 analog output to the ESP8266 ADC was changed: the intended approach is to use the MQ-2 module's digital comparator output for an alarm indication. An earlier plan to reserve A0 for an LDR was retired on 2026-10-20 when the flame sensor replaced the LDR in the active sensor scope; any future use of A0 (for example battery-voltage measurement) requires a new decision. The exact implementation and calibration still need testing.
- MQ-2 digital output may be pulled up to the module's 5 V rail depending on the specific breakout design. It must not be connected directly to an ESP8266 GPIO unless the output voltage/electrical topology is verified safe. Use a suitable level-shift/divider/open-collector interface as required by the actual module.
- A0 usage note (LDR retired 2026-10-20): if A0 is ever used (for example for battery-voltage measurement, which is not yet decided), verify whether the particular NodeMCU clone has an onboard A0 divider and its allowable external A0 voltage; the bare ESP8266 ADC input is not the same as every development board's A0 pin range.
- Reducing MQ-2 energy use using a MOSFET-controlled power rail was explored. However, switching an MQ-2 on and off changes its warm-up/baseline behavior. A reading immediately after power-up must not be treated as a stable measurement. Determine the warm-up time and duty cycle experimentally, and document that the sensor is not continuously monitoring while switched off.
- CN3791 solar charging and a lithium-ion battery were considered. Exact controller module wiring/configuration must be verified against the actual module, panel, and cell chemistry. Do not assume a generic CN3791 board is automatically correct for the battery.
- A compact mesh protocol, hashed broadcasts, and an idea called “MULTICAST WHISPER” were discussed as concepts early on. “Hashed broadcasts” and “MULTICAST WHISPER” were retired on 2026-10-20 (they were an early HTTP-method-like sketch, superseded by the approved protocol design in docs/protocol-spec.md). Do not implement behavior based on those names.

4. ELECTRICAL AND HARDWARE SAFETY REQUIREMENTS
----------------------------------------------
These requirements are important because the ESP8266 uses 3.3 V logic and the project combines radio modules, 5 V sensor boards, lithium-ion batteries, and solar charging.

ESP8266 / logic:
- ESP8266 GPIO is 3.3 V logic and is not 5 V tolerant. Never drive a GPIO with a 5 V signal.
- Do not power CC1101 logic from 5 V unless the exact breakout documentation explicitly supports it; most CC1101 RF modules operate from 3.3 V. Verify the module and supply stability.
- Do not assume every NodeMCU clone has the same onboard regulator, A0 divider, USB-to-serial circuit, pin labels, or current capability.
- Verify the board's exact pinout before coding or wiring. Record both the physical silkscreen name and the ESP8266 GPIO number.
- Check the ESP8266 boot-strap requirements before assigning pins. Avoid pin choices that prevent normal boot or cause incorrect startup levels.
- Keep the radio wiring short and stable; check SPI pin mapping, common ground, power decoupling, and any level issues.

MQ-2:
- Provide a supply capable of meeting the actual module's heater current and startup demand.
- Measure current for the specific module rather than relying on a generic estimate. Regulator and wiring must be able to handle steady and transient load.
- Protect ESP8266 input pins from any 5 V digital or analog output.
- MQ-2 readings are affected by warm-up, aging, humidity, temperature, and other gases. A comparator threshold is not a validated forest-fire alarm threshold. Calibrate and test with controlled, safe methods.
- MQ-2 modules can become hot and are not low-power sensors by default. Do not place them in a sealed enclosure without considering heat and ventilation.
- Do not perform hazardous burn or smoke tests indoors. Design bench tests safely and follow local fire-safety requirements.

Battery / charging:
- Confirm battery chemistry, cell count, BMS rating, charging voltage/current, overcharge/over-discharge protection, and controller board settings before connecting a cell or panel.
- A BMS is not automatically a solar charge controller. Use a charger designed for the chosen cell chemistry and configuration.
- Do not charge damaged, mismatched, unprotected, or unknown cells. Use appropriate fusing and insulated holders/connections; prevent shorts.
- Confirm the CN3791 module supports the actual solar panel voltage and selected battery charging profile. Check the specific board's resistor configuration and datasheet.
- Check regulator dissipation and heat, especially if dropping a high solar/battery voltage to a 5 V MQ-2 supply.
- Calculate energy budget using measured load current and duty cycle, including MQ-2 heater, ESP8266 Wi-Fi (if ever used), CC1101 TX/RX, display/alarm load at gateway, converter losses, and seasonal solar limitations.
- If MOSFET-switched sensors are used, choose a switch and topology rated for the current and voltage; define safe default state during boot and reset.

RF / legal:
- Confirm the CC1101 module's actual frequency band and the local frequency/power/duty-cycle rules for Türkiye before deployment. Antenna and RF settings must match the module and local requirements.
- Keep antennas clear of large metal objects and characterize actual range in field conditions. Do not advertise a range without testing.

Environmental / enclosure:
- A forest deployment requires protection from rain, condensation, dust, insects, UV, temperature extremes, and cable strain. The enclosure must not make the MQ-2 readings meaningless or trap unsafe heat.
- A real deployment needs mounting, labeling, maintenance access, battery replacement/inspection, and a plan for false positives and radio outages.

5. SENSOR STRATEGY
------------------
Current sensor intentions:

A. MQ-2 gas/smoke sensor module
- Role: one of the local indicators for smoke/combustible-gas-related conditions.
- Planned signal path: use the module's digital comparator output for the ESP8266 alarm input rather than using its analog output for A0.
- Module supply: intended 5 V; verify exact board requirements.
- Unresolved: exact module, D0 output circuitry and voltage, comparator calibration, required warm-up, sampling schedule, active polarity, pin assignment, and how the firmware distinguishes a real event from noise.
- Do not present a digital threshold as a calibrated gas concentration. D0 typically reflects an adjustable comparator, not a reliable ppm measurement.
- If power-gated, represent sensor state and readiness explicitly in firmware. Ignore readings until the chosen warm-up period has elapsed, and document that a duty-cycled MQ-2 may miss events during its off period.

B. DHT22 temperature/humidity sensor
- Role: ambient temperature and relative humidity context, which may help interpret other readings and trigger a broader alert policy.
- Quantity on parts list: 2.
- Unresolved: which nodes will receive one, GPIO assignment, whether bare sensor or breakout is used, pull-up resistor requirements, sampling cadence, invalid-reading handling, and any alert thresholds.
- Read errors, impossible readings, and stale data should be detected. A missing/stale reading must not silently become zero or a valid measurement.

C. Flame-sensor module (IR-based flame detector)
- Role: direct indication of an open flame in the sensor's field of view. It is not by itself a reliable fire detector (range, angle, and false-trigger behavior must be characterized).
- Intended use: digital output to an ESP8266 GPIO with verified voltage levels; output polarity and noise behavior to be confirmed for the exact module.
- Unresolved: exact module and its output circuitry/voltage, quantity available, GPIO assignment, sensitivity and range under field conditions, and how firmware distinguishes a real flame from sunlight or other IR sources.
- Treat the digital output as a boolean indicator only; do not interpret it as a calibrated intensity. Apply debouncing and persistence in firmware before raising an ALERT.

(An earlier plan to use an LDR on A0 as a light-level context sensor was retired on 2026-10-20; decision D1.)

Sensor fusion / alarm policy:
- Do not finalize a single hard-coded threshold from guesswork.
- Start by logging raw sensor state and timestamps on the bench, then collect baseline data under expected conditions.
- The policy could combine multiple indicators, persistence over multiple samples, and/or messages from neighboring nodes, but the exact policy is undecided.
- Add hysteresis, debounce, repeated-sample confirmation, and an explicit clear/recovery policy where appropriate so alerts do not rapidly flap.
- Record confidence/status and data age. Distinguish “sensor reading indicates possible event,” “node health fault,” and “radio link problem.”
- False positives and false negatives must be assessed in safe, controlled testing. Do not claim validated early detection without evidence.

6. POWER SYSTEM AND LOW-POWER DESIGN
------------------------------------
Intended concept:
Solar panel -> suitable solar charge controller (CN3791 board was considered) -> protected lithium-ion cell/battery -> regulated rails for ESP8266 / CC1101 and the MQ-2 module. Exact wiring remains unapproved until the parts are identified.

Potential rails:
- 3.3 V: ESP8266 and CC1101, via a regulator with sufficient peak current, low noise, and appropriate decoupling.
- 5 V: MQ-2 module, if its exact board requires 5 V. A converter must handle heater current and transient demand.
- The DHT22 and the flame sensor should use a compatible supply/reference arrangement based on the exact module wiring. Avoid feeding 5 V outputs into MCU pins. With the LDR retired, A0 has no assigned purpose; any future use (for example battery-voltage measurement) requires an approved decision.

Important constraints:
- The ESP8266's Wi-Fi is not a requirement for the planned CC1101 inter-node radio link. Keep Wi-Fi disabled in production node firmware unless a specific need is approved, because it changes the power budget.
- ESP8266 sleep modes and wake behavior need to be compatible with radio reception. A node that sleeps cannot receive continuous messages during deep sleep; the mesh strategy must account for listening windows and missed packets.
- MQ-2 heater load may dominate energy consumption. Measure average and peak current for every operating mode before designing the panel/battery.
- If the MQ-2 is kept continuously heated, calculate runtime/solar generation based on its measured load. If duty-cycled, explicitly account for warm-up and missed-detection windows.
- Do not pick solar-panel or battery capacity until an energy budget table exists.
- Add brownout handling: avoid radio transmission or sensor assumptions when the supply is below reliable operating voltage; report low-battery status if measured safely.
- Determine whether the node must support one 18650 cell or a different pack; listed holders suggest 18650 cells but count per holder and node assignment are not yet confirmed.

Power-budget table to complete later:
- ESP8266: sleep current, average active current, peak current.
- CC1101: RX, TX at selected output power, sleep current.
- MQ-2: measured startup and steady-state current at selected voltage; on/off duty cycle and warm-up.
- DHT22 and flame sensor: supply/current contribution.
- Converter efficiency under actual loads.
- Gateway screen and siren: separate budget from small field nodes.
- Solar harvest assumptions, charge-controller losses, usable battery capacity, temperature/aging derating, and required autonomy during cloudy days.

7. RADIO, PACKET FORMAT, AND MESH NETWORK GOALS
------------------------------------------------
Radio module: CC1101. A suitable Arduino library and exact radio configuration must be chosen and documented before implementing the network.

Still to decide and record:
- Exact frequency band/channel and local regulatory compliance (band 433 MHz confirmed, D10; channel choice, e.r.p. incl. antenna gain, and the occupied-bandwidth mask still open).
- Modulation, data rate, frequency deviation, receiver bandwidth, sync word, whitening/CRC settings, transmit power, preamble, and antenna (implemented 2026-11-06: 434.0 MHz, 4.8 kbps, 25 kHz deviation, 135 kHz RX BW, 0 dBm, sync 0x12AD demo-only — firmware/lib/yanki-protocol/docs/protocol-spec.md §14 and firmware/lib/MeshForestBoard/src/MeshForestBoard.h; 38.4 kBaud deferred to v1.1 per D17).
- Library name/version and its framing behavior.
- Gateway identity hardware (board/display/siren) and any pairing/enrollment method.

Decided on 2026-10-20 and specified in docs/protocol-spec.md (that document is now the authoritative protocol reference):
- Packet format, versioning, and the packet-size budget (60-byte total packet, 48-byte payload ceiling).
- Managed-flood forwarding, duplicate suppression, TTL/hop limits, and randomized backoff.
- Acknowledgement and retry behavior (end-to-end gateway ACK for alerts; bounded retries).
- Node ID allocation (build-time table) and network identification.
- Listening model (continuous RX; every node is sensor and relay; no deep sleep in v1).
- Message authentication (AES-CMAC with a pre-shared network key) and windowed replay protection.

Application requirements:
- Target message size: ~50–60 bytes **total packet** (60-byte cap, 48-byte payload ceiling; docs/protocol-spec.md §4.2).
- Include protocol version, message type, source node ID, message ID/sequence number, hop count or TTL, and only the sensor/status data required for that message.
- Keep encoding deterministic and compact. Avoid large JSON strings for constrained radio packets. A compact binary structure or a small, clearly specified text encoding may be used after comparing robustness and ease of debugging.
- Specify byte order, integer widths, units, missing-value markers, and version compatibility. Do not serialize raw C/C++ structs directly over the air unless padding/alignment and compatibility are explicitly controlled.
- Validate every received packet's length, version, fields, ranges, and checksum/CRC result before using it.
- Handle duplicate messages using a bounded cache keyed by source and sequence/message ID; expire old entries safely.
- Use a hop limit / TTL to prevent infinite forwarding loops.
- Add randomized backoff or another collision-control method before relaying to reduce simultaneous transmissions.
- Do not forward every packet indefinitely. Define whether a node forwards only alerts, all measurements, health/status, or selected message types.
- Keep alert forwarding fast but bounded. Avoid retry storms during a network partition.
- Track message age / originating timestamp or a safe substitute. Do not assume clocks are synchronized unless a synchronization scheme exists.
- Distinguish the original source node from the current forwarder.
- Consider per-neighbor/link quality statistics only if memory and payload costs are justified.

Possible message categories to define:
- SENSOR_REPORT: compact sensor readings or status.
- ALERT: possible fire-related condition with cause/level and source identity.
- NODE_STATUS: battery/health/sensor availability.
- ACK: optional delivery acknowledgement, if enabled.
- CLEAR / ALERT_RESOLVED: only after a reliable clearing policy is defined.
- CONFIG: optional remotely requested configuration; must be authenticated if implemented.

“Hashed broadcasts” and “MULTICAST WHISPER” were retired on 2026-10-20 and must not be implemented. Message authentication is decided: AES-CMAC (RFC 4493) with a pre-shared network key, truncated to a 32-bit MIC, plus windowed per-source counters, specified in docs/protocol-spec.md. A hash or CRC alone still never makes a packet authentic or private; key provisioning and its limitations are documented there.

8. GATEWAY BEHAVIOR
-------------------
Intended gateway features:
- Receive reports/alerts from nodes and, if the mesh protocol supports it, relay or coordinate messages.
- Show current alarm/status on a display.
- Trigger a siren/buzzer for a qualifying alert.
- Have a backup/UPS power arrangement so the gateway does not immediately fail with a mains interruption.
- Show node ID, alert cause, timestamp/age, radio/health status, and whether information is stale where display capacity allows.
- Distinguish confirmed packet receipt from inferred node health: silence may mean a dead node, poor radio link, sleeping node, or no event.
- Avoid an endless siren due to repeated copies of one packet; deduplicate alert IDs and define acknowledgement/muting behavior.

Not finalized:
- Gateway MCU / board.
- Display technology and interface.
- Siren voltage/current and driver circuit.
- UPS/battery details.
- Whether the gateway has Wi-Fi, logging, a local network dashboard, or only a display/siren.
- Whether it forwards alerts to a phone or another service.

Do not design gateway GPIO or power connections until the exact components are known.

9. FIRMWARE STRUCTURE RECOMMENDATION
------------------------------------
Suggested separation of responsibilities (adapt to repository size; do not create empty abstractions without a use case):

- board_config.h: confirmed pin mappings and board-dependent constants.
- project_config.h: radio settings, sample intervals, packet version, and documented configuration.
- sensor module(s): read sensor data, report validity/staleness, manage MQ-2 readiness/warm-up.
- power_manager module: determine sensor/radio operating modes and react to low voltage, if battery sensing is added.
- radio_driver wrapper: initialize CC1101, send/receive packets, expose errors and radio state.
- protocol module: encode/decode packets, validate length/version/fields, handle message identifiers.
- mesh module: duplicate suppression, hop limits, relay decisions, retry/backoff policy.
- alert_logic module: apply sensor thresholds/persistence/hysteresis after values are validated through testing.
- node_health module: uptime, resets, sensor faults, battery status when available.
- gateway UI/alarm module: only in the gateway firmware target if gateway hardware differs.

Use non-blocking timing (for example, millis-based state machines) where feasible. Avoid long delay() calls in the main loop if they would prevent radio reception, message forwarding, or watchdog servicing. Sensor warm-up delays should be represented as state/timing rather than blind waits whenever the rest of the node must continue running.

Good error-handling expectations:
- Radio init failure should be visible in logs/status and should not masquerade as a working node.
- Sensor read failure should be represented as invalid data, not zero.
- Packet parsing must never read beyond packet bounds.
- Counters should avoid dangerous overflow assumptions; sequence-number wraparound needs defined comparison/cache behavior.
- Reset reason and uptime are useful for debugging where practical.
- Debug logging must be optional/controllable so it does not consume power or block time-sensitive handling.

10. DEVELOPMENT ENVIRONMENT AND WORKFLOW
----------------------------------------
- Firmware language: Arduino C++.
- MCU framework: Arduino Core for ESP8266.
- Target hardware: NodeMCU V3 ESP8266 ESP-12E, 32-pin board; exact board mapping to confirm.
- Radio: CC1101.
- IDE/editor context: VS Code is an intended development environment; GitHub Copilot may be used to inspect/edit project files.
- Build environment (recorded 2026-10-24): arduino-cli 1.5.1; ESP8266 core 3.1.2; FQBN `esp8266:esp8266:nodemcuv2`.
- Libraries (pinned 2026-10-24): RadioLib 7.8.1, DHT sensor library 1.4.7, Adafruit Unified Sensor 1.1.15, Adafruit GFX + SSD1306.

Recommended workflow:
1. Inventory current project files and record the existing compile status.
2. Confirm exact board revision and pin map; create a pin map document.
3. Compile an empty/minimal ESP8266 Arduino sketch.
4. Test CC1101 initialization and one-way packet send/receive on a bench before mesh behavior.
5. Test each sensor independently and record raw values and electrical levels.
6. Test level shifting for MQ-2 D0 and the flame-sensor output before attaching those signals to the MCU.
7. Build a packet encoder/decoder with host-side test vectors if practical, then compile it for the MCU.
8. Test two nodes at short range; then add message IDs, deduplication and bounded forwarding.
9. Test multiple nodes, packet loss, collisions, duplicated messages, radio loss, reboot, and low battery.
10. Only after behavior is stable, add sleep/wake scheduling and optimize power.
11. Keep a bench-test log with date, board/module revision, library version, power supply, and results.
12. Use Git branches/commits or otherwise keep easy rollback points. Never overwrite known-good firmware without preserving it.

11. TEST PLAN / ACCEPTANCE CHECKS
---------------------------------
Hardware tests:
- Confirm the MCU boots reliably with the chosen pin assignments and attached CC1101.
- Confirm all rails remain within component limits during MCU boot, radio TX, and MQ-2 warm-up.
- Measure the MQ-2 module current instead of estimating it.
- Verify the MQ-2 D0 high/low voltage and electrical type before connecting it to GPIO.
- Verify the flame-sensor module's digital output voltage and polarity before connecting it to a GPIO; ensure levels stay within the exact board's limits.
- Confirm CC1101 supply stability and successful initialization.
- Confirm that sensor/power switching does not reset the ESP8266 or corrupt the radio.

Firmware / protocol tests:
- Unit-test packet encoding and decoding against known byte examples.
- Reject zero-length, truncated, oversized, wrong-version, malformed, and invalid-field packets.
- Ensure duplicates are not forwarded repeatedly.
- Ensure hop limits terminate loops.
- Confirm multiple nodes sending together do not produce a runaway retransmission storm.
- Confirm one bad sensor value does not create an unbounded alert.
- Test radio range in representative conditions; measure packet-delivery rate and latency rather than just whether a packet sometimes arrives.
- Test alert persistence, hysteresis, and clear behavior after thresholds are chosen from test data.
- Test sensor faults, disconnected sensors, battery sag, brownouts, and node reboots.

Power / field-readiness tests:
- Measure average and peak current for each firmware state.
- Test the selected battery/charger/regulator on the bench before solar use.
- Measure runtime and charging behavior, including realistic periods with low sunlight.
- Assess temperature, rain/condensation exposure, enclosure effects, antenna placement, and mounting stability.
- Record false-positive/false-negative observations under safe test conditions.

An acceptance result must be based on actual measurements/logs. Do not claim a test passed merely because the code compiles.

12. UNRESOLVED QUESTIONS / DECISION REGISTER
--------------------------------------------
The items below are open and should be resolved and added to this document as evidence becomes available.

Hardware identity and connections:
[RESOLVED 2026-10-24 — D14] Board revision and pin-to-GPIO map: this clone's D-labels are raw GPIO numbers (D2/D4/D5/D12/D13/D14 probe-confirmed); see docs/hardware.md §4.1.
[RESOLVED 2026-10-24] GPIO assignments: CC1101 SCK/MOSI/MISO/CSN = D14/D13/D12/D15, GDO0 = D4; DHT22 = D0 (GPIO0); MQ-2 D0 = D5 (GPIO5, via divider); flame sensor = D16 (GPIO16). No power-control signal assigned yet.
[RESOLVED 2026-11-06] GDO2 is not used; RadioLib needs only CS + GDO0 in this configuration.
[RESOLVED 2026-10-20 — D10] CC1101 module: Robotistan 433 MHz, 1.8-3.6 V, antenna included (vendor page + owner-confirmed marking).
[OPEN] Exact MQ-2 PCB/module and its D0 electrical topology/output voltage.
[OPEN] Exact NodeMCU A0 allowed voltage / built-in divider ratio.
[OPEN] Flame-sensor module model, quantity, and output polarity/voltage (pin assigned; polarity UNVERIFIED).
[OPEN] DHT22 bare sensor versus breakout module (an 8.2 kΩ pull-up is fitted and is boot-critical on GPIO0).
[OPEN] Final number of nodes and which components are fitted to each node.

Power:
[OPEN] Exact 18650 cell make/model, capacity, chemistry, count per node, and condition.
[OPEN] Exact BMS board specifications and wiring.
[OPEN] Exact CN3791 module, its configured battery charging profile, solar input range, and charge current.
[OPEN] Solar panel voltage/current/wattage and enclosure placement.
[OPEN] 3.3 V and 5 V regulator part numbers and verified current/thermal capability.
[OPEN] Whether the MQ-2 is continuously powered or duty-cycled; if duty-cycled, validated warm-up and sampling interval.
[OPEN] Battery voltage measurement method and low-battery threshold if battery monitoring is implemented.

Radio/protocol:
[RESOLVED 2026-11-06 — D12/D14] CC1101 library: RadioLib **7.8.1, pinned and bench-validated**.
[DECIDED 2026-10-20 — D10] Band = 433 MHz under general-purpose SRD rules (10 mW e.r.p., ≤%10 duty cycle); alarm-specific bands deliberately not used. Channel choice, e.r.p. verification incl. antenna gain, and the occupied-bandwidth mask check remain open.
[DECIDED 2026-10-20 — see docs/protocol-spec.md] Packet representation and the packet-size budget (60-byte total packet, 48-byte payload ceiling).
[DECIDED 2026-10-20 — see docs/protocol-spec.md] Node ID allocation, sequence/message IDs, hop limit, duplicate-cache size/expiry, retry limits, random backoff, ACK policy, and node listening schedule.
[DECIDED 2026-10-20] “Hashed broadcasts” and “MULTICAST WHISPER” retired (decision D9).
[DECIDED 2026-10-20 — see docs/protocol-spec.md] Cryptographic authentication/replay protection is in scope for v1 (AES-CMAC MIC + windowed per-source counters).

Detection / gateway:
[OPEN] Validated sensor sampling period and alarm thresholds.
[OPEN] How temperature/humidity/flame-sensor readings affect alert interpretation, if at all.
[PARTIALLY DECIDED 2026-10-20 — D11] Gateway = stripped-down node + OLED display + speaker module, mains + battery UPS at a lookout station; authorized personnel escalate alerts. Exact OLED/speaker parts, wiring, UPS details, and acknowledgement/mute behavior remain open.
[OPEN] Enclosure, antenna mounting, deployment spacing, maintenance, and test/field trial plan.

Software:
[RESOLVED 2026-11-22] Repository structure and compile status: all seven sketches compile for `esp8266:esp8266:nodemcuv2`; toolchain and library versions recorded in §10; the two-repository split is planned per decision D18.
[OPEN] Logging strategy and whether logs are retained at nodes or only at the gateway.
[OPEN] Whether any remote configuration, gateway connectivity, or mobile notification feature is required.

13. CURRENT PRIORITY ORDER
--------------------------
Priority 1 — identify the actual boards and modules. Make a verified wiring/pin map, especially for ESP8266 logic levels, MQ-2 D0, the flame-sensor output, and CC1101.
Priority 2 — establish a reproducible Arduino build for the selected ESP8266 board.
Priority 3 — make a two-board CC1101 bench test (send and receive a minimal message).
Priority 4 — prove one sensor path at a time with safe electrical interfacing and raw-data logging.
Priority 5 — specify and test packet encoding/decoding within the size budget.
Priority 6 — add bounded mesh forwarding, deduplication, hop limit, and collision handling.
Priority 7 — develop a defensible alert policy using measured data and controlled tests.
Priority 8 — measure power, decide continuously powered versus duty-cycled MQ-2 operation, and size the solar/battery hardware.
Priority 9 — build and test the gateway display/siren behavior.
Priority 10 — outdoor/field tests and documentation; do not describe it as reliable until evidence supports that claim.

14. SHORT PROJECT SUMMARY FOR FUTURE PROMPTS
--------------------------------------------
I am developing a low-cost, solar/battery-powered, mesh-connected forest-fire early-warning prototype. The intended field node uses an ESP8266 NodeMCU V3 ESP-12E 32-pin board, a CC1101 radio, an MQ-2 gas/smoke sensor module, DHT22 temperature/humidity sensing where available, and a flame-sensor module (the earlier LDR plan was retired on 2026-10-20). Firmware must use Arduino C++ and the Arduino Core for ESP8266. There are 3 MQ-2 modules, 3 nominal 3.7 V / 3 A BMS boards, 2 18650 holders, 2 DHT22 sensors, and 4 small 5 × 10 cm perfboards in the current parts list; actual ownership/stock and per-node allocation are not confirmed. Resistors on hand: 15 kΩ, 22 kΩ, 27 kΩ, 30 kΩ, and 8.2 kΩ. Solar charging with a CN3791 module, lithium-ion cells, and LM2596 converters has been considered. MQ-2 current is a major concern and power-gating it with a MOSFET was considered, but its warm-up and missed-detection tradeoffs must be tested. Current sensor plan is MQ-2 digital comparator output and flame-sensor digital output to GPIOs, with proper voltage interfacing; do not connect any possible 5 V output directly to ESP8266 pins. CC1101 pins and the packet format are decided and bench-verified (docs/hardware.md §4, docs/protocol-spec.md); power-chain details, sensor thresholds, node count, and the remaining gateway hardware remain open. The radio application payload target of about 50–60 bytes is implemented as a 60-byte total-packet cap (48-byte payload ceiling), specified in docs/protocol-spec.md. Mesh goals — compact packets, node/message IDs, duplicate suppression, TTL/hop limit, controlled forwarding, and authenticated alert delivery with gateway acknowledgement — are specified there. “Hashed broadcasts” and “MULTICAST WHISPER” were retired on 2026-10-20. The planned gateway should have a display, siren, and backup power. Treat this as an experimental prototype, not certified fire safety equipment. Inspect existing code first, make small changes, verify electrical assumptions, and compile/test after each step.

END OF WORKING SPECIFICATION
