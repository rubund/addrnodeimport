/*
 *	getmissingandreport.c
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
#include <sqlite3.h>


char verbose = 0;
char *xmlfilename1;
char *xmlfilename2;
double addTo = 0;

static int sql_callback(void *NotUsed, int argc, char **argv, char **azColName){
	return 0;
}

void basic_query(sqlite3 *db,char *query){
	int ret;
	char *zErrMsg = 0;
	ret = sqlite3_exec(db,query,sql_callback,0, &zErrMsg);
	if(ret != SQLITE_OK){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
	else{
		if(verbose)
			fprintf(stdout, "Successfull query! (%s)\n",query);
	}
}

void populate_database(xmlNode * a_node, sqlite3 *db){
	xmlNode *cur_node = NULL;
	xmlAttr *attribute;
	xmlChar *text;
	xmlNode *child_node = NULL;

	double latitude;
	double longitude;
	int rowcounter = 0;
	char querybuffer[256];

	char addr_housenumber[100];
	char addr_street[256];
	char addr_postcode[10];
	char addr_city[256];
	char osmid[100];

	char hasfound = 0;

	//basic_query(db,"drop table if exists existing;");
	basic_query(db,"create table existing (id int auto_increment primary key not null, osm_id bigint, addr_housenumber varchar(10), addr_street varchar(255), addr_postcode varchar(10), addr_city varchar(255), isway boolean, foundindataset boolean default 0);");

	for(cur_node = a_node->children; cur_node; cur_node = cur_node->next){
		if(cur_node->type == XML_ELEMENT_NODE) {
			hasfound = 0;
			text = xmlGetProp(cur_node, "id");
			if(text == 0) continue;
			strncpy(osmid,text,99);
			for(child_node = cur_node->children; child_node ; child_node = child_node->next){
				if(cur_node->type == XML_ELEMENT_NODE) {
					text = xmlGetProp(child_node, "k");
					if(text != 0){
						if(verbose)
							printf("This node: %s\n",text);
						if(strcmp(text,"addr:housenumber") == 0){
							text = xmlGetProp(child_node, "v");
							strncpy(addr_housenumber,text,99);
						}
						else if(strcmp(text,"addr:street") == 0){
							text = xmlGetProp(child_node, "v");
							strncpy(addr_street,text,255);
							hasfound = 1;
						}
						else if(strcmp(text,"addr:postcode") == 0){
							text = xmlGetProp(child_node, "v");
							strncpy(addr_postcode,text,9);
						}
						else if(strcmp(text,"addr:city") == 0){
							text = xmlGetProp(child_node, "v");
							strncpy(addr_city,text,255);
						}
						if(verbose)
							printf("   value: %s\n",text);
					}
				}
			}
			if(hasfound) {
				char isway = 0;
				snprintf(querybuffer,255,"insert into existing (id,osm_id,addr_housenumber,addr_street,addr_postcode,addr_city,isway) values (%d,'%s','%s','%s','%s','%s','%d');",rowcounter,osmid,addr_housenumber,addr_street,addr_postcode,addr_city,isway);
				basic_query(db,querybuffer);
				rowcounter++;
			}
		} 
	}
	basic_query(db,"create index addr_street_index on existing (addr_street ASC);");
	basic_query(db,"create index addr_housenumber_index on existing (addr_housenumber ASC);");

}

int parse_cmdline(int argc, char **argv){
	int s;
	opterr = 0;
	while((s = getopt(argc, argv, "vs:")) != -1) {
		switch (s) {
			case 's':
				addTo = atof(optarg);
				break;
			case 'v':
				verbose = 1;
				break;
			case '?':
				if(optopt == 's')
					fprintf(stderr, "Option -%c requires an argument.\n",optopt);
				else if(isprint(optopt)) 
					fprintf(stderr, "Unknown option '-%c'.\n",optopt);
				return -1;
			default:
				abort();
		}
	}

	if(argc != (optind + 2)){
		fprintf(stderr,"Usage: %s <arguments> existingnodes.osm newnodes.osm\n",argv[0]);
		return -1;
	}
	xmlfilename1 = (char*) malloc(strlen(argv[optind])+1);
	snprintf(xmlfilename1,strlen(argv[optind])+1,"%s",argv[optind]);
	xmlfilename2 = (char*) malloc(strlen(argv[optind+1])+1);
	snprintf(xmlfilename2,strlen(argv[optind+1])+1,"%s",argv[optind+1]);
	return 0;
}

int main(int argc, char **argv){

	xmlDoc *doc = NULL;
	xmlNode *root_element = NULL;

	sqlite3 *db = NULL;
	int ret;
	
	if(parse_cmdline(argc, argv) != 0){
	 	return -1;
	}

	LIBXML_TEST_VERSION

	ret = sqlite3_open(":memory:", &db);
	if(ret){
		fprintf(stderr, "Cannot open database: %s\n",sqlite3_errmsg(db));
		return -1;
	}
	else {
		if(verbose)
			printf("Opened database successfully\n");
	}


	doc = xmlReadFile(xmlfilename1,NULL, 0);
	if(doc == NULL) {
		printf("error: could not parse file %s\n", xmlfilename1);
	}
	root_element = xmlDocGetRootElement(doc);
	populate_database(root_element,db);
	xmlFreeDoc(doc);


	sqlite3_close(db);
	xmlCleanupParser();

	return 0;

}

