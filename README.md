# QE calculation

## Files

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

## Format
```
./qe_calculation YYYY_MMM_DD input_folder output_folder
./qe_calculation YYYY_MMM_DD input_folder output_folder run_min run_max
```

- week: YYYY_MMM_DD
- input_folder: folder in which a ```YYYY_MMM_DD.list``` file is stored (containing paths to run files in that week)
- output_folder: where the outputs ```*_QE.txt``` and ```_Week.txt``` will be saved
- run_min and run_max: optional boundaries of the week, used when the week is stretched to ignore the runs that don't belong to the week when looking for the first run

Output:
- ```output_folder/YYYY_MMM_DD_QE.txt```: QE file that will be uploaded to the database
- ```output_folder/YYYY_MMM_DD_Week.txt```: extra information in each week e.g. ratio of cone to no cone etc.

Note: the folder output_folder should already exist.


## Structure

```
qe_calculation
|      |   |
|      | QeSample
|      | |
|      Run
|      | |
Database |
         Technical
```

# Discarding PMTs

```assign_first_week.py```: since there is no week before the first week, we assign to disabled and discarded PMTs the average QE of the enabled PMTs

## Python

1. ```submit-list.py```: generating a submission file for bxsubmitter given a list of weeks and input folder (containing lists of runs for each week)

Format:
``` python submit-list.py list_of_weeks.list input_folder macro ```	
* ```list_of_weeks.list```: list of YYYY_MMM_DD on which to launch the calculation
* ```input_folder```: folder in which corresponding ```YYYY_MMM_DD.list``` files are stored
* ```macro```: name of the macro (qe_calculation)

Output:
* empty folder ```list_of_weeks_macro/``` for future QE output
* ```list_of_weeks_macro_submission.sh``` with submission lines for each week in the list of weeks


2. ```qe_merge.py```

merge the QE results from each week into one table, to later plot using python

Format:

```
qe_merge.py folder
```

Output: ```folder.qetotal.csv```
