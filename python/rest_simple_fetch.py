#!/usr/bin/env python3


import re
import os
import sys
import subprocess

borderfile = sys.argv[1]

corners = subprocess.check_output("./src/getedges %s" % (borderfile), shell=True)
cornersd = corners.decode('utf-8').strip()

m1 = re.compile(r"\(([^,]+),([^,]+),([^,]+),([^,]+)\)")

matches = m1.match(cornersd)

if matches == None:
    print("Failed")
    sys.exit(-1)

print(matches)

nordLL=float(matches.group(1))
nordUR=float(matches.group(3))
austLL=float(matches.group(2))
austUR=float(matches.group(4))


antPerSide=20
side=0

os.system("wget -O test.htm \"http://ws.geonorge.no/AdresseWS/adresse/boundingbox?nordLL=%.3f&austLL=%.3f&nordUR=%.3f&austUR=%.3f&antPerSide=%d&side=%d\"" % (nordLL, austLL, nordUR, austUR, antPerSide, side))
