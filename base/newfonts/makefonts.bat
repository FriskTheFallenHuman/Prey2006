@echo off
cd ..
SET BASEPATH=%CD%
cd newfonts

REM Do latin languages first
for %%i in ( *.?tf ) do idFont newfonts\%%i -fs_basepath %BASEPATH%

REM Export Japanese next
idFont newfonts\japanese.ttf -sys_lang japanese -fs_basepath %BASEPATH%

REM Then do Cyrillic
REM mkdir cyrillic
REM copy /Y *.?tf cyrillic\
REM del cyrillic\japanese.*
REM for %%i in ( cyrillic\*.?tf ) do idFont newfonts\%%i -sys_lang russian -fs_basepath %BASEPATH%
REM del /Q cyrillic\*.?tf

REM Then do Polish
REM mkdir polish
REM copy /Y *.?tf polish\
REM del polish\japanese.*
REM for %%i in ( polish\*.?tf ) do idFont newfonts\%%i -sys_lang polish -fs_basepath %BASEPATH%
REM del /Q polish\*.?tf

REM Then do Czech
REM mkdir czech
REM copy /Y *.?tf czech\
REM del czech\japanese.*
REM for %%i in ( czech\*.?tf ) do idFont newfonts\%%i -sys_lang czech -fs_basepath %BASEPATH%
REM del /Q czech\*.?tf

pause