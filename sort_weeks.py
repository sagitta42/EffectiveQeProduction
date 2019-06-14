import sys
import pandas as pd

def sortw(lst):
    ''' sort weeks in chronological order '''

    ## list of weeks to sort
    df = pd.read_csv(lst, names=['week'])
    df['date'] = [pd.to_datetime(w.replace('_','-')) for w in df['week']]
    df = df.sort_values('date')
    df['week'].to_csv(lst.split('.')[0] + '_sorted.list', header=False, index=False)

sortw(sys.argv[1])    
