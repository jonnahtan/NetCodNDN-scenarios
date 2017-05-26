import random

class Node:
    pass

class Link:
    pass

def get_l1_nodes ( ):
    #Layer 1 nodes (IXP)
    id = [ 'DUB' , 'MAD' , 'LHR' , 'CDG', 'AMS' ,'CGN' ,'FRA' , 'MXP' , 'ARN' , 'HEL' ]
    loc = [ 'Dublin', 'Madrid' , 'London' , 'Paris' , 'Amsterdam' , 'Cologne' , 'Frankfurt' , 'Milan' , 'Stockholm' , 'Helsinki' ]
    if (len(id) != len(loc)):
        return ''

    l = []

    for i in range(len(id)):
        n = Node()
        n.id = id[i] + '-IXP'
        n.loc = loc[i]
        l.append(n)

    return l

def get_l2_nodes ( ):
    # Country codes
    id = [ 'AT','BE','CH','DE','DK','ES','FI','FO','FR','GB','IE','IT','NL','NO','PL','SE']
    lc = [ 'Austria','Belgium','Switzerland','Germany','Denmark','Spain','Finland','Faroe Islands','France','Great Bretain','Ireland','Italy','Netherlands','Norway','Poland','Sweden']

    if (len(id) != len(lc)):
        return ''

    l = []

    for i in range(len(id)):
        for j in range(random.randint(4, 10)):
            n = Node()
            n.id = id[i] + '-ISP' + str(j+1)
            n.loc = lc[i]
            l.append(n)

    return l

def get_l3_nodes ( ):
    # Country codes
    id = [ 'AT','BE','CH','DE','DK','ES','FI','FO','FR','GB','IE','IT','NL','NO','PL','SE']
    lc = [ 'Austria','Belgium','Switzerland','Germany','Denmark','Spain','Finland','Faroe Islands','France','Great Bretain','Ireland','Italy','Netherlands','Norway','Poland','Sweden']

    if (len(id) != len(lc)):
        return ''

    l3_nodes = []
    l3_loc = {}

    speed_dist = [20, 60, 20]

    for i in range(len(id)):
        clients_at_location = random.randint(10, 20)
        # Create the client node items
        for j in range(clients_at_location):
            n = Node()
            n.id = id[i] + '-C' + str(j+1).zfill(3)
            n.loc = lc[i]
            l3_nodes.append(n)
        # Create the location items
        l3_loc[lc[i]] = [int(clients_at_location * sp / 100) for sp in speed_dist]
        print l3_loc[lc[i]]
        if sum(l3_loc[lc[i]]) != clients_at_location:
            l3_loc[lc[i]][1] +=  clients_at_location - sum(l3_loc[lc[i]])

    return l3_nodes , l3_loc

def get_links ( l0_nodes , l1_nodes , l2_nodes , l3_nodes , l3_location ):
    ls = []
    
    # L1 -> L0 links (connected to all sources)
    #max_parents_l1 = min(1, len(l0_nodes))
    for x in l1_nodes:
        for y in l0_nodes:
            l = Link()
            l.x = x
            l.y = y
            l.bw = str(50) + 'Mbps'
            ls.append(l)

    # L2 -> L1 links
    max_parents_l2 = min(2, len(l1_nodes))
    for x in l2_nodes:
        for y in random.sample(l1_nodes, max_parents_l2):
            l = Link()
            l.x = x
            l.y = y
            l.bw = str(50) + 'Mbps'
            ls.append(l)

    # L3 -> L2 links
    max_parents_l3 = min(2, len(l2_nodes))

    for x in l3_nodes:
        # Compute bandwidth
        l3_loc = l3_location[x.loc]
        # By Akamai BW bands:
        if l3_loc[0] > 0:
            l3_loc[0] -= 1
            bw = 1
        elif l3_loc[1] > 0:
            l3_loc[1] -= 1
            bw = 5
        elif l3_loc[2] > 0:
            l3_loc[2] -= 1
            bw = 10
        else:
            bw = 5
        # By Simpson index:
        mu = 4
        sigma = 1.5

        # Create link
        parents_in_location = [n for n in l2_nodes if n.loc == x.loc]
        for y in random.sample(parents_in_location, max_parents_l3):
            l = Link()
            l.x = x
            l.y = y
            bw = float(round(random.gauss(mu,sigma),1))
            if bw < 1.5:
                bw = 1.5
            l.bw = str(bw) + 'Mbps'
            ls.append(l)

    return ls

# L0 nodes (source)
l0 = []
n = Node()
n.id = 'SRC'
n.loc = 'Global'
l0.append(n)

# L1 nodes (IXP)
l1 = get_l1_nodes()

# L2 nodes (ISP)
l2 = get_l2_nodes()

# L3 nodes (clients)
l3 , l3_loc = get_l3_nodes()

# L3 nodes (clients)
ls = get_links(l0,l1,l2,l3,l3_loc)

# Save to topology file
header = '# Layered topology atomatically generated\n'

body = '\nrouter\n'
body += '\n#node_ID\tlocation\n'

# Layer 0
header += '# Number of nodes in layer 0: ' + str(len(l0)) + '\n' 
for n in l0:
    body += n.id + '\t\t' + n.loc + '\n'

# Layer 1
header += '# Number of nodes in layer 1: ' + str(len(l1)) + '\n' 
for n in l1:
    body += n.id + '\t\t' + n.loc + '\n'

# Layer 2
header += '# Number of nodes in layer 2: ' + str(len(l2)) + '\n'    
for n in l2:
    body += n.id + '\t\t' + n.loc + '\n'

# Layer 3
header += '# Number of nodes in layer 3: ' + str(len(l3)) + '\n' 
for n in l3:
    body += n.id + '\t\t' + n.loc + '\n'

body += '\nlink\n'
body += '\n#x\t\ty\tcapacity(kbps)\tOSPF\tDelay\tMaxPackets\n'

for l in ls:
    body += l.x.id + '\t' + l.y.id + '\t' + l.bw + '\t' + '1' + '\t' + '1ms' + '\t' + '20000' + '\n'


# Name of topology
topo_name = 'generated'

# Save to txt file
f = open('../layer-generated.txt', 'w')
f.write(header)
f.write(body)

f.close()

# Save to aux C++ file
f = open('../layer-generated-cpp.txt', 'w')

f.write ('// Adding sources\n')
for n in l0:
    f.write ('sources.Add("' + n.id + '");\n')

f.write ('\n// Adding routers\n')
for n in l1:
    f.write ('routers.Add("' + n.id + '");\n')
for n in l2:
    f.write ('routers.Add("' + n.id + '");\n')

f.write ('\n// Adding clients\n')
for n in l3:
    f.write ('clients.Add("' + n.id + '");\n')

f.write ('\n// Install NDN stack\n')
f.write ('StackHelper ndnSources;\n')
f.write ('ndnSources.setCsSize(1);\n')
f.write ('ndnSources.Install(sources);\n')
f.write ('StackHelper ndnRouters;\n')
f.write ('ndnRouters.setCsSize(1000000);\n')
f.write ('ndnRouters.Install(routers);\n')
f.write ('StackHelper ndnClients;\n')
f.write ('ndnClients.setCsSize(1);\n')
f.write ('ndnClients.Install(clients);\n')

f.write ('\n// Calculate and install FIBs\n')

for l in ls:
    f.write ('FibHelper::AddRoute("' + l.x.id + '", "/unibe", "' + l.y.id + '", 1);\n')

f.close()