#!/usr/bin/env python3


import re
import os
import sys
import subprocess
import requests
import json
import tempfile
import shutil
import datetime

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

requesten = "http://ws.geonorge.no/AdresseWS/adresse/boundingbox?nordLL=%.3f&austLL=%.3f&nordUR=%.3f&austUR=%.3f&antPerSide=%d&side=%d" % (nordLL, austLL, nordUR, austUR, antPerSide, side)
print(requesten)
r = requests.get(requesten)
print("Done requesting over REST API. Status code: %s" % (r.status_code))
#print(r)
#print(r.text)
fpo_filename = "".join([tmpdir, "/", "insquare.osm"])

fpo = open(fpo_filename, "w")

written_lut = {}

fpo.write("<?xml version=\"1.0\"?>\n<osm version=\"0.6\" upload=\"false\" generator=\"rest-simple-fetch\">\n")
#print(r.json())
j = r.json()
if 'adresser' in j:
    for a in j['adresser']:
        current = {}
        if 'adressenavn' in a:
            #print(a['kortadressenavn'], end=" ")
            #print(a['husnr'], end=", ")
            #print(a['postnr'], end=" ")
            #print(a['poststed'], end="\n")
            current['ADRESSE'] = a['adressenavn']
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
            adresse = current['ADRESSE']
            adresse = adresse.replace("'", "’")
            xmlline = xmlline + "\n" + "<tag k=\"addr:street\" v=\"%s\" />" % (adresse)
            if 'BOKSTAV' in current:
                husnummer = "%s%s" % (current['NUMMER'], current['BOKSTAV'])
            else:
                husnummer = "%s" % (current['NUMMER'])
            xmlline = xmlline + "\n" + "<tag k=\"addr:housenumber\" v=\"%s\" />" % (husnummer)
            xmlline = xmlline + "\n" + "</node>\n"

            #print("Has: %s" % current)
            is_duplicate = False
            if current['ADRESSE'] in written_lut:
                adresse_current = current['ADRESSE']
                already_obj = written_lut[adresse_current]

                for i in already_obj:
                    #print("Looking at: %s" % i)
                    if current['NUMMER'] == i['NUMMER'] and current['POSTNUMMER'] == i['POSTNUMMER'] and current['POSTSTED'] == i['POSTSTED']:
                        if 'BOKSTAV' in current:
                            if 'BOKSTAV' in i and current['BOKSTAV'] == i['BOKSTAV']:
                                is_duplicate = True
                                break
                        else:
                            if 'BOKSTAV' not in i:
                                is_duplicate = True
                                break


            if not is_duplicate:
                if current['ADRESSE'] not in written_lut:
                    written_lut[current['ADRESSE']] = []
                written_lut[current['ADRESSE']].append(current)
                fpo.write(xmlline)
                cnt = cnt + 1
            else:
                print("Found duplicate skipping: %s" % current)
                with open("/tmp/duplicates", "a") as fp:
                    fp.write("%s: %s\n" % (str(datetime.datetime.today()), current))

fpo.write("</osm>")

fpo.close()

print("Done writing OSM file")

txt_munip_filename = "".join([tmpdir, "/", "border.txt"])

subprocess.check_output("perl /usr/lib/addrnodeimport/perl/osm2poly.pl %s > %s" % (borderfile, txt_munip_filename), shell=True)

inside_border_filename = "".join([tmpdir, "/", "newnodeswithin.osm"])

subprocess.check_output("osmosis --read-xml enableDateParsing=no file=\"%s\" --bounding-polygon file=\"%s\" --write-xml file=\"%s\"" % (fpo_filename, txt_munip_filename, inside_border_filename), shell=True)

print("Done filtering out nodes outside border")

os.system("cp \"%s\" \"%s\"" % (inside_border_filename, output_filename))

print("Saved to %s" % (output_filename))

shutil.rmtree(tmpdir)

#os.system("wget -O test.htm \"http://ws.geonorge.no/AdresseWS/adresse/boundingbox?nordLL=%.3f&austLL=%.3f&nordUR=%.3f&austUR=%.3f&antPerSide=%d&side=%d\"" % (nordLL, austLL, nordUR, austUR, antPerSide, side))
