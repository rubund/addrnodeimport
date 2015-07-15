#!/usr/bin/env python

import MySQLdb
import time
import os

import mypasswords


def update_request_checker():
	while 1:
		db = MySQLdb.connect(host="localhost",user=mypasswords.sqldbuser,passwd=mypasswords.sql,db=mypasswords.sqldbname)
		cursor = db.cursor()
		ecursor = db.cursor()
		cursor.execute("set names utf8")
		ecursor.execute("set names utf8")
		cursor.execute("select distinct kommunenummer from update_requests where ferdig = 0 and (addtime(tid , '0:05:00') < now() or kommunenummer = 0);")
		rows = cursor.fetchall()
		for row in rows:
			if row[0] == 0:  # Update all
				print ("Updating all")
				command = "/usr/lib/addrnodeimport/sh/update_all.sh"
				print command
				os.system(command)
				uquery = "update update_requests set ferdig=1 where kommunenummer=\""+str(row[0])+"\";"
				print uquery
				ret = ecursor.execute(uquery)
			else:
				command = "/usr/bin/addrnodeimport.py /var/cache/addrnodeimport/Vegdata_Norge_Adresser_UTM33_SOSI.zip "+str(row[0])+" /var/cache/addrnodeimport 0"
				print command
				os.system(command)
				uquery = "update update_requests set ferdig=1 where kommunenummer=\""+str(row[0])+"\";"
				print uquery
				ret = ecursor.execute(uquery)
				uquery = "update municipalities set updated=now() where muni_id=\""+str(row[0])+"\";"
				# (id int not null primary key auto_increment, muni_id int, updated datetime);
				print uquery
				ret = ecursor.execute(uquery)
				if(ecursor.rowcount == 0):
					uquery = "insert into municipalities (updated,muni_id) values (now(),\""+str(row[0])+"\");"
					print uquery
					ret = ecursor.execute(uquery)
			db.commit()



		db.close()
		time.sleep(15)
		


update_request_checker()
