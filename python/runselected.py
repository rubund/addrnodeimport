#!/usr/bin/env python

import os


zipfile = "/home/ruben/Vegdata_Norge_UTM33_Geometri_SOSI.zip"

munips = range(0,2000)


for m in munips:
	command ="./addrnodeimport.py "+zipfile+" %04d" % (int(m))
	os.system(command)



