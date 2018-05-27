#!/usr/bin/env python3


import os

nordLL=63
nordUR=64
austLL=10.2
austUR=11

antPerSide=20
side=0

os.system("wget -O test.htm \"http://ws.geonorge.no/AdresseWS/adresse/boundingbox?nordLL=%.3f&austLL=%.3f&nordUR=%.3f&austUR=%.3f&antPerSide=%d&side=%d\"" % (nordLL, austLL, nordUR, austUR, antPerSide, side))
