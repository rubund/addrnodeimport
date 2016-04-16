/*
 *	findchanged.c
 *
 *	(c) Ruben Undheim 2016
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>


char verbose = 0;
char *xmlfilename1;
char *xmlfilename2;
double addTo = 0;

typedef struct {
	char addr_street[256];
	char addr_city[256];
	char addr_housenumber[11];
	char addr_postcode[5];
} t_addrdata;


t_addrdata * get_addrdata(t_addrdata *out, xmlNode *node){
	xmlNode *cur_node = NULL;
	xmlChar *text;
	for(cur_node = node->children; cur_node; cur_node = cur_node->next){
		if(cur_node->type == XML_ELEMENT_NODE) {
			text = xmlGetProp(cur_node, "k");
			if(text != 0){
				if(strcmp(text,"addr:street") == 0){
					xmlFree(text);
					text = xmlGetProp(cur_node,"v");
					strncpy(out->addr_street,text,255);
					xmlFree(text);
				}
				else if(strcmp(text,"addr:housenumber") == 0){
					xmlFree(text);
					text = xmlGetProp(cur_node,"v");
					strncpy(out->addr_housenumber,text,10);
					xmlFree(text);
				}
				else if(strcmp(text,"addr:postcode") == 0){
					xmlFree(text);
					text = xmlGetProp(cur_node,"v");
					strncpy(out->addr_postcode,text,4);
					xmlFree(text);
				}
				else if(strcmp(text,"addr:city") == 0){
					xmlFree(text);
					text = xmlGetProp(cur_node,"v");
					strncpy(out->addr_city,text,255);
					xmlFree(text);
				}
			}
		}
	}
}

void iterate_and_get_elements(xmlNode * a_node1, xmlNode * a_node2, xmlDoc * doc_output)
{
	xmlNode *cur_node = NULL;
	xmlNode *cur_node2 = NULL;
	xmlNode *new_node = NULL;
	xmlAttr *attribute;
	xmlChar *text;

	double minLatitude = 200;
	double maxLatitude = -200;
	double minLongitude = 200;
	double maxLongitude = -200;
	double latitude;
	double longitude;
	double latitude2;
	double longitude2;
	double diff1;
	double diff2;
	t_addrdata addrdata1;
	t_addrdata addrdata2;
	char found;

	for(cur_node = a_node1->children; cur_node; cur_node = cur_node->next){
		if(cur_node->type == XML_ELEMENT_NODE && strcmp(cur_node->name,"node") == 0) {
			found = 0;
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
			get_addrdata(&addrdata1,cur_node);
			if(verbose) {
				printf("addr:street:\t%s\n",addrdata1.addr_street);
				printf("addr:housenumber:\t%s\n",addrdata1.addr_housenumber);
				printf("addr:postcode:\t%s\n",addrdata1.addr_postcode);
				printf("addr:city:\t%s\n",addrdata1.addr_city);
			}
			//if(latitude > maxLatitude) maxLatitude = latitude;
			//if(latitude < minLatitude) minLatitude = latitude;
			//if(longitude < minLongitude) minLongitude = longitude;
			//if(longitude > maxLongitude) maxLongitude = longitude;
			for(cur_node2 = a_node2->children; cur_node2; cur_node2 = cur_node2->next){
				if(cur_node2->type == XML_ELEMENT_NODE && strcmp(cur_node2->name,"node") == 0) {
			//		attribute = cur_node2->properties;
					text = xmlGetProp(cur_node2,"lat");
					latitude2 = atof(text);
					text = xmlGetProp(cur_node2,"lon");
					longitude2 = atof(text);
					diff1 = latitude2 - latitude;
					diff1 = diff1 > 0 ? diff1 : -diff1;
					diff2 = longitude2 - longitude;
					diff2 = diff2 > 0 ? diff2 : -diff2;
					if (diff1 < 0.00001 && diff2 < 0.00001){
						found = 1;
						get_addrdata(&addrdata2,cur_node2);
						if(verbose){
							printf("Similar position\n");
							printf("Latitude: %f\n", latitude2);
							printf("Longitude: %f\n", longitude2);
							printf("addr:street:\t%s\n",addrdata2.addr_street);
							printf("addr:housenumber:\t%s\n",addrdata2.addr_housenumber);
							printf("addr:postcode:\t%s\n",addrdata2.addr_postcode);
							printf("addr:city:\t%s\n",addrdata2.addr_city);
						}
						printf("%20s %5s, %5s %20s    replaced by   %20s %5s, %5s %20s\n",addrdata1.addr_street,addrdata1.addr_housenumber,addrdata1.addr_postcode,addrdata1.addr_city,addrdata2.addr_street,addrdata2.addr_housenumber,addrdata2.addr_postcode,addrdata2.addr_city);
						new_node = xmlCopyNode(cur_node,1);

						xmlNode *tag_node;
						tag_node = xmlNewNode(NULL,"tag");
						xmlSetProp(tag_node,"k","addr:postcode");
						xmlSetProp(tag_node,"v",addrdata2.addr_postcode);
						xmlAddChild(new_node,tag_node);

						tag_node = xmlNewNode(NULL,"tag");
						xmlSetProp(tag_node,"k","addr:city");
						xmlSetProp(tag_node,"v",addrdata2.addr_city);
						xmlAddChild(new_node,tag_node);

						tag_node = xmlNewNode(NULL,"tag");
						xmlSetProp(tag_node,"k","addr:housenumber");
						xmlSetProp(tag_node,"v",addrdata2.addr_housenumber);
						xmlAddChild(new_node,tag_node);

						tag_node = xmlNewNode(NULL,"tag");
						xmlSetProp(tag_node,"k","addr:street");
						xmlSetProp(tag_node,"v",addrdata2.addr_street);
						xmlAddChild(new_node,tag_node);

						xmlSetProp(new_node,"action","modify");
						xmlNode *root_element_new = xmlDocGetRootElement(doc_output);
						xmlAddChild(root_element_new,new_node);
					}
				}
				if (found)
					break;
			} 
		}
	}

}

int parse_cmdline(int argc, char **argv)
{
	int s;
	opterr = 0;
	while((s = getopt(argc, argv, "vm:")) != -1) {
		switch (s) {
			case 'm':
				addTo = atof(optarg);
				break;
			case 'v':
				verbose = 1;
				break;
			case '?':
				if(optopt == 'm')
					fprintf(stderr, "Option -%c requires an argument.\n",optopt);
				else if(isprint(optopt)) 
					fprintf(stderr, "Unknown option '-%c'.\n",optopt);
				return -1;
			default:
				abort();
		}
	}

	if(argc != (optind + 2)){
		fprintf(stderr,"Usage: %o [options] notmatched.osm newnodes.osm\n",argv[0]);
		return -1;
	}
	xmlfilename1 = (char*) malloc(strlen(argv[optind])+1);
	xmlfilename2 = (char*) malloc(strlen(argv[optind+1])+1);
	snprintf(xmlfilename1,strlen(argv[optind])+1,"%s",argv[optind]);
	snprintf(xmlfilename2,strlen(argv[optind+1])+1,"%s",argv[optind+1]);
	return 0;
}

int main(int argc, char **argv)
{

	xmlDoc *doc1 = NULL;
	xmlDoc *doc2 = NULL;
	xmlDoc *doc_output = NULL;
	xmlNode *root_element1 = NULL;
	xmlNode *root_element2 = NULL;
	
	if(parse_cmdline(argc, argv) != 0){
	 	return -1;
	}

	LIBXML_TEST_VERSION

	doc1 = xmlReadFile(xmlfilename1,NULL, 0);
	doc2 = xmlReadFile(xmlfilename2,NULL, 0);

	if(doc1 == NULL) {
		printf("error: could not parse file %s\n", xmlfilename1);
	}
	if(doc2 == NULL) {
		printf("error: could not parse file %s\n", xmlfilename2);
	}

	doc_output = xmlCopyDoc(doc1,1);
	xmlNode * root;
	root = xmlDocGetRootElement(doc_output);
	xmlNode *cur_node;
	for(cur_node = root->children;cur_node;){
		xmlNode *tmp_node;
		tmp_node = cur_node;
		cur_node = cur_node->next;
		xmlUnlinkNode(tmp_node);
		xmlFreeNode(tmp_node);
	}

	root_element1 = xmlDocGetRootElement(doc1);
	root_element2 = xmlDocGetRootElement(doc2);


	// Here iterate
	iterate_and_get_elements(root_element1, root_element2, doc_output);

	xmlSaveFileEnc("test.osm", doc_output, "UTF-8");
	xmlFreeDoc(doc_output);
	xmlFreeDoc(doc1);
	xmlFreeDoc(doc2);

	xmlCleanupParser();

	return 0;

}

