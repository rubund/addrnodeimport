#!/usr/bin/env python
#
# addrnodeimport.py
#
# (c) Ruben Undheim 2014
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 

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


reportcontent = os.popen("getmissingandreport -o newnodes.osm nodes.osm "+osmfilename+"").read()

os.system("mkdir -p reports")
reportfile = open("reports/report_"+munipnumberpadded+".txt","w")
reportfile.write(reportcontent)
reportfile.close()


