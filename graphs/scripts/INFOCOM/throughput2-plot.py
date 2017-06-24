import pandas as pd
from common import *

# Config
N_SEGMENTS = 50
N_RUN = 1
RESULT_PREFIX = '../../../results/generated/'
RESULT_NAME = 'dash-trace_{0}Mbps.txt'
CAPACITIES = [2.5, 5, 7.5, 10, 12.5, 15, 17.5, 20]
REPRESENTATIONS = ['3_1080']

#import matplotlib.lines as mlines
#from scipy.interpolate import interp1d


def read_quality ( path, representation ):
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

    total_segments = a.groupby(level=['SegmentNumber']).sum().sum()
    segments_rep = a.loc[0:N_SEGMENTS,[representation]].sum()

    return 100*segments_rep/total_segments


fig, ax = newfig(0.32)

MARKERS = ['>', '*', 'o' ]
COLORS = np.linspace(0.5,0.9,len(REPRESENTATIONS))

for i,rep in enumerate(REPRESENTATIONS):
    throughput_ncn = np.array([])
    throughput_ndn = np.array([])

    for r in range(N_RUN):
        for c in CAPACITIES:
            throughput_ncn = np.append(throughput_ncn, read_quality ( RESULT_PREFIX + str(r+1) + '/netcodndn/' + (RESULT_NAME).format(c), rep ))
            throughput_ndn = np.append(throughput_ndn, read_quality ( RESULT_PREFIX + str(r+1) + '/ndn/' + (RESULT_NAME).format(c), rep ))

    ax.plot(CAPACITIES, throughput_ncn, label='NetCodNDN-DASH', color=plt.get_cmap('Blues')(COLORS[i]), markersize = 4, marker=MARKERS[i]) #
    ax.plot(CAPACITIES, throughput_ndn, label='NDN-DASH', color=plt.get_cmap('Reds')(COLORS[i]), markersize = 5, marker=MARKERS[i]) # 

ax.legend(loc='best')

ax.set_ylabel("Segments delivered at 1080p [%]")
ax.set_ylim([0,100])

ax.set_xlabel("Routers link capacity [Mbps]")
#ax.set_xlim([2.5,17.5])
ax.set_xticks(CAPACITIES)

# Save the figure
savefig('segments_delivered')

#Show the figure
plt.show()