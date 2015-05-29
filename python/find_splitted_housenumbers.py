#!/usr/bin/env python3

import sys
import xml.dom.minidom
import sqlite3

if len(sys.argv) != 4:
	print ("Usage "+sys.argv[0]+" <notmatched.osm> <newnodes.osm> <output.osm>")
	sys.exit(-1)


conn = sqlite3.connect(":memory:")
cur = conn.cursor()

cur.execute('create table if not exists notmatched (id int auto_increment primary key not null, addr_housenumber varchar(10), addr_street varchar(255), addr_postcode varchar(10), addr_city varchar(255))')
cur.execute('create table if not exists newnodes (id int auto_increment primary key not null, addr_housenumber varchar(10), addr_street varchar(255), addr_postcode varchar(10), addr_city varchar(255))')


def fill_into_sql(cur, nodes, sqltablename):
	idcnt = 0
	for node in nodes:
		addr_housenumber = ""
		addr_street = ""
		addr_postcode = ""
		addr_city = ""
		tags = node.getElementsByTagName("tag")
		for tag in tags:
			attr = tag.getAttribute("k")
			if attr == "addr:street":
				addr_street = tag.getAttribute("v")
			elif attr == "addr:housenumber":
				addr_housenumber = tag.getAttribute("v")
			elif attr == "addr:postcode":
				addr_postcode = tag.getAttribute("v")
			elif attr == "addr:city":
				addr_city = tag.getAttribute("v")
		query = "insert into "+str(sqltablename)+" (id,addr_housenumber, addr_street, addr_postcode, addr_city) values ('"+str(idcnt)+"',?,?,?,?);"
		values = (addr_housenumber,addr_street,addr_postcode,addr_city)
		cur.execute(query,values)
		idcnt = idcnt + 1


notmatched = xml.dom.minidom.parse(sys.argv[1])
notmatched_top = notmatched.documentElement
notmatched_nodes = notmatched_top.getElementsByTagName("node")
fill_into_sql(cur, notmatched_nodes, "notmatched")
conn.commit()

newnodes = xml.dom.minidom.parse(sys.argv[2])
newnodes_top = newnodes.documentElement
newnodes_nodes = newnodes_top.getElementsByTagName("node")
fill_into_sql(cur, newnodes_nodes, "newnodes")
conn.commit()

newdoc = xml.dom.minidom.Document()
newdoc.appendChild(notmatched_top.cloneNode(False))
newdoc_top = newdoc.getElementsByTagName("osm")[0]

cur2 = conn.cursor()
cur.execute("select * from notmatched")	
for row in cur.fetchall():
	addr_street = row[2]
	addr_housenumber = row[1]
	addr_postcode = row[3]
	addr_city = row[4]
	cur2.execute("select * from newnodes where addr_street = ? and addr_postcode = ? and addr_city = ? and (addr_housenumber like '"+addr_housenumber+"a' or addr_housenumber like '"+addr_housenumber+"b' or addr_housenumber like '"+addr_housenumber+"c' or addr_housenumber like '"+addr_housenumber+"d' or addr_housenumber like '"+addr_housenumber+"e' or addr_housenumber like '"+addr_housenumber+"f' or addr_housenumber like '"+addr_housenumber+"g' or addr_housenumber like '"+addr_housenumber+"h' or addr_housenumber like '"+addr_housenumber+"i') order by addr_street, addr_housenumber",(addr_street,addr_postcode,addr_city))
	res = cur2.fetchall()
	if len(res) != 0:
		first = 1
		firstofthem = notmatched_nodes[row[0]].cloneNode(False)
		newdoc_top.appendChild(firstofthem)
		print (addr_street+" "+addr_housenumber)
		for rowres in res:
			if first:
				latitude = newnodes_nodes[rowres[0]].getAttribute("lat")
				longitude = newnodes_nodes[rowres[0]].getAttribute("lon")
				addr_housenumber = ""
				addr_street = ""
				addr_postcode = ""
				addr_city = ""
				tags = newnodes_nodes[rowres[0]].getElementsByTagName("tag")
				for tag in tags:
					attr = tag.getAttribute("k")
					if attr == "addr:street":
						addr_street = tag.getAttribute("v")
					elif attr == "addr:housenumber":
						addr_housenumber = tag.getAttribute("v")
					elif attr == "addr:postcode":
						addr_postcode = tag.getAttribute("v")
					elif attr == "addr:city":
						addr_city = tag.getAttribute("v")
				t = newdoc.createElement("tag")
				t.setAttribute("k","addr:street")
				t.setAttribute("v",addr_street)
				firstofthem.appendChild(t)
				t = newdoc.createElement("tag")
				t.setAttribute("k","addr:housenumber")
				t.setAttribute("v",addr_housenumber)
				firstofthem.appendChild(t)
				t = newdoc.createElement("tag")
				t.setAttribute("k","addr:postcode")
				t.setAttribute("v",addr_postcode)
				firstofthem.appendChild(t)
				t = newdoc.createElement("tag")
				t.setAttribute("k","addr:city")
				t.setAttribute("v",addr_city)
				firstofthem.appendChild(t)
				firstofthem.attributes["lat"].value = latitude
				firstofthem.attributes["lon"].value = longitude
				firstofthem.setAttribute("action","modify")
			else:
				current = newnodes_nodes[rowres[0]].cloneNode(True)
				newdoc_top.appendChild(current)
			#print (rowres)
			#print (str(row[0])+": "+str(rowres[0]))
			#print (newnodes_nodes[rowres[0]].getElementsByTagName("tag")[3].getAttribute("v"))#getAttribute("lat"))
			first = 0
	else:
		newdoc_top.appendChild(notmatched_nodes[row[0]].cloneNode(True))


fout = open(sys.argv[3],"w")
fout.write(newdoc.toprettyxml())
fout.close()

conn.close()
