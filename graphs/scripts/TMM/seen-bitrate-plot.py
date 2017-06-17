import pandas as pd
from common import *

N_SEGMENTS = 50
N_RUN = 1
RESULT_PREFIX = '../../../results/generated/'
RESULT_NAME = 'dash-trace_10Mbps.txt'

def read_bitrate ( path ):
    f = pd.read_csv(path, sep='\t')
    fx = pd.DataFrame(f,columns=['SegmentNumber','SegmentExperiencedBitrate(bit/s)'])
    a =  fx.groupby(['SegmentNumber'])['SegmentExperiencedBitrate(bit/s)'].median()
    return a

bitrate_ncn = pd.DataFrame()
bitrate_ndn = pd.DataFrame()

for i in range(N_RUN):
    bitrate_ncn = pd.concat([bitrate_ncn, read_bitrate ( RESULT_PREFIX + str(i+1) + '/netcodndn/' + RESULT_NAME )], axis=1, ignore_index=True)
    bitrate_ndn = pd.concat([bitrate_ndn, read_bitrate ( RESULT_PREFIX + str(i+1) + '/ndn/' + RESULT_NAME )], axis=1, ignore_index=True)

# Scale the bitrate from bps to Mbps
bitrate_ncn = bitrate_ncn / 1000000
bitrate_ndn = bitrate_ndn / 1000000

upper_limit_ncn = bitrate_ncn.max(axis=1)
lower_limit_ncn = bitrate_ncn.min(axis=1)
mean_ncn = bitrate_ncn.median(axis=1)

upper_limit_ndn = bitrate_ndn.max(axis=1)
lower_limit_ndn = bitrate_ndn.min(axis=1)
mean_ndn = bitrate_ndn.median(axis=1)

# Simple plot
fig, ax = newfig(0.32)

#ax.fill_between(upper_limit_ncn.index.values, upper_limit_ncn, lower_limit_ncn, color='#52be80', alpha=0.5)
ax.plot(mean_ncn, label='NetCodNDN-DASH', color='blue') #marker=">",

#ax.fill_between(upper_limit_ndn.index.values, upper_limit_ndn, lower_limit_ndn, color='#5dade2', alpha=0.5)
ax.plot(mean_ndn, label='NDN-DASH', color='red') #marker="*", 

ax.legend(loc='best')

ax.set_ylabel("Goodput [Mbps]")
#ax.set_ylim([6.5,8.5])

ax.set_xlabel("Segment")
ax.set_xlim([0,N_SEGMENTS])

# Save the figure
savefig("dash-goodput")

#Show the figure
plt.show()
