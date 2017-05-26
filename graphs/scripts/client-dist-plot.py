import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from scipy.interpolate import interp1d
from matplotlib.ticker import FuncFormatter

def to_percent(y, position):
    return str(100 * y)

def read_clients ( path ):
    f = pd.read_csv(path, sep='\t')
    print f
    fx = pd.DataFrame(f,columns=['Source','Bandwidth'])
    fx['Bandwidth'] = fx['Bandwidth'].map(lambda x: x.rstrip('Mbps'))
    fx['Bandwidth'] = fx['Bandwidth'].apply(pd.to_numeric)
    a =  fx.groupby(['Source']).sum()
    #a = a.xs('1080', level='SegmentRepID')
    return a

clients = pd.DataFrame()
clients = read_clients ( '../../results/layer-generated.txt' )

fig,ax = plt.subplots()
ax.hist(clients.Bandwidth, bins=10, normed=True, color='blue')
ax.set_ylabel("Percentage of clients [%]")
ax.set_xlabel("Total Bandwidth [Mbps]")

formatter = FuncFormatter(to_percent)
plt.gca().yaxis.set_major_formatter(formatter)

plt.show()
