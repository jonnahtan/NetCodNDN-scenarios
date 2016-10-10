import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from scipy.interpolate import interp1d

def get_cache_hit_ratio( path ):
    f = pd.read_csv(path,
                    sep='\t',
                    dtype={ 'Time':np.float64,
                            'FaceId':np.int32,
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
    # Get only nodes of a specific layer (ISP)
    fx = fx[ (fx.Node.str.contains('IXP')) ].reset_index(drop=True)
    # Get the interesting columns only
    fx = pd.DataFrame(fx,columns=['Time', 'Node','FaceId','Type','PacketRaw'])
    # Get the Information grouped by time and type (In or Out)
    fx = fx.groupby(['Time','Type'])['PacketRaw'].sum()
    # Get on array for In Interests and another for Out Interests
    fx_in = fx.xs('InInterests', level='Type')
    fx_out = fx.xs('OutInterests', level='Type')
    # Compute the cache hits
    fx_hit = fx_in - fx_out
    # Compute the cache hit ratio
    fx_hit_ratio = 100 * (fx_hit / fx_in)
    
    return fx_hit_ratio

chr_ncn = get_cache_hit_ratio ( '../star/netcod/l3-rate-trace.txt' )
chr_ndn = get_cache_hit_ratio ( '../star/traditional/l3-rate-trace.txt' )

plt.plot(chr_ncn, label='NetCodNDN')
plt.plot(chr_ndn, label='NDN')
plt.legend(loc='lower right')
plt.show()
