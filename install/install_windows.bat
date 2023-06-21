@echo off
rem Copyright 2022-2023 u-blox
rem
rem Licensed under the Apache License, Version 2.0 (the "License");
rem you may not use this file except in compliance with the License.
rem You may obtain a copy of the License at
rem
rem  http://www.apache.org/licenses/LICENSE-2.0
rem
rem Unless required by applicable law or agreed to in writing, software
rem distributed under the License is distributed on an "AS IS" BASIS,
rem WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
rem See the License for the specific language governing permissions and

net session >nul 2>nul || (echo ** This script must be run as administrator. & pause & exit /b 1)
echo.
echo === Installing all the required tools and packages for cellular applications for the XPLR-IoT-1 development kit
echo === This will be a lengthy process, please be patient
echo.

setlocal

rem ************************************************************************************
rem Set repository name to clone
set REPOSITORY=ubxlib_cellular_applications_xplr_iot

rem ************************************************************************************
rem Set directory paths
set ERR_FILE=%TEMP%\_err.txt
set ROOT_DIR=%USERPROFILE%\xplriot1
set ENV_DIR=%ROOT_DIR%\env
set GIT_ENV_DIR=%ENV_DIR:\=/%
set NCS_VERS=v2.1.0
set PIP_COM=pip3 --disable-pip-version-check install -q

echo ROOT: %ROOT_DIR%
echo ENV: %ENV_DIR%

rem ************************************************************************************
rem Set ARM compiller details
set GCCLoc=https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-rm/10-2020q4
set GCCName=gcc-arm-none-eabi-10-2020-q4-major
set GCCZip=%GCCName%-win32.zip

rem ************************************************************************************
rem Start installation
echo Started at: %date% %time%

rem Avoid possible path length problem
call :SilentCom "reg add HKLM\SYSTEM\CurrentControlSet\Control\FileSystem /f /v LongPathsEnabled /t REG_DWORD /d 1"

cd /d "%USERPROFILE%"
echo Changed directory to [%cd%]

rem ************************************************************************************
echo Installing tool packages...
"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -InputFormat None -ExecutionPolicy Bypass ^
   -Command "[System.Net.ServicePointManager]::SecurityProtocol = 3072; iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))" ^
   1>nul 2>%ERR_FILE% || (type %ERR_FILE% & exit /b 1)
set PATH=%ALLUSERSPROFILE%\chocolatey\bin;%PATH%
call :SilentCom "choco install -y --no-progress cmake --installargs 'ADD_CMAKE_TO_PATH=System'"
call :SilentCom "choco install -y --no-progress ninja gperf git.commandline dtc-msys2 wget curl unzip nrfjprog tartool ccache which"
call refreshenv >nul

rem ************************************************************************************
:checkPython
call python --version 2>&1 | findstr 3. >NUL
IF %ERRORLEVEL% EQU 0 goto installWest

:installPython
echo Downloading Python...
set PY_INST=py_inst.exe
curl -s https://www.python.org/ftp/python/3.11.3/python-3.11.3-amd64.exe >%PY_INST%

echo Installing Python... [%ENV_DIR%\Python]
call %PY_INST% /quiet PrependPath=1 InstallAllUsers=0 Include_test=0 TargetDir=%ENV_DIR%\Python
del %PY_INST%
call refreshenv >nul

rem ************************************************************************************
:installWest
echo Checking if PIP3 is installed...
which pip3 | findstr Not >NUL
IF %ERRORLEVEL% EQU 0 (echo "PIP3 is not installed properly, please check." & pause & exit 1)

echo Installing west... [%cd%]
call :SilentCom "%PIP_COM% west"
set TarFile=newtmgr.tar.gz
curl -s https://archive.apache.org/dist/mynewt/apache-mynewt-1.4.1/apache-mynewt-newtmgr-bin-windows-1.4.1.tgz >%TarFile%
call tar -O -xf %TarFile% "*newtmgr.exe" >%ALLUSERSPROFILE%\chocolatey\bin\newtmgr.exe
del %TarFile%

rem ************************************************************************************
:installnRFConnectSDK
mkdir %ENV_DIR%\ncs
cd /d  %ENV_DIR%\ncs
echo Installing nRF Connect SDK... [%cd%]
call west init -m https://github.com/nrfconnect/sdk-nrf --mr %NCS_VERS%

echo Running owner protection checks... [%ENV_DIR%\ncs]
rem Avoid possible owner protection problems as we will clone as admin
call :SilentCom "takeown /r /f %ENV_DIR%\ncs"

rem ************************************************************************************
echo Updating west - this can take some time!
call :SilentCom "west update"

echo Updating zephyr-export
call :SilentCom "west zephyr-export"

echo Installing additional zyphyr requirements...
%PIP_COM% -r zephyr/scripts/requirements.txt >nul 2>&1

echo Installing additional nRF requirements...
%PIP_COM% -r nrf/scripts/requirements.txt >nul 2>&1

echo Installing additional bootloader requirements...
%PIP_COM% -r bootloader/mcuboot/scripts/requirements.txt >nul 2>&1

rem ************************************************************************************
:installARMCompiler
cd ..
echo Downloading ARM compiler... [%cd%]
wget -q %GCCLoc%/%GCCZip%

echo Installing ARM compiler... [%cd%]
call unzip -q -o %GCCZip%
del %GCCZip%

rem ************************************************************************************
:installVSCode
echo Dwonloading Visual Studio Code... [%cd%]
set CodeInstaller=code_inst.exe
curl -s -L "https://code.visualstudio.com/sha/download?build=stable&os=win32-x64" >%CodeInstaller%

echo Installing Visual Studio Code... [%cd%]
%CodeInstaller% /VERYSILENT /NORESTART /MERGETASKS=!runcode
call refreshenv >nul
del %CodeInstaller%

rem ************************************************************************************
:installExtensions
echo Installing VSCode extensions... [%cd%]
call :SilentCom "code --install-extension ms-vscode.cpptools-extension-pack"
call :SilentCom "code --install-extension marus25.cortex-debug"
call :SilentCom "code --uninstall-extension ms-vscode.cmake-tools"

rem ************************************************************************************
:installJLink
echo Downloading JLink software... [%cd%]
set JLinkExe=JLink_Windows_x86_64.exe
curl -s -X POST https://www.segger.com/downloads/jlink/%JLinkExe% ^
       -H "Content-Type: application/x-www-form-urlencoded" ^
       -d "accept_license_agreement=accepted" >%JLinkExe%

echo Installing JLink software... [%cd%]
%JLinkExe% /S
del %JLinkExe%

rem ************************************************************************************
:installSerialPortDriver
echo Downloading serial port driver... [%cd%]
set DriverName=CP210x_Universal_Windows_Driver
set DriverZip=%DriverName%.zip
wget -q https://www.silabs.com/documents/public/software/%DriverZip%

echo Installing serial port driver... [%cd%]
call unzip -q -o -d %DriverName% %DriverZip%
del %DriverZip%
cd %DriverName%
call :SilentCom "pnputil /add-driver silabser.inf"
cd ..
rmdir /S /Q %DriverName%

rem ************************************************************************************
:getRepository
cd %ROOT_DIR%
echo Getting the source code repositories... [%cd%]
call git clone --recursive -q https://github.com/u-blox/%REPOSITORY%

rem ************************************************************************************
:confBuildSettings
cd %ROOT_DIR%\%REPOSITORY%
echo Configuring build .settings file... [%cd%]
call python do -n %ENV_DIR%\ncs -t %ENV_DIR%\%GCCName% save

rem ************************************************************************************
echo Running owner protection [%ROOT_DIR%\%REPOSITORY%]
call :SilentCom "takeown /r /f %ROOT_DIR%\%REPOSITORY%"
call git config --global --add safe.directory %GIT_ENV_DIR%/ncs/zephyr

rem ************************************************************************************
echo.
echo Installation completed, ended at: %date% %time%
echo.
echo ======= All done! =======
echo.
pause
goto:eof

:SilentCom
rem Execute a command without any printouts unless an error occurs
%~1 1>nul 2>%ERR_FILE% || (type %ERR_FILE% & pause & exit 1)
exit /b 0
