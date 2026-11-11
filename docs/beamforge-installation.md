# BeamForge Installation

BeamForge connects a Raspberry Pi running VectorPie to an arcade vector monitor. This guide shows a Raspberry Pi 5 with its active cooler, a GPIO extender, and BeamForge mounted above it.

## Included with BeamForge

| Quantity | Part |
|---|---|
| 1 | Assembled BeamForge board |
| 4 | M2.5 male-female standoffs, **20 mm body**, excluding the threaded stud |
| 4 | M2.5 × 6 mm screws |
| 4 | M2.5 nuts |
| 1 | GPIO stacking extender: **2 × 20 pins, 2.54 mm pitch, 8.4 mm socket body, 6 mm exposed pins** |
| 1 | Short USB-A to USB-C **data cable**, approximately 6 in / 15 cm, for the Pi-to-BeamForge connection |

## You also need

- A Raspberry Pi 5 with the active cooler installed and its fan connected.
- Storage with the [VectorPie image installed](../README.md#download--installation).
- A compatible arcade vector monitor.
- A compatible cable to connect BeamForge to your monitor. For a **WG6100 CRT monitor**, see Barry's [DVG to Stock WG6100 cable at Vector Head Arcade](https://www.vectorheadarcade.com/product-page/dvg-to-stock-wg6100).
- A suitable power source for the Raspberry Pi.

## Assemble BeamForge and the Pi

The drawing below shows how BeamForge mounts above a Raspberry Pi 5 with an active cooler using the included hardware.

[![Exploded BeamForge assembly: nuts, Pi 5 with active cooler, GPIO extender, 20 mm standoffs, BeamForge, and screws](images/assembly/beamforge-pi5-technical.png)](images/assembly/beamforge-pi5-technical.png)

The supplied **20 mm standoffs and GPIO extender** are sized for the Pi 5 with active cooler shown above. A different header or cooler can change the required spacing. VectorPie also supports the Pi 4; check physical clearances before using a different stack.

1. Fit the four **20 mm M2.5 standoffs** above the Pi. Their male studs pass down through the Pi's mounting holes. Secure them with the four nuts underneath.
2. Seat the GPIO extender on the Pi's 40-pin header, **socket side down and exposed pins up**. Align both rows and all 40 positions; do not offset it by a row or a pin.
3. Orient BeamForge as drawn, with its underside GPIO socket directly above the extender. Lower it evenly until the header engages and the board rests on all four standoffs.
4. Fit the four **M2.5 × 6 mm screws** through BeamForge into the standoffs. Tighten gently. The boards must remain parallel; do not use the screws to pull an incorrectly seated header into place.
5. Check that BeamForge clears the cooler and fan, and that the fan cable is not trapped.

## Connect USB and power

[![Assembled BeamForge on a Raspberry Pi 5 with active cooler and the USB data cable connected from the Pi's upper USB 3 port to BeamForge's USB-C socket J4](images/assembly/beamforge-pi5-installed.png)](images/assembly/beamforge-pi5-installed.png)

With the Pi PCB horizontal, **upper** means the USB socket farther from the PCB.

| Connection | Cable / destination |
|---|---|
| Pi **upper blue USB 3** socket → BeamForge **USB-C**, J4 on BeamForge | Included USB-A to USB-C data cable |
| Pi **USB-C power** socket → power source | Suitable Pi power supply |

## Cabinet adapters

To simplify connection to an existing cabinet, we offer adapter boards for [Asteroids / Asteroids Deluxe](asteroids-adapter-installation.md) and [Space Duel / Gravitar / Black Widow](space-duel-adapter-installation.md).

## First startup

1. Check GPIO alignment, mounting clearance, the USB data cable, Pi power, and the vector cable before switching on.
2. Boot VectorPie. Open **Settings → DVG → HARDWARE** and confirm that it reports **BEAMFORGE**. A keyboard can be used for initial setup; **Tab** opens Settings.
3. **Choose the monitor preset:** open **Settings → DVG → SETTINGS...** and set **CRT TYPE** to your monitor model, such as **WG6100**, **19V2000**, or **AMPLIFONE**. Use **Up/Down** to select a setting, **Select** to edit it, and **Left/Right** to change its value. Choose the preset before fine-tuning: it supplies the initial speed and pincushion settings.
4. Close BeamForge settings with **Config** to save, then open **Settings → DVG → CRT CALIBRATION**. Use **Left/Right** to select **CONCENTRIC BOXES**.
5. **Adjust the image size:** using a small insulated screwdriver, turn the **XSIZE** and **YSIZE** trimmers on BeamForge in small increments until the outer box fits inside the visible screen.
6. **Center the image:** adjust the **XCTR** and **YCTR** trimmers until the boxes are centered with equal margins on opposite sides. Recheck size and centering, then press **Cancel** to exit calibration.
7. **Adjust drawing speed if needed:** return to **Settings → DVG → SETTINGS...** and select **MAX CRT SPEED**. Start with the preset value. Higher values allow faster beam movement; lower values give the monitor more time to follow. Change one step at a time and check the picture. If lines distort or corners become rounded, reduce the value.
8. **Adjust brightness:** use **INTENSITY** to set the overall brightness. Start with the default **75%** and adjust gradually for clear lines without excessive glow or loss of detail.
9. **Adjust pincushion only if needed:** if straight lines bow near the screen edges, adjust **PINCUSHION** in small increments. Keep the preset value if the lines are already straight. Save and check the **GRID** pattern in **CRT CALIBRATION** between adjustments; recheck size and centering afterward.
10. **Save and check:** use **Config** to close and save BeamForge settings, or **Cancel** from its top level. Check **GRID** for geometry and **COLOR INTENSITY BARS** for brightness, then try a game. Selecting the monitor preset again reapplies its speed and pincushion values.

[Back to the VectorPie manual](../README.md)
