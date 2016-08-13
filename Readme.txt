This folder contains a CoIDE and "System Workbench for STM32"(SW4STM32) project.


========== System Workbench for STM32(SW4STM32) ==========
- For details on using this board with the SW4STM32 IDE, see:
  http://wiki.modtronix.com/doku.php?id=tutorials:sw4stm32-with-nz32-boards
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
  
  
========== CoIDE ==========
!!!!!!!!!! CoIDE is currently not implemented !!!!!!!!!!
- For details on using this board with the SW4STM32 IDE, see:
  http://wiki.modtronix.com/doku.php?id=tutorials:coide-with-nz32-boards
- The IDE can be downloaded from http://coocox.org/
- Download and use V2!
- After installing program, open CoIDE project in CoIDE folder.


========== Upgrading Firmware ==========
After building the project, the NZ32-SC151 board can be programmed with the new code using the a
USB Bootloader(via USB port on NZ32-SC151), or via an ST-Link/V2.1 programmer.
For details, see:
wiki.modtronix.com/doku.php?id=products:nz-stm32:nz32-sc151#programming_and_debugging


========== USB drivers ==========
For STM32 virtual com port, download drivers here:
http://www.st.com/web/en/catalog/tools/PF257938

This contains the current working project. This is the standard
c project. Fixed USB issue where program froze when receiving data.
