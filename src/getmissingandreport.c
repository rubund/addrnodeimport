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
char *xmlfilename3 = NULL;
char *outputxmlfilename = NULL;
double addTo = 0;
char exists = 0;
char only_info = 0;
int number_new = 0;
int number_old = 0;
int number_nonexisting = 0;

static int sql_callback(void *info, int argc, char **argv, char **azColName){
	int i;
	exists = 1;
	if(verbose){
		printf("\n\n");
		printf("Node already exists: \n");
		for(i=0;i<argc;i++){
			printf("%s = %s\n",azColName[i],argv[i] ? argv[i] : "NULL");
		}
	}
	return 0;
}

void basic_query(sqlite3 *db,char *query, void *info){
	int ret;
	char *zErrMsg = 0;
	ret = sqlite3_exec(db,query,sql_callback, info, &zErrMsg);
	if(ret != SQLITE_OK){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
	else{
		if(verbose)
			fprintf(stdout, "Successfull query! (%s)\n",query);
	}
}

void compare_to_database(xmlNode * a_node, sqlite3 *db, xmlDoc *doc_output){
	xmlNode *cur_node = NULL;
	xmlChar *text;
	xmlNode *child_node = NULL;
	xmlNode *newNode = NULL;

	char addr_housenumber[100];
	char addr_street[256];
	char addr_postcode[10];
	char addr_city[256];

	char * querybuffer;

	char hasfound = 0;
	int number = 0;

	for(cur_node = a_node->children; cur_node; cur_node = cur_node->next){
		if(cur_node->type == XML_ELEMENT_NODE) {
			hasfound = 0;
			for(child_node = cur_node->children; child_node ; child_node = child_node->next){
				if(cur_node->type == XML_ELEMENT_NODE) {
					text = xmlGetProp(child_node, "k");
					if(text != 0){
						if(verbose)
							printf("This node: %s\n",text);
						if(strcmp(text,"addr:housenumber") == 0){
							xmlFree(text);
							text = xmlGetProp(child_node, "v");
							strncpy(addr_housenumber,text,99);
							xmlFree(text);
						}
						else if(strcmp(text,"addr:street") == 0){
							xmlFree(text);
							text = xmlGetProp(child_node, "v");
							strncpy(addr_street,text,255);
							xmlFree(text);
							hasfound = 1;
						}
						else if(strcmp(text,"addr:postcode") == 0){
							xmlFree(text);
							text = xmlGetProp(child_node, "v");
							strncpy(addr_postcode,text,9);
							xmlFree(text);
						}
						else if(strcmp(text,"addr:city") == 0){
							xmlFree(text);
							text = xmlGetProp(child_node, "v");
							strncpy(addr_city,text,255);
							xmlFree(text);
						}
						else {
							xmlFree(text);
						}
					}
				}
			}
			if(hasfound) {
				querybuffer = sqlite3_mprintf("select id,addr_street,addr_housenumber,addr_postcode,addr_city from existing where addr_street='%q' and addr_housenumber='%q'",addr_street,addr_housenumber);
				exists = 0;
				basic_query(db,querybuffer,0);
				if(exists && verbose)
					printf("Got the status also in the main \"thread\"\n");
				if(!exists){
					if(!only_info)
						printf("This node is missing:\n %s\n %s\n %s\n %s\n\n",addr_housenumber,addr_street,addr_postcode,addr_city);
					if(outputxmlfilename){
						newNode = xmlCopyNode(cur_node, 1);
						xmlNode *root_element = xmlDocGetRootElement(doc_output);
						xmlAddChild(root_element,newNode);
					}
					number_nonexisting++;
				}
				sqlite3_free(querybuffer);
				number++;
				number_new++;
			}
		}
	}
}

void populate_database(xmlNode * a_node, sqlite3 *db, char isway){
	xmlNode *cur_node = NULL;
	xmlAttr *attribute;
	xmlChar *text;
	xmlNode *child_node = NULL;

	double latitude;
	double longitude;
	static int rowcounter = 0;
	int file_index = 0;
	char *querybuffer;

	char addr_housenumber[100];
	char addr_street[256];
	char addr_postcode[10];
	char addr_city[256];
	char osmid[100];

	char hasfound = 0;

	//basic_query(db,"drop table if exists existing;");
	basic_query(db,"create table if not exists existing (id int auto_increment primary key not null, file_index int, osm_id bigint, addr_housenumber varchar(10), addr_street varchar(255), addr_postcode varchar(10), addr_city varchar(255), isway boolean, foundindataset boolean default 0);",0);

	for(cur_node = a_node->children; cur_node; cur_node = cur_node->next){
		if(cur_node->type == XML_ELEMENT_NODE) {
			hasfound = 0;
			text = xmlGetProp(cur_node, "id");
			if(text == 0) continue;
			strncpy(osmid,text,99);
			xmlFree(text);
			for(child_node = cur_node->children; child_node ; child_node = child_node->next){
				if(cur_node->type == XML_ELEMENT_NODE) {
					text = xmlGetProp(child_node, "k");
					if(text != 0){
						if(verbose)
							printf("This node: %s\n",text);
						if(strcmp(text,"addr:housenumber") == 0){
							xmlFree(text);
							text = xmlGetProp(child_node, "v");
							strncpy(addr_housenumber,text,99);
							xmlFree(text);
						}
						else if(strcmp(text,"addr:street") == 0){
							xmlFree(text);
							text = xmlGetProp(child_node, "v");
							strncpy(addr_street,text,255);
							hasfound = 1;
							xmlFree(text);
						}
						else if(strcmp(text,"addr:postcode") == 0){
							xmlFree(text);
							text = xmlGetProp(child_node, "v");
							strncpy(addr_postcode,text,9);
							xmlFree(text);
						}
						else if(strcmp(text,"addr:city") == 0){
							xmlFree(text);
							text = xmlGetProp(child_node, "v");
							strncpy(addr_city,text,255);
							xmlFree(text);
						}
						else {
							xmlFree(text);
						}
					}
				}
			}
			if(hasfound) {
				querybuffer = sqlite3_mprintf("insert into existing (id,file_index,osm_id,addr_housenumber,addr_street,addr_postcode,addr_city,isway) values (%d,%d,'%q','%q','%q','%q','%q','%d');",rowcounter,file_index,osmid,addr_housenumber,addr_street,addr_postcode,addr_city,(int)isway);
				basic_query(db,querybuffer,0);
				sqlite3_free(querybuffer);
				rowcounter++;
				number_old++;
				file_index++;
			}
		} 
	}
	basic_query(db,"create index if not exists file_index_index on existing (file_index ASC);",0);
	basic_query(db,"create index if not exists addr_street_index on existing (addr_street ASC);",0);
	basic_query(db,"create index if not exists addr_housenumber_index on existing (addr_housenumber ASC);",0);

}

int parse_cmdline(int argc, char **argv){
	int s;
	opterr = 0;
	while((s = getopt(argc, argv, "vso:w:")) != -1) {
		switch (s) {
			case 's':
				only_info = 1;
				break;
			case 'v':
				verbose = 1;
				break;
			case 'o':
				outputxmlfilename = (char*) malloc(strlen(optarg)+1);
				snprintf(outputxmlfilename,strlen(optarg)+1,"%s",optarg);
				break;
			case 'w':
				xmlfilename3 = (char*) malloc(strlen(optarg)+1);
				snprintf(xmlfilename3,strlen(optarg)+1,"%s",optarg);
				break;
			case '?':
				if(optopt == 'o')
					fprintf(stderr, "Option -%c requires an argument.\n",optopt);
				else if(optopt == 'w')
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

	xmlDoc *doc_old = NULL;
	xmlDoc *doc = NULL;
	xmlDoc *doc_output = NULL;
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


	doc_old = xmlReadFile(xmlfilename1,NULL, 0);
	if(doc_old == NULL) {
		printf("error: could not parse file %s\n", xmlfilename1);
	}
	root_element = xmlDocGetRootElement(doc_old);
	populate_database(root_element,db,0);

	// Remove all nodes from this one
	xmlNode *cur_node;
	for(cur_node = root_element->children;cur_node;){
		xmlNode *tmp_node;
		tmp_node = cur_node;
		cur_node = cur_node->next;	
		xmlUnlinkNode(tmp_node);
		xmlFreeNode(tmp_node);	
	}
	doc_output = doc_old;
	doc_old = NULL;

	if(xmlfilename3 != NULL){
		doc_old = xmlReadFile(xmlfilename3,NULL, 0);
		if(doc_old == NULL) {
			printf("error: could not parse file %s\n", xmlfilename3);
		}
		root_element = xmlDocGetRootElement(doc_old);
		populate_database(root_element,db,1);
	}


	doc = xmlReadFile(xmlfilename2,NULL, 0);
	if(doc == NULL) {
		printf("error: could not parse file %s\n", xmlfilename2);
	}
	root_element = xmlDocGetRootElement(doc);
	compare_to_database(root_element,db,doc_output);

	if(outputxmlfilename){
		xmlSaveFileEnc(outputxmlfilename, doc_output, "UTF-8");
	}


	xmlFreeDoc(doc_output);
	xmlFreeDoc(doc_old);
	xmlFreeDoc(doc);


	sqlite3_close(db);
	xmlCleanupParser();

	free(xmlfilename1);
	free(xmlfilename2);
	free(outputxmlfilename);

	printf("Existing:\t%d\n",number_old);
	printf("New:\t\t%d\n",number_new);
	printf("Missing:\t%d\n",number_nonexisting);

	return 0;

}

