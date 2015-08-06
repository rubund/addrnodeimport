#!/usr/bin/env python3

import os


zipfile = "/home/ruben/Vegdata_Norge_Adresser_UTM33_SOSI.zip"

munips = range(0,2100)


for m in munips:
	command ="./addrnodeimport "+zipfile+" %04d" % (int(m))
	os.system(command)



