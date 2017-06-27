import pandas as pd
from common import *
import itertools

N_SEGMENTS = 40
N_RUN = 1
RESULT_PREFIX = '../../../results/generated/1'
RESULT_NAME = 'l3-rate-trace_{0}.txt'
NCN_POLICIES = ['nocache', 'ddd', 'dd', 'p50+lru_100K', 'd500+d_100K', 'lce'] #'d500+d_10K',
NDN_POLICIES = []

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
    r = r.loc[1:2*N_SEGMENTS]

    return r

#data_packets_ncn = pd.DataFrame()
#data_packets_ndn = pd.DataFrame()

fig,ax = newfig(0.90)

MARKERS = itertools.cycle((',', '+', '>', 'o', '*')) 

COLORS_NCN = np.linspace(0.4,0.9,len(NCN_POLICIES))
for i,ncn_p in enumerate(NCN_POLICIES):
    data_packets_ncn = read_data_packets_at_source ( RESULT_PREFIX + '/netcodndn/' + RESULT_NAME.format(ncn_p) )
    data_packets_ncn[0] = data_packets_ncn[0] * 1.455
    data_packets_ncn[0] = data_packets_ncn[0] / 1000
    ax.plot(data_packets_ncn, label='NC-{0}'.format(ncn_p), color=plt.get_cmap('Blues')(COLORS_NCN[i]), marker=MARKERS.next(), markersize = 4, markevery=2)

COLORS_NDN = np.linspace(0.4,0.9,len(NDN_POLICIES))
for i,ndn_p in enumerate(NDN_POLICIES):
    data_packets_ndn = read_data_packets_at_source ( RESULT_PREFIX + '/ndn/' + RESULT_NAME.format(ndn_p) )
    data_packets_ndn[0] = data_packets_ndn[0] * 1.455
    data_packets_ndn[0] = data_packets_ndn[0] / 1000
    ax.plot(data_packets_ndn, label='NDN-{0}'.format(ndn_p), color=plt.get_cmap('Reds')(COLORS_NDN[i]), marker=MARKERS.next(), markersize = 4, markevery=2)

ax.legend(loc='best')
ax.set_ylabel("Data delivered by the source [MB]")
ax.set_xlabel("Time [s]")


# Save the figure
savefig('data-from-source')