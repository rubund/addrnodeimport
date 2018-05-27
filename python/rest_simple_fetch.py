#!/usr/bin/env python3


import re
import os
import sys
import subprocess
import requests
import json

borderfile = sys.argv[1]

corners = subprocess.check_output("./src/getedges %s" % (borderfile), shell=True)
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


antPerSide=100
side=0

r = requests.get("http://ws.geonorge.no/AdresseWS/adresse/boundingbox?nordLL=%.3f&austLL=%.3f&nordUR=%.3f&austUR=%.3f&antPerSide=%d&side=%d" % (nordLL, austLL, nordUR, austUR, antPerSide, side))
#print(r)
#print(r.text)

print("<?xml version=\"1.0\"?>\n<osm version=\"0.6\" upload=\"false\" generator=\"python-script\">\n")
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
            current['lat'] = 0
            current['lon'] = 0

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
            print(xmlline)
            cnt = cnt + 1

print("</osm>")

#os.system("wget -O test.htm \"http://ws.geonorge.no/AdresseWS/adresse/boundingbox?nordLL=%.3f&austLL=%.3f&nordUR=%.3f&austUR=%.3f&antPerSide=%d&side=%d\"" % (nordLL, austLL, nordUR, austUR, antPerSide, side))
