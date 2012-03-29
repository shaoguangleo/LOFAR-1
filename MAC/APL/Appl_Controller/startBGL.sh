#!/bin/bash
# startBGL.sh jobName partition executable workingDir paramfile noNodes
#
# jobName
# executable      executable file (should be in a place that is readable from BG/L)
# workingDir      directory for output files (should be readable by BG/L)
# parameterfile   jobName.ps
# observationID   observation number
# noNodes         number of nodes of the partition to use
#
# start the job and stores the jobID in jobName.jobID
#
# all ACC processes expect to be started with "ACC" as first parameter

JOBNAME=$1
PARSET=$4
OBSID=$5

source /opt/lofar/bin/locations.sh

(
# Remove values which should be derived
sed -i 's/.*OLAP.CNProc.integrationSteps.*//' $PARSET
sed -i 's/.*OLAP.IONProc.integrationSteps.*//' $PARSET

# Convert keys where needed
/opt/lofar/bin/LOFAR/Parset.py $PARSET /opt/lofar/etc/OLAP.parset <(echo "$EXTRA_KEYS") > $IONPROC_PARSET

# Distribute the parset to the storage nodes
cp $IONPROC_PARSET $STORAGE_PARSET

# Inject the parset into the correlator
/opt/lofar/bin/commandOLAP.py -P $PARTITION parset $IONPROC_PARSET

) >> $LOGDIR/runParset.py.log 2>&1
