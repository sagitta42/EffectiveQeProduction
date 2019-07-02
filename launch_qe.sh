#!/bin/bash
# input: week
week=$1
# create a list of runs for this week (stretch if needed) and a qe launching submission script
python make_week.py $week
# launch qe
cat launch_qe_${week}.sh
./launch_qe_${week}.sh
rm -f launch_qe_${week}.sh
