#!/bin/bash

source /etc/addrnodeimport.conf

echo 'insert into update_requests (kommunenummer,ip,tid,ferdig) values (0,"min",now(),0);' | mysql -u $DBUSER -p$DBPASS $DBNAME
