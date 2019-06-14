import pandas as pd
import sys
import os

# folder from which we take the QE files
folder = 'c19_all_qe/'
# output folder for after discarding
outfolder = 'c19_all_qe_discarded/'
# threshold for single PMT statistics
thr = 150000
# list of weeks on which we are going to orient in time to assign "value from prev week"
alllist = 'c19_all_sorted.list'
allweeks = [w.rstrip() for w in open(alllist).readlines()]

def discard(week):
    ## open the week's file
    df = pd.read_csv(folder + week + '_QE.txt')
    df = df.set_index('HoleLabel', drop=False)
    # mark discarded PMTs
    dis = df[(df['QeError']/df['Qe']*100 > 2) & (df['Nevents'] < 150000)]
    df.at[dis.index, 'Status'] = 3 # 1 - enabled, 2 - disabled, 3 - discarded
    # database bug: if a PMT claims it's enabled, but sees 0 hits, it's actually disabled
    bug = df[ (df['NhitsCollected'] == 0) & (df['Status'] == 1) ]
    df.at[bug.index, 'Status'] = 2
    # disabled and discarded PMTs
    dfd = df[df['Status'] > 1]
#    print 'this', df.loc[-1153]['Qe']

    ## look for dis. pmts in the last week
    prev = allweeks[allweeks.index(week) - 1]
    dfp = pd.read_csv(outfolder + prev + '_QE.txt')
    dfp = dfp.drop_duplicates('HoleLabel')
    # ONLY HOLES OF INTEREST
    dfp = dfp[dfp['HoleLabel'].isin(dfd.index)]
    dfp = dfp.set_index('HoleLabel')
#    print 'prev', prev, dfp.loc[-1153]['Qe']
    df.at[dfd.index, 'Qe'] = dfp['Qe']
    df.at[dfd.index, 'QeError'] = dfp['QeError']
    df.at[dfd.index, 'QeWoc'] = dfp['QeWoc']
    df.at[dfd.index, 'QeWocError'] = dfp['QeWocError']

    # save new QE
    df.to_csv(outfolder + week + '_QE.txt', index=False)


    
def put100():
    ''' assign default value to disabled and discarded PMTs in 2007 May 13 (first week ever)    '''

    ## original file
    df = pd.read_csv(folder + '2007_May_13_QE.txt')
    # mark discarded PMTs
    dis = df[(df['QeError']/df['Qe']*100 > 2) & (df['Nevents'] < 150000)]
    df.at[dis.index, 'Status'] = 3 # 1 - enabled, 2 - disabled, 3 - discarded
    # database bug: if a PMT claims it's enabled, but sees 0 hits, it's actually disabled
    # should not appear after the Run.cc check
#    bug = df[ (df['NhitsCollected'] == 0) & (df['Status'] == 1) ]
#    df.at[bug.index, 'Status'] = 2
    # mean of enabled PMTs
    mn = df[df['Status'] == 1].mean()['Qe']
    mnwoc = df[df['Status'] == 1].mean()['QeWoc']
    # disabled and discarded PMTs
    dfd = df[df['Status'] > 1]

    # assign disabled PMTs
    df.at[dfd.index, 'Qe'] = round(mn,6)
    df.at[dfd.index, 'QeError'] = 0
    df.at[dfd.index, 'QeWoc'] = round(mnwoc,6)
    df.at[dfd.index, 'QeError'] = 0

    # save new QE
    df.to_csv(outfolder + '2007_May_13_QE.txt', index=False)


## create the output folder and put the first week there to begin
os.mkdir(outfolder)    
put100()    
## process all weeks, but ignore the first one (has no prev week)
for w in allweeks[1:]:
    print w
    discard(w)    
