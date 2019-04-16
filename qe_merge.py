print 'Importing...'
import sys
import os
import pandas as pd
from numpy import sqrt

def mergedst(folder):
	### read
	print 'Reading', folder
	if folder[-1] == '/': folder = folder[:-1]
	ffiles = os.listdir(folder)
	ffiles = [f for f in ffiles if 'QE' in f]

	print 'Sorting chronologically...'
	# these files are not chronologically sorted
	# let's sort now, it's easier to sort first, and then merge, rather than sort a dataframe
	fdf = pd.DataFrame()
	fdf['Files'] = ffiles
	# to datetime does not work with "_" only with "-"
	fdf['Date'] = [pd.to_datetime(f.split('_QE')[0].replace('_', '-')) for f in ffiles]
	fdf = fdf.sort_values('Date')

	### merge
	print 'Merging...'
	dfres = pd.DataFrame()
	for f in fdf['Files']:
#	for f in ffiles:
		print f
		df = pd.read_csv(folder + '/' + f, sep=' ')
#		df['Dst'] = f.split('_QE')[0] # already present in new QE
		df['Date'] = pd.to_datetime(f.split('_QE')[0].replace('_','-'))

#		df['Dst'] = f.split('.')[0]
		dfres = pd.concat([dfres,df], ignore_index=True)

	### sort by date and hole label
#	print 'Sorting...'
#	dfres['Date'] = pd.to_datetime( [w.replace('_','-') for w in dfres['Dst']] )
#	dfres = dfres.sort_values(by=['Date','HoleLabel'])

	### save
	print 'Saving...'
	outname = folder + '.qetotal.csv'
	dfres.to_csv(outname, index=False)
#	dfres.to_csv(folder + '_corr.qetotal.csv', index=False)
	print '-->', outname
	print 'Saved'





## merge into one file
folder = sys.argv[1]
if folder[-1] == '/': folder = folder[:-1]
mergedst(folder)


