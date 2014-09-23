#include <stdio.h>
#include <libxml/parser.h>
#include <libxml/tree.h>


void iterate_and_get_elements(xmlNode * a_node){
	xmlNode *cur_node = NULL;
	xmlAttr *attribute;
	xmlChar *text;

	for(cur_node = a_node->children; cur_node; cur_node = cur_node->next){
		if(cur_node->type == XML_ELEMENT_NODE) {
			attribute = cur_node->properties;
			printf("node type: Element, name: %s\n", cur_node->name);
			text = xmlGetProp(cur_node,"lat");
			printf("  latitude: %s\n", text);
			text = xmlGetProp(cur_node,"lon");
			printf("  longitude: %s\n", text);
		}
	}

}

int main(int argc, char **argv){

	xmlDoc *doc = NULL;
	xmlNode *root_element = NULL;
	
	if(argc != 2)
		return(-1);

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

