# QE calculation

## Main script

The main script to launch QE calculation for a given week is ```launch_qe.sh```.
Format: ```./launch_qe.sh YYYY_MMM_DD```
The script calls a python code to compose a list of runs belonging to the given week (if "stretching" is needed, it includes runs from the weeks before and after). Then proceeds to launch a job to borexino_physics using a C++/ROOT macro that calculates QE.

## C++/ROOT macro

File  | Description
------------- | -------------
```qe_calculation.cc``` | main (compiled with "make")
```B900.txt``` | list of B900 PMTs (as in Echidna)
```list_of_all_hole_labels_cone.txt``` | list of hole labels and cone information
```profile_channel_to_hole/``` | txt files for each profile with channel to hole mapping taken from DB (alternative: read directly from DB?)
```reference_channels/``` | txt files for each profile with channels that are non-ordinary (laser, trigger and CNGS ref. channels) (alternative: read directly from DB?)
```modules/``` | included in ```qe_calculation.cc```

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
