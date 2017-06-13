import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from scipy.interpolate import interp1d

N_RUN = 1

def read_bitrate ( path ):
    f = pd.read_csv(path, sep='\t')
    fx = pd.DataFrame(f,columns=['SegmentNumber','StallingTime(msec)'])
    a =  fx.groupby(['SegmentNumber'])['StallingTime(msec)'].mean()
    return a

bitrate_ncn = pd.DataFrame()
bitrate_ndn = pd.DataFrame()

for i in range(N_RUN):
    bitrate_ncn = pd.concat([bitrate_ncn, read_bitrate ( '../../results/generated/' + str(i) + '/netcodndn/dash-trace.txt' )], axis=1, ignore_index=True)
    bitrate_ndn = pd.concat([bitrate_ndn, read_bitrate ( '../../results/generated/' + str(i) + '/ndn/dash-trace.txt' )], axis=1, ignore_index=True)

upper_limit_ncn = bitrate_ncn.max(axis=1)
lower_limit_ncn = bitrate_ncn.min(axis=1)
mean_ncn = bitrate_ncn.mean(axis=1)

upper_limit_ndn = bitrate_ndn.max(axis=1)
lower_limit_ndn = bitrate_ndn.min(axis=1)
mean_ndn = bitrate_ndn.mean(axis=1)

fig,ax = plt.subplots()

ax.fill_between(upper_limit_ncn.index.values, upper_limit_ncn, lower_limit_ncn, color='#52be80', alpha=0.5)
ax.plot(mean_ncn, label='NetCodNDN', color='#145a32')

ax.fill_between(upper_limit_ndn.index.values, upper_limit_ndn, lower_limit_ndn, color='#5dade2', alpha=0.5)
ax.plot(mean_ndn, label='NDN', color='#21618c')

ax.legend(loc='lower right')

ax.set_ylabel("Seen bitrate [Mbps]")
#ax.set_ylim([7,8])

ax.set_xlabel("Segment")
#ax.set_xlim([0,30])

plt.show()
