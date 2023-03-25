@echo off
pushd "%~dp0"\..\
call external\premake\premake5.exe vs2022
popd
PAUSE