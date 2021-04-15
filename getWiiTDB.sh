#!/bin/bash

if [ -e "./wiitdb.xml" ] ;then 
	echo "wiitdb.xml already exists"
else
	if [ ! -e "./wiitdb.zip" ] ;then
		echo "Downloading wiitdb.zip"
		curl 'http://wiitdb.com/wiitdb.zip' '--output' './wiitdb.zip' '--silent'
	fi
	unzip -o ./wiitdb.zip
fi
 
