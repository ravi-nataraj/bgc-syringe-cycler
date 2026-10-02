# Mechanical

The fixture cycles ten 1 mL syringes in parallel for balloon guide catheter inflation fatigue testing. Each air cylinder drives a printed pusher against a syringe plunger. The barrel is held in a fixed printed holder. All ten cylinders are plumbed through two manifolds to one 5/2 solenoid valve, so every station actuates together.

The source drawing is `bgc-inflation-fatigue-assy.pdf`.
## Bill of Materials

| Item | Part Number | Description | Qty |
|---|---|---|---|
| 1 | 6498K604 | Round body air cylinder mount **[verify]** | 10 |
| 2 | 4952K126 | Foot bracket, 7/16" | 20 |
| 3 | 1ml-syringe-push | Syringe pusher (printed) | 10 |
| 4 | stationary-mount | Syringe holder (printed) | 10 |
| 5 | 5779K246 | Push-to-connect fitting | 20 |
| 6 | 92220A162 | Low-profile SHCS | 60 |
| 7 | baseplate | Baseplate (MIC-6 aluminum, machined) | 1 |
| 8 | 5469K191 | Right-angle flow manifold | 2 |
| 9 | 6124K277 | 5/2 solenoid valve | 1 |
| 10 | 7397N268 | Push-to-connect fitting | 4 |
| 11 | 53535A11 | Rubber foot | 4 |
| 12 | 7397N239 | Push-to-connect fitting | 20 |
| 13 | 90107A010 | 316 SS washer | 7 |
| 14 | 92220A157 | Low-profile SHCS | 7 |
| 15 | pcb-mount_v1 | PCB mount on solenoid (printed) | 1 |
| — | 9164K12 | Exhaust flow control, slotted screw, 1/4 NPT (not on drawing) | 2 |

Part numbers are from McMaster-Carr unless noted.
## Custom Parts

- **Baseplate:** MIC-6 aluminum, 13.976 × 11.811 in.
  - Station pitch is 1.384 ±.001.
  - 40X 10-32 holes for the cylinder brackets, 20X 10-32 for the syringe holders, and 11X 8-32 for the manifolds and valve **[verify]**.
- **Syringe holder:** FDM (PLA, ABS, or PETG), ±.020.
  - Ø5.0 mm barrel slot with a Ø9.8 mm flange pocket.
  - 2X counterbored holes for 10-32.
- **Syringe pusher:** FDM, ±.020, bore axis vertical, 4 perimeters minimum.
  - A 0.118 in slot captures the plunger flange.
  - 8-32 heat-set insert for the cylinder rod.
- **PCB mount:** FDM, 65.5 × 45.5 mm.
  - 4X M3 heat-set inserts.
  - Mounts directly to the solenoid body.

## Pneumatics

Regulated supply air feeds the 5/2 solenoid valve. The valve's two outputs each feed a manifold, one for extend and one for retract. Each manifold has 10 outlets, one per cylinder, so every cylinder has an extend line and a retract line. All air lines are 1/4" OD tubing.

Stroke speed is set by the two 9164K12 meter-out flow controls in valve exhaust ports 3 and 5. Each control sets the speed of one stroke direction.


**Setting the return speed:**

1. Retract using the manual override and find which exhaust port vents. That port controls the return.
2. Start with that port's flow control about ¾ closed. Open it a ¼ turn at a time until the cycle time is met.
3. Lock it and record the number of turns out from fully closed.
4. Run at least 200 cycles and confirm the plunger rods stay seated.

## Known Limitations

- **No independent station control.** All stations actuate together.
- **Open-loop stroke.** There are no end-of-travel sensors, so verify stroke visually.
- **Plunger separation.** A fast return can pull the white plunger rod out of the stopper. The return-side flow control limits this.
- **Uneven speed between stations.** Stations with lower friction return faster than others. Per-cylinder flow controls would even this out.
- **Tight bracket spacing.** The 10-32 bracket holes leave little clearance between cylinder brackets.
  - *Next revision:* respace the hole pattern. The station pitch and the holder pattern must change to match.
- **Printed parts.** They are uninspected at ±.020. Ream bores to fit and replace worn pushers.