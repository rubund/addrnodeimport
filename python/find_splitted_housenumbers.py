#!/usr/bin/env python

import sys
import xml.dom.minidom
import sqlite3

if len(sys.argv) != 3:
	print "Usage "+sys.argv[0]+" <notmatched.osm> <newnodes.osm>"
	sys.exit(-1)


conn = sqlite3.connect(":memory:")
cur = conn.cursor()

cur.execute('create table if not exists notmatched (id int auto_increment primary key not null, addr_housenumber varchar(10), addr_street varchar(255), addr_postcode varchar(10), addr_city varchar(255))')
cur.execute('create table if not exists newnodes (id int auto_increment primary key not null, addr_housenumber varchar(10), addr_street varchar(255), addr_postcode varchar(10), addr_city varchar(255))')


def fill_into_sql(cur, nodes, sqltablename):
	idcnt = 1
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
		query = "insert into "+str(sqltablename)+" (id,addr_housenumber, addr_street, addr_postcode, addr_city) values ('"+str(idcnt)+"','"+addr_housenumber+"','"+addr_street+"','"+addr_postcode+"','"+addr_city+"');"
		print query
		cur.execute(query)
		idcnt = idcnt + 1


notmatched = xml.dom.minidom.parse(sys.argv[1])
notmatched_top = notmatched.documentElement
notmatched_nodes = notmatched_top.getElementsByTagName("node")
fill_into_sql(cur, notmatched_nodes, "notmatched")
conn.commit()

newnodes = xml.dom.minidom.parse(sys.argv[1])
newnodes_top = newnodes.documentElement
newnodes_nodes = newnodes_top.getElementsByTagName("node")
fill_into_sql(cur, newnodes_nodes, "newnodes")
conn.commit()
		


conn.close()
