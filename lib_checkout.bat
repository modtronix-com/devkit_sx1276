::Creates clones of Mercurial libraries, and updates to correct version(tag)

::Read Library versions from "lib_versions.txt" file
@echo off
echo.
echo.

::Read library version file. Use # for comments in file
for /f "eol=# delims=" %%x in (lib_versions.txt) do (set "%%x")


:: ==================== mbed_nz32sc151 ====================
SET lib_name=mbed_nz32sc151
SET lib_url=https://modtronix@developer.mbed.org/users/modtronix/code/mbed_nz32sc151
IF NOT EXIST "D:\prj\stm32\devkit\devkit_sx1276\%lib_name%" GOTO start_%lib_name%
echo.
echo "!!!!! Folder %lib_name% already exists - skipping checkout !!!!!"
GOTO end_mbed_nz32sc151
:start_mbed_nz32sc151
echo.
echo.
echo.
echo ===== Checking out: %lib_name% =====
@echo on
hg clone %lib_url%
@echo off
if "%lib_LATEST%"=="yes" GOTO end_mbed_nz32sc151
cd %lib_name%
@echo on
echo.
@echo -- Updating library to version %mbed_nz32sc151__version% --
hg update '%mbed_nz32sc151__version%'
@echo off
cd ..
:end_mbed_nz32sc151


:: ==================== modtronix_im4OLED ====================
SET lib_name=modtronix_im4OLED
SET lib_url=https://modtronix@developer.mbed.org/users/modtronix/code/modtronix_im4OLED
IF NOT EXIST "D:\prj\stm32\devkit\devkit_sx1276\%lib_name%" GOTO start_%lib_name%
echo.
echo "!!!!! Folder %lib_name% already exists - skipping checkout !!!!!"
GOTO end_modtronix_im4OLED
:start_modtronix_im4OLED
echo.
echo.
echo.
echo ===== Checking out: %lib_name% =====
@echo on
hg clone %lib_url%
@echo off
if "%lib_LATEST%"=="yes" GOTO end_modtronix_im4OLED
cd %lib_name%
@echo on
echo.
@echo -- Updating library to version %modtronix_im4OLED__version% --
hg update '%modtronix_im4OLED__version%'
@echo off
cd ..
:end_modtronix_im4OLED



:: ==================== modtronix_inAir ====================
SET lib_name=modtronix_inAir
SET lib_url=https://modtronix@developer.mbed.org/users/modtronix/code/modtronix_inAir
IF NOT EXIST "D:\prj\stm32\devkit\devkit_sx1276\%lib_name%" GOTO start_%lib_name%
echo.
echo "!!!!! Folder %lib_name% already exists - skipping checkout !!!!!"
GOTO end_modtronix_inAir
:start_modtronix_inAir
echo.
echo.
echo.
echo ===== Checking out: %lib_name% =====
@echo on
hg clone %lib_url%
@echo off
if "%lib_LATEST%"=="yes" GOTO end_modtronix_inAir
cd %lib_name%
@echo on
echo.
@echo -- Updating library to version %modtronix_inAir__version% --
hg update '%modtronix_inAir__version%'
@echo off
cd ..
:end_modtronix_inAir


:: ==================== modtronix_NZ32S ====================
SET lib_name=modtronix_NZ32S
SET lib_url=https://modtronix@developer.mbed.org/users/modtronix/code/modtronix_NZ32S
IF NOT EXIST "D:\prj\stm32\devkit\devkit_sx1276\%lib_name%" GOTO start_%lib_name%
echo.
echo "!!!!! Folder %lib_name% already exists - skipping checkout !!!!!"
GOTO end_modtronix_NZ32S
:start_modtronix_NZ32S
echo.
echo.
echo.
echo ===== Checking out: %lib_name% =====
@echo on
hg clone %lib_url%
@echo off
if "%lib_LATEST%"=="yes" GOTO end_modtronix_NZ32S
cd %lib_name%
@echo on
echo.
@echo -- Updating library to version %modtronix_NZ32S__version% --
hg update '%modtronix_NZ32S__version%'
@echo off
cd ..
:end_modtronix_NZ32S


@echo off
echo.
echo.
echo.
pause