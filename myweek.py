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

        
    def get_duration(self):        
        ''' calculate total duration of this week in hours '''

        # to get the year, "RunDate" does not work for weeks that start late Dec
        # RootFiles does not work with a constant number, because there are 1-digit cycles (cycle_9, not cycle_09)
        # getting cycle from "ProductionCycle" does not work, because there is a mismatch (e.g. ProdC says 8, but in the root path it's 9)
        # --> simply try shooting for 2 digits, and if that yields an empty table, try for 1 digit
        sql = "select \"RunNumber\", \"Duration\" from \"ValidRuns\" where substring(\"RootFiles\", 65, 4) = '" + self.year + "' and \"Groups\" = '" + self.group + "';"
        dat = sqlio.read_sql_query(sql, self.conn)
        if len(dat) == 0:
            # maybe the cycle is 1 digit, try again
            sql = "select \"RunNumber\", \"Duration\" from \"ValidRuns\" where substring(\"RootFiles\", 64, 4) = '" + self.year + "' and \"Groups\" = '" + self.group + "';"
            dat = sqlio.read_sql_query(sql, self.conn)
            if len(dat) == 0:
               print 'Week', self.week, 'does not exist!'
               sys.exit()
        # these run boundaries are "official" original ones
        self.rmin = dat['RunNumber'].min()
        self.rmax = dat['RunNumber'].max()
        # these run boundaries will be updated as the run is stretched
        self.runs[0] = self.rmin 
        self.runs[1] = self.rmax
        self.duration = dat['Duration'].sum() * 1. / 60 / 60
        print 'Runs:', self.runs[0], '-', self.runs[1]
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

        # close connection to the DB
        self.conn.close()
        self.conn = None


    def qe_launch(self):
        ''' create a .sh file which launches the QE calculation '''
        ## create output folder if it doesn't exist yet
        if not os.path.exists('qe_output'):
            os.mkdir('qe_output')

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


    def copy_last(self):
        ''' Copy the QE values from the last week (done when stretching is not enough)'''

        ##            

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
