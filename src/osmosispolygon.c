/*
 *	osmosispolygon.c
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
char *veivegfilename = NULL;
char *duplicatefilename = NULL;
char *extranodesfilename = NULL;
double addTo = 0;
char exists = 0;
char only_info = 0;
int number_new = 0;
int number_old = 0;
int number_nonexisting = 0;
int number_veivegfixes = 0;
int number_nodeswithotherthings = 0;
int number_duplicates = 0;
int rowcounter = 0;
char foundid[20];
char foundisway;
FILE *tmpfile_handle;

static int sql_callback(void *info, int argc, char **argv, char **azColName){
	int i;
	exists = 1;
	if(verbose){
		printf("\n\n");
		printf("Node already exists: \n");
	}
	foundid[0] = 0;	
	foundisway = 0;	
	for(i=0;i<argc;i++){
		if(verbose)
			printf("%s = %s\n",azColName[i],argv[i] ? argv[i] : "NULL");
		if(strcmp(azColName[i],"file_index") == 0) {
			strncpy(foundid,argv[i],19);
			//printf("found and copy %s\n",argv[i]);
		}
		else if(strcmp(azColName[i],"isway") == 0) {
			//printf("Isway: %s\n",argv[i]);
			if(strcmp(argv[i],"1") == 0)
				foundisway = 1;
			else
				foundisway = 0;
			//printf("Isway set: %d\n",foundisway);
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

xmlNode *get_xml_node(xmlDoc *doc,int index,int isway){
	xmlNode *tmp_node;
	xmlChar *text;
	char hasfound2=0;
	int i = 0;
	tmp_node = xmlDocGetRootElement(doc)->children;
	//char latest[50];
	//printf("\nId to find: %d\n",foundid_int);
	//printf("Copy at %d in %d\n",foundid_int, foundisway);
	while(tmp_node->next != NULL){
		tmp_node = tmp_node->next;
		char bool_tmp;
		if(isway)
			bool_tmp = (strcmp(tmp_node->name,"way") == 0);
		else
			bool_tmp = (strcmp(tmp_node->name,"node") == 0);

		if(tmp_node->type == XML_ELEMENT_NODE && bool_tmp) {
			//printf("Here\n");
			xmlNode *tag_node;
			hasfound2 = 0;
			for(tag_node = tmp_node->children; tag_node ; tag_node = tag_node->next){
				if(tag_node->type == XML_ELEMENT_NODE) {
					text = xmlGetProp(tag_node, "k");
					if(text != 0){
						//printf(" found key: %s\n",text);
						if(strcmp(text,"OBJTYPE") == 0){
							xmlFree(text);
							hasfound2 = 1;
						}
						else
							xmlFree(text);
					}
				}
			}
			if(hasfound2) {
				if(i==index){
					break;
				}
				i++;	
			}
		}
	}
	return tmp_node;
}

void populate_database(xmlDoc *doc_old1, sqlite3 *db, xmlDoc *doc_output1){
	xmlNode *cur_node = NULL;
	xmlAttr *attribute;
	xmlChar *text;
	xmlNode *child_node = NULL;

	int file_index = 0;
	char *querybuffer;
	sqlite3_stmt *stmt;
	int ret;

	char addr_housenumber[100];
	char addr_street[256];
	char addr_postcode[10];
	char addr_city[256];
	char osmid[100];
	char latitude[100];
	char longitude[100];

	char hasfound = 0;
	xmlNode *a_node = xmlDocGetRootElement(doc_old1);

	//basic_query(db,"drop table if exists existing;");
	basic_query(db,"create table if not exists objects (id int auto_increment primary key not null, file_index int, osm_id bigint, latitude double, longitude double, first_nd int, last_nd int, isway boolean);",0);

	for(cur_node = a_node->children; cur_node; cur_node = cur_node->next){
		addr_housenumber[0] = 0;
		addr_street[0] = 0;
		addr_postcode[0] = 0;
		addr_city[0] = 0;
		if(cur_node->type == XML_ELEMENT_NODE) {
			char bool_tmp;
			hasfound = 0;
			text = xmlGetProp(cur_node, "id");
			if(text == 0) continue;
			strncpy(osmid,text,99);
			xmlFree(text);
			int tag_number = 0;
			int isbuilding = 0;
			bool_tmp = (strcmp(cur_node->name,"node") == 0);
			if(bool_tmp) {
				text = xmlGetProp(cur_node, "lat");
				if(text == 0) continue;
				strncpy(latitude,text,99);
				xmlFree(text);
				text = xmlGetProp(cur_node, "lon");
				if(text == 0) continue;
				strncpy(longitude,text,99);
				xmlFree(text);
				xmlNode *newNode_intern;
				newNode_intern = xmlCopyNode(cur_node,2);
				xmlNode *root_element_intern = xmlDocGetRootElement(doc_output1);
				xmlAddChild(root_element_intern,newNode_intern);
				
				querybuffer = sqlite3_mprintf("insert into objects (id,osm_id,latitude,longitude,file_index,isway) values (%d,'%q','%q','%q','%d',0);",rowcounter,osmid,latitude,longitude,file_index);
				basic_query(db,querybuffer,0);
				sqlite3_free(querybuffer);
				rowcounter++;
				file_index++;
			}
		} 
	}
	//basic_query(db,"create index if not exists file_index_index on existing (file_index ASC);",0);
	//basic_query(db,"create index if not exists addr_street_index on existing (addr_street ASC);",0);
	//basic_query(db,"create index if not exists addr_housenumber_index on existing (addr_housenumber ASC);",0);

}

void iterate_ways(xmlDoc *doc_old1, sqlite3 *db, xmlDoc *doc_output1, xmlNode *bigway){
	xmlNode *a_node = xmlDocGetRootElement(doc_old1);
	xmlNode *cur_node = NULL;
	xmlChar *text;
	xmlNode *child_node = NULL;
	char hasfound = 0;
	char osmid[100];
	char *querybuffer;
	sqlite3_stmt *stmt;
	int ret;
	xmlNode *new_node = NULL;
	int file_index = 0;
	int first;
	int last;
	char donotinclude=0;

	for(cur_node = a_node->children; cur_node; cur_node = cur_node->next){
		if(cur_node->type == XML_ELEMENT_NODE) {
			char bool_tmp;
			hasfound = 0;
			donotinclude = 0;
			text = xmlGetProp(cur_node, "id");
			if(text == 0) continue;
			strncpy(osmid,text,99);
			xmlFree(text);
			int tag_number = 0;
			int isbuilding = 0;
			int started = 0;
			first = 0;
			last = 0;
			
			bool_tmp = (strcmp(cur_node->name,"way") == 0);
			if(cur_node->type == XML_ELEMENT_NODE && bool_tmp) {
				text = xmlGetProp(cur_node, "id");
				if(verbose)
					printf("Way-id: %s\n",text);
				xmlFree(text);
				for(child_node = cur_node->children; child_node ; child_node = child_node->next){
					text = xmlGetProp(child_node, "ref");
					if(text != 0){
						if(verbose)
							printf(" node-id: %s\n",text);
						querybuffer = sqlite3_mprintf("select latitude,longitude,file_index from objects where osm_id='%q' and isway=0;",text);
						if(started ==0){
							first = atoi(text);
							started = 1;
						}
						last = atoi(text);
						ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt,0);
						sqlite3_free(querybuffer);
						hasfound = 1;
						while((ret = sqlite3_step(stmt)) == SQLITE_ROW){
							//printf("Latitude: %10.5f, Longitude: %10.5f\n",atof(sqlite3_column_text(stmt,0)),atof(sqlite3_column_text(stmt,1)));
							//new_node = xmlNewNode(NULL,"nd");
							//xmlSetProp(new_node,"ref", text);
							//xmlAddChild(bigway,new_node);
							xmlFree(text);

							//xmlNode *newNode_intern;
							//newNode_intern = xmlCopyNode(get_xml_node(doc_old1,atoi(sqlite3_column_text(stmt,2)),0), 1);
							//xmlNode *root_element_intern = xmlDocGetRootElement(doc_output1);
							//xmlAddChild(root_element_intern,newNode_intern);
						}
						sqlite3_finalize(stmt);
					}
					text = xmlGetProp(child_node, "k");
					if(text != 0){
						//printf("%s: ",text);
						if(strcmp(text,"OBJTYPE") == 0){
							xmlFree(text);
							text = xmlGetProp(child_node,"v");
							if(strcmp(text,"Grunnlinje") == 0){
								donotinclude = 1;
								if(verbose)
									printf("Skipping way\n");
							}
							xmlFree(text);
						}
						else
							xmlFree(text);
					}
				}
			}
			if(hasfound){
				if(!donotinclude){
					querybuffer = sqlite3_mprintf("insert into objects (id,osm_id,first_nd,last_nd,file_index,isway) values (%d,'%q','%d','%d','%d',1);",rowcounter,osmid,first,last,file_index);
					if(verbose)
						printf("%s\n",querybuffer);
					basic_query(db,querybuffer,0);
					sqlite3_free(querybuffer);
					rowcounter++;
				}
				file_index++;
			}
		} 
	}

}

void create_one_polygon(xmlDoc *doc_old, sqlite3 *db, xmlDoc *doc_output1, xmlNode *bigway){
	char *querybuffer;
	sqlite3_stmt *stmt;
	int ret;
	xmlChar *text;
	int first=0;
	int last=0;
	int veryfirst=-1;
	int started=0;
	char thisisreverse=0;
	char lastwasreverse=0;
	char found = 0;
	int lastid = -99;
	while(last != veryfirst){
		if(verbose)
			printf("run\n");
		found = 0;
		if(started == 0)
			querybuffer = sqlite3_mprintf("select first_nd,last_nd,osm_id,id,file_index from objects where isway = 1 limit 1;");
		else {
			if(!lastwasreverse){
				if(thisisreverse)
					querybuffer = sqlite3_mprintf("select first_nd,last_nd,osm_id,id,file_index from objects where isway = 1 and last_nd='%d' and id != '%d' limit 1;",last,lastid);
				else
					querybuffer = sqlite3_mprintf("select first_nd,last_nd,osm_id,id,file_index from objects where isway = 1 and first_nd='%d' and id != '%d' limit 1;",last,lastid);
			}
			else {
				if(thisisreverse)
					querybuffer = sqlite3_mprintf("select first_nd,last_nd,osm_id,id,file_index from objects where isway = 1 and last_nd='%d' and id != '%d' limit 1;",first,lastid);
				else
					querybuffer = sqlite3_mprintf("select first_nd,last_nd,osm_id,id,file_index from objects where isway = 1 and first_nd='%d' and id != '%d' limit 1;",first,lastid);
			}
		}
		if(verbose)
			printf("%s\n",querybuffer);
		ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt,0);
		sqlite3_free(querybuffer);
		while((ret = sqlite3_step(stmt)) == SQLITE_ROW){
			found = 1;
			first = atoi(sqlite3_column_text(stmt,0));	
			last = atoi(sqlite3_column_text(stmt,1));	
			if(verbose)
				printf("Her, first:%d, last:%d\n",first,last);
			xmlNode *newNode_intern;
			if(verbose)
				printf("File-index.: %s\n",sqlite3_column_text(stmt,4));
			xmlNode *current_way = get_xml_node(doc_old,atoi(sqlite3_column_text(stmt,4)),1);
			if(verbose)
				printf("node-name: %s\n",current_way->name);
			xmlNode *child_node = NULL;
			char bool_tmp;
			text = xmlGetProp(current_way, "id");
			if(verbose)
				printf("Id: %s\n",text);
			xmlFree(text);
			xmlNode *firstreversenode = NULL;
			xmlNode *firstnode = NULL;
			for(child_node = current_way->children; child_node ; child_node = child_node->next){
				bool_tmp = (strcmp(child_node->name,"nd") == 0);
				if(child_node->type == XML_ELEMENT_NODE && bool_tmp) {
					newNode_intern = xmlCopyNode(child_node, 1);
					if(thisisreverse){
						if(firstreversenode == NULL){
							if(verbose)
								printf("Adding first in reverse\n");
							xmlAddChild(bigway,newNode_intern);
						}
						else {
							if(child_node->next != NULL){
								xmlAddPrevSibling(firstnode, newNode_intern);
								if(verbose)
									printf("Adding in reverse\n");
							}
						}
						firstreversenode = newNode_intern;
					}
					else {
						if(started == 0){
							if(verbose)
								printf("Adding very first\n");
							xmlAddChild(bigway,newNode_intern);
						}
						else {
                          	if (firstnode != NULL){
								if(verbose)
									printf("Adding in normal\n");
								xmlAddChild(bigway,newNode_intern);
							}
						}
					}
					text = xmlGetProp(child_node, "ref");
					if(verbose)
						printf(" Ref: %s\n",text);
					xmlFree(text);
					firstnode = newNode_intern;
					if(started == 0){
						veryfirst = first;
						started = 1;
					}
				}
			}

			lastid = atoi(sqlite3_column_text(stmt,3));
		}
		if(!found){
			if(verbose)
				printf("Reversing..\n");
			thisisreverse = !thisisreverse;
		}
		else {
			lastwasreverse = thisisreverse;
		}
		if(verbose)
			printf("thisisreverse: %d. lastwasreverse: %d\n",thisisreverse,lastwasreverse);
		sqlite3_finalize(stmt);
	}

}

int parse_cmdline(int argc, char **argv){
	int s;
	opterr = 0;
	while((s = getopt(argc, argv, "vso:w:t:d:e:")) != -1) {
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
			case '?':
				if(optopt == 'o')
					fprintf(stderr, "Option -%c requires an argument.\n",optopt);
				else if(isprint(optopt)) 
					fprintf(stderr, "Unknown option '-%c'.\n",optopt);
				return -1;
			default:
				abort();
		}
	}

	if(argc != (optind + 1)){
		fprintf(stderr,"Usage: %s <arguments> input.osm\n",argv[0]);
		return -1;
	}
	xmlfilename1 = (char*) malloc(strlen(argv[optind])+1);
	snprintf(xmlfilename1,strlen(argv[optind])+1,"%s",argv[optind]);
	//xmlfilename2 = (char*) malloc(strlen(argv[optind+1])+1);
	//snprintf(xmlfilename2,strlen(argv[optind+1])+1,"%s",argv[optind+1]);
	return 0;
}

int main(int argc, char **argv){

	xmlDoc *doc_old1 = NULL;
	xmlDoc *doc_old2 = NULL;
	xmlDoc *doc = NULL;
	xmlDoc *doc_output1 = NULL;
	xmlNode *root_element = NULL;
	xmlDoc *doc_output2 = NULL;
	xmlDoc *doc_output3 = NULL;
	xmlDoc *doc_output4 = NULL;
	xmlNode *new_node = NULL;

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


	doc_old1 = xmlReadFile(xmlfilename1,NULL, 0);
	if(doc_old1 == NULL) {
		printf("error: could not parse file %s\n", xmlfilename1);
	}

	root_element = xmlDocGetRootElement(doc_old1);
	// Remove all nodes from this one
	xmlNode *cur_node;
	for(cur_node = root_element->children;cur_node;){
		xmlNode *tmp_node;
		tmp_node = cur_node;
		cur_node = cur_node->next;	
		xmlUnlinkNode(tmp_node);
		xmlFreeNode(tmp_node);	
	}
	doc_output1 = doc_old1;
	doc_output2 = xmlCopyDoc(doc_output1,1);
	doc_output3 = xmlCopyDoc(doc_output1,1);
	doc_output4 = xmlCopyDoc(doc_output1,1);
	doc_old1 = NULL;

	doc_old1 = xmlReadFile(xmlfilename1,NULL, 0);
	if(doc_old1 == NULL) {
		printf("error: could not parse file %s\n", xmlfilename1);
	}

	populate_database(doc_old1,db,doc_output1);
	new_node = xmlNewNode(NULL,"way");
	xmlSetProp(new_node,"id", "-99999");
	xmlSetProp(new_node,"version", "1");
	xmlSetProp(new_node,"visible", "true");

	root_element = xmlDocGetRootElement(doc_output1);
	xmlAddChild(root_element,new_node);
	iterate_ways(doc_old1,db,doc_output1,new_node);
	create_one_polygon(doc_old1, db, doc_output1, new_node);
	

	if(outputxmlfilename){
		xmlSaveFileEnc(outputxmlfilename, doc_output1, "UTF-8");
	}
	if(veivegfilename){
		xmlSaveFileEnc(veivegfilename, doc_output2, "UTF-8");
	}
	if(duplicatefilename){
		xmlSaveFileEnc(duplicatefilename, doc_output3, "UTF-8");
	}
	if(extranodesfilename){
		xmlSaveFileEnc(extranodesfilename, doc_output4, "UTF-8");
	}


	xmlFreeDoc(doc_output1);
	xmlFreeDoc(doc_output2);
	xmlFreeDoc(doc_output3);
	xmlFreeDoc(doc_output4);
	xmlFreeDoc(doc_old1);
	xmlFreeDoc(doc_old2);
	xmlFreeDoc(doc);


	sqlite3_close(db);
	xmlCleanupParser();

	free(xmlfilename1);
	free(xmlfilename2);
	free(outputxmlfilename);
	if(veivegfilename != NULL)
		free(veivegfilename);
	if(duplicatefilename != NULL)
		free(duplicatefilename);
	if(extranodesfilename != NULL)
		free(extranodesfilename);

	//printf("Existing:\t%d\n",number_old);
	return 0;

}

