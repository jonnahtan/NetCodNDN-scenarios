import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from scipy.interpolate import interp1d

N_RUN = 1

def read_quality ( path ):
    f = pd.read_csv(path, sep='\t')
    fx = pd.DataFrame(f,columns=['SegmentNumber','SegmentRepID'])
    a =  fx.groupby(['SegmentNumber','SegmentRepID']).size()
    t480 = a.xs('1_480', level='SegmentRepID').sum()
    t720 = a.xs('2_720', level='SegmentRepID').sum()
    t1080 = a.xs('3_1080', level='SegmentRepID').sum()
    t = t480 + t720 + t1080
    return np.array([(100.0*t480/t), (100.0*t720/t), (100.0*t1080/t)])

quality_ncn = np.zeros(3)
quality_ndn = np.zeros(3)

for i in range(N_RUN):
    quality_ncn = quality_ncn + read_quality ( '../../../results/generated/' + str(i+1) + '/netcodndn/dash-trace.txt' )
    quality_ndn = quality_ndn + read_quality ( '../../../results/generated/' + str(i+1 ) + '/ndn/dash-trace.txt' )

#upper_limit_ncn = quality_ncn.max(axis=1)
#lower_limit_ncn = quality_ncn.min(axis=1)
mean_ncn = quality_ncn#.mean(axis=1)

#upper_limit_ndn = quality_ndn.max(axis=1)
#lower_limit_ndn = quality_ndn.min(axis=1)
mean_ndn = quality_ndn#.mean(axis=1)

x = np.array([1,2,3])

fig,ax = plt.subplots()

#ax.fill_between(upper_limit_ncn.index.values, upper_limit_ncn, lower_limit_ncn, color='#52be80', alpha=0.5)
plt.bar(x, mean_ncn, 0.35, label='NetCodNDN', color='#145a32')

#ax.fill_between(upper_limit_ndn.index.values, upper_limit_ndn, lower_limit_ndn, color='#5dade2', alpha=0.5)
plt.bar(np.add(x,0.35), mean_ndn, 0.35, label='NDN', color='#21618c')

plt.legend(loc='lower right')
plt.show()
