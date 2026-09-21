# Design Notes

Why the fixture is built the way it is. The README covers what it does
and how to run it; this file covers the reasoning, so that anyone
changing something knows what constraint they might be breaking.

---

## Pneumatics

### Why a 5/2 spring-return valve

The cylinder is double-acting, so it needs air on both sides — a 5/2
valve is the standard part for that. Spring return means a single coil
and a single control signal, and the fail-safe state on power loss is
retracted.

A bistable double-solenoid valve was considered. It only needs a brief
pulse to shift, so total coil-on time across a run would drop from 20
minutes to about 2 seconds, and it holds position on power loss. At
2 W the thermal argument does not matter, and retracted is the safer
fail-safe for a syringe, so spring return won.

### Air path

Shop air → filter/regulator (60 psi) → valve inlet

Valve outlet A → manifold A → ten cylinder extend ports
Valve outlet B → manifold B → ten cylinder retract ports

Both valve exhausts → muffler flow controls (meter-out, sets stroke speed)

1/4" OD trunk from valve to manifolds, 1/8" OD branches to cylinders.
Cylinder ports are 10-32.

### Why 60 psi

The valve is pilot-operated — incoming air pressure shifts the spool
and the solenoid only gates the pilot. **Below 44 psi it will not shift
reliably.** That is a hard floor, not a guideline.

Force is not the constraint. At 60 psi the 7/16" bore produces about
40 N gross, roughly 24 N net after meter-out back pressure, against a
worst-case plunger requirement near 12.6 N. That is around 2× margin,
and more like 4–6× for a typical balloon inflation pressure.

60 psi sits comfortably above the shift threshold while keeping force
modest. The risk to watch is **transient droop**: ten cylinders crack
open simultaneously, the manifold refills, and line pressure dips. If
the gauge dives toward 44 psi during actuation, raise supply pressure
or add a small receiver near the manifold.

### Why a 7/16" bore cylinder

Sized on force, but the real reason it works is that **delivered volume
is set by stroke length, not force**. Surplus force does not push more
fluid; it only has to exceed the resistance.

For a 1 mL syringe, 0.6 mL is about 34 mm of plunger travel. The 2"
stroke covers that with room to spare, and using two-thirds of the
stroke means a ±1 mm stop error is under 3% of dose.

**The surplus force is the hazard.** If the line ever blocks, 40 N on a
17.5 mm² plunger generates roughly 335 psi of fluid pressure — enough
to burst a small balloon or pop a luer-slip joint. Use luer lock, and
rely on the mechanical stop so the fixture delivers a fixed volume
rather than pushing until something fails.

### Why flow control on the exhaust, not the inlet

Meter-out. Throttling the exhaust gives smooth, controlled motion;
throttling the inlet gives jerky, unpredictable strokes. The two
exhaust ports also need silencing — an unmuffled 1.3 Cv valve is loud —
so muffler-flow-control units do both jobs in one body.

Note that exhaust metering controls the **aggregate** of all ten
cylinders, not each one. They will not move in perfect synchrony; the
one with least seal friction moves first. This is acceptable for this
application. Per-cylinder trim would need flow controls at each
cylinder port instead.

### Why tubing size barely matters

The cylinder's own 10-32 ports are the tightest restriction in the
system and cannot be opened up, so upsizing tubing downstream buys
nothing.

The numbers: each cylinder sweeps 0.30 in³, about 0.05 scfm if the
stroke takes a second. Ten cylinders is roughly 0.27 scfm against a
valve rated 43.1 scfm — around 160× more valve than needed.

1/4" OD on the trunk from valve to manifold, 1/8" OD on the branches,
is correct and has margin.

---

## Electronics

### Why low-side switching

The MOSFET sits between the coil and ground. Source is grounded, drain
faces the load. This is the simplest arrangement for a microcontroller
to drive, because the gate voltage is referenced to ground, which the
MCU shares.

**Source must be grounded.** Gate drive is measured gate-to-source, so
if the source floats the gate voltage means nothing. Wiring drain and
source backwards also lets the MOSFET's internal body diode conduct,
which energises the valve permanently and leaves the gate with no
authority at all.

### Why the two gate resistors are different things

**R1, 220 Ω in series.** The gate is a capacitor. At the instant the
pin goes high the gate looks like a short to ground, and without series
resistance the GPIO takes the full inrush. 3.3 V / 220 Ω = 15 mA peak,
comfortably inside spec. The cost is a switching time of a couple of
microseconds, which against a 60 second hold is nothing.

**R2, 10 kΩ pulldown.** During boot the GPIO is a floating input for a
few hundred milliseconds before firmware runs. A floating gate with no
leakage path accumulates stray charge, and if it drifts past the
threshold the valve fires. **On every power-up and every reset —
including each time new firmware is uploaded.** The pulldown gives that
charge somewhere to go.

10 kΩ is the value because it forms a divider with R1 in steady state.
1 kΩ would cost 18% of the gate drive; 10 kΩ costs 2% and still bleeds
charge fast enough.

### Why the flyback diode

The coil stores energy in a magnetic field. Switching it off abruptly
produces V = -L(di/dt), which without a path can reach several hundred
volts across a MOSFET rated for 55 V.

The diode sits **in parallel with the coil**, band toward +12 V. In
normal operation it is reverse-biased and does nothing. At switch-off
it forward-biases and the current circulates in a closed loop through
coil and diode until it decays, clamping the spike to about 12.7 V.

Installed backwards it is a direct short across the supply.

Fitting it at the coil terminals rather than on the PCB catches the
spike at its source. The board has a footprint for it either way.

### Why the P-FET is wired backwards

Reverse-polarity protection. Barrel jacks are physically identical
whether centre-positive or centre-negative, so plugging in the wrong
adapter is easy and would destroy the electrolytic and the regulator.

A series diode would work but costs 0.4–0.7 V continuously. A MOSFET
does it in a few milliohms.

**The wiring is deliberately reversed:**

| Pin | Connects to |
|---|---|
| Drain | input side, from the polyfuse |
| Source | output side, the +12 V rail |
| Gate | ground, through 10 kΩ |

Correct polarity: the body diode conducts first, lifting the source to
about 11.4 V. Gate sits at 0 V, so Vgs ≈ −11.4 V, which is solidly on
for a P-channel. The channel then shorts out its own body diode.

Reverse polarity: the body diode faces the wrong way to conduct, and
the gate ends up positive relative to source, which is off. Two
independent mechanisms both block.

**If wired the intuitive way** — source to input, drain to output — the
body diode would conduct freely on reverse polarity and the protection
would do nothing, while looking perfectly correct on the schematic.
This is the connection most likely to be "corrected" by someone who
does not know why it is like that.

### Why 0.9 A on the polyfuse

The board draws about 320 mA: 167 mA for the coil plus roughly 150 mA
at 12 V to make the XIAO's 5 V.

Sizing rule is 2–3× normal draw. 0.9 A hold gives 2.8× headroom — no
nuisance tripping — and trips at 1.8 A, a fault current the board
survives briefly.

A 1.85 A part was ordered first. It holds until 3.7 A, which is 11×
normal draw and only protects against a hard short. The 0.9 A part is
also physically smaller and uses the same 5.08 mm radial footprint.

Derating matters: at 40 °C the hold current drops to 0.75 A, at 60 °C
to 0.61 A. Still well above 320 mA.

### Why a regulator module and not a designed-in one

The OKI-78SR-5 is a switching regulator in a **three-terminal TO-220
package pin-compatible with a 7805**. It takes 7–36 V in, gives fixed
5 V out at up to 1.5 A, needs no external components, and runs at about
90% efficiency.

That combination removes three problems at once:

- **No switching layout to get wrong.** Inductor placement and feedback
  routing are already solved inside the module.
- **No trimpot.** An adjustable buck module ships at an arbitrary
  voltage, and connecting the XIAO before setting it is the single
  easiest way to destroy the board.
- **No custom footprint.** TO-220 is already in the KiCad library and
  already used for both MOSFETs.

**A linear 7805 is not an acceptable substitute.** Dropping 7 V at the
XIAO's 250–500 mA WiFi transmit peaks means 2–3.5 W in a TO-220, which
hits thermal shutdown. The symptom looks exactly like a random software
reset, which is miserable to diagnose.

### Why the XIAO is socketed

The USB port stays accessible for programming, a damaged module is a
$5 swap rather than desoldering 14 castellated pads, and the module can
move between boards.

Machined round-pin sockets rather than stamped — better contact, and
they survive repeated insertion.

**Assembly note:** plug the two 7-pin sockets onto the XIAO's own pins
first, then drop the whole assembly into the board and solder. That
guarantees the spacing matches the module even if the footprint is
slightly off.

### Why the 100 µF is where it is

Bulk decoupling on the 12 V rail. When the MOSFET turns on, the coil
demands 167 mA instantly, and that current has to travel through cable,
a barrel jack, and PCB traces — all of which have inductance. The rail
sags briefly.

The capacitor supplies that first gulp locally. Without it, a deep
enough dip trips the regulator's undervoltage lockout and brownouts the
XIAO mid-run, which presents as a random crash.

Electrolytic is correct here. Nothing measures its value and its
leakage is irrelevant — the job is cheap farads close to the load.

---

## Firmware

### Why millis() and not delay()

`delay(60000)` would freeze the processor for a minute, during which
the button is unreadable. The abort gesture could not exist.

The loop runs thousands of times per second doing three cheap things:
read the button, check whether the current state's timer has elapsed,
update the LED. Nothing blocks, so the button stays responsive through
the entire run.

The `now - stateEnteredAt` pattern is also overflow-safe. `millis()`
wraps after 49 days and unsigned subtraction still gives the correct
elapsed time across the wrap.

### Why the long press fires on threshold, not release

The abort fires the instant the hold passes one second, rather than
waiting for the button to be let go. The operator gets immediate
feedback and the valve drops the moment they have held long enough.

A flag then suppresses the short press that the release would otherwise
generate.

### Why a short press does nothing mid-run

Two different stop gestures would mean a knocked bench could end a
23-minute test at cycle 17. Stopping requires the deliberate hold.

### Why settings are not saved to flash

An earlier version used the Preferences library, so settings survived a
power cycle. That was changed deliberately.

On shared equipment, a stale setting is worse than an inconvenient one.
If someone sets 5 s / 5 s / 3 cycles for a quick check and walks away,
the next person pressing the button gets 5/5/3 without knowing. Holding
settings in RAM means **power-off always returns the fixture to
60 s / 10 s / 20 cycles**, so the button is predictable for anyone who
did not configure it.

The trade is that a non-default run has to be configured each time the
fixture is powered up. If one non-default configuration becomes the
norm, change `DEF_EXTEND` and friends at the top of the sketch and
re-flash.

### Why an access point and not a WiFi client

The board broadcasts its own network rather than joining an existing
one. No IT involvement, no credentials, no dependence on site coverage,
and the address is always **192.168.4.1** rather than whatever a router
assigns.

### Why the safe state is set first in setup()

`digitalWrite(PIN_VALVE, LOW)` runs before serial, before the button,
before anything. If the board resets mid-run the coil de-energises in
the first microseconds rather than waiting for the rest of
initialisation.

---

## Things deliberately not done

**Independent station control.** Ten cylinders run in parallel off one
valve because the test does not require staggering them. Adding it
would mean ten valves, ten MOSFETs and ten channels — the electronics
scale linearly, but nothing in the protocol asks for it.

**Closed-loop position sensing.** Reed switches on the cylinder barrels
would confirm the piston actually reached each end rather than assuming
it after a timer, and would catch a stuck cylinder during an unattended
run. The spare GPIO exists for this. It was not needed for the initial
protocol.

**Pressure logging.** A transducer on the manifold would record what
supply pressure actually does during a ten-cylinder actuation, which is
currently verified by watching a gauge.

**A discrete 555 version.** Fully designed — CMOS 555 astable at
60 s / 10 s, CD4026 counters driving seven-segment displays, all of it
running directly from 12 V with no regulator. Set aside because the
microcontroller version was already working and is far easier to
reconfigure.
