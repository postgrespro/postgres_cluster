@echo off
PATH=C:\Windows\system32;C:\Perl64\bin;C:\Program Files\Git\cmd

IF "%ARCH%"==""	   SET ARCH=X86
IF "%ARCH%"=="x86" SET ARCH=X86
IF "%ARCH%"=="x64" SET ARCH=X64
IF "%SDK%"=="" 	   SET SDK=SDK71

echo ARCH=%ARCH%
echo SDK=%SDK%

IF %SDK% == SDK71 SET DEPENDENCIES_BIN_DIR=C:\pgfarm\deps_%ARCH%_%SDK%
IF %SDK% == MSVC2013 SET DEPENDENCIES_BIN_DIR=C:\pgfarm\deps_%ARCH%

SET PERL32_PATH=C:\Perl
SET PERL64_PATH=C:\Perl64
SET PYTHON32_PATH=C:\Python27x86
SET PYTHON64_PATH=C:\Python27x64

IF %SDK% == SDK71 (
  CALL "C:\Program Files\Microsoft SDKs\Windows\v7.1\Bin\SetEnv" /%ARCH% || GOTO :ERROR
  ECHO ON
)

IF %SDK% == MSVC2013 (
  IF %ARCH% == X86 CALL "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall" x86 || GOTO :ERROR
  ECHO ON
  IF %ARCH% == X64 CALL "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall" amd64 || GOTO :ERROR
  ECHO ON
)

>src\tools\msvc\config.pl  ECHO use strict;
>>src\tools\msvc\config.pl ECHO use warnings;
>>src\tools\msvc\config.pl ECHO our $config = {
>>src\tools\msvc\config.pl ECHO asserts ^=^> 0^,    ^# --enable-cassert
>>src\tools\msvc\config.pl ECHO ^# integer_datetimes^=^>1,
>>src\tools\msvc\config.pl ECHO ^# float4byval^=^>1,
>>src\tools\msvc\config.pl ECHO ^# float8byval^=^>0,
>>src\tools\msvc\config.pl ECHO ^# blocksize ^=^> 8,
>>src\tools\msvc\config.pl ECHO ^# wal_blocksize ^=^> 8,
>>src\tools\msvc\config.pl ECHO ^# wal_segsize ^=^> 16,
>>src\tools\msvc\config.pl ECHO ldap    ^=^> 1,
>>src\tools\msvc\config.pl ECHO nls     ^=^> '%DEPENDENCIES_BIN_DIR%\libintl',
>>src\tools\msvc\config.pl ECHO tcl     ^=^> undef,
IF %ARCH% == X64 (>>src\tools\msvc\config.pl ECHO perl    ^=^> '%PERL64_PATH%',   )
IF %ARCH% == X86 (>>src\tools\msvc\config.pl ECHO perl    ^=^> '%PERL32_PATH%',   )
IF %ARCH% == X64 (>>src\tools\msvc\config.pl ECHO python  ^=^> '%PYTHON64_PATH%', )
IF %ARCH% == X86 (>>src\tools\msvc\config.pl ECHO python  ^=^> '%PYTHON32_PATH%', )
>>src\tools\msvc\config.pl ECHO openssl ^=^> '%DEPENDENCIES_BIN_DIR%\openssl',
>>src\tools\msvc\config.pl ECHO uuid    ^=^> '%DEPENDENCIES_BIN_DIR%\uuid',
>>src\tools\msvc\config.pl ECHO xml     ^=^> '%DEPENDENCIES_BIN_DIR%\libxml2',
>>src\tools\msvc\config.pl ECHO xslt    ^=^> '%DEPENDENCIES_BIN_DIR%\libxslt',
>>src\tools\msvc\config.pl ECHO iconv   ^=^> '%DEPENDENCIES_BIN_DIR%\iconv',
>>src\tools\msvc\config.pl ECHO zlib    ^=^> '%DEPENDENCIES_BIN_DIR%\zlib',
>>src\tools\msvc\config.pl ECHO icu     ^=^> '%DEPENDENCIES_BIN_DIR%\icu'
>>src\tools\msvc\config.pl ECHO ^};
>>src\tools\msvc\config.pl ECHO 1^;

type .\src\tools\msvc\config.pl

perl src\tools\msvc\build.pl || GOTO :ERROR

GOTO :DONE
:ERROR
ECHO Failed with error #%errorlevel%.
EXIT /b %errorlevel%
:DONE
ECHO Done.
