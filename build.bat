@echo off
setlocal
if not exist "C:\Temp" mkdir "C:\Temp"
set "TMP=C:\Temp"
set "TEMP=C:\Temp"
set "PATH=C:\ST\STM32CubeIDE_2.2.0\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.make.win32_2.2.200.202604021615\tools\bin;%PATH%"

make -j4
if %ERRORLEVEL% equ 0 (
    echo.
    echo ============================================================
    echo   BUILD SUCCESSFUL! Firmware ready: build\h753zi_sd.bin
    echo ============================================================
) else (
    echo.
    echo [ERROR] Build failed!
)
