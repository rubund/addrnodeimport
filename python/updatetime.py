#!/usr/bin/env python3

import mysql.connector
import sys
import mypasswords


if len(sys.argv) != 2:
	print ("Not correct argument")
	sys.exit(-1)

muninumber = int(sys.argv[1])


db = mysql.connector.connect(host="localhost",user="ruben",password=mypasswords.sql,database="beebeetle")
ecursor = db.cursor()
ecursor.execute("set names utf8")

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
