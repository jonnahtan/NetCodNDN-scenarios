import pandas as pd
from common import *

# Config
N_SEGMENTS = 50
N_RUN = 1
RESULT_PREFIX = '../../../results/generated/'
RESULT_NAME = 'dash-trace_{0}Mbps.txt'
CAPACITIES = [5,10,15]

#import matplotlib.lines as mlines
#from scipy.interpolate import interp1d


def read_quality ( path ):
    f = pd.read_csv(path,
                    sep='\t',
                    dtype={ 'Time':np.float64,
                            'Node':str,
                            'SegmentNumber':np.int32,
                            'SegmentRepID':str,
                            'SegmentExperiencedBitrate(bit/s)':np.int32,
                            'BufferLevel(s)':object
                    })
    fx = pd.DataFrame(f,columns=['SegmentNumber','SegmentRepID'])
    a =  fx.groupby(['SegmentNumber','SegmentRepID']).size()
    a = 100 * a/a.groupby(level=['SegmentNumber']).sum()

    a = a.loc[0:N_SEGMENTS,['3_1080']]

    return a

throughput_ncn = pd.DataFrame()
throughput_ndn = pd.DataFrame()

for r in range(N_RUN):
    for c in CAPACITIES:
        print (RESULT_NAME).format(c)
        throughput_ncn = pd.concat([throughput_ncn, read_quality ( RESULT_PREFIX + str(r+1) + '/netcodndn/' + (RESULT_NAME).format(c) )], axis=1, ignore_index=True)
        throughput_ndn = pd.concat([throughput_ndn, read_quality ( RESULT_PREFIX + str(r+1) + '/ndn/' + (RESULT_NAME).format(c) )], axis=1, ignore_index=True)

#upper_limit_ncn = bitrate_ncn.max(axis=1)
#lower_limit_ncn = bitrate_ncn.min(axis=1)
#mean_ncn = bitrate_ncn.median(axis=1)

#upper_limit_ndn = bitrate_ndn.max(axis=1)
#lower_limit_ndn = bitrate_ndn.min(axis=1)
#mean_ndn = bitrate_ndn.median(axis=1)

throughput_ncn.index = throughput_ncn.index.droplevel(1)
throughput_ncn = throughput_ncn.reindex(range(0,49))
throughput_ncn = throughput_ncn.fillna(value=0.0)

throughput_ndn.index = throughput_ndn.index.droplevel(1)
throughput_ndn = throughput_ndn.reindex(range(0,49))
throughput_ndn = throughput_ndn.fillna(value=0.0)

# Simple plot
fig, ax = newfig(0.5)

MARKERS = ['>', '*', 'o' ]
COLORS = np.linspace(0.5,0.9,len(CAPACITIES))
i = 0
for c in CAPACITIES:
    ax.plot(throughput_ncn[i], label='NCN-{}Mbps'.format(c), color=plt.get_cmap('Blues')(COLORS[i]), markersize = 5, markevery=5, marker=MARKERS[i]) #marker=">",
    ax.plot(throughput_ndn[i], label='NDN-{}Mbps'.format(c), color=plt.get_cmap('Reds')(COLORS[i]), markersize = 5, markevery=5, marker=MARKERS[i]) #marker="*", 
    i += 1

ax.legend(loc='best')

ax.set_ylabel("Clients at 1080p [%]")
#ax.set_ylim([6.5,8.5])

ax.set_xlabel("Segment")

# Save the figure
savefig("test")

#Show the figure
plt.show()