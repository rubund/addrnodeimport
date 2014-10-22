#!/usr/bin/env python3

import os


zipfile = "/home/ruben/Vegdata_Norge_UTM33_Geometri_SOSI.zip"

munips = range(0,2100)


for m in munips:
	command ="./addrnodeimport.py "+zipfile+" %04d" % (int(m))
	os.system(command)



