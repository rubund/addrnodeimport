#!/usr/bin/env python

import MySQLdb
import time
import os

import mypasswords


def update_request_checker():
	while 1:
		hasdone = False
		try:
			db = MySQLdb.connect(host="localhost",user=mypasswords.sqldbuser,passwd=mypasswords.sql,db=mypasswords.sqldbname)
			cursor = db.cursor()
			ecursor = db.cursor()
			e2cursor = db.cursor()
			cursor.execute("set names utf8")
			ecursor.execute("set names utf8")
			e2cursor.execute("set names utf8")
			cursor.execute("select distinct kommunenummer from update_requests where upload = 1 and ferdig = 0 and (tid < now());")
			rows = cursor.fetchall()
			for row in rows:
				e2cursor.execute("select kommunenummer from update_requests where kommunenummer = '"+str(row[0])+"' and upload = 0 and ferdig = 0;")
				update_rows = e2cursor.fetchall()
				if len(update_rows) == 0:
					hasdone = True
					command = "cd /usr/lib/addrnodeimport/python/ ; ./donextstep.py "+str(row[0])+""
					print command
					os.system(command)
					uquery = "update update_requests set ferdig=1 where kommunenummer=\""+str(row[0])+"\" and upload = 1;"
					print uquery
					ret = ecursor.execute(uquery)
					db.commit()



			db.close()
			if not hasdone:
				time.sleep(15)
		except MySQLdb.OperationalError:
			print("Mysql server unavailable. Trying again...")
			time.sleep(2)
		


update_request_checker()
