@echo off
rem Tsumugi no Sato - build with MSVC (Visual Studio 2022 or later, x64)
setlocal
cd /d "%~dp0"
where cl >nul 2>nul
if not errorlevel 1 goto build
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
for /f "usebackq tokens=*" %%i in (`call "%%VSWHERE%%" -latest -products * -property installationPath`) do set "VSDIR=%%i"
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul
:build
if not exist bin mkdir bin
if not exist obj mkdir obj
cl /nologo /std:c++17 /utf-8 /EHsc /O2 /W3 /MD /DUNICODE /D_UNICODE /Fo:obj\ /Fe:bin\TsumugiNoSato.exe ^
  src\main.cpp src\Renderer.cpp src\Game.cpp src\Village.cpp src\Dungeon.cpp src\Characters.cpp src\Sora.cpp src\Sheet.cpp src\Town.cpp ^
  src\Characters2.cpp src\DungeonDraw.cpp src\Events.cpp src\Party.cpp src\Interior.cpp src\WeaponScreen.cpp src\GeoMenu.cpp src\VillageExtra.cpp src\Enemies2.cpp src\Talk.cpp ^
  /link /SUBSYSTEM:WINDOWS user32.lib ole32.lib shell32.lib
if errorlevel 1 (
  echo BUILD FAILED
  exit /b 1
)
echo BUILD OK: bin\TsumugiNoSato.exe
