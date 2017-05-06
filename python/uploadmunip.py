#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from xml.dom.minidom import parse, parseString, Document
import re
import os,sys
import kommunenummer
import mypasswords
import mysql.connector
from osmapi import OsmApi


if len(sys.argv) != 2:
	print ("Usage command <osm-file>")
	sys.exit()

cachedir = "/var/cache/addrnodeimport"

munipnumber = sys.argv[1]
munipnumberpadded = "%04d" % (int(munipnumber))

report1 = open(cachedir+"/reports/report_"+str(munipnumberpadded)+".txt","r")
content = report1.read()
#match1 = re.compile(u"Existing:\s+(\d+)\s+New")
matches = re.match(r"Existing:\s+(\d+)\s+New:\s+(\d+)\s+Missing:\s+(\d+)\s+Otherthings:\s+(\d+)\s+Duplicates:\s+(\d+)\s+Veivegfixes:\s+(\d+)\s+Buildings:\s+(\d+)\s+Abandoned:\s+(\d+)\s+Notmatched:\s+(\d+)\s+NotmatchedPOIs:\s+(\d+)",content,re.MULTILINE);
report1.close()
print (matches.group(0))
missing=matches.group(3)
new=matches.group(2)
notmatched=matches.group(9)

report1 = open(cachedir+"/reports/report2_"+str(munipnumberpadded)+".txt","r")
content = report1.read()
#match1 = re.compile(u"Existing:\s+(\d+)\s+New")
report1.close()
matches = re.match(r"Fixes:\s+(\d+)\s+Errors:\s+(\d+)\s+Onlynumber:\s+(\d+)\s+",content,re.MULTILINE);
print (matches.group(0))
onlynumber=matches.group(3)
fixes=matches.group(1)
print (new)
print (notmatched)
print (missing)
print (onlynumber)
print (fixes)
if int(notmatched) == 0 and int(fixes) == 0 and int(onlynumber) == 0 and int(new) != 0 and int(missing) != 0:
	print ("Can be imported")
else:
	print ("Cannot be imported")
	sys.exit()


db = mysql.connector.connect(host="localhost",user=mypasswords.sqldbuser,password=mypasswords.sql,database=mypasswords.sqldbname)
cursor = db.cursor()
cursor.execute("set names utf8")
cursor.execute("select person from osmimportresp where kommunenummer=\""+str(munipnumber)+"\" and person != 'rubund' and deleted != 1;")
rows = cursor.fetchall()
if(len(rows) > 0):
	print ("Someone else is responsible for this one. Not importing...")
	sys.exit()
else:
	print ("Nobody is responsible for this one")

cursor.execute("select tid from update_requests where kommunenummer=\"%s\" and ferdig=0 and upload=0 order by tid desc limit 1" % (munipnumber,))
rows = cursor.fetchall()
if(len(rows) > 0):
	print("Waiting for getting updated")
	sys.exit()
	
db.close()

#api = OsmApi(api="api06.dev.openstreetmap.org", username="", password="", changesetauto=True, changesetautotags={"source":"Kartverket"})
#api = OsmApi(api="api06.dev.openstreetmap.org", username="rubund_import", passwordfile="./password.txt")
api = OsmApi(api="api.openstreetmap.org", username=mypasswords.osmuser, password=mypasswords.osm)
#api.NodeGet(123)
mycomment=u"addr node import "+kommunenummer.nrtonavn[int(munipnumber)]+" kommune"
#mycomment=u"addr node import municipality number "+munipnumberpadded+", Norway"
#api.ChangesetCreate({"comment":u"addr node import "+str(sys.argv[2].decode('utf-8')), "source":"Kartverket", "source:date":"2014-08-24"})
api.ChangesetCreate({"comment": mycomment, "source":"Kartverket", "source:date":"2017-02-23"})

dom1 = parse(cachedir+"/reports/newnodes_"+str(munipnumberpadded)+".osm")
mainelement1 = dom1.getElementsByTagName("osm")[0]
nodes = mainelement1.getElementsByTagName("node")
counter = 1
for node in nodes:
	if(node.nodeType == 1):
		housenumber = ""
		street      = ""
		postcode    = ""
		city        = ""
		osm_id = node.attributes["id"].value
		latitude = node.attributes["lat"].value
		longitude = node.attributes["lon"].value
		tags = node.getElementsByTagName("tag")
		for tag in tags:
			if tag.attributes["k"].value == "addr:housenumber":
				housenumber = tag.attributes["v"].value
			if tag.attributes["k"].value == "addr:street":
				street = tag.attributes["v"].value
			if tag.attributes["k"].value == "addr:postcode":
				postcode    = tag.attributes["v"].value
			if tag.attributes["k"].value == "addr:city":
				city        = tag.attributes["v"].value
		if housenumber != "":
			uploadnode = {"id":-counter , "lat" : latitude, "lon": longitude , "tag":{"addr:housenumber":housenumber, "addr:street":street, "addr:postcode":postcode,"addr:city":city}}
			#print (str(latitude)+""+str(longitude)+""+street+" "+housenumber)
			api.NodeCreate(uploadnode)
			print (uploadnode)
			counter = counter + 1
		
api.ChangesetClose()

db = mysql.connector.connect(host="localhost",user=mypasswords.sqldbuser,password=mypasswords.sql,database=mypasswords.sqldbname)
cursor = db.cursor()
cursor.execute("set names utf8")
cursor.execute("insert into update_requests (kommunenummer,ip,tid) values ('"+str(munipnumber)+"','script',addtime(now(),\"0:05:00\"));")
db.commit()
db.close()
#os.system("echo \"insert into update_requests (kommunenummer,ip,tid) values (543,'script',now())\" | mysql -u ruben --password="+mypasswords.sql+" beebeetle")
##api.flush()
#api.NodeCreate
