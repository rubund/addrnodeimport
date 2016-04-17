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

report1 = open(cachedir+"/reports/report4_"+str(munipnumberpadded)+".txt","r")
content = report1.read()
#match1 = re.compile(u"Existing:\s+(\d+)\s+New")
report1.close()
matches = re.match(r"Changed:\s+(\d+)\s+",content,re.MULTILINE);
print (matches.group(0))
changed=matches.group(1)

print (new)
print (notmatched)
print (missing)
print (onlynumber)
print (fixes)
print (changed)
if int(notmatched) != 0 and int(fixes) == 0 and int(onlynumber) == 0 and int(new) != 0 and int(missing) != 0 and int(changed) != 0:
	print ("Uploading changed")
	os.system("./uploadchanged.py %d" % (int(munipnumber),))
elif int(notmatched) != 0 and int(fixes) == 0 and int(onlynumber) == 0 and int(new) != 0 and int(missing) != 0 and int(changed) == 0:
	print ("Deleting outdated nodes")
	os.system("./deletenotmatchednodes.py %d" % (int(munipnumber),))
elif int(notmatched) == 0 and int(fixes) == 0 and int(onlynumber) == 0 and int(new) != 0 and int(missing) != 0:
	print ("Uploading new nodes")
	os.system("./uploadmunip.py %d" % (int(munipnumber),))


