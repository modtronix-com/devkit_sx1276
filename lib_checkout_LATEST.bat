::Same as lib_checkout.bat, but will checkout latest version of library!
::In Mercurial this is actually called "Update" - we first clone library and then update
::to desired tag(version).
::To update any library to latest version, simply delete or rename library folder, and
::run this batch file.
@echo off
SET lib_LATEST=yes

CALL lib_checkout.bat

