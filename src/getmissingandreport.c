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
						if(strcmp(text,"addr:street") == 0){
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

void compare_to_database(xmlDoc *doc_old1, xmlDoc *doc_old2, xmlNode * a_node, sqlite3 *db, xmlDoc *doc_output, xmlDoc *doc_output2, xmlDoc *doc_output3, xmlDoc *doc_output4){
	int i;
	int ret;
	xmlNode *cur_node = NULL;
	xmlChar *text;
	xmlNode *child_node = NULL;
	xmlNode *newNode = NULL;
	sqlite3_stmt *stmt;

	char addr_housenumber[100];
	char addr_street[256];
	char addr_postcode[10];
	char addr_city[256];

	char * querybuffer;

	char hasfound = 0;
	char hasfound2 = 0;
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
			int basicnumber=0;
			if(hasfound) {
				querybuffer = sqlite3_mprintf("select id,file_index,isway,addr_street,addr_housenumber,addr_postcode,addr_city from existing where addr_street='%q' and lower(addr_housenumber)=lower('%q') and addr_postcode='%q' and addr_city='%q' and ((tag_number <= 4) or (tag_number <= 5 and building = 1))",addr_street,addr_housenumber,addr_postcode,addr_city);
				exists = 0;
				basic_query(db,querybuffer,0);
				sqlite3_free(querybuffer);
				querybuffer = sqlite3_mprintf("select count(*) from existing where addr_street='%q' and lower(addr_housenumber)=lower('%q') and addr_postcode='%q' and addr_city='%q' and ((tag_number <= 4) or (tag_number <= 5 and building = 1))",addr_street,addr_housenumber,addr_postcode,addr_city);
				ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt,0);
				if (ret){
					fprintf(stderr,"SQL Error");
					return;
				}
				sqlite3_free(querybuffer);
				sqlite3_step(stmt);
				const char *result = (const char*)sqlite3_column_text(stmt,0);
				basicnumber = atoi(result);
				sqlite3_finalize(stmt);	

				if(basicnumber > 1 && duplicatefilename != NULL){
					querybuffer = sqlite3_mprintf("select file_index,isway from existing where addr_street='%q' and lower(addr_housenumber)=lower('%q') and addr_postcode='%q' and addr_city='%q' and ((tag_number <= 4) or (tag_number <= 5 and building = 1))",addr_street,addr_housenumber,addr_postcode,addr_city);
					ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt,0);
					if (ret){
						fprintf(stderr,"SQL Error");
						return;
					}
					sqlite3_free(querybuffer);
					while((ret = sqlite3_step(stmt)) == SQLITE_ROW){
						const char *resultLoop = (const char*)sqlite3_column_text(stmt,0);
						xmlNode *newNode_intern;
						if(strcmp(sqlite3_column_text(stmt,1),"1") == 0)
							newNode_intern = xmlCopyNode(get_xml_node(doc_old2,atoi(resultLoop),1), 1);
						else
							newNode_intern = xmlCopyNode(get_xml_node(doc_old1,atoi(resultLoop),0), 1);
						xmlNode *root_element_intern = xmlDocGetRootElement(doc_output3);
						xmlAddChild(root_element_intern,newNode_intern);
					}
					sqlite3_finalize(stmt);	
				}

				//sqlite3_free(result);
				if(exists && verbose)
					printf("Got the status also in the main \"thread\"\n");
				if(!exists){
					if(veivegfilename != NULL){
						char addr_street_2[256];
						char *mysubstr = NULL;
						char hasfoundvei;
						hasfoundvei = 0;
						strncpy(addr_street_2,addr_street,255);
						if(!hasfoundvei){
							mysubstr = strstr(addr_street_2,"veg");
							if(mysubstr != NULL){
								*(mysubstr+2) = 'i';
								hasfoundvei = 1;
							}
						}
						if(!hasfoundvei){
							mysubstr = strstr(addr_street_2,"vei");
							if(mysubstr != NULL){
								*(mysubstr+2) = 'g';
								hasfoundvei = 1;
							}
						}
						if(!hasfoundvei){
							mysubstr = strstr(addr_street_2,"Vei");
							if(mysubstr != NULL){
								*(mysubstr+2) = 'g';
								hasfoundvei = 1;
							}
						}
						if(!hasfoundvei){
							mysubstr = strstr(addr_street_2,"veg");
							if(mysubstr != NULL){
								*(mysubstr+2) = 'i';
								hasfoundvei = 1;
							}
						}
						if(hasfoundvei){
							if(verbose)
								printf("Has changed %s to %s\n",addr_street,addr_street_2);
							exists = 0;
							querybuffer = sqlite3_mprintf("select id,file_index,isway,addr_street,addr_housenumber,addr_postcode,addr_city from existing where addr_street='%q' and lower(addr_housenumber)=lower('%q') and addr_postcode='%q' and addr_city='%q' and ((tag_number <= 4) or (tag_number <= 5 and building = 1))",addr_street_2,addr_housenumber,addr_postcode,addr_city);
							basic_query(db,querybuffer,0);
							sqlite3_free(querybuffer);
							if(exists) {
								if(verbose) {
									printf("There is probably a veg/vei mistake\n");
									printf("Copy at %s in %d\n",foundid, foundisway);
								}
								xmlNode *tmp_node;
								int foundid_int;
								foundid_int = atoi(foundid);
								if(foundisway)
									tmp_node = get_xml_node(doc_old2,foundid_int,foundisway);
								else
									tmp_node = get_xml_node(doc_old1,foundid_int,foundisway);

								//printf("latest: %s\n",latest);
								if(tmp_node != NULL) {
									//printf("tmp_node: %d: %s\n",(int)tmp_node,tmp_node->name);
									xmlNode *tag_node;
									for(tag_node = tmp_node->children; tag_node ; tag_node = tag_node->next){
										if(tag_node->type == XML_ELEMENT_NODE) {
											text = xmlGetProp(tag_node, "k");
											if(text != 0){
												if(strcmp(text,"addr:street") == 0){
													//printf("text here: %s\n",text);
													xmlFree(text);
													xmlSetProp(tag_node, "v",addr_street);
												}
											}
										}
									}
									xmlSetProp(tmp_node,"action","modify");
									//printf("At %s %s (%s):\n",addr_street,addr_housenumber,addr_street_2);
									newNode = xmlCopyNode(tmp_node, 1);
									xmlNode *root_element = xmlDocGetRootElement(doc_output2);
									xmlAddChild(root_element,newNode);
									number_veivegfixes++;
								}
							}
						}
					}
					
					if(!only_info)
						printf("This node is missing:\n %s\n %s\n %s\n %s\n\n",addr_housenumber,addr_street,addr_postcode,addr_city);
					if(outputxmlfilename){
						newNode = xmlCopyNode(cur_node, 1);
						xmlNode *root_element = xmlDocGetRootElement(doc_output);
						xmlAddChild(root_element,newNode);
					}
					number_nonexisting++;
				}
				if(basicnumber > 1)
					number_duplicates++;
				querybuffer = sqlite3_mprintf("select count(*) from existing where addr_street='%q' and lower(addr_housenumber)=lower('%q') and addr_postcode='%q' and addr_city='%q' and ((tag_number > 4 and building = 0) or (tag_number > 5))",addr_street,addr_housenumber,addr_postcode,addr_city);
				ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt,0);
				if (ret){
					fprintf(stderr,"SQL Error");
					return;
				}
				sqlite3_free(querybuffer);
				sqlite3_step(stmt);
				const char *result2 = (const char*)sqlite3_column_text(stmt,0);
				basicnumber = atoi(result2);
				number_nodeswithotherthings	+= basicnumber;
				sqlite3_finalize(stmt);	

				if(basicnumber > 0 && extranodesfilename != NULL){
					querybuffer = sqlite3_mprintf("select file_index,isway from existing where addr_street='%q' and lower(addr_housenumber)=lower('%q') and addr_postcode='%q' and addr_city='%q' and ((tag_number > 4 and building = 0) or (tag_number > 5))",addr_street,addr_housenumber,addr_postcode,addr_city);
					ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt,0);
					if (ret){
						fprintf(stderr,"SQL Error");
						return;
					}
					sqlite3_free(querybuffer);
					while((ret = sqlite3_step(stmt)) == SQLITE_ROW){
						const char *resultLoop = (const char*)sqlite3_column_text(stmt,0);
						xmlNode *newNode_intern;
						if(strcmp(sqlite3_column_text(stmt,1),"1") == 0)
							newNode_intern = xmlCopyNode(get_xml_node(doc_old2,atoi(resultLoop),1), 1);
						else
							newNode_intern = xmlCopyNode(get_xml_node(doc_old1,atoi(resultLoop),0), 1);
						xmlNode *root_element_intern = xmlDocGetRootElement(doc_output4);
						xmlAddChild(root_element_intern,newNode_intern);
					}
					sqlite3_finalize(stmt);	
				}

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
	basic_query(db,"create table if not exists existing (id int auto_increment primary key not null, file_index int, osm_id bigint, addr_housenumber varchar(10), addr_street varchar(255), addr_postcode varchar(10), addr_city varchar(255), isway boolean, tag_number int, building boolean, foundindataset boolean default 0);",0);

	for(cur_node = a_node->children; cur_node; cur_node = cur_node->next){
		addr_housenumber[0] = 0;
		addr_street[0] = 0;
		addr_postcode[0] = 0;
		addr_city[0] = 0;
		if(cur_node->type == XML_ELEMENT_NODE) {
			hasfound = 0;
			text = xmlGetProp(cur_node, "id");
			if(text == 0) continue;
			strncpy(osmid,text,99);
			xmlFree(text);
			int tag_number = 0;
			int isbuilding = 0;
			for(child_node = cur_node->children; child_node ; child_node = child_node->next){
				char bool_tmp;
				if(isway)
					bool_tmp = (strcmp(cur_node->name,"way") == 0);
				else
					bool_tmp = (strcmp(cur_node->name,"node") == 0);
				if(cur_node->type == XML_ELEMENT_NODE && bool_tmp) {
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
						else if(strcmp(text,"building") == 0){
							xmlFree(text);
							text = xmlGetProp(child_node, "v");
							//strncpy(addr_city,text,255);
							xmlFree(text);
							isbuilding = 1;
						}
						else if(strcmp(text,"source") == 0){
							xmlFree(text);
							tag_number--;
						}
						else if(strcmp(text,"addr:country") == 0){
							xmlFree(text);
							tag_number--;
						}
						else if(strcmp(text,"operator") == 0){
							xmlFree(text);
							tag_number--;
						}
						else if(strcmp(text,"entrance") == 0){
							xmlFree(text);
							tag_number--;
						}
						else if(strcmp(text,"access") == 0){
							xmlFree(text);
							tag_number--;
						}
						else {
							xmlFree(text);
						}
						tag_number++;
					}
				}
			}
			if(hasfound) {
				querybuffer = sqlite3_mprintf("insert into existing (id,file_index,osm_id,addr_housenumber,addr_street,addr_postcode,addr_city,isway,tag_number,building) values (%d,%d,'%q','%q','%q','%q','%q','%d','%d','%d');",rowcounter,file_index,osmid,addr_housenumber,addr_street,addr_postcode,addr_city,(int)isway,tag_number,isbuilding);
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
			case 'w':
				xmlfilename3 = (char*) malloc(strlen(optarg)+1);
				snprintf(xmlfilename3,strlen(optarg)+1,"%s",optarg);
				break;
			case 't':
				veivegfilename = (char*) malloc(strlen(optarg)+1);
				snprintf(veivegfilename,strlen(optarg)+1,"%s",optarg);
				break;
			case 'd':
				duplicatefilename = (char*) malloc(strlen(optarg)+1);
				snprintf(duplicatefilename,strlen(optarg)+1,"%s",optarg);
				break;
			case 'e':
				extranodesfilename = (char*) malloc(strlen(optarg)+1);
				snprintf(extranodesfilename,strlen(optarg)+1,"%s",optarg);
				break;
			case '?':
				if(optopt == 'o')
					fprintf(stderr, "Option -%c requires an argument.\n",optopt);
				else if(optopt == 'w')
					fprintf(stderr, "Option -%c requires an argument.\n",optopt);
				else if(optopt == 'd')
					fprintf(stderr, "Option -%c requires an argument.\n",optopt);
				else if(optopt == 't')
					fprintf(stderr, "Option -%c requires an argument.\n",optopt);
				else if(optopt == 'e')
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

	xmlDoc *doc_old1 = NULL;
	xmlDoc *doc_old2 = NULL;
	xmlDoc *doc = NULL;
	xmlDoc *doc_output = NULL;
	xmlNode *root_element = NULL;
	xmlDoc *doc_output2 = NULL;
	xmlDoc *doc_output3 = NULL;
	xmlDoc *doc_output4 = NULL;

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
	doc_output = doc_old1;
	doc_output2 = xmlCopyDoc(doc_output,1);
	doc_output3 = xmlCopyDoc(doc_output,1);
	doc_output4 = xmlCopyDoc(doc_output,1);
	doc_old1 = NULL;

	doc_old1 = xmlReadFile(xmlfilename1,NULL, 0);
	if(doc_old1 == NULL) {
		printf("error: could not parse file %s\n", xmlfilename1);
	}

	if(xmlfilename3 != NULL){
		doc_old2 = xmlReadFile(xmlfilename3,NULL, 0);
		if(doc_old2 == NULL) {
			printf("error: could not parse file %s\n", xmlfilename3);
		}
		root_element = xmlDocGetRootElement(doc_old2);
		populate_database(root_element,db,1);
	}
	


	doc = xmlReadFile(xmlfilename2,NULL, 0);
	if(doc == NULL) {
		printf("error: could not parse file %s\n", xmlfilename2);
	}
	root_element = xmlDocGetRootElement(doc);
	compare_to_database(doc_old1, doc_old2, root_element,db,doc_output,doc_output2,doc_output3,doc_output4);

	if(outputxmlfilename){
		xmlSaveFileEnc(outputxmlfilename, doc_output, "UTF-8");
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


	xmlFreeDoc(doc_output);
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

	printf("Existing:\t%d\n",number_old);
	printf("New:\t\t%d\n",number_new);
	printf("Missing:\t%d\n",number_nonexisting);
	printf("Otherthings:\t%d\n",number_nodeswithotherthings);
	printf("Duplicates:\t%d\n",number_duplicates);
	if(veivegfilename != NULL)
		printf("Veivegfixes:\t%d\n",number_veivegfixes);
	return 0;

}

