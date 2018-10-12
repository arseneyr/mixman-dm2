# mixman-dm2
Windows 10 driver for the Mixman DM2 USB turntable
## Prerequisites
You must be running an x64 version of Windows 10
## Installation
1. Disable driver signature enforcement as described [in option #1 here](https://www.maketecheasier.com/install-unsigned-drivers-windows10/)
2. Download the and unzip the [latest release](https://github.com/arseneyr/mixman-dm2/releases/latest)
3. Right-click on `Mixman_Filter.inf` and select 'Install'
4. Select 'Install this driver software anyway' on the dialog boxes
5. Reboot your PC to re-enable driver signature enforcement

## Calibration
The crossfader and joysticks have potentially non-linear behavior, so the driver runs a calibration routine every time the turntable is plugged in. Make sure that the crossfader and joystick are centered during the power-on light sequence. After the lights turn off, move the crossfader and joystick through their full range of motion a couple times.

## Midi mapping
