import pandas as pd
from common import *

import matplotlib.colors as mcol

# Config
N_SEGMENTS = 50
N_RUN = 1
RESULT_PREFIX = '../../../results/generated/'
RESULT_NAME = 'dash-trace_10Mbps.txt'

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

    a = a.loc[0:N_SEGMENTS]
    return a

quality_ncn = pd.DataFrame()
quality_ndn = pd.DataFrame()

for i in range(N_RUN):
    quality_ncn = pd.concat([quality_ncn, read_quality ( RESULT_PREFIX + str(i+1) + '/netcodndn/' + RESULT_NAME )], axis=1, ignore_index=True)
    quality_ndn = pd.concat([quality_ndn, read_quality ( RESULT_PREFIX + str(i+1) + '/ndn/' + RESULT_NAME )], axis=1, ignore_index=True)

quality_ncn = quality_ncn.unstack(level='SegmentRepID', fill_value=0.0)[0]
quality_ndn = quality_ndn.unstack(level='SegmentRepID', fill_value=0.0)[0]

#quality_ndn = quality_ncn.rename('Representation', level=['SegmentRepID'])

# Simple plot
fig_ncn, ax_ncn = newfig(0.32)
fig_ndn, ax_ndn = newfig(0.32)

# Color Maps
lvTmp = np.linspace(0.25,0.75,3)

cBluesTmp = plt.cm.Blues(lvTmp)
cBlues = mcol.ListedColormap(cBluesTmp)

cRedsTmp = plt.cm.Reds(lvTmp)
cReds = mcol.ListedColormap(cRedsTmp)

# Rename columns
quality_ncn.columns = ['480p', '720p', '1080p']
quality_ndn.columns = ['480p', '720p', '1080p']

#ax_ncn = 
quality_ncn.plot(kind='area', stacked=True, colormap=cBlues, ax=ax_ncn, lw=0.1)
#ax_ndn = 
quality_ndn.plot(kind='area', stacked=True, colormap=cReds, ax=ax_ndn, lw=0.1)


#mean_ncn = pd.DataFrame()
#mean_ndn = pd.DataFrame()

#for r in REPRESENTATIONS:
#    if r in quality_ncn.index.get_level_values('SegmentRepID').unique():
#        mean_ncn = quality_ncn.xs(r, level='SegmentRepID').mean(axis=1)
#        mean_ndn = quality_ndn.xs(r, level='SegmentRepID').mean(axis=1)
#        ax_ncn.stackplot(mean_ncn, label='NetCodNDN '+ str(r))#, color='#145a32')
#        ax_ndn.stackplot(mean_ndn, label='NDN '+ str(r))#, color='#21618c')

ax_ncn.set_ylabel("Clients [%]")
ax_ncn.set_ylim([0,100])
ax_ncn.set_xlabel("Segment")

ax_ndn.set_ylabel("Clients [%]")
ax_ndn.set_ylim([0,100])
ax_ndn.set_xlabel("Segment")

#Save/show the plot(s)
plt.figure(fig_ncn.number)
savefig("representations-segment-netcodndn")

plt.figure(fig_ndn.number)
savefig("representations-segment-ndn")

#plt.show()


