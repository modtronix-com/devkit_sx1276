::Show summary of of all Mercurial libraries compared to remote version.
::If remote repository is same, will output:
::  remote: (synced)
::Else, will output:
::  remote: 1 or more incoming
::
::If remote changes available, go to library folder and pull current remote:
::hg pull      (This will pull new code, but not to working directory yet)    
::hg update    (Update working directory with pulled code)
::
::Can maybe combine top two commands with "hg pull -u"????

@echo off
echo ========= modtronix_inAir ============
cd modtronix_inAir
@echo on
hg summary --remote

@echo off
cd ..
echo.
echo.
echo.
echo ========= modtronix_NZ32S ============
cd modtronix_NZ32S
@echo on
hg summary --remote

@echo off
cd ..
echo.
echo.
echo.
echo ========= mbed_nz32sc151 ============
cd mbed_nz32sc151
@echo on
hg summary --remote

@echo off
cd ..
echo.
echo.
echo.
echo ========= modtronix_im4OLED ============
cd modtronix_im4OLED
@echo on
hg summary --remote

@echo off
echo.
echo.
echo.
pause