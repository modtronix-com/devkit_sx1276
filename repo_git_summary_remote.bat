::Show summary of this GIT repository compared to remote
::If remote repository is same, will output:
::  ????
::Else, will output:
::  ?????
::
::If remote changes available, go to library folder and pull current remote:
::hg pull      (This will pull new code, but not to working directory yet)    
::hg update    (Update working directory with pulled code)
::
::Can maybe combine top two commands with "hg pull -u"????

@echo off
echo ========= GIT Remote status ============
@echo on
git remote show origin

pause