import random

class Node:
    pass

def get_l1_nodes ( ):
    #Layer 1 nodes (IXP)
    id = [ 'DUB' , 'MAD' , 'LHR' , 'CDG', 'AMS' ,'CGN' ,'FRA' , 'MXP' , 'ARN' , 'HEL' ]
    lc = [ 'Dublin', 'Madrid' , 'London' , 'Paris' , 'Amsterdam' , 'Cologne' , 'Frankfurt' , 'Milan' , 'Stockholm' , 'Helsinki' ]
    if (len(id) != len(lc)):
        return ''

    l = []

    for i in range(len(id)):
        n = Node()
        n.id = id[i] + '-IXP'
        n.loc = lc[i]
        l.append(n)

    return l

def get_l2_nodes ( ):
    #Layer 2 ID (country) (ISP)
    id = [ 'AT','BE','CH','DE','DK','ES','FI','FO','FR','GB','IE','IT','NL','NO','PL','SE']
    lc = [ 'Austria','Belgium','Switzerlan','Germany','Denmark','Spain','Finland','Faroe Islands','France','Great Bretain','Ireland','Italy','Netherlands','Norway','Poland','Sweden']

    if (len(id) != len(lc)):
        return ''

    l = []

    for i in range(len(id)):
        for j in range(random.randint(2, 6)):
            n = Node()
            n.id = id[i] + '-ISP' + str(j+1)
            n.loc = lc[i]
            l.append(n)

    return l

def get_l3_nodes ( ):
    #Layer 2 ID (country) (ISP)
    id = [ 'AT','BE','CH','DE','DK','ES','FI','FO','FR','GB','IE','IT','NL','NO','PL','SE']
    lc = [ 'Austria','Belgium','Switzerlan','Germany','Denmark','Spain','Finland','Faroe Islands','France','Great Bretain','Ireland','Italy','Netherlands','Norway','Poland','Sweden']

    if (len(id) != len(lc)):
        return ''

    l = []

    for i in range(len(id)):
        for j in range(random.randint(2, 6)):
            n = Node()
            n.id = id[i] + '-C' + str(j+1).zfill(3)
            n.loc = lc[i]
            l.append(n)

    return l

# L0 nodes (source)
l0 = ['SRC', 'Global']
n = Node()
n.id = 'SRC'
n.loc = 'Global'
l0.append(n)

# L1 nodes (IXP)
l1 = get_l1_nodes()

# L2 nodes (ISP)
l2 = get_l2_nodes()

# L3 nodes (clients)
l3 = get_l3_nodes()

# Print
print 'routers\n'

print '#node ID\tlocation\n'

for n in l1:
    print n.id + '\t' + n.loc
    
for n in l2:
    print n.id + '\t' + n.loc

for n in l3:
    print n.id + '\t' + n.loc
