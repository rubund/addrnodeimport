#!/bin/bash

../../src/getmissingandreport -s -c corrections.xml -t veiveg.xml.out -w existingways.osm existingnodes.osm newnodes.osm | tee response1-1.txt.out
../../src/getmissingandreport -s -c corrections.xml -t veiveg.xml.out existingnodes.osm newnodes.osm | tee response1-2.txt.out

diff response1-1.txt response1-1.txt.out
if [ $? -ne 0 ]
then echo "Failed test"
return -1
fi
diff response1-2.txt response1-2.txt.out
if [ $? -ne 0 ]
then echo "Failed test"
return -1
fi
