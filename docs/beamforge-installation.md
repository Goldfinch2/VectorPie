# BeamForge Installation

BeamForge connects a Raspberry Pi running VectorPie to an arcade vector monitor. This guide shows a Raspberry Pi 5 with its active cooler, a GPIO extender, and BeamForge mounted above it.

For mounting the complete assembly on a cabinet adapter, continue with the [Asteroids / Asteroids Deluxe guide](asteroids-adapter-installation.md) or the [Space Duel / Gravitar / Black Widow guide](space-duel-adapter-installation.md).

## Included with BeamForge

| Quantity | Part |
|---|---|
| 1 | Assembled BeamForge board |
| 4 | M2.5 male-female standoffs, **20 mm body**, excluding the threaded stud |
| 4 | M2.5 × 6 mm screws |
| 4 | M2.5 nuts |
| 1 | GPIO stacking extender: **2 × 20 pins, 2.54 mm pitch, 8.4 mm socket body, 6 mm exposed pins** |
| 1 | Short USB-A to USB-C **data cable**, approximately 6 in / 15 cm, for the Pi-to-BeamForge connection |

These dimensions are for the illustrated Pi 5 and active cooler assembly. A different header or cooler can change the required spacing. VectorPie also supports the Pi 4; check physical clearances before using a different stack.

## You also need

- A Raspberry Pi 5 with the active cooler installed and its fan connected.
- Storage with the [VectorPie image installed](../README.md#download--installation).
- A compatible cabinet adapter with its included vector cable, or a correctly wired harness for your monitor.
- A suitable Pi power source. Either adapter above includes a USB power cable from its J1 output to the Pi's USB-C power socket; this is separate from BeamForge's included USB data cable.

## Assemble BeamForge and the Pi

Turn off and unplug the cabinet before installing or reconnecting anything. Handle boards by their edges. A CRT can retain dangerous voltages after power is removed; this installation does not require opening the monitor or touching its high-voltage connections.

[![Exploded BeamForge assembly: nuts, Pi 5 with active cooler, GPIO extender, 20 mm standoffs, BeamForge, and screws](images/assembly/beamforge-pi5-technical.png)](images/assembly/beamforge-pi5-technical.png)

*Select the drawing to open it at full size. The exploded gaps show assembly order, not installed spacing.*

1. Fit the four **20 mm M2.5 standoffs** above the Pi. Their male studs pass down through the Pi's mounting holes. Secure them with the four nuts underneath.
2. Seat the GPIO extender on the Pi's 40-pin header, **socket side down and exposed pins up**. Align both rows and all 40 positions; do not offset it by a row or a pin.
3. Orient BeamForge as drawn, with its underside GPIO socket directly above the extender. Lower it evenly until the header engages and the board rests on all four standoffs.
4. Fit the four **M2.5 × 6 mm screws** through BeamForge into the standoffs. Tighten gently. The boards must remain parallel; do not use the screws to pull an incorrectly seated header into place.
5. Check that BeamForge clears the cooler and fan, and that the fan cable is not trapped.

When installing this stack on an adapter board, replace the nuts under the Pi with **10 mm lower standoffs**, then reuse those nuts beneath the adapter. The adapter guides show this step.

## Connect USB and power

With the Pi PCB horizontal, **upper** means the USB socket farther from the PCB, and **lower** means the socket closer to it.

| Connection | Cable / destination |
|---|---|
| Pi **upper blue USB 3** socket → BeamForge **USB-C**, J4 on BeamForge | Included USB-A to USB-C data cable |
| Pi **lower blue USB 3** socket → USB audio dongle | Used when feeding an adapter's audio input |
| Pi **black USB 2** sockets → I-PAC2 and PACLED64 | The USB cables for those Ultimarc boards, if fitted |
| Pi **USB-C power** socket → power source | Adapter **J1 PI POWER**, or a suitable external Pi supply |

Connect the included USB data cable even with the GPIO extender installed. Use one Pi power source at a time.

The complete cable arrangement is shown in the [Asteroids](asteroids-adapter-installation.md#connect-the-cables) and [Space Duel](space-duel-adapter-installation.md#connect-the-cables) adapter guides.

## Connect the vector monitor

With a cabinet adapter, connect BeamForge's **15-pin vector output, J2**, to the adapter's **J5 VECTOR INPUT** using the VGA-style vector cable included with the adapter.

**This is a vector-monitor connection.** Although the connector is VGA-shaped, its signals are not ordinary PC VGA. Do not connect it to a conventional VGA display or PC video output.

The adapter connects to the cabinet's monitor through the existing harness. Without an adapter, use a harness verified for your particular vector monitor; do not guess the monitor pinout from wire colors.

## First startup

1. Check GPIO alignment, mounting clearance, the USB data cable, Pi power, and the vector cable before switching on.
2. Boot VectorPie. Open **Settings → DVG → HARDWARE** and confirm that it reports **BEAMFORGE**. A keyboard can be used for initial setup; **Tab** opens Settings.
3. Open **Settings → DVG → SETTINGS...** to configure BeamForge for the connected monitor.
4. Use **Settings → DVG → CRT CALIBRATION** to check centering, size, and orientation. Start with a modest image size and brightness. If the image collapses to a bright stationary point or line, switch the monitor off and correct the cause before continuing.
5. Configure controls, audio, and optional LEDs using the [VectorPie manual](../README.md).

## Firmware updates

Connect VectorPie to the network, then open **Settings → DVG → UPDATE...**. Read the release notes and select **UPDATE**. Keep power connected until the update and reboot finish. The vector display goes dark during flashing.

BeamForge uses one firmware for all boards. Updates and board settings are available from VectorPie; there is no need to reach the buttons on the mounted board. See [Firmware updates](../README.md#firmware-updates).

[Back to the VectorPie manual](../README.md)
