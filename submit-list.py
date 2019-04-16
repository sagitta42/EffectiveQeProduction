import sys
import os

def submitlist(listpath, input_folder, macro):
	'''
	listpath: path to the list of weeks 
	e.g. blah/mylist.list, each line YYYY_MMM_DD

	input_folder: path to a folder which contains actual lists with run paths
	e.g. blah/myfolder/, inside of which there are files YYYY_MMM_DD.list

	Qe format:
	./qe_calculation week input_folder output_folder
	
	week: YYYY_MMM_DD
	input_folder: folder in which YYYY_MMM_DD.list is stored
	output_folder: where the outputs *_QePerRun.txt, *_RunInfo.txt and *_QE.txt will go

	Generate submissions for every YYYY_MMM_DD.list in the listname for given macro and output folder

	Output:
	an empty folder mylist_macro/ for future output
	submission file mylist_macro_submission.sh each line of which is 
	bsub -q borexino_physics -e mylist_macro/YYYY_MMM_DD.err -o mylist_macro/YYYY_MMM_DD.log ./qe_calculation YYYY_MMM_DD blah mylist_macro
	'''

	print "Inputs:", listname
	print "Input folder:", input_folder
	print "Macro:", macro

	finp = open(listname)
	weeks = finp.readlines()
	weeks = [w.rstrip() for w in weeks]
	
	# output folder
	foldername = listpath.split('/')[-1].split('.')[0] + '_' + macro 
	os.mkdir(foldername)
	
	# submission file
	subname = foldername + '_submission.sh'
	sub = open(subname,'w')

	for w in weeks:
		print w
		print >> sub, 'bsub -q borexino_physics -e', foldername + "/" + w + '.err',\
			'-o', foldername + "/" + w  + '.log', './' + macro, w, input_folder, foldername
	
	sub.close()
	
	print "nohup bxsubmitter", subname, "&"


if len(sys.argv) == 4:
	listname = sys.argv[1] # list of week names only
	input_folder = sys.argv[2] # where to take the lists of weekly runs from
	macro = sys.argv[3]

	submitlist(listname, input_folder, macro)

else:
	print 'Syntax:'
	print 'python submit-list.py path/to/list_of_weeks.list path/to/input_folder macro'
