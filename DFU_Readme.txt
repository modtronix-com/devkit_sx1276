To program the board via USB using the DFU programmer, the HEX or BIN file must first be
converted to a DFU file. This resulting DFU file is then used to program the STM32 board.


===== Convert HEX or BIN file to DFU file =====
1) Download and install v3.0.3 of the DFU app(STSW-STM32080)
   IMPORTANT!!! V3.0.4 has a bug, and does not work for generating DFU files!
   We used V3.0.3, located here:
   https://drive.google.com/file/d/0B7OY5pub_GfIV2IyV0JQWEdPNTQ/view?usp=sharing

2) Start "DFU File Manager v3.0.3" app. Open HEX file, and click "Generate" button.
   This will generate a DFU file.

   
===== Program STM32 board with generated DFU file =====   
1) Download and install DFU app (STSW-STM32080). For programming DFU to STM32 chip,
   we used v3.0.4! Search for "STSW-STM32080". At time of writing this, it was located here:
   http://www.st.com/web/en/catalog/tools/FM147/CL1794/SC961/SS1533/PF257916

2) Start "DfuSe Demonstaration" application.

3) Power up NZ32-ST1L while holding "BOOT" button down. The "DfuSeDemo" application should now
   show a device in "Available DFU Devices" box.
   
4) Click "Choose" button, and select *.dfu file. Do NOT use th "Choose" button in "Upload Action" section!

5) Click "Upgrade" button, and upgrade firmware. Do NOT use th "Upload" button in "Upload Action" section, this
   is to download the current firmware on the device!