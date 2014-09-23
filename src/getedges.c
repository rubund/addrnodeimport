#include <stdio.h>
#include <libxml/parser.h>
#include <libxml/tree.h>


char verbose = 0;

void iterate_and_get_elements(xmlNode * a_node){
	xmlNode *cur_node = NULL;
	xmlAttr *attribute;
	xmlChar *text;

	double minLatitude = 200;
	double maxLatitude = -200;
	double minLongitude = 200;
	double maxLongitude = -200;
	double latitude;
	double longitude;

	for(cur_node = a_node->children; cur_node; cur_node = cur_node->next){
		if(cur_node->type == XML_ELEMENT_NODE) {
			attribute = cur_node->properties;
			if(verbose)
				printf("node type: Element, name: %s\n", cur_node->name);
			text = xmlGetProp(cur_node,"lat");
			latitude = atof(text);
			if(verbose)
				printf("  latitude: %s\n", text);
			text = xmlGetProp(cur_node,"lon");
			longitude = atof(text);
			if(verbose)
				printf("  longitude: %s\n", text);
			if(latitude > maxLatitude) maxLatitude = latitude;
			if(latitude < minLatitude) minLatitude = latitude;
			if(longitude < minLongitude) minLongitude = longitude;
			if(longitude > maxLongitude) maxLongitude = longitude;
		} 
	}

	if(verbose){
		printf("The maximum latitude is: %8.8f\n",maxLatitude);
		printf("The minimum latitude is: %8.8f\n",minLatitude);
		printf("The maximum longitude is: %8.8f\n",maxLongitude);
		printf("The minimum longitude is: %8.8f\n",minLongitude);
	}
	printf("(");
	printf("%8.7f,",minLatitude);
	printf("%8.7f,",minLongitude);
	printf("%8.7f,",maxLatitude);
	printf("%8.7f",maxLongitude);
	printf(")\n");

}

int main(int argc, char **argv){

	xmlDoc *doc = NULL;
	xmlNode *root_element = NULL;
	
	if(argc != 2){
		fprintf(stderr, "Usage: %s <osm-file>\n",argv[0]);
		return(-1);
	}

	LIBXML_TEST_VERSION

	doc = xmlReadFile(argv[1],NULL, 0);

	if(doc == NULL) {
		printf("error: could not parse file %s\n", argv[1]);
	}

	root_element = xmlDocGetRootElement(doc);

	// Here iterate
	iterate_and_get_elements(root_element);

	xmlFreeDoc(doc);

	xmlCleanupParser();

	return 0;

}

