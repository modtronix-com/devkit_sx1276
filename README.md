## Firmware for SX1276 Development Kit
This repository contains the firmware for the SX1276 Development kit available from Modtronix.com. This development kit
consists out of two identical modules. Each module can be configured to be the master or slave by pressing the "OK" button.
To use the development kit, configure one module as master and the other as slave. Ensure all radio settings(Frequency, BW, SF..) on both modules are the same.
The OLED display on both modules will show their current status. The master will send packets every couple of seconds. The slave device will receive these packets, and reply to the master. On receiving a successful reply from the slave, the Master will display the RSSI(receive signal strength) of the local(master) and remote(slave) device. For example:
L=030, R=028
For this example, it will indicate that the RSSI of the packet received by the master(L for local) was -30 RSSI, and for the slave(R for remote) -28 dBi.

## Building project
This repository contains the firmware for the SX1276 Development kit available from Modtronix.com.
It can be built with the free "System Workbench for STM32"(SW4STM32) IDE. For details, see the 
"System Workbench for STM32" section below.

After building the project, the *.elf, *.bin and *.hex file will be created in the debug folder.


### Building project using System Workbench for STM32(SW4STM32)
- The IDE can be downloaded from http://www.openstm32.org
- After installing program, start it and select the SW4STM32 folder as the workspace.
- Once app has started, click on <File> <Import> menu. In <General>, select
  "Existing Project into Workspace". Browse to "SW4STM32" folder, and import
  project.
- Once import done, build project (ctrl-B).
- To debug and program board, following must be done once. Click on <Run> <Debug Configuration>
  menu. Double click on "AC6 STM32 Debugging". A "New Configuration" will be created, select it.
  In dialog box(for new configuration) select current project (Browse button). This should
  add the created *.elf file. If not, do it manually. Click "Debug" button at bottom of dialog.
  This assumes a board is connected via ST-Link programmer (SWD connection).
- If a error message "openocd script not found" appears. Go to the "Debugger" tab in the
  "New Configuration" creaetd above, and ensure "nz32sc151.cfg" is in the "Configuration Script" box.
For details, see:
http://wiki.modtronix.com/doku.php?id=tutorials:sw4stm32-with-nz32-boards



## Upgrading Firmware
Both modules of the development kit are programmed with same firmware(HEX or DFU file) Pre built HEX and
DFU files are located in the "dist" folder. The firmware is upgraded via the USB port as follows:

1) Download DFU app (STSW-STM32080). At time of writing
this, it was located here:
http://www.st.com/web/en/catalog/tools/FM147/CL1794/SC961/SS1533/PF257916

2) Start "DfuSe Demonstaration" application.

3) Enter bootloader mode on module. To do this, connect to PC via USB, press and hold "BOOT" button,
   toggle "RESET" button, release "BOOT" button. The "DfuSeDemo" application should now show a device in
   "Available DFU Devices" box.
  
4) Click "Choose" button, and select *.dfu file. Do NOT use th "Choose" button in "Upload Action" section!

5) Click "Upgrade" button, and upgrade firmware. Do NOT use th "Upload" button in "Upload Action" section, this
   is to download the current firmware on the device!  
   
For details, see:
http://wiki.modtronix.com/doku.php?id=products:nz-stm32:nz32-sc151#programming_and_debugging
