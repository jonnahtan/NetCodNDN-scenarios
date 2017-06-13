import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.lines as mlines
from scipy.interpolate import interp1d

N_RUN = 1
REPRESENTATIONS = ['480','720','1080','4K']

#			StallingTime(msec)	SegmentDepIds
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
    return a

quality_ncn = pd.DataFrame()
quality_ndn = pd.DataFrame()

for i in range(N_RUN):
    quality_ncn = pd.concat([quality_ncn, read_quality ( '../../../results/generated/' + str(i+1) + '/netcodndn/dash-trace.txt' )], axis=1, ignore_index=True)
    quality_ndn = pd.concat([quality_ndn, read_quality ( '../../../results/generated/' + str(i+1) + '/ndn/dash-trace.txt' )], axis=1, ignore_index=True)


#select only up to certain time
quality_ncn = quality_ncn.loc[0:30]
quality_ndn = quality_ndn.loc[0:30]

quality_ncn = quality_ncn.unstack(level='SegmentRepID', fill_value=0.0)[0]
quality_ndn = quality_ndn.unstack(level='SegmentRepID', fill_value=0.0)[0]

#quality_ncn = quality_ncn.rename('RepresentationID', level=['SegmentRepID'])

print quality_ncn
ax_ncn = quality_ncn.plot(kind='area', stacked=True, colormap='Dark2')
ax_ndn = quality_ndn.plot(kind='area', stacked=True, colormap='Dark2')

#mean_ncn = pd.DataFrame()
#mean_ndn = pd.DataFrame()

#for r in REPRESENTATIONS:
#    if r in quality_ncn.index.get_level_values('SegmentRepID').unique():
#        mean_ncn = quality_ncn.xs(r, level='SegmentRepID').mean(axis=1)
#        mean_ndn = quality_ndn.xs(r, level='SegmentRepID').mean(axis=1)
#        ax_ncn.stackplot(mean_ncn, label='NetCodNDN '+ str(r))#, color='#145a32')
#        ax_ndn.stackplot(mean_ndn, label='NDN '+ str(r))#, color='#21618c')

#ax_ncn.set_title('NCN')
ax_ncn.set_ylabel("Percentage of NetCodNDN clients [%]")
ax_ncn.set_ylim([0,100])
ax_ncn.set_xlabel("Segment")
ax_ncn.set_xlim([0,30])
#ax_ncn.legend(["480p", "720p","1080p"])

ax_ndn.set_ylabel("Percentage of NDN clients [%]")
ax_ndn.set_ylim([0,100])
ax_ndn.set_xlabel("Segment")
ax_ndn.set_xlim([0,30])
#ax_ndn.legend(["480p", "720p","1080p"], loc='lower right')

#quality_ncn.plot.bar(stacked=True);
plt.show()


