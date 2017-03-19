#!/usr/bin/env python3


import re
import sys
import pyproj

fp = open(sys.argv[1], "r")
filecontent = fp.read()
fp.close()


lines = filecontent.split("\n")

state = 0
pid = -1

rpunkt      = re.compile(r'^.*?PUNKT ([^ ]+):.*$')
rend        = re.compile(r'^.*?NØ')

raddr       = re.compile(r'^\.*?ADRESSENAVN "([^"]+)".*$')
rkode       = re.compile(r'^\.*?ADRESSEKODE ([^ ]+).*$')
rnum        = re.compile(r'^\.*?NUMMER ([^ ]+).*$')
rbokstav    = re.compile(r'^\.*?BOKSTAV "([^"]+)".*$')
rpostnum    = re.compile(r'^\.*?POSTNUMMER ([^ ]+).*$')
rpoststed   = re.compile(r'^\.*?POSTSTED "([^"]+)".*$')

current = {}
counter = 0

def getCoordSys(index):
    if index == 21:
        return "+proj=utm +zone=31 +ellps=GRS80 +units=m +no_defs "
    if index == 22:
        return "+proj=utm +zone=32 +ellps=GRS80 +units=m +no_defs "
    if index == 23:
        return "+proj=utm +zone=33 +ellps=GRS80 +units=m +no_defs "
    if index == 24:
        return "+proj=utm +zone=34 +ellps=GRS80 +units=m +no_defs "
    if index == 25:
        return "+proj=utm +zone=35 +ellps=GRS80 +units=m +no_defs "
    if index == 26:
        return "+proj=utm +zone=36 +ellps=GRS80 +units=m +no_defs "
    if index == 31:
        return "+proj=utm +zone=31 +ellps=intl +units=m +no_defs "
    if index == 32:
        return "+proj=utm +zone=32 +ellps=intl +units=m +no_defs "
    if index == 33:
        return "+proj=utm +zone=33 +ellps=intl +units=m +no_defs "
    if index == 34:
        return "+proj=utm +zone=34 +ellps=intl +units=m +no_defs "
    if index == 35:
        return "+proj=utm +zone=35 +ellps=intl +units=m +no_defs "
    if index == 36:
        return "+proj=utm +zone=36 +ellps=intl +units=m +no_defs "

    return ""
 

inproj  = None  # Will be set with input from file
outproj = pyproj.Proj('+proj=latlon +datum=WGS84')

cnt = 1

print("<?xml version=\"1.0\"?>\n<osm version=\"0.6\" upload=\"false\" generator=\"python-script\">\n")

for l in lines:
    if state == 0:
        m = re.search(r'TEGNSETT ([^ ]+)', l)
        if m != None:
            if m.group(1) != "UTF-8":
                print("Aborting: only supports UTF-8")
                sys.exit(-1)
        m = re.search(r'^\.*KOORDSYS ([^ ]+)', l)
        if m != None:
            koordsys = int(m.group(1))
            inproj  = pyproj.Proj(getCoordSys(koordsys))
            state = 1
    elif state == 1:
        m = rpunkt.match(l)
        if m != None:
            pid = int(m.group(1))
            #print(pid)
            current = {}
            state = 2
    elif state == 2:
        m = raddr.match(l)
        if m != None:
            current['ADRESSE'] = m.group(1)
            continue
        m = rnum.match(l)
        if m != None:
            current['NUMMER'] = m.group(1)
            continue
        m = rbokstav.match(l)
        if m != None:
            current['BOKSTAV'] = m.group(1)
            continue
        m = rpostnum.match(l)
        if m != None:
            current['POSTNUMMER'] = m.group(1)
            continue
        m = rpoststed.match(l)
        if m != None:
            current['POSTSTED'] = m.group(1)
            continue
        m = rend.match(l)
        if m != None:
            state = 3
            continue
    elif state == 3:
        m = re.search(r'([^ ]+) ([^ ]+)', l)
        if m != None:
            x = float(int(m.group(1))) / 100
            y = float(int(m.group(2))) / 100
            #print(x)
            #print(y)
            a = pyproj.transform(inproj, outproj, y, x)
            #print(a)
            xo = a[0]
            yo = a[1]
            current['lon'] = xo
            current['lat'] = yo
        counter = counter + 1
        #print(current)
        if 'NUMMER' in current and 'ADRESSE' in current:
            xmlline = "<node id=\"-%d\" lat=\"%f\" lon=\"%f\" version=\"1\" visible=\"true\">" % (cnt, current['lat'], current['lon'])
            xmlline = xmlline + "\n" + "<tag k=\"addr:postcode\" v=\"%s\" />" % (current['POSTNUMMER'])
            poststed = current['POSTSTED'].title()
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
        state = 1
        continue
    # TODO: Make sure last node is also handled
    
print("</osm>")
    
