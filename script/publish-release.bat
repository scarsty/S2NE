@echo off
setlocal

call copy-json.bat

set "ROOT=%~dp0"
set "SEVENZIP=%ROOT%7z.exe"
set "BINARY=%ROOT%S2NE\out\x64\Release"
set "ARCHIVE=%ROOT%S2NE-game-win64.zip"

if not exist "%SEVENZIP%" exit /b 2
if not exist "%BINARY%\S2NE.exe" exit /b 3
for %%F in (SDL3.dll SDL3_mixer.dll SDL3_ttf.dll libfluidsynth-3.dll mpg123.dll freetype.dll z.dll bz2.dll libpng16.dll brotlicommon.dll brotlidec.dll) do if not exist "%BINARY%\%%F" exit /b 4
for %%F in (SUPERSP.PAL SUPERSP.BIN SUPERSP.CEL SUPERSP.PRG SUPERSP.CFG SUPERSP.DAT ITEMSP.BIN KINDSP.BIN chinese.ttf) do if not exist "%ROOT%%%F" exit /b 5
if not exist "%ROOT%texture" exit /b 6
if not exist "%ROOT%cc\TSCharacters.txt" exit /b 7
if not exist "%ROOT%cc\TSPhrases.txt" exit /b 7
if not exist "%ROOT%cc\STCharacters.txt" exit /b 7
if not exist "%ROOT%cc\STPhrases.txt" exit /b 7

if exist "%ARCHIVE%" del /q "%ARCHIVE%"
pushd "%ROOT%"
"%SEVENZIP%" a -tzip "%ARCHIVE%" ^
 "%BINARY%\S2NE.exe" "%BINARY%\SDL3.dll" "%BINARY%\SDL3_mixer.dll" "%BINARY%\SDL3_ttf.dll" ^
 "%BINARY%\libfluidsynth-3.dll" "%BINARY%\mpg123.dll" "%BINARY%\freetype.dll" "%BINARY%\z.dll" ^
 "%BINARY%\bz2.dll" "%BINARY%\libpng16.dll" "%BINARY%\brotlicommon.dll" "%BINARY%\brotlidec.dll" ^
 SUPERSP.PAL SUPERSP.BIN SUPERSP.CEL SUPERSP.PRG SUPERSP.CFG SUPERSP.DAT ITEMSP.BIN KINDSP.BIN ^
 chinese.ttf cc\ texture\ json\ MP3\
set "RESULT=%ERRORLEVEL%"
popd
if not "%RESULT%"=="0" exit /b %RESULT%
exit /b %RESULT%
