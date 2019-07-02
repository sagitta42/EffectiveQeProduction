from myweek import *

def make_week(week):
    ''' Create a file weeks/week.list with the list of runs belonging to that week
    '''
    
    # our threshold to reach 200 000 14C events (corr. to ~50% of full week)
    thr = 7*24*0.75 # 75% of a full week in hours
#    thr = 400 # test of running out of boundaries
    print 'Minimum requirement:', thr, 'hours'
  
    # initialize week
    myweek = Week(week)       
    
    # calculate total duration
    myweek.get_duration()

    # if needed, stretch week
    print 'Stretching (if needed)...'

    while(myweek.duration < thr):
        myweek.stretch()

        ## if we went out of boundaries in both prev and next, stop no matter what duration
        if (myweek.pn_cnt[0] == 2) and (myweek.pn_cnt[1] == 2):
            print 'Merged full prev and next week but still do not reach threshold'
            myweek.duration = 100000


    print 'Final scope:', myweek.runs[0], '-', myweek.runs[1]            

    # save run paths
    myweek.save_paths()

    # create QE launching .sh file
    myweek.qe_launch()


make_week(sys.argv[1])    
