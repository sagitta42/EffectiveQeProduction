# QE calculation

## Main script

The main script to launch QE calculation for a given week is ```launch_qe.sh```.

Format: ```./launch_qe.sh YYYY_MMM_DD```

The script calls a python code to compose a list of runs belonging to the given week (if "stretching" is needed, it includes runs from the weeks before and after). Then proceeds to launch a job to borexino_physics using a C++/ROOT macro that calculates QE.

### Examples ###

1. **Week that needs stretching**

```
$ ./launch_qe.sh 2017_Jul_16
Importing modules...
Week: 2017_Jul_16
Runs: 29112 - 29144
Duration: 64.38 hours
Minimum requirement: 126.0 hours
Stretching (if needed)...
+ 29111 ( Jul_09 ) --> 70.37 hours
+ 29151 ( Jul_23 ) --> 76.37 hours
+ 29110 ( Jul_09 ) --> 82.37 hours
+ 29152 ( Jul_23 ) --> 88.36 hours
+ 29109 ( Jul_09 ) --> 94.36 hours
+ 29165 ( Jul_23 ) --> 100.36 hours
+ 29108 ( Jul_09 ) --> 106.35 hours
+ 29166 ( Jul_23 ) --> 112.35 hours
+ 29107 ( Jul_09 ) --> 118.34 hours
+ 29167 ( Jul_23 ) --> 124.34 hours
+ 29106 ( Jul_09 ) --> 130.34 hours
Final scope: 29106 - 29167
Input: weeks/2017_Jul_16.list
Future output: qe_output/2017_Jul_16_QE.txt
bsub -q borexino_physics -e qe_output/2017_Jul_16.err -o qe_output/2017_Jul_16.log ./qe_calculation 2017_Jul_16 weeks qe_output 29112 29144
Job <46170874> is submitted to queue <borexino_physics>.
```

2. **Normal week**

```
$ ./launch_qe.sh 2016_Nov_27
Importing modules...
Week: 2016_Nov_27
Runs: 27759 - 27797
Duration: 163.86 hours
Minimum requirement: 126.0 hours
Stretching (if needed)...
Final scope: 27759 - 27797
Input: weeks/2016_Nov_27.list
Future output: qe_output/2016_Nov_27_QE.txt
bsub -q borexino_physics -e qe_output/2016_Nov_27.err -o qe_output/2016_Nov_27.log ./qe_calculation 2016_Nov_27 weeks qe_output 27759 27797
Job <46170569> is submitted to queue <borexino_physics>.
```

3. **Current week that needs to be stretched -> need to wait for the next week**

```
$ ./launch_qe.sh 2019_Jun_02
Importing modules...
Week: 2019_Jun_02
Runs: 32691 - 32706
Duration: 67.35 hours
Minimum requirement: 126.0 hours
Stretching (if needed)...
+ 32676 ( May_26 ) --> 73.34 hours
Next week not present! Please launch me later!
QE is NOT launched
```

(note: relies on the information in ValidRuns; "next week does not exist" means is not in ValidRuns)

4. **Non existent week**

$ ./launch_qe.sh 2016_Jun_35
Importing modules...
Week: 2016_Jun_35
Week 2016_Jun_35 does not exist!
QE is NOT launched

(note: relies on ValidRuns)




## C++/ROOT macro

File  | Description
------------- | -------------
```qe_calculation.cc``` | main (compiled with "make")
```B900.txt``` | list of B900 PMTs (as in Echidna)
```list_of_all_hole_labels_cone.txt``` | list of hole labels and cone information
```profile_channel_to_hole/``` | txt files for each profile with channel to hole mapping
```reference_channels/``` | txt files for each profile with channels that are non-ordinary (laser, trigger and CNGS ref. channels)
```modules/``` | included in ```qe_calculation.cc```

Note, hole label and cone information, profile mapping and reference channels information is taken from the database. This is not a huge amount of information which also does not change in time, so I decided to save it offline and read from the txt files to avoid connecting to different databases many times. In principle the macro can read this information directly from the DB. For now it is equivalent, with the only difference that if we have a new profile, either an additional txt file for profile 26 has to be created, or I should move this to the database to make it more universal.

### Modules

Modules  | Description
----------------------- | ----------------
```QeSample.cc``` and ```.hh``` | calculating QE on a given list of runs (currently weekly basis, but does not intrinsically depend on each week, calculates QE on any given range of runs)
```Run.cc``` and ```.hh``` | collecting hits from 14C in each run
```Database.cc``` and ```.hh``` | connecting to databases and reading tables for disabled channels and last valid event number
```Technical.C``` | other things that are packed to this separate file not to distract in the main code

### Format
```
./qe_calculation YYYY_MMM_DD input_folder output_folder
./qe_calculation YYYY_MMM_DD input_folder output_folder run_min run_max
```

- week: YYYY_MMM_DD
- input_folder: folder in which a ```YYYY_MMM_DD.list``` file is stored (containing paths to run files in that week)
- output_folder: where the outputs ```*_QE.txt```
- run_min and run_max: optional boundaries of the week, used when the week is stretched to ignore the runs that don't belong to the week when looking for the first run

Output: ```output_folder/YYYY_MMM_DD_QE.txt```: QE file that will be uploaded to the database

Note: the folder output_folder should already exist.


### Structure

```
qe_calculation
|      |   
| QeSample 
| |    | | 
| |    Run 
| |    | |
| |    | Technical
Database
```

## Python script

1. ```make_week.py```: create a list of runs corresponding to the given week, "stretch" it if needed
2. ```myweek.py```: class used by make_week.py

Format:
``` python make_week.py YYYY_MMM_DD ```	

Output:
* on the first run, empty folder ```weeks/``` where the input lists are saved
* list of runs corresponding to the week (```weeks/YYYY_MMM_DD.list```)
* on the first run, empty folder ```qe_output/``` where the output files YYYY_MMM_DD_QE.txt will be saved later (as well as long and err files of the submission)
* a temporary script ```launch_qe_YYYY_MMM_DD.sh``` which inside has a call ```./qe_calculation YYYY_MMM_DD weeks qe_output rmin rmax``` where ```rmin``` and ```rmax``` are the first and last run of the week (needed if the week is "stretched", to give the actual "boundaries" of the week; optional when the week is not "stretched")
