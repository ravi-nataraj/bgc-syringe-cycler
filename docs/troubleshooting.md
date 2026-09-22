# Troubleshooting

## Before probing

Disconnect shop air first. The valve is spring return, so an
de-energised coil parks the cylinders retracted, but a board fault can
leave the coil energised with the cylinders extended.

Measure with 12 V connected and the board idle unless stated otherwise.

## Test points

Seven points are brought out. Values are nominal — record the readings
from a known-good board and compare against those.

| Point | Expected | Notes |
|---|---|---|
| `GND` | 0 V | Reference. Probe here first. |
| `12VIN` | 12.0 V | Supply as delivered, ahead of protection. |
| `12V` | ~11.7–12.0 V | After the IRF9540N and polyfuse. Small drop is normal. |
| `5V` | 5.0 V | OKI-78SR-5 output. Holds regardless of valve state. |
| `3V3` | 3.3 V | XIAO onboard regulator. |
| `GATE` | 0 V idle, 3.3 V energised | Driven by the ESP32 GPIO. |
| `DRAIN` | ~12 V idle, ~0 V energised | Inverts relative to `GATE`. |

`GATE` is what the firmware asked for. `DRAIN` is what the hardware did.
If `GATE` swings and `DRAIN` does not, the fault is the MOSFET or the
coil, not the firmware.

## Nothing powers on

No LED, no WiFi network.

`12VIN` at 0 V — fault is upstream. Check the supply, jack and cable.
The plug is 5.5 × 2.5 mm; a 2.1 mm plug fits loosely and makes
intermittent contact.

`12VIN` good, `12V` at 0 V — either reverse polarity (centre pin must be
positive) or the polyfuse has tripped. It resets on its own once cool,
so pull power for a minute and retest. Repeat trips mean a real
overcurrent — check for a shorted coil or a solder bridge.

`12V` good, `5V` at 0 V — regulator failed or shorted downstream.

`5V` good, `3V3` at 0 V — reseat the XIAO.

## Board runs, valve does not fire

Probe `GATE` through a cycle. If it swings 0 to 3.3 V on schedule, the
firmware is fine.

Then probe `DRAIN`:

- Stays near 12 V with `GATE` high — MOSFET is not switching. Open gate
  connection, damaged IRLZ44N, or open coil. Measure coil resistance
  with the connector off.
- Stays near 0 V regardless of `GATE` — MOSFET shorted drain-to-source.
  Replace it. The valve is permanently energised in this state.

## Valve fires, cylinders do not move

Valve clicks, no motion.

The valve needs ~44 psi to shift, so it can click without shifting at
low pressure. Confirm regulated supply is at 60 psi.

If pressure is right, check the meter-out flow controls on the exhaust
ports. Fully closed will stall the cylinders.

## Some stations weak or slow

All ten run off one valve through two manifolds, so a fault affecting
only some stations is downstream of the manifolds — kinked branch line,
loose fitting, or an unevenly set flow control.

Pull and reseat the fitting at both ends. Push-to-connect fittings that
are not fully home leak under load but hold at rest.

## Delivered volume drifting

Inspect the printed pusher and stationary mount at the affected station.

The cylinder stalls at full pressure against the printed stop every
extend stroke. PETG takes it, but creep or wear at the contact face
lengthens the stroke and changes delivered volume. Check the mount first,
heat-set inserts second.

## WiFi network not visible

The board is its own access point and does not join site WiFi.

If the network is absent, check `3V3` — the XIAO is unpowered or
unseated. If `3V3` is good, reflash. A failed upload can leave the board
powered but never reaching `WiFi.softAP()`.

## Joined but the page will not load

The address is always `192.168.4.1`.

Type `http://192.168.4.1` explicitly. Browsers assume HTTPS and will
fail rather than fall back — the board serves plain HTTP.

Phones may drop the network automatically and return to cellular. Tell
the phone to stay connected when prompted.

## Run stopped unexpectedly

If settings reverted to 60 / 10 / 20, the board lost power and
restarted. Settings are RAM only. Check the supply and the jack, not the
firmware.

If settings are intact, the run completed or was aborted. A one-second
button hold and the web Stop do the same thing.

## Serial

USB at 115200 baud. Prints each extend and retract with the cycle
number.

| Command | Effect |
|---|---|
| `E60` | Extend time, seconds |
| `R10` | Retract time, seconds |
| `C20` | Cycle count |
| `S` | Start |
| `X` | Stop and reset |
| `D` | Restore defaults |

Fastest way to confirm the state machine is running with the pneumatics
disconnected.
