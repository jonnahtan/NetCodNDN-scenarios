import pandas as pd
from common import *

from matplotlib.ticker import FuncFormatter
import matplotlib.mlab as mlab
import math

# Config
RESULT_PREFIX = '../../../topologies/'

#from scipy.interpolate import interp1d

def to_percent(y, position):
    return str(100 * y)

def read_clients ( path ):
    f = pd.read_csv(path, sep=',')
    fx = pd.DataFrame(f,columns=['Source','Bandwidth'])
    fx['Bandwidth'] = fx['Bandwidth'].map(lambda x: x.rstrip('Mbps'))
    fx['Bandwidth'] = fx['Bandwidth'].apply(pd.to_numeric)
    a =  fx.groupby(['Source']).sum()
    return a

clients = pd.DataFrame()
clients = read_clients ( RESULT_PREFIX + 'layer-generated-clients-TMM.txt' )

fig, ax = newfig(0.32)

ax.hist(clients.Bandwidth, bins=11, normed=True, color='silver', label=None)
ax.set_ylabel("Percentage of clients [%]")
ax.set_xlabel("Total client bandwidth [Mbps]")
ax.set_ylim([0,0.3])

mu = 8
variance = 4.5
sigma = math.sqrt(variance)
x = np.linspace(0,16,100)
plt.plot(x,mlab.normpdf(x, mu, sigma), color='black', label=r'$\mu=2 \times 4$' '\n' r'$\sigma=\sqrt{2} \times 1.5')

formatter = FuncFormatter(to_percent)
plt.gca().yaxis.set_major_formatter(formatter)

ax.legend(loc='best')

savefig('client-bandwidth-distribution')
