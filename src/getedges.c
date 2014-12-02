/*
 *	getedges.c
 *
 *	(c) Ruben Undheim 2014
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
char *xmlfilename;
double addTo = 0;

void iterate_and_get_elements(xmlNode * a_node)
{
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
	printf("%8.7f,",minLatitude-addTo);
	printf("%8.7f,",minLongitude-addTo);
	printf("%8.7f,",maxLatitude+addTo);
	printf("%8.7f",maxLongitude+addTo);
	printf(")\n");

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

	if(argc != (optind + 1)){
		fprintf(stderr,"Usage: %s <arguments> inputfile.osm\n",argv[0]);
		return -1;
	}
	xmlfilename = (char*) malloc(strlen(argv[optind])+1);
	snprintf(xmlfilename,strlen(argv[optind])+1,"%s",argv[optind]);
	return 0;
}

int main(int argc, char **argv)
{

	xmlDoc *doc = NULL;
	xmlNode *root_element = NULL;
	
	if(parse_cmdline(argc, argv) != 0){
	 	return -1;
	}

	LIBXML_TEST_VERSION

	doc = xmlReadFile(xmlfilename,NULL, 0);

	if(doc == NULL) {
		printf("error: could not parse file %s\n", xmlfilename);
	}

	root_element = xmlDocGetRootElement(doc);

	// Here iterate
	iterate_and_get_elements(root_element);

	xmlFreeDoc(doc);

	xmlCleanupParser();

	return 0;

}

