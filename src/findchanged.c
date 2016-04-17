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
char *xmlfilename3;
double addTo = 0;

typedef struct {
	char addr_street[256];
	char addr_city[256];
	char addr_housenumber[11];
	char addr_postcode[5];
} t_addrdata;

int totn = 0;

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
	char latString[20];
	char lonString[20];

	for(cur_node = a_node1->children; cur_node; cur_node = cur_node->next){
		if(cur_node->type == XML_ELEMENT_NODE && strcmp(cur_node->name,"node") == 0) {
			found = 0;
			attribute = cur_node->properties;
			//if(verbose)
			//	printf("node type: Element, name: %s\n", cur_node->name);
			text = xmlGetProp(cur_node,"lat");
			latitude = atof(text);
			//if(verbose)
			//	printf("  latitude: %s\n", text);
			text = xmlGetProp(cur_node,"lon");
			longitude = atof(text);
			//if(verbose)
			//	printf("  longitude: %s\n", text);
			get_addrdata(&addrdata1,cur_node);
			if(verbose) {
				//printf("addr:street:\t%s\n",addrdata1.addr_street);
				//printf("addr:housenumber:\t%s\n",addrdata1.addr_housenumber);
				//printf("addr:postcode:\t%s\n",addrdata1.addr_postcode);
				//printf("addr:city:\t%s\n",addrdata1.addr_city);
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
					strncpy(latString,text,19);	
					text = xmlGetProp(cur_node2,"lon");
					longitude2 = atof(text);
					strncpy(lonString,text,19);	
					diff1 = latitude2 - latitude;
					diff1 = diff1 > 0 ? diff1 : -diff1;
					diff2 = longitude2 - longitude;
					diff2 = diff2 > 0 ? diff2 : -diff2;
					char sameLocation=0;
					char nearLocation=0;
					get_addrdata(&addrdata2,cur_node2);
					if (diff1 < 0.00001 && diff2 < 0.00001)
						sameLocation = 1;
					else if (diff1 < 0.001 && diff2 < 0.001){
						int length_number_1 = strlen(addrdata1.addr_housenumber);
						if( strncmp(addrdata1.addr_street,addrdata2.addr_street,255) == 0 &&
							strncmp(addrdata1.addr_postcode,addrdata2.addr_postcode,4) == 0 &&
							strncmp(addrdata1.addr_city,addrdata2.addr_city,255) == 0 && 
							strncmp(addrdata1.addr_housenumber,addrdata2.addr_housenumber,length_number_1) == 0
							)
							nearLocation = 1;
					}
					if (sameLocation || nearLocation){
						if(verbose){
							//printf("Similar position\n");
							//printf("Latitude: %f\n", latitude2);
							//printf("Longitude: %f\n", longitude2);
							//printf("addr:street:\t%s\n",addrdata2.addr_street);
							//printf("addr:housenumber:\t%s\n",addrdata2.addr_housenumber);
							//printf("addr:postcode:\t%s\n",addrdata2.addr_postcode);
							//printf("addr:city:\t%s\n",addrdata2.addr_city);
							if(nearLocation)
								printf("near");
							else if (sameLocation)
								printf("same");
							printf("%20s %5s, %5s %20s    replaced by   %20s %5s, %5s %20s (diff1: %f, diff2: %f)\n",addrdata1.addr_street,addrdata1.addr_housenumber,addrdata1.addr_postcode,addrdata1.addr_city,addrdata2.addr_street,addrdata2.addr_housenumber,addrdata2.addr_postcode,addrdata2.addr_city,diff1,diff2);
						}
						if(!found)
							new_node = xmlCopyNode(cur_node,1);
						else
							new_node = xmlCopyNode(cur_node2,1);

						xmlNode *cur_node3;
						xmlChar *text2;

						char found_housenumber=0,found_street=0,found_postcode=0,found_city=0;

						for(cur_node3 = new_node->children; cur_node3;){
							xmlNode *tmp_node;
							tmp_node = cur_node3;
							cur_node3 = tmp_node->next;
							if(tmp_node->type == XML_ELEMENT_NODE){
								text2 = xmlGetProp(tmp_node, "k");
								if(text2 != 0){
									//if(strcmp(text2,"addr:housenumber") == 0 || strcmp(text2,"addr:postcode") == 0 || strcmp(text2,"addr:street") == 0 || strcmp(text2,"addr:city") == 0){
									//	xmlFree(text2);
									//	xmlUnlinkNode(tmp_node);
									//	xmlFreeNode(tmp_node);
									//}
									if(strcmp(text2,"addr:housenumber") == 0){
										xmlFree(text2);
										xmlSetProp(tmp_node,"v",addrdata2.addr_housenumber);
										found_housenumber = 1;
									}
									else if(strcmp(text2,"addr:street") == 0){
										xmlFree(text2);
										xmlSetProp(tmp_node,"v",addrdata2.addr_street);
										found_street = 1;
									}
									else if(strcmp(text2,"addr:postcode") == 0){
										xmlFree(text2);
										xmlSetProp(tmp_node,"v",addrdata2.addr_postcode);
										found_postcode = 1;
									}
									else if(strcmp(text2,"addr:city") == 0){
										xmlFree(text2);
										xmlSetProp(tmp_node,"v",addrdata2.addr_city);
										found_city = 1;
									}
								}
							}
							else {
								//xmlUnlinkNode(tmp_node);
								//xmlFreeNode(tmp_node);
							}
						}
						xmlNode *tag_node;

						if (!found_street){
							tag_node = xmlNewNode(NULL,"tag");
							xmlSetProp(tag_node,"k","addr:street");
							xmlSetProp(tag_node,"v",addrdata2.addr_street);
							xmlAddChild(new_node,tag_node);
						}

						if (!found_housenumber){
							tag_node = xmlNewNode(NULL,"tag");
							xmlSetProp(tag_node,"k","addr:housenumber");
							xmlSetProp(tag_node,"v",addrdata2.addr_housenumber);
							xmlAddChild(new_node,tag_node);
						}

						if (!found_postcode){
							tag_node = xmlNewNode(NULL,"tag");
							xmlSetProp(tag_node,"k","addr:postcode");
							xmlSetProp(tag_node,"v",addrdata2.addr_postcode);
							xmlAddChild(new_node,tag_node);
						}

						if (!found_city){
							tag_node = xmlNewNode(NULL,"tag");
							xmlSetProp(tag_node,"k","addr:city");
							xmlSetProp(tag_node,"v",addrdata2.addr_city);
							xmlAddChild(new_node,tag_node);
						}

						if(nearLocation){
							xmlSetProp(new_node,"lat",latString);
							xmlSetProp(new_node,"lon",lonString);
						}

						if(!found){
							xmlSetProp(new_node,"action","modify");
						}
						xmlNode *root_element_new = xmlDocGetRootElement(doc_output);
						xmlAddChild(root_element_new,new_node);
						// To prevent the same node from being used several times: -->
						xmlSetProp(cur_node2,"lat","0");
						xmlSetProp(cur_node2,"lon","0");
						// <--
						totn++;
						found = 1;
					}
				}
				if(found)
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

	if(argc != (optind + 3)){
		fprintf(stderr,"Usage: %o [options] notmatched.osm newnodes.osm output.osm\n",argv[0]);
		return -1;
	}
	xmlfilename1 = (char*) malloc(strlen(argv[optind])+1);
	xmlfilename2 = (char*) malloc(strlen(argv[optind+1])+1);
	xmlfilename3 = (char*) malloc(strlen(argv[optind+2])+1);
	snprintf(xmlfilename1,strlen(argv[optind])+1,"%s",argv[optind]);
	snprintf(xmlfilename2,strlen(argv[optind+1])+1,"%s",argv[optind+1]);
	snprintf(xmlfilename3,strlen(argv[optind+2])+1,"%s",argv[optind+2]);
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

	xmlSaveFileEnc(xmlfilename3, doc_output, "UTF-8");
	xmlFreeDoc(doc_output);
	xmlFreeDoc(doc1);
	xmlFreeDoc(doc2);

	xmlCleanupParser();
	printf("Changed: %d\n",totn);

	return 0;

}

