#!/usr/bin/env python3

import mysql.connector
import sys
import mypasswords


if len(sys.argv) != 2 and len(sys.argv) != 3:
	print ("Not correct argument")
	sys.exit(-1)

muninumber = int(sys.argv[1])

if len(sys.argv) == 3:
	mode = int(sys.argv[2])
else:
	mode = 1


db = mysql.connector.connect(host="localhost",user=mypasswords.sqldbuser,password=mypasswords.sql,database=mypasswords.sqldbname)
ecursor = db.cursor()
ecursor.execute("set names utf8")

if mode == 0:
	updatestring = "date_sub(now(),INTERVAL 0 DAY)"
else:
	updatestring = "date_sub(now(),INTERVAL 2 DAY)"
#updatestring = "now()"

uquery = "update municipalities set updated="+updatestring+" where muni_id=\""+str(muninumber)+"\";"
print (uquery)
ret = ecursor.execute(uquery)
if(ecursor.rowcount == 0):
	uquery = "insert into municipalities (updated,muni_id) values ("+updatestring+",\""+str(muninumber)+"\");"
	print (uquery)
	ret = ecursor.execute(uquery)
db.commit()
db.close()
