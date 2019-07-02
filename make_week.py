from myweek import *

# our threshold to reach 200 000 14C events (corr. to ~50% of full week)
THR = 7*24*0.75 # 75% of a full week in hours

def make_week(week):
    ''' Create a file weeks/week.list with the list of runs belonging to that week
    '''
  
    # initialize week
    myweek = Week(week)       
    
    # calculate total duration
    myweek.get_duration()
    
    print 'Minimum requirement:', THR, 'hours'

    # if needed, stretch week
    print 'Stretching (if needed)...'

    while(myweek.duration < THR):
        myweek.stretch()

        ## if we went out of boundaries in both prev and next, stop no matter what duration
        if (myweek.pn_cnt[0] == 2) and (myweek.pn_cnt[1] == 2):
            print '! Merged full prev and next week but still do not reach threshold !'
            print 'Launching QE anyway. Cancel job if you disagree'
            myweek.duration = 100000


    print 'Final scope:', myweek.runs[0], '-', myweek.runs[1]            

    # save run paths
    myweek.save_paths()

    # create QE launching .sh file
    myweek.qe_launch()


make_week(sys.argv[1])    
