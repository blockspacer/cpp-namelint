@ECHO OFF

python --version > NUL 
IF "9009"=="%ERRORLEVEL%"   (GOTO :NOPY)

python cppnamelint.py chkenv
IF NOT "0"=="%ERRORLEVEL%"   (GOTO :NOTOOL)


ECHO.
ECHO ==============================================================
ECHO Generate makefile via CMake
ECHO ==============================================================
SET ROOT_DIR=..
SET BUILD_DIR=..\Build
SET BUILD_TYPE=DEBUG
::IF EXIST %BUILD_DIR% (RD %BUILD_DIR%)
MKDIR %BUILD_DIR%
python cppnamelint.py bldgcfg %ROOT_DIR% %BUILD_DIR% %BUILD_TYPE%


:NOPY
ECHO Please install python.
:NOTOOL
ECHO.
:PASS
timeout /t 5
@ECHO ON
