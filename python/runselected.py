#!/usr/bin/env python

import os


zipfile = "/lager2/ruben/kartverket/Vegdata_Norge_UTM33_Geometri_SOSI.zip"

munips = [1121,1120,1119,1122]


for m in munips:
	command ="./addrnodeimport.py "+zipfile+" %04d" % (int(m))
	os.system(command)



