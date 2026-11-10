@echo off
rem ============================================================================
rem PROJETO: PICO ARTNET DMX NODE (RP2040)
rem ARQUIVO: compile_targets.bat
rem AUTOR: Matheus Dev (@Matheus-devpixel)
rem DESCRIÇÃO: Script em lote para disparo rápido do compilador multialvo.
rem ============================================================================
chcp 65001 >nul
echo ============================================================
echo   COMPILADOR AUTOMATICO PICO ARTNET DMX (SDK NATIVO C)
echo ============================================================
echo.

set "PY_EXE=python"
where %PY_EXE% >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    if exist "C:\Program Files\Inkscape\bin\python.exe" (
        set "PY_EXE=C:\Program Files\Inkscape\bin\python.exe"
    )
)

"%PY_EXE%" "%~dp0compile_targets.py"
echo.
pause
