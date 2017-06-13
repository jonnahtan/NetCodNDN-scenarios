import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from scipy.interpolate import interp1d

N_RUN = 1

def read_data_packets_at_source ( path ):
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

    # Input faces of the node 256, 257, 258, 259

    f1 = f[(f.Node=='SRC') & (f.FaceId=='256') & (f.Type=='OutData')].reset_index(drop=True)
    f2 = f[(f.Node=='SRC') & (f.FaceId=='257') & (f.Type=='OutData')].reset_index(drop=True)
    f3 = f[(f.Node=='SRC') & (f.FaceId=='258') & (f.Type=='OutData')].reset_index(drop=True)
    f4 = f[(f.Node=='SRC') & (f.FaceId=='259') & (f.Type=='OutData')].reset_index(drop=True)
    f5 = f[(f.Node=='SRC') & (f.FaceId=='260') & (f.Type=='OutData')].reset_index(drop=True)
    f6 = f[(f.Node=='SRC') & (f.FaceId=='261') & (f.Type=='OutData')].reset_index(drop=True)
    f7 = f[(f.Node=='SRC') & (f.FaceId=='262') & (f.Type=='OutData')].reset_index(drop=True)
    f8 = f[(f.Node=='SRC') & (f.FaceId=='263') & (f.Type=='OutData')].reset_index(drop=True)
    f9 = f[(f.Node=='SRC') & (f.FaceId=='264') & (f.Type=='OutData')].reset_index(drop=True)
    f10 = f[(f.Node=='SRC') & (f.FaceId=='265') & (f.Type=='OutData')].reset_index(drop=True)
    
    t = f1['Time']
    p = f1.PacketRaw + f2.PacketRaw + f3.PacketRaw + f4.PacketRaw + f5.PacketRaw + f6.PacketRaw + f7.PacketRaw + f8.PacketRaw + f9.PacketRaw + f10.PacketRaw
    #p = f1.Kilobytes + f2.Kilobytes + f3.Kilobytes + f4.Kilobytes + f5.Kilobytes + f6.Kilobytes + f7.Kilobytes + f8.Kilobytes + f9.Kilobytes + f10.Kilobytes

    r = pd.DataFrame(p.as_matrix(), index=t).cumsum()
    r = r.loc[0:60]
    print r

    return r

data_packets_ncn = pd.DataFrame()
data_packets_ndn = pd.DataFrame()

for i in range(N_RUN):
    data_packets_ncn = pd.concat([data_packets_ncn, read_data_packets_at_source ( '../../../results/generated/' + str(i+1) + '/netcodndn/l3-rate-trace.txt' )], axis=1, ignore_index=True)
    data_packets_ndn = pd.concat([data_packets_ndn, read_data_packets_at_source ( '../../../results/generated/' + str(i+1) + '/ndn/l3-rate-trace.txt' )], axis=1, ignore_index=True)

# multiply packets to KB
data_packets_ncn[0] = data_packets_ncn[0] * 1.455
data_packets_ndn[0] = data_packets_ndn[0] * 1.455

# multiply packets to  MB
data_packets_ncn[0] = data_packets_ncn[0] / 1000
data_packets_ndn[0] = data_packets_ndn[0] / 1000

upper_limit_ncn = data_packets_ncn.max(axis=1)
lower_limit_ncn = data_packets_ncn.min(axis=1)
mean_ncn = data_packets_ncn.mean(axis=1)

upper_limit_ndn = data_packets_ndn.max(axis=1)
lower_limit_ndn = data_packets_ndn.min(axis=1)
mean_ndn = data_packets_ndn.mean(axis=1)

fig,ax = plt.subplots()

ax.fill_between(upper_limit_ncn.index.values, upper_limit_ncn, lower_limit_ncn, color='blue', alpha=0.5)
ax.plot(mean_ncn, label='NetCodNDN-DASH', color='blue', marker=">")

ax.fill_between(upper_limit_ndn.index.values, upper_limit_ndn, lower_limit_ndn, color='red', alpha=0.5)
ax.plot(mean_ndn, label='NDN-DASH', color='red', marker="*")

ax.legend(loc='lower right')
ax.set_ylabel("Data provided by the source [MB]")
ax.set_xlabel("Time [s]")

plt.show()
