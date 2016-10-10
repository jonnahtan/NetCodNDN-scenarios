import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from scipy.interpolate import interp1d

def read_Packets( path ):
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

    # Input faces of the node 256, 257, 258, 259

    f1 = f[(f.Node=='SRC') & (f.FaceId==256) & (f.Type=='OutData')].reset_index(drop=True)
    f2 = f[(f.Node=='SRC') & (f.FaceId==257) & (f.Type=='OutData')].reset_index(drop=True)
    f3 = f[(f.Node=='SRC') & (f.FaceId==258) & (f.Type=='OutData')].reset_index(drop=True)
    f4 = f[(f.Node=='SRC') & (f.FaceId==259) & (f.Type=='OutData')].reset_index(drop=True)
    f5 = f[(f.Node=='SRC') & (f.FaceId==260) & (f.Type=='OutData')].reset_index(drop=True)
    f6 = f[(f.Node=='SRC') & (f.FaceId==261) & (f.Type=='OutData')].reset_index(drop=True)
    f7 = f[(f.Node=='SRC') & (f.FaceId==262) & (f.Type=='OutData')].reset_index(drop=True)
    f8 = f[(f.Node=='SRC') & (f.FaceId==263) & (f.Type=='OutData')].reset_index(drop=True)
    f9 = f[(f.Node=='SRC') & (f.FaceId==264) & (f.Type=='OutData')].reset_index(drop=True)
    f10 = f[(f.Node=='SRC') & (f.FaceId==265) & (f.Type=='OutData')].reset_index(drop=True)
    
    t = f1['Time']
    p = f1.PacketRaw + f2.PacketRaw + f3.PacketRaw + f4.PacketRaw + f5.PacketRaw + f6.PacketRaw + f7.PacketRaw + f8.PacketRaw + f9.PacketRaw + f10.PacketRaw
    #p = f1.KilobytesRaw + f2.KilobytesRaw  + f3.KilobytesRaw  + f4.KilobytesRaw

    r = pd.DataFrame(p.as_matrix(), index=t).cumsum()

    return r

def read_Quality ( path ):
    f = pd.read_csv(path, sep='\t')
    a =  f.groupby(['SegmentNumber'])['SegmentRepID'].mean()
    return a

netcod_Packets = read_Packets ('../star/netcod/l3-rate-trace.txt')
traditional_Packets = read_Packets ('../star/traditional/l3-rate-trace.txt')
plt.plot(netcod_Packets, label='NetCodNDN')
plt.plot(traditional_Packets, label='NDN')
plt.legend(loc='lower right')
plt.show()

netcod_Quality = read_Quality ('../star/netcod/dash-trace.txt')
traditional_Quality = read_Quality ('../star/traditional/dash-trace.txt')

plt.plot(netcod_Quality, label='NetCodNDN')
plt.plot(traditional_Quality, label='NDN')
plt.legend(loc='lower right')
plt.show()
