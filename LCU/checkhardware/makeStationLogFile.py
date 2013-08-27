#!/usr/bin/python
# format converter
# from csv file to log file
#
# P.Donker

import sys
import os
from time import gmtime

libPath = libPath = '/opt/stationtest/lib'
sys.path.insert(0, libPath)

from general_lib import *

f = open("/opt/stationtest/checkHardware.conf", 'r')
data = f.readlines()
f.close()
logdir = ''
for line in data:
    if line.find('log-dir-global') != -1:
        key, logdir = line.strip().split('=')
        if os.path.exists(logdir):
            break
    if line.find('log-dir-local') != -1:
        key, logdir = line.strip().split('=')
        if not os.path.exists(logdir):
            print "No log dir found"
            sys.exit(-1)


testfilename = '%s_StationTest.csv' %(getHostName())

fullFilename = os.path.join(logdir, testfilename)

try:        
    f = open(fullFilename, 'r')
    testdata = f.readlines()
    f.close()
except:
    print "Can not open/read %s" %(fullFilename)
    sys.exit(-1)

fileinfo = os.stat(fullFilename)
log = cStationLogger(logdir, gmtime(fileinfo[-1]))

last_c_summator = -1
for line in testdata:
    if line[0] == '#':
        continue
    keyinfo = dict()
    info = line.split(',')
    #print info
    date    = info[0]
    part    = info[1]
    partNr = '---'
    if info[2] != '---':
        partNr  = int(info[2])
    msgType = info[3].strip()
    for i in range(4,len(info)):
            if info[i].find('=') != -1:
                key, valstr = info[i].split('=')
                vallist = valstr.split()
                if len(vallist) == 1:
                    keyinfo[key] = vallist[0]
                elif len(vallist) > 1:
                    keyinfo[key] = vallist
            else:
                keyinfo[info[i]] = '-'
    
    if part == 'TBB':
        if msgType == 'VERSION':
            if partNr == '---':
                log.addLine('TBBver>: Sv=normal  Pr=normal , Version Error TBBdriver=%s tbbctl=%s' %\
                           (keyinfo['TBBDRIVER'], keyinfo['TBBCTL']))
            else:
                log.addLine('TBBver>: Sv=normal  Pr=normal , Board=%d Version Error TP=%s MP=%s' %\
                           (partNr, keyinfo['TP'], keyinfo['MP']))
        elif msgType == 'MEMORY':
            log.addLine('TBBmem>: Sv=normal  Pr=normal , Board=%d Memory Error' %\
                           (partNr))
    
    if part == 'RSP':
        if msgType == 'VERSION':
            log.addLine('TBBver>: Sv=normal  Pr=normal , Board=%d, Version Error BP=%s AP=%s' %\
                       (partNr, keyinfo['BP'], keyinfo['AP']))
    
    if part == 'LBL':
        rcu = partNr * 2 
        if msgType == 'LOW_NOISE':
    		log.addLine('LBAmd1>: Sv=normal  Pr=normal , LBA Outer (LBL) low noise: RCU: (%d,%d)' %\
                        (partNr*2, partNr*2+1))
        elif msgType == 'HIGH_NOISE':
    		log.addLine('LBAmd1>: Sv=normal  Pr=normal , LBA Outer (LBL) high noise: RCU: (%d,%d)' %\
                        (partNr*2, partNr*2+1))
        elif msgType == 'OSCILLATION':
    		log.addLine('LBAosc1>: Sv=normal  Pr=normal , LBA Outer (LBL) large oscillation: RCU: (%d,%d)' %\
                        (partNr*2, partNr*2+1))                
        elif msgType == 'FAIL':
            if keyinfo.has_key('X'):
                log.addLine('LBAmd1>: Sv=normal  Pr=normal , LBA Outer (LBL) defect: RCU: %d factor: %3.1f' %\
                           (partNr*2, float(keyinfo['X'])))
            if keyinfo.has_key('Y'):
                log.addLine('LBAmd1>: Sv=normal  Pr=normal , LBA Outer (LBL) defect: RCU: %d factor: %3.1f' %\
                           (partNr*2+1, float(keyinfo['Y'])))               
        
        elif msgType == 'DOWN':
            log.addLine('LBAdn1>: Sv=normal  Pr=normal , LBA Outer (LBL) down: RCU: %d factor: %3.1f offset: %d' %\
                       (partNr*2, float(keyinfo['X']), int(keyinfo['Xoff'])))
            log.addLine('LBAdn1>: Sv=normal  Pr=normal , LBA Outer (LBL) down: RCU: %d factor: %3.1f offset: %d' %\
                       (partNr*2+1, float(keyinfo['Y']), int(keyinfo['Yoff'])))
        
        elif msgType == 'TOOLOW':
                log.addLine('LBAmd1>: Sv=normal  Pr=normal , LBA	levels to low!!!' )               
            
    if part == 'LBH':
        if msgType == 'LOW_NOISE':
    		log.addLine('LBAmd3>: Sv=normal  Pr=normal , LBA Inner (LBH) low noise: RCU: (%d,%d)' %\
                        (partNr*2, partNr*2+1))
        elif msgType == 'HIGH_NOISE':
    		log.addLine('LBAmd3>: Sv=normal  Pr=normal , LBA Inner (LBH) high noise: RCU: (%d,%d)' %\
                        (partNr*2, partNr*2+1))
        elif msgType == 'OSCILLATION':
    		log.addLine('LBAosc3>: Sv=normal  Pr=normal , LBA Inner (LBH) large oscillation: RCU: (%d,%d)' %\
                        (partNr*2, partNr*2+1))                
        elif msgType == 'FAIL':
            if keyinfo.has_key('X'):
                log.addLine('LBAmd3>: Sv=normal  Pr=normal , LBA Inner (LBH) defect: RCU: %s factor: %3.1f' %\
                           (partNr*2, float(keyinfo['X'])))
            if keyinfo.has_key('Y'):
                log.addLine('LBAmd3>: Sv=normal  Pr=normal , LBA Inner (LBH) defect: RCU: %s factor: %3.1f' %\
                           (partNr*2+1, float(keyinfo['Y'])))           
        
        elif msgType == 'DOWN':
            log.addLine('LBAdn3>: Sv=normal  Pr=normal , LBA Inner (LBH) down: RCU: %d factor: %3.1f offset: %d' %\
                       (partNr*2, float(keyinfo['X']), int(keyinfo['Xoff'])))
            log.addLine('LBAdn3>: Sv=normal  Pr=normal , LBA Inner (LBH) down: RCU: %d factor: %3.1f offset: %d' %\
                       (partNr*2+1, float(keyinfo['Y']), int(keyinfo['Yoff'])))
        
        elif msgType == 'TOOLOW':
                log.addLine('LBAmd3>: Sv=normal  Pr=normal , LBA	levels to low!!!' )               

    if part == 'HBA':
    	if msgType == 'LOW_NOISE':
    		log.addLine('HBAmd5>: Sv=normal  Pr=normal , Tile %d - RCU (%d,%d); low noise' %\
                        (partNr, partNr*2, partNr*2+1))
        
        elif msgType == 'HIGH_NOISE':
    		log.addLine('HBAmd5>: Sv=normal  Pr=normal , Tile %d - RCU (%d,%d); high noise' %\
                        (partNr, partNr*2, partNr*2+1))                
        
        elif msgType == 'OSCILLATION':
    		log.addLine('HBAosc>: Sv=normal  Pr=normal , Tile %d - RCU (%d,%d); Large oscillation' %\
                        (partNr, partNr*2, partNr*2+1))                
                
        elif msgType == 'C_SUMMATOR':
    		last_c_summator = partNr
    		log.addLine('HBAmdt>: Sv=normal  Pr=normal , Tile %d - RCU %d; Broken. No modem communication' %\
                                (partNr, partNr*2+1))
        elif last_c_summator != partNr and msgType == 'FAIL':
            for elem_nr in range(1,17,1):

                if keyinfo.has_key('M%d' %(elem_nr)):
                    val = keyinfo.get('M%d' %(elem_nr))
                    log.addLine('HBAmdt>: Sv=normal  Pr=normal , Tile %d - RCU %d; Element %s Broken. No modem communication : (%s)' %\
                                (partNr, partNr*2+1, elem_nr, val))

                if keyinfo.has_key('X%d' %(elem_nr)):
                    val = keyinfo.get('X%d' %(elem_nr))

                    log.addLine('HBAmd5>: Sv=normal  Pr=normal , Tile %d - RCU %d; Element %d Broken. RF-signal to low : (Factor = %s, CtrlWord = 128)' %\
                               (partNr, partNr*2, elem_nr, val[0]))
                    log.addLine('HBAmd5>: Sv=normal  Pr=normal , Tile %d - RCU %d; Element %d Broken. RF-signal to low : (Factor = %s, CtrlWord = 253)' %\
                               (partNr, partNr*2, elem_nr, val[3]))
                               
                if keyinfo.has_key('Y%d' %(elem_nr)):
                    val = keyinfo.get('Y%d' %(elem_nr))
                    log.addLine('HBAmd5>: Sv=normal  Pr=normal , Tile %d - RCU %d; Element %d Broken. RF-signal to low : (Factor = %s, CtrlWord = 128))' %\
                               (partNr, partNr*2+1, elem_nr, val[0]))
                    log.addLine('HBAmd5>: Sv=normal  Pr=normal , Tile %d - RCU %d; Element %d Broken. RF-signal to low : (Factor = %s, CtrlWord = 253))' %\
                               (partNr, partNr*2+1, elem_nr, val[3]))