import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import scipy.stats as st
from scipy.interpolate import interp1d

N_RUN = 1

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
    fx = fx[ (fx.Node.str.contains('IXP')) ].reset_index(drop=True)
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
    return fx_hit_ratio

chr_ncn = pd.DataFrame()
chr_ndn = pd.DataFrame()

for i in range(N_RUN):
    chr_ncn = pd.concat([chr_ncn, get_cache_hit_ratio ( '../../results/results_4/' + str(i) + '/netcodndn/l3-rate-trace.txt' )], axis=1, ignore_index=True)
    chr_ndn = pd.concat([chr_ndn, get_cache_hit_ratio ( '../../results/results_4/' + str(i) + '/ndn/l3-rate-trace.txt' )], axis=1, ignore_index=True)

mean_ncn = chr_ncn.mean(axis=1).median(level=0)
#sem_ncn = chr_ncn.sem(axis=1)
#conf_int_ncn = st.t.interval(0.95, len(chr_ncn)-1, loc=mean_ncn, scale=sem_ncn)

mean_ndn = chr_ndn.mean(axis=1).median(level=0)
#sem_ndn = chr_ndn.sem(axis=1)
#conf_int_ndn = st.t.interval(0.95, len(chr_ndn)-1, loc=mean_ndn, scale=sem_ndn)

print mean_ncn[25]
print mean_ndn[25]

fig,ax = plt.subplots()

#ax.fill_between(mean_ncn.index.values, conf_int_ncn[0], conf_int_ncn[1], color='#a9dfbf', alpha=0.5)
ax.plot(mean_ncn, label='NetCodNDN-DASH', color='blue', marker=">")

#ax.fill_between(mean_ndn.index.values, conf_int_ndn[0], conf_int_ndn[1], color='#aed6f1', alpha=0.5)
ax.plot(mean_ndn, label='NDN-DASH', color='red', marker="*")

ax.legend(loc='lower right')

ax.set_ylabel("Cache hit rate [%]")
ax.set_ylim([0,100])

ax.set_xlabel("Time [s]")
ax.set_xlim([0,60])

plt.show()
