#!/usr/bin/env python
# -*- coding: utf-8 -*-

from xml.dom.minidom import parse, parseString, Document
import re
import os,sys
from osmapi import OsmApi

if len(sys.argv) != 3:
	print "Usage command <osm-file> <munip-name>" 
	sys.exit()
#api = OsmApi(api="api06.dev.openstreetmap.org", username="", password="", changesetauto=True, changesetautotags={"source":"Kartverket"})
api = OsmApi(api="api06.dev.openstreetmap.org", username="", password="")
#api.NodeGet(123)

api.ChangesetCreate({"comment":"addr node import "+str(sys.argv[2]), "source":"Kartverket", "source:date":"2014-08-24"})

dom1 = parse(sys.argv[1])
mainelement1 = dom1.getElementsByTagName("osm")[0]
nodes = mainelement1.getElementsByTagName("node")
counter = 1
for node in nodes:
	if(node.nodeType == 1):
		housenumber = ""
		street      = ""
		postcode    = ""
		city        = ""
		osm_id = node.attributes["id"].value
		latitude = node.attributes["lat"].value
		longitude = node.attributes["lon"].value
		tags = node.getElementsByTagName("tag")
		for tag in tags:
			if tag.attributes["k"].value == "addr:housenumber":
				housenumber = tag.attributes["v"].value
			if tag.attributes["k"].value == "addr:street":
				street = tag.attributes["v"].value
			if tag.attributes["k"].value == "addr:postcode":
				postcode    = tag.attributes["v"].value
			if tag.attributes["k"].value == "addr:city":
				city        = tag.attributes["v"].value
		if housenumber != "":
			uploadnode = {"id":-counter , "lat" : latitude, "lon": longitude , "tag":{"addr:housenumber":housenumber, "addr:street":street, "addr:postcode":postcode,"addr:city":city}}
			#print str(latitude)+""+str(longitude)+""+street+" "+housenumber
			api.NodeCreate(uploadnode)
			print uploadnode
			counter = counter + 1
		
print node
api.ChangesetClose()
##api.flush()
#api.NodeCreate
