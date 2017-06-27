import pandas as pd
from common import *

import itertools

N_SEGMENTS = 30
N_RUN = 1
RESULT_PREFIX = '../../../results/generated/1'
RESULT_NAME = 'l3-rate-trace_{0}.txt'
NCN_POLICIES = ['nocache', 'dd', 'p50+lru_100K', 'd500+d_100K', 'lce'] #'d500+d_10K',
NDN_POLICIES = []
LAYER = 'IXP'

def print_full(x):
    pd.set_option('display.max_rows', len(x))
    print(x)
    pd.reset_option('display.max_rows')

def get_cache_hit_ratio( path ):
    f = pd.read_csv(path,
                    sep='\t',
                    dtype={ 'Time':np.float64,
                            'FaceId':object,
                            'Packets':np.float64,
                            'Kilobytes':np.float64,
                            'PacketRaw':np.float64,
                            'KilobytesRaw':np.float64
                    })

    f.Packets = f.Packets.apply(np.floor)
    f.Kilobytes = f.Kilobytes.apply(np.floor)
    f.PacketRaw = f.PacketRaw.apply(np.floor)
    f.KilobytesRaw = f.KilobytesRaw.apply(np.floor)

    # Get only interesting records, the ones that contain information about network faces, and only information about In and Out Interests
    fx = f[ (f.FaceDescr=='netDeviceFace://') &  ( (f.Type=='InInterests') | (f.Type=='OutInterests') ) ].reset_index(drop=True)
    # Get only nodes of a specific layer (IXP or ISP)
    fx = fx[ (fx.Node.str.contains(LAYER)) ].reset_index(drop=True)
    # Get the interesting columns only
    fx = pd.DataFrame(fx,columns=['Time', 'Node','FaceId','Type','PacketRaw'])
    # Get the Information grouped by time and type (In or Out)
    fx = fx.groupby(['Time','Node','Type'])['PacketRaw'].sum()
    # Get on array for In Interests and another for Out Interests
    fx_in = fx.xs('InInterests', level='Type')
    fx_out = fx.xs('OutInterests', level='Type')
    # Compute the cache hits
    fx_hit = fx_in - fx_out
    # Compute the cache hit ratio
    fx_hit_ratio = 100 * (fx_hit / fx_in)   

    fx_hit_ratio = fx_hit_ratio.loc[1:2*N_SEGMENTS]
    return fx_hit_ratio.median(level=0)

fig,ax = newfig(0.90)

MARKERS = itertools.cycle((',', '+', '>', 'o', '*')) 

COLORS_NCN = np.linspace(0.4,0.9,len(NCN_POLICIES))
for i,ncn_p in enumerate(NCN_POLICIES):
    cache_hit_ncn = get_cache_hit_ratio ( RESULT_PREFIX + '/netcodndn/' + RESULT_NAME.format(ncn_p) )
    ax.plot(cache_hit_ncn, label='NC-{0}'.format(ncn_p), color=plt.get_cmap('Blues')(COLORS_NCN[i]), marker=MARKERS.next(), markersize = 4, markevery=2)

COLORS_NDN = np.linspace(0.4,0.9,len(NDN_POLICIES))
for i,ndn_p in enumerate(NDN_POLICIES):
    cache_hit_ndn = get_cache_hit_ratio ( RESULT_PREFIX + '/ndn/' + RESULT_NAME.format(ndn_p) )
    ax.plot(cache_hit_ndn, label='NDN-{0}'.format(ndn_p), color=plt.get_cmap('Reds')(COLORS_NDN[i]), marker=MARKERS.next(), markersize = 4, markevery=2)

ax.legend(loc='best')

ax.set_ylabel("Cache-hit rate [%]")
ax.set_ylim([0,100])

ax.set_xlabel("Time [s]")
#ax.set_xlim([1,2*N_SEGMENTS])

# Save the figure
savefig('cache-hit-' + LAYER)