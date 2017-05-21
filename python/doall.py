#!/usr/bin/env python3

import os
import sys
import mysql.connector
import mypasswords

db = mysql.connector.connect(host="localhost",user=mypasswords.sqldbuser,password=mypasswords.sql,database=mypasswords.sqldbname)

cursor = db.cursor()
cursor.execute("set names utf8")

# Mode==0: Downloads existing data directly (network intensive, but fresh data)
# Mode==1: Downloads all of Norway (1-2 days old, then extracts)
mode = 1

if mode == 0:
	cursor.execute("select updated from municipalities where muni_id="+str(sys.argv[1])+" and updated > date_sub(now(),INTERVAL 0 DAY)")
else:
	cursor.execute("select updated from municipalities where muni_id="+str(sys.argv[1])+" and updated > date_sub(now(),INTERVAL 2 DAY)")

rows = cursor.fetchall()
db.close()
if len(rows) == 0:
	os.system("/usr/bin/addrnodeimport /var/cache/addrnodeimport/Vegdata_Norge_Adresser_UTM33_SOSI.zip "+sys.argv[1]+" /var/cache/addrnodeimport "+str(mode))
	os.system("/usr/lib/addrnodeimport/python/updatetime.py "+sys.argv[1]+" "+str(mode)+"")
	os.system("/usr/lib/addrnodeimport/python/uploadmunip_new.py "+sys.argv[1]+"")
else:
	print ("Not doing anything for "+sys.argv[1])

