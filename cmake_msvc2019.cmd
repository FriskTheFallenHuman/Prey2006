@echo off
cls
mkdir build\msvc2019
pushd build\msvc2019
cmake -G "Visual Studio 16" %* ../../neo
popd
@pause