@echo off
pushd "%~dp0"\..\
call external\premake\premake5.exe vs2022
mkdir screenshots
mkdir measurements
mkdir measurements\fps
mkdir measurements\error
mkdir measurements\error_center
mkdir measurements\position
popd
PAUSE