#!/usr/bin/env python3


import re
import os
import sys
import subprocess
import requests
import json
import tempfile
import shutil

borderfile = sys.argv[1]
output_filename = sys.argv[2]

corners = subprocess.check_output("getedges -m 0.01 %s" % (borderfile), shell=True)
cornersd = corners.decode('utf-8').strip()

m1 = re.compile(r"\(([^,]+),([^,]+),([^,]+),([^,]+)\)")

matches = m1.match(cornersd)

if matches == None:
    print("Failed")
    sys.exit(-1)


nordLL=float(matches.group(1))
nordUR=float(matches.group(3))
austLL=float(matches.group(2))
austUR=float(matches.group(4))

cnt = 1


antPerSide=10000000
side=0

tmpdir = tempfile.mkdtemp()

r = requests.get("http://ws.geonorge.no/AdresseWS/adresse/boundingbox?nordLL=%.3f&austLL=%.3f&nordUR=%.3f&austUR=%.3f&antPerSide=%d&side=%d" % (nordLL, austLL, nordUR, austUR, antPerSide, side))
#print(r)
#print(r.text)
fpo_filename = "".join([tmpdir, "/", "insquare.osm"])

fpo = open(fpo_filename, "w")

fpo.write("<?xml version=\"1.0\"?>\n<osm version=\"0.6\" upload=\"false\" generator=\"rest-simple-fetch\">\n")
#print(r.json())
j = r.json()
if 'adresser' in j:
    for a in j['adresser']:
        current = {}
        if 'kortadressenavn' in a:
            #print(a['kortadressenavn'], end=" ")
            #print(a['husnr'], end=", ")
            #print(a['postnr'], end=" ")
            #print(a['poststed'], end="\n")
            current['ADRESSE'] = a['kortadressenavn']
            current['NUMMER'] = a['husnr']
            current['POSTNUMMER'] = a['postnr']
            current['POSTSTED'] = a['poststed']
            if 'bokstav' in a:
                current['BOKSTAV'] = a['bokstav']
            current['lat'] = float(a['nord'])
            current['lon'] = float(a['aust'])

            xmlline = "<node id=\"-%d\" lat=\"%f\" lon=\"%f\" version=\"1\" visible=\"true\">" % (cnt, current['lat'], current['lon'])
            xmlline = xmlline + "\n" + "<tag k=\"addr:postcode\" v=\"%s\" />" % (current['POSTNUMMER'])
            poststed = current['POSTSTED'].strip().title()
            poststed = poststed.replace(" I "," i ")
            poststed = poststed.replace(" På "," på ")
            xmlline = xmlline + "\n" + "<tag k=\"addr:city\" v=\"%s\" />" % (poststed)
            xmlline = xmlline + "\n" + "<tag k=\"addr:street\" v=\"%s\" />" % (current['ADRESSE'])
            if 'BOKSTAV' in current:
                husnummer = "%s%s" % (current['NUMMER'], current['BOKSTAV'])
            else:
                husnummer = "%s" % (current['NUMMER'])
            xmlline = xmlline + "\n" + "<tag k=\"addr:housenumber\" v=\"%s\" />" % (husnummer)
            xmlline = xmlline + "\n" + "</node>\n"
            fpo.write(xmlline)
            cnt = cnt + 1

fpo.write("</osm>")

fpo.close()


txt_munip_filename = "".join([tmpdir, "/", "border.txt"])

subprocess.check_output("perl /usr/lib/addrnodeimport/perl/osm2poly.pl %s > %s" % (borderfile, txt_munip_filename), shell=True)

inside_border_filename = "".join([tmpdir, "/", "newnodeswithin.osm"])

subprocess.check_output("osmosis --read-xml enableDateParsing=no file=\"%s\" --bounding-polygon file=\"%s\" --write-xml file=\"%s\"" % (fpo_filename, txt_munip_filename, inside_border_filename), shell=True)

os.system("cp \"%s\" \"%s\"" % (inside_border_filename, output_filename))

shutil.rmtree(tmpdir)

#os.system("wget -O test.htm \"http://ws.geonorge.no/AdresseWS/adresse/boundingbox?nordLL=%.3f&austLL=%.3f&nordUR=%.3f&austUR=%.3f&antPerSide=%d&side=%d\"" % (nordLL, austLL, nordUR, austUR, antPerSide, side))
