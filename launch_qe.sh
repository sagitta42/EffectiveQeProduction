#!/bin/bash
# input: week
week=$1
# create a list of runs for this week (stretch if needed) and a qe launching submission script
python make_week.py $week
# launch qe
exe="launch_qe_${week}.sh"
if [ -e $exe ]
then
    cat $exe
    ./$exe
    rm -f $exe
else
    echo "QE is NOT launched"
fi    
