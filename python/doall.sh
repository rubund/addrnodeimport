#!/bin/sh

./addrnodeimport.py /home/ruben/Vegdata_Norge_UTM33_Geometri_SOSI.zip $1
./uploadmunip.py $1
