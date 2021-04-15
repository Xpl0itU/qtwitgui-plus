@echo off

IF EXIST wiitdb.xml goto end

IF EXIST wiitdb.zip goto unz

echo Downloading WiiTDB.zip
curl http://wiitdb.com/wiitdb.zip --output .\wiitdb.zip --silent

:unz
unzip -o wiitdb.zip

:end
