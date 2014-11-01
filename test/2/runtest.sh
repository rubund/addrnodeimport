#!/bin/bash

../../src/getmissingandreport -s -c corrections.xml -t veiveg.osm.out -w existingways.osm existingnodes.osm newnodes.osm | tee response1-1.txt.out
../../src/getmissingandreport -s -c corrections.xml -t veiveg.osm.out existingnodes.osm newnodes.osm | tee response1-2.txt.out
../../src/getmissingandreport -s -t veiveg.osm.out existingnodes.osm newnodes.osm | tee response1-3.txt.out
../../src/getmissingandreport -s -t veiveg.osm.out -w existingways.osm existingnodes.osm newnodes.osm | tee response1-4.txt.out
../../src/getmissingandreport -s -c corrections.xml -t veiveg.osm.out -n notmatched.osm.out -d duplicates.osm.out -e otherobjects.osm.out -o output.osm.out -w existingways.osm existingnodes.osm newnodes.osm | tee response1-5.txt.out
../../src/getmissingandreport -s -c corrections.xml -w existingways.osm existingnodes.osm newnodes.osm | tee response1-6.txt.out

diff response1-1.txt response1-1.txt.out
if [ $? -ne 0 ]
then echo "Failed test"
exit -1
fi
diff response1-2.txt response1-2.txt.out
if [ $? -ne 0 ]
then echo "Failed test"
exit -1
fi
diff response1-3.txt response1-3.txt.out
if [ $? -ne 0 ]
then echo "Failed test"
exit -1
fi
diff response1-4.txt response1-4.txt.out
if [ $? -ne 0 ]
then echo "Failed test"
exit -1
fi
diff response1-1.txt response1-5.txt.out # The number shall be the same as for 1-1
if [ $? -ne 0 ]
then echo "Failed test"
exit -1
fi
diff response1-6.txt response1-6.txt.out
if [ $? -ne 0 ]
then echo "Failed test"
exit -1
fi
diff output.osm output.osm.out 
if [ $? -ne 0 ]
then echo "Failed test"
exit -1
fi
diff veiveg.osm veiveg.osm.out 
if [ $? -ne 0 ]
then echo "Failed test"
exit -1
fi
diff notmatched.osm notmatched.osm.out 
if [ $? -ne 0 ]
then echo "Failed test"
exit -1
fi
