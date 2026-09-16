@echo off
set "CLI_PATH=C:\ST\STM32CubeIDE_2.2.0\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.500.202603051304\tools\bin\STM32_Programmer_CLI.exe"

if exist "build\h753zi_sd.bin" (
    echo [FLASHING] Programming firmware via ST-LINK SWD...
    "%CLI_PATH%" -c port=SWD -w build\h753zi_sd.bin 0x08000000 -v -rst
) else (
    echo [ERROR] build\h753zi_sd.elf not found! Run build.bat first.
)
