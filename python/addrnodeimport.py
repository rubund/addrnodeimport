#!/usr/bin/env python
import os
import sys

if len(sys.argv) != 3:
	print "Missing command line argument.\nUsage: "+sys.argv[0]+" <top-zip-file> <municipality number>"
	sys.exit()


topzipfile = sys.argv[1]
munipnumber = sys.argv[2]
munipnumberpadded = "%04d" % (int(munipnumber))
basenametopzipfile = os.path.basename(topzipfile)
prefix = basenametopzipfile.split('.')[0]


if not os.path.isdir(prefix):
	os.system("unzip "+topzipfile+" -d "+prefix+"")

if not os.path.isfile(prefix+"/"+str(munipnumberpadded)+"Adresser.SOS"):
	ret = os.system("cd "+prefix+" ; unzip "+str(munipnumberpadded)+"Elveg.zip");
	if ret != 0:
		print "Failed to find this municipality"
		sys.exit()

os.system("sosi2osm "+prefix+"/"+str(munipnumberpadded)+"Adresser.SOS adresser.lua > "+prefix+"/"+str(munipnumberpadded)+"Adresser.osm")

osmfilename = prefix+"/"+str(munipnumberpadded)+"Adresser.osm"

boundarea = os.popen("getedges -m 0.001 "+osmfilename).read()
os.system(" wget \"http://overpass-api.de/api/interpreter?data=((way[\\\"addr:housenumber\\\"] "+boundarea+";>;););out meta;\" -O ways.osm")
os.system(" wget \"http://overpass-api.de/api/interpreter?data=((node[\\\"addr:housenumber\\\"] "+boundarea+";<;););out meta;\" -O nodes.osm")



