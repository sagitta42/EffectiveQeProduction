import pandas as pd

def put100():
    ''' assign default value to disabled and discarded PMTs in 2007 May 13 (first week ever)    '''

    ## original file
    df = pd.read_csv('c19_all_qe/2007_May_13_QE.txt')
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
    df.to_csv('2007_May_13_QE.txt', index=False)


put100()    
