# mixman-dm2
Windows 10 driver for the Mixman DM2 USB turntable
## Prerequisites
You must be running an x64 version of Windows 10
## Installation
1. Download the and unzip the [latest release](https://github.com/arseneyr/mixman-dm2/releases/latest)
2. Disable driver signature enforcement as described [here](https://www.thewindowsclub.com/disable-driver-signature-enforcement-windows)
3. Right-click on `Mixman_Filter.inf` and select 'Install'
4. Select 'Install this driver software anyway' on the dialog boxes

## Calibration
The crossfader and joysticks have potentially non-linear behavior, so the driver runs a calibration routine every time the turntable is plugged in. Make sure that the crossfader and joystick are centered during the power-on light sequence. After the lights turn off, move the crossfader and joystick through their full range of motion a couple times.
