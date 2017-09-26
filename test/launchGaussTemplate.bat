@echo off

REM set location of Gauss Algorithms development
set PROD_HOME=C:\Users\you\software\GaussAlgs

REM add locations of JVM and Gauss Algorithms DLL to path
set JVM_PATH=C:\Users\you\software\jdk1.8.0_131_x86\jre\bin\client
set GALIB_PATH=%PROD_HOME%\lib\win32\Release
set PATH=%JVM_PATH%;%GALIB_PATH%;%PATH%

REM set up java class path
set CLASSPATH=%PROD_HOME%\lib\GaussAlgorithms.jar;%PROD_HOME%\lib\commons-math3-3.3.jar

REM construct full path filename to test spectrum
set TESTFILENAME=%PROD_HOME%\bin\PGNAA_antifreeze.chn

REM set output filename
set LOGFILENAME=testGauss.log
del /f %LOGFILENAME%

REM launch the program
testGauss32.exe "%CLASSPATH%" "%TESTFILENAME%" > %LOGFILENAME%
