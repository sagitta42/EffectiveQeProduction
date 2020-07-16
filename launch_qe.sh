#!/bin/bash
# input: week
week=$1
## create a list of runs for this week (stretch if needed) and a qe launching submission script
python make_week.py $week

## launch qe
exe="launch_qe_${week}.sh" # executable
cdr="condor_qe_${week}.sub" # condor system
if [ -e $cdr ]
then
    echo '---'
    cat $exe
    echo '---'
    ## bsub system
    #nohup bxsubmitter $exe
    #nohup ./$exe
    #rm -f $exe
    ## condor system
    condor_submit -spool -name sn-01.cr.cnaf.infn.it $cdr
else
    echo "QE is NOT launched"
fi
