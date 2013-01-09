#!/bin/bash

logdir="/globalhome/log/"

host=`hostname -s`
filenameFrom=$logdir$host"_StationTest.csv"
filenameToo=$logdir$host"_StationTestHistory.csv"
cat $filenameFrom >> $filenameToo

# Check hardware in CheckLevel 2, do all tests
checkHardware.py -l 2

# Add test results too PVSS
pvssFile=$logdir$host"_StationTest_PVSS.log"
setObjectState $pvssFile

# Make old station log files
makeStationLogFile.py