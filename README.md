# mixman-dm2
Windows 10 driver for the Mixman DM2 USB turntable
## Prerequisites
You must be running an x64 version of Windows 10
## Installation
1. [Disable Secure Boot.](https://docs.microsoft.com/en-us/windows-hardware/manufacture/desktop/disabling-secure-boot) If you have BitLocker enabled, you may want to [suspend it first](https://www.windowscentral.com/how-suspend-bitlocker-encryption-perform-system-changes-windows-10) to avoid any recovery key nonsense
2. To enable test-signing, run the following in an administrator command prompt and reboot:
  ```
  bcdedit -set testsigning on
  ```
3. Download the and unzip the [latest release](https://github.com/arseneyr/mixman-dm2/releases/latest)
4. Right-click on `Mixman_Filter.inf` and select 'Install'
5. Select 'Install this driver software anyway' on the dialog boxes

## Calibration
The crossfader and joysticks have potentially non-linear behavior, so the driver runs a calibration routine every time the turntable is plugged in. Make sure that the crossfader and joystick are centered during the power-on light sequence. After the lights turn off, move the crossfader and joystick through their full range of motion a couple times.

## Caveats
- The driver is not WHQL-certified, so it will require test-signing mode to be enabled (and thus Secure Boot to be disabled). **Before reenabling Secure Boot, make sure to run uninstall.ps1 in an administrator Powershell console or else your USB devices may not work.**
- The Mixman device reports an invalid USB configuration descriptor, which the Windows USB 3.0 bus driver rejects and enumerates as an `Unknown USB Device (Invalid Configuration Descriptor)`. To work around this bug in the Mixman firmware, this driver package includes a USB 3.0 bus filter driver to rewrite the configuration descriptor on the fly. While the filter driver takes care to only touch the very specific configuration descriptor associated with the Mixman device, it is inherently dangerous to filter the entire USB bus so proceed with caution and don't install this driver anywhere important.
