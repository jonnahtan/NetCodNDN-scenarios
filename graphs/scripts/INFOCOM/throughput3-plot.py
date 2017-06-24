import pandas as pd
from common import *

import matplotlib.colors as mcol

# Config
N_SEGMENTS = 50
N_RUN = 1
RESULT_PREFIX = '../../../results/generated/'
RESULT_NAME = 'dash-trace_{0}Mbps.txt'
CAPACITIES = [2.5, 5, 7.5, 10, 12.5, 15, 17.5, 20]
REPRESENTATIONS = ['3_1080']

#import matplotlib.lines as mlines
#from scipy.interpolate import interp1d


def read_quality ( path ):
    f = pd.read_csv(path, sep='\t')
    fx = pd.DataFrame(f,columns=['SegmentNumber','SegmentRepID'])
    a =  fx.groupby(['SegmentNumber','SegmentRepID']).size()
    t480 = a.xs('1_480', level='SegmentRepID').sum()
    t720 = a.xs('2_720', level='SegmentRepID').sum()
    t1080 = a.xs('3_1080', level='SegmentRepID').sum()
    t = t480 + t720 + t1080
    a = np.array([(100.0*t480/t), (100.0*t720/t), (100.0*t1080/t)])
    return a

throughput_ncn = pd.DataFrame(columns=['480p', '720p', '1080p'])
throughput_ndn = pd.DataFrame(columns=['480p', '720p', '1080p'])

for r in range(N_RUN):
    for c in CAPACITIES:
        throughput_ncn.loc[c] = read_quality ( RESULT_PREFIX + str(r+1) + '/netcodndn/' + (RESULT_NAME).format(c) )
        throughput_ndn.loc[c] = read_quality ( RESULT_PREFIX + str(r+1) + '/ndn/' + (RESULT_NAME).format(c) )

# Simple plot
fig_ncn, ax_ncn = newfig(0.32)
fig_ndn, ax_ndn = newfig(0.32)

# Color Maps
lvTmp = np.linspace(0.25,0.75,3)

cBluesTmp = plt.cm.Blues(lvTmp)
cBlues = mcol.ListedColormap(cBluesTmp)

cRedsTmp = plt.cm.Reds(lvTmp)
cReds = mcol.ListedColormap(cRedsTmp)

#ax_ncn = 
throughput_ncn.plot(kind='area', stacked=True, colormap=cBlues, ax=ax_ncn, lw=0.1)
#ax_ndn = 
throughput_ndn.plot(kind='area', stacked=True, colormap=cReds, ax=ax_ndn, lw=0.1)

#ax_ncn.set_title('NCN')
ax_ncn.set_ylabel("Segments requested [%]")
ax_ncn.set_ylim([0,100])
ax_ncn.set_xlabel("Core links bandwidth [Mbps]")
ax_ncn.set_xticks(CAPACITIES)

ax_ndn.set_ylabel("Segments requested [%]")
ax_ndn.set_ylim([0,100])
ax_ndn.set_xlabel("Core links bandwidth [Mbps]")
ax_ndn.set_xticks(CAPACITIES)

#Save/show the plot(s)
plt.figure(fig_ncn.number)
savefig("representation-bandwidth-netcodndn")

plt.figure(fig_ndn.number)
savefig("representation-bandwidth-ndn")