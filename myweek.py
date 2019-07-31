print 'Importing modules...'
import sys
import os
import pandas as pd
import pandas.io.sql as sqlio
import psycopg2


class Week():
    ''' Class for managing the week and its duration and runs '''

    def __init__(self, week):
        self.week = week # e.g. 2017_Jul_16
        self.year = week.split('_')[0] # e.g. 2017
        self.group = week.split(self.year + '_')[1] # e.g. Jul_16
        self.conn = psycopg2.connect("host='bxdb.lngs.infn.it' dbname='bx_runvalidation' user=borex_guest")
        self.runs = [0,0] # run boundaries (min and max) that will be updated as the week is stretched
        # rmin and max which belong to this week "officially" (need to remember if stretched)
        self.rmin = 0 
        self.rmax = 0
        self.duration = 0
        self.stretch_idx = 0 # 0 - prev, 1 - next
        self.pn_group = [self.group, self.group] # keeping track of current prev and next groups, not do go out 2 weeks before or after
        self.pn_cnt = [0,0] # counting number of prev and next groups, if 2, we stop
        print 'Week:', self.week
        
        ## create output folder if it doesn't exist yet
        if not os.path.exists('qe_output'):
            os.mkdir('qe_output')


    def get_runs(self):
        ''' Get min and max run from storage. These runs are not necessarily valid, but this is the first step '''
        # list of files from storage
        path = '/storage/gpfs_data/borexino/rootfiles/cycle_19/' + self.year + '/' + self.group
        if not os.path.exists(path):
            print 'Week', self.week, 'does not exist!'
            sys.exit()

        runs = [r for r in os.listdir(path) if '.root' in r]
        # integers
        runs = [int(r.split('Run')[1].split('_')[0]) for r in runs]
        self.rmin = min(runs)
        self.rmax = max(runs)
        print 'On storage:', self.rmin, '-', self.rmax, '(', len(runs), 'runs )'
        
        # getting week runs info trough ValidRuns sucks because there is no year info and it has to be extracted from the RootFiles path
        # but depending on the cycle it's different, and if during the same week there are paths in different cycles, ......
        # therefore, we just got the info about runs from storage, now get only valid ones
        sql = "select \"RunNumber\" from \"ValidRuns\" where \"RunNumber\" >= " + str(self.rmin) + " and \"RunNumber\" <=" + str(self.rmax) + ";"
        dat = sqlio.read_sql_query(sql, self.conn)
        
        if len(dat) == 0:
            print 'Week', self.week, 'has no runs!'
            sys.exit()
        
        # these run boundaries are "official" original boundaries
        self.rmin = dat['RunNumber'].min()
        self.rmax = dat['RunNumber'].max()
        print 'Valid:', self.rmin, '-', self.rmax, '(', len(dat), 'runs )'
        
        # these run boundaries will be updated as the run is stretched
        self.runs[0] = self.rmin 
        self.runs[1] = self.rmax


        
    def get_duration(self):        
        ''' calculate total duration of this week in hours '''

        sql = "select \"Duration\" from \"ValidRuns\" where \"RunNumber\" >= " + str(self.rmin) + ' and ' + "\"RunNumber\" <= " + str(self.rmax) + ";"
        dat = sqlio.read_sql_query(sql, self.conn)

        self.duration = dat['Duration'].sum() * 1. / 60 / 60
        print 'Duration:', round(self.duration, 2), 'hours'


    def stretch(self):
        ''' make one stretching step: append a run from the week before or after this week one by one'''
            
        if self.pn_cnt[self.stretch_idx] >= 2: return
        
        ## get info of the run in the past or future from our week
        sql = "select \"RunNumber\", \"Duration\", \"Groups\", \"RootFiles\" from \"ValidRuns\" where \"RunNumber\" = "
    
        if self.stretch_idx == 0:
            # stretch back
            sql += "(select max(\"RunNumber\") from \"ValidRuns\" where \"Groups\" != 'filling' and \"RunNumber\" < " + str(self.runs[0]) + ");"
        else:
            # stretch forward
            sql += "(select min(\"RunNumber\") from \"ValidRuns\" where \"Groups\" != 'filling' and \"RunNumber\" > " + str(self.runs[1]) + ");"
           
        dat = sqlio.read_sql_query(sql, self.conn)
        # if there is no next week
        if len(dat) == 0:
            print 'Next (prev?) week not present! Please launch me later!'
            sys.exit()

#        print dat
        group = dat['Groups'].loc[0] # e.g. Jul_09

        ## do not use if there are two or more unvalid weeks in between
        # extract year from path
        year = dat['RootFiles'].loc[0].split('cycle')[1].split('/')[1]
        group = year + '_' + group
        # time difference between this week and last valid week: 7 just means normal, 14 means one week is skipped
        dt = abs(weektime(group) - weektime(self.week))
        if(int(dt.days) > 14):
            print group, ': time difference is more than two weeks! Not stretching'
            self.pn_cnt[self.stretch_idx] = 3
            return

        # count groups to avoid stretching farther than one week behind or ahead
        if self.pn_group[self.stretch_idx] != group:
            self.pn_group[self.stretch_idx] = group
            self.pn_cnt[self.stretch_idx] += 1

        if self.pn_cnt[self.stretch_idx] < 2:
            ## add duration of this run and stretch the week
            self.duration += dat['Duration'].loc[0] * 1. / 60 / 60
            self.runs[self.stretch_idx] = dat['RunNumber'].loc[0]
            print '+', self.runs[self.stretch_idx], '(', group, ') -->', round(self.duration,2), 'hours'

    
    def stretch_step(self):
        ## update stretch index (if this time we stretched prev, next time will stretch next and vice versa)
        self.stretch_idx = (self.stretch_idx + 1) % 2


    def save_paths(self):
        ''' save run paths '''
        
        # get paths from DB (bxmaster paths)            
        sql = "select \"RootFiles\" from \"ValidRuns\" where \"RunNumber\" >=" + str(self.runs[0]) + " and \"RunNumber\" <= " + str(self.runs[1]) + " order by \"RunNumber\";"
        dat = sqlio.read_sql_query(sql, self.conn)
        dat['cnaf'] = dat['RootFiles'].apply(cnaf_path)
        if not os.path.exists('weeks'):
            os.mkdir('weeks')
        outname = 'weeks/' + self.week + '.list'            
        dat['cnaf'].to_csv(outname, header=False, index=False)
        print 'Input:', outname



    def qe_launch(self):
        ''' create a .sh file which launches the QE calculation '''

        outname = 'launch_qe_' + self.week + '.sh'
        out = open(outname, 'w')
        ## format: ./qe_caltulation YYYY_MMM_DD intput_folder output_folder rmin rmax
        # rmin and rmax are optional, needed if the week is stretched
        print >> out, 'bsub -q borexino_physics -e qe_output/' + self.week + '.err', '-o qe_output/' + self.week + '.log', './qe_calculation', self.week, 'weeks qe_output', self.rmin, self.rmax
        out.close()
        make_executable(outname)
        print 'Future output: qe_output/' + self.week + '_QE.txt'


    def massive_sub(self):
        ''' append this week to the total submission '''
        ## create output folder if it doesn't exist yet
        if not os.path.exists('qe_output'):
            os.mkdir('qe_output')

        outname = 'launch_qe_all.sh'
        out = open(outname, 'a')
        ## format: ./qe_caltulation YYYY_MMM_DD intput_folder output_folder rmin rmax
        # rmin and rmax are optional, needed if the week is stretched
        print >> out, 'bsub -q borexino_physics -e qe_output/' + self.week + '.err', '-o qe_output/' + self.week + '.log', './qe_calculation', self.week, 'weeks qe_output', self.rmin, self.rmax
        out.close()
        print 'Future output: qe_output/' + self.week + '_QE.txt'


    def assign_prev(self):
        ''' Copy the QE values from the last week (done when stretching is not enough)'''

        print 'Assigning all values from previous week...'            

        ## last week QE            
        conn_qe = psycopg2.connect("host='bxdb.lngs.infn.it' dbname='bx_calib' user=borex_guest")
        # if prev week has two profiles i.e. profile change in the middle of the week, this query will pick up only the part with the last profile which is fine with us
        sql = "select  * from \"QuantumEfficiencyNew\" where \"RunNumber\" = (select max(\"RunNumber\") from \"QuantumEfficiencyNew\" where \"RunNumber\" < " + str(self.rmin) + ");"
        dat = sqlio.read_sql_query(sql, conn_qe)
        print 'Prev week:', dat['DST_index'].unique()

        ## find out which profile(s) this week corresponds to
        dfprof = pd.read_csv('ProfileStartRun.csv')
        dfprof = dfprof.set_index('RunNumber', drop=False)
        # profile of first run
        pr1 = dfprof.loc[:self.rmin].iloc[-1]['ProfileID']
        profiles = [pr1]
        # last run
        pr2 = dfprof.loc[:self.rmax].iloc[-1]['ProfileID']
        if not pr2 == pr1: profiles.append(pr2)

        ## change the mapping info
        print 'Mapping info...'
        datfinal = pd.DataFrame()
        # if the profile of the previous week is the same as this one, simply change the run number
        prev_prof = dat['ProfileID'].unique()
        if (len(prev_prof) == 1) and (len(profiles) == 1) and (prev_prof[0] == profiles[0]):
            print 'Profile is the same as prev week, reassigning run number'
            datfinal = dat
            datfinal['RunNumber'] = self.rmin
        # otherwise need to replace the mapping            
        else:
            print 'Profile is different from last week, reassigning profiles and channel mapping'
            # get rid of the run and channel mapping info, pure QE info
            dat = dat.drop(['RunNumber','ChannelID'], axis=1)
            dat = dat.drop_duplicates('HoleLabel')
            # add mapping for each profile
            dfprof = dfprof.set_index('ProfileID')

            for pr in profiles:
                print '...', pr
                dtemp = dat.copy()
                # the first profile corresponds to the first run of this week; the later ones to the start of the profile
                run = self.rmin if pr == profiles[0] else dfprof.loc[pr]['RunNumber']
                dtemp['RunNumber'] = run
                dtemp['ProfileID'] = pr
                # channel mapping
                dfch = pd.read_csv('profile_channel_to_hole/profile' + str(pr) + 'map.txt', sep = ' ', names = ['ChannelID', 'HoleLabel', 'Cone'])
                dfch = dfch.sort_values('HoleLabel')
                dfch = dfch[dfch['HoleLabel'] != 0]
                # all hole labels
                allholes = list(pd.read_csv('list_of_all_hole_labels_cone.txt', sep = ' ', names = ['HoleLabel', 'Cone'])['HoleLabel'].unique())
                dfch = dfch.set_index('HoleLabel')                
#                print dfch.head(30)
#                print allholes
                # add missing holes, assign channel zero (means disabled)
                dfch = dfch.reindex(allholes) # sorted the same as the QE info, inc. in HoleLabel
                dfch = dfch.fillna(0)
                # add channel info
                dtemp = dtemp.set_index('HoleLabel', drop=False)
                dtemp['ChannelID'] = dfch['ChannelID'].astype(int) # index is the same, so gets assigned autoimatically
                datfinal = pd.concat([datfinal, dtemp], ignore_index=True)

        ## change week from prev to this one
        datfinal['DST_index'] = self.week

        ## correct order
        datfinal = datfinal[['RunNumber', 'DST_index', 'HoleLabel', 'ProfileID', 'ChannelID', 'Qe', 'QeError', 'Status', 'Noise', 'QeWoc', 'QeWocError', 'NhitsCollected', 'NhitsCone', 'NhitsBias', 'Nevents', 'NeventsFraction', 'NrunsFraction']] 
        datfinal = datfinal.sort_values(['HoleLabel','RunNumber'])
        ## save
        datfinal.to_csv('qe_output/' + self.week + '_QE.txt', index=False)

        conn_qe.close()
        conn_qe = None # is it like deleting a pointer?
                

    def __del__(self):
        # close connection to the DB
        self.conn.close()
        self.conn = None

        
        
        

##########
# helper functions            
##########


def cnaf_path(s):
    ''' replace bxmaster path by cnaf path '''
    r = s.replace('http://bxmaster-data.lngs.infn.it//bxstorage', '/storage/gpfs_data/borexino')
    # in ValidRuns the path is in the cycle it was validated in at  that moment; find and replace
    cycle = r.split('cycle_')[1].split('/')[0]
    r = r.replace('cycle_' + cycle, 'cycle_19')
    r = r.replace('c' + cycle, 'c19')
    return r


def make_executable(path):
    ''' chmod +x the file'''
    mode = os.stat(path).st_mode
    mode |= (mode & 0o444) >> 2    # copy R bits to X
    os.chmod(path, mode)


def weektime(group):
    ''' convert YYYY_MMM_DD to datetime '''
    return pd.to_datetime(group.replace('_','-'))
