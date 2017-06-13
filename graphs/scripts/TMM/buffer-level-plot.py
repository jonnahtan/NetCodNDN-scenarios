import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from scipy.interpolate import interp1d

N_RUN = 1

def read_bitrate ( path ):
    f = pd.read_csv(path, sep='\t')
    fx = pd.DataFrame(f,columns=['SegmentNumber','BufferLevel'])

    #g =  fx.groupby(['SegmentNumber'])
    #for name, group in g:
    #     group = group[(group.BufferLevel >= group.BufferLevel.quantile(.05)) & (group.BufferLevel <= group.BufferLevel.quantile(.95))]
    #    g.BufferLevel = group
    #g = g.mean()
    #print g

    b =  fx.groupby(['SegmentNumber'])['BufferLevel'].count()
    print b
    
    a =  fx.groupby(['SegmentNumber'])['BufferLevel'].quantile(.95)

    return a

bitrate_ncn = pd.DataFrame()
bitrate_ndn = pd.DataFrame()

for i in range(N_RUN):
    bitrate_ncn = pd.concat([bitrate_ncn, read_bitrate ( '../../results/generated/' + str(i+1) + '/netcodndn/dash-trace.txt' )], axis=1, ignore_index=True)
    bitrate_ndn = pd.concat([bitrate_ndn, read_bitrate ( '../../results/generated/' + str(i+1) + '/ndn/dash-trace.txt' )], axis=1, ignore_index=True)

upper_limit_ncn = bitrate_ncn.max(axis=1)
lower_limit_ncn = bitrate_ncn.min(axis=1)
mean_ncn = bitrate_ncn.median(axis=1)

upper_limit_ndn = bitrate_ndn.max(axis=1)
lower_limit_ndn = bitrate_ndn.min(axis=1)
mean_ndn = bitrate_ndn.median(axis=1)

fig,ax = plt.subplots()

ax.fill_between(upper_limit_ncn.index.values, upper_limit_ncn, lower_limit_ncn, color='#52be80', alpha=0.5)
ax.plot(mean_ncn, label='NetCodNDN', color='#145a32')

ax.fill_between(upper_limit_ndn.index.values, upper_limit_ndn, lower_limit_ndn, color='#5dade2', alpha=0.5)
ax.plot(mean_ndn, label='NDN', color='#21618c')

ax.legend(loc='lower right')

ax.set_ylabel("Buffer Level [s]")
#ax.set_ylim([7,8])

ax.set_xlabel("Segment")
#ax.set_xlim([0,30])

plt.show()
