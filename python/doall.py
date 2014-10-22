#!/usr/bin/env python

import os
import sys
import MySQLdb

db = MySQLdb.connect(host="localhost",user="ruben",passwd="elgelg",db="beebeetle")
cursor = db.cursor()
cursor.execute("set names utf8")

cursor.execute("select updated from municipalities where muni_id="+str(sys.argv[1])+" and updated > date_sub(now(),INTERVAL 2 DAY)")
rows = cursor.fetchall()
db.close()
if len(rows) == 0:
	os.system("./addrnodeimport.py /home/ruben/Vegdata_Norge_UTM33_Geometri_SOSI.zip "+sys.argv[1]+" 1")
	os.system("./updatetime.py "+sys.argv[1]+"")
	#os.system("./uploadmunip.py "+sys.argv[1]+"")
else:
	print "Not doing anything for "+sys.argv[1]

