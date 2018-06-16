/*
 *	fillinpostcode.c
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
char *correctionsfilename = NULL;
double addTo = 0;
char exists = 0;
char only_info = 0;
int number_new = 0;
int number_old = 0;
int number_nonexisting = 0;
int number_veivegfixes = 0;
int number_nodeswithotherthings = 0;
int number_duplicates = 0;
int number_fixes = 0;
int number_error = 0;
int number_onlynumber = 0;
char foundid[20];
char foundisway;
FILE *tmpfile_handle;

static int sql_callback(void *info, int argc, char **argv, char **azColName)
{
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

void basic_query(sqlite3 *db,char *query, void *info)
{
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

xmlNode *get_xml_node(xmlDoc *doc,int index,int isway)
{
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

void compare_to_database(xmlDoc *doc_old1, xmlDoc *doc_old2, xmlNode * a_node, sqlite3 *db, xmlDoc *doc_output, xmlDoc *doc_output2, xmlDoc *doc_output3, xmlDoc *doc_output4)
{
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
		addr_housenumber[0] = 0;
		addr_street[0] = 0;
		addr_postcode[0] = 0;
		addr_city[0] = 0;
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
							if(strlen(text) == 3){
								strncpy(addr_postcode+1,text,9);
								addr_postcode[0] = '0';
							}
							else {
								strncpy(addr_postcode,text,9);
							}
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
				if(correctionsfilename != NULL){
					querybuffer = sqlite3_mprintf("select toname from corrections where fromname='%q';",addr_street);
					ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt,0);
					sqlite3_free(querybuffer);
					if (ret){
						fprintf(stderr,"SQL Error");
						return;
					}
					if((ret = sqlite3_step(stmt)) == SQLITE_ROW){
						if(verbose)
							fprintf(stdout,"Found correction: %s %s",addr_street, sqlite3_column_text(stmt,0));
						strncpy(addr_street,sqlite3_column_text(stmt,0),255);
					}
					sqlite3_finalize(stmt);	

					querybuffer = sqlite3_mprintf("select toname from corrections where fromname='%q';",addr_city);
					ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt,0);
					sqlite3_free(querybuffer);
					if (ret){
						fprintf(stderr,"SQL Error");
						return;
					}
					if((ret = sqlite3_step(stmt)) == SQLITE_ROW){
						strncpy(addr_city,sqlite3_column_text(stmt,0),255);
					}
					sqlite3_finalize(stmt);	
				}
				int numrows = 0;
				querybuffer = sqlite3_mprintf("select count(*) from existing where addr_street='%q' and lower(addr_housenumber)=lower('%q') and ((tag_number > 4) or (tag_number > 5 and building = 1))",addr_street,addr_housenumber);
				ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt,0);
				sqlite3_free(querybuffer);
				while((ret = sqlite3_step(stmt)) == SQLITE_ROW){
					numrows = atoi(sqlite3_column_text(stmt,0));	
				}
				sqlite3_finalize(stmt);	

				if(numrows > 0){
					querybuffer = sqlite3_mprintf("select id,file_index,isway,addr_street,addr_housenumber,addr_postcode,addr_city,osm_id from existing where addr_street='%q' and lower(addr_housenumber)=lower('%q') and ((tag_number > 4) or (tag_number > 5 and building = 1))",addr_street,addr_housenumber);
					ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt,0);
					sqlite3_free(querybuffer);
					if(1){
						//printf("\n\n\n%s %s, %s %s:\n",addr_street,addr_housenumber,addr_postcode,addr_city);	
						while((ret = sqlite3_step(stmt)) == SQLITE_ROW){
							char missingpostcode = 0;
							char missingcity = 0;
							char otherisok = 0;
							missingpostcode = strcmp(sqlite3_column_text(stmt,5),"") == 0;
							missingcity = strcmp(sqlite3_column_text(stmt,6),"") == 0;
							if(missingpostcode && !missingcity){
								if(strcmp(sqlite3_column_text(stmt,6),addr_city) == 0) otherisok = 1;
								else {if(verbose) printf("Error - not equal (%s != %s) (osmid=%s)\n",sqlite3_column_text(stmt,6),addr_city,sqlite3_column_text(stmt,7));
									number_error++;}
							}	
							else if(!missingpostcode && missingcity){
								if(strcmp(sqlite3_column_text(stmt,5),addr_postcode) == 0) otherisok = 1;
								else {if(verbose) printf("Error - not equal (%s != %s) (osmid=%s)\n",sqlite3_column_text(stmt,5),addr_postcode,sqlite3_column_text(stmt,7));
									number_error++;}
							}	
							else {
								otherisok = 1;
							}
							if((missingpostcode || missingcity) && otherisok){
								if(verbose)
									printf("%s %s:",addr_street,addr_housenumber);	

								xmlNode *newNode_intern;
								if(strcmp(sqlite3_column_text(stmt,2),"1") == 0)
									newNode_intern = xmlCopyNode(get_xml_node(doc_old2,atoi(sqlite3_column_text(stmt,1)),1), 1);
								else
									newNode_intern = xmlCopyNode(get_xml_node(doc_old1,atoi(sqlite3_column_text(stmt,1)),0), 1);

								if(verbose){
									if(missingpostcode && !missingcity){
										printf(" Missing postcode but has city. %s == %s ? %s == %s ?\n",sqlite3_column_text(stmt,5),addr_postcode,sqlite3_column_text(stmt,6),addr_city);
									}
									else if(missingcity && !missingpostcode){
										printf(" Missing city but has postcode. %s == %s ? %s == %s ?\n",sqlite3_column_text(stmt,5),addr_postcode,sqlite3_column_text(stmt,6),addr_city);
									}
									else if(missingcity && missingpostcode){
										printf(" Missing postcode and city. %s == %s ? %s == %s ?\n",sqlite3_column_text(stmt,5),addr_postcode,sqlite3_column_text(stmt,6),addr_city);
									}
								}
								xmlNode *tag_node;
								if(missingpostcode){
									tag_node = xmlNewNode(NULL,"tag");
									xmlSetProp(tag_node,"k", "addr:postcode");
									xmlSetProp(tag_node,"v", addr_postcode);
									xmlAddChild(newNode_intern,tag_node);
								}
								if(missingcity){
									tag_node = xmlNewNode(NULL,"tag");
									xmlSetProp(tag_node,"k", "addr:city");
									xmlSetProp(tag_node,"v", addr_city);
									xmlAddChild(newNode_intern,tag_node);
								}
								//for(tag_node = newNode_intern->children; tag_node ; tag_node = tag_node->next){
								//	if(tag_node->type == XML_ELEMENT_NODE) {
								//		text = xmlGetProp(tag_node, "k");
								//		if(text != 0){
								//			printf("her: %s\n",text);
								//			if(strcmp(text,"addr:postcode") == 0){
								//				xmlFree(text);
								//				xmlSetProp(tag_node, "v",addr_postcode);
								//			}
								//			else if(strcmp(text,"addr:city") == 0){
								//				xmlFree(text);
								//				xmlSetProp(tag_node, "v",addr_city);
								//			}
								//		}
								//	}
								//}
								xmlSetProp(newNode_intern,"action","modify");
								xmlNode *root_element_intern = xmlDocGetRootElement(doc_output);
								xmlAddChild(root_element_intern,newNode_intern);
								number_fixes++;
							}
						}
					}
					//else{
					//	printf(" More than one match (%d) - %s %s\n",numrows,addr_street,addr_housenumber);
					//}
						//printf("Done with - %s %s\n",addr_street,addr_housenumber);
					sqlite3_finalize(stmt);	
				}
			}
		}
	}
}

void populate_database(xmlNode * a_node, sqlite3 *db, char isway)
{
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
	char hasfoundnumber = 0;

	//basic_query(db,"drop table if exists existing;");
	basic_query(db,"create table if not exists existing (id int auto_increment primary key not null, file_index int, osm_id bigint, addr_housenumber varchar(10), addr_street varchar(255), addr_postcode varchar(10), addr_city varchar(255), isway boolean, tag_number int, building boolean, foundindataset boolean default 0);",0);

	for(cur_node = a_node->children; cur_node; cur_node = cur_node->next){
		addr_housenumber[0] = 0;
		addr_street[0] = 0;
		addr_postcode[0] = 0;
		addr_city[0] = 0;
		if(cur_node->type == XML_ELEMENT_NODE) {
			hasfound = 0;
			hasfoundnumber = 0;
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
							hasfoundnumber = 1;
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
						else if(strcmp(text,"building:levels") == 0){
							xmlFree(text);
							tag_number--;
						}
						else if(strcmp(text,"building:roof:shape") == 0){
							xmlFree(text);
							tag_number--;
						}
						else if(strcmp(text,"roof:shape") == 0){
							xmlFree(text);
							tag_number--;
						}
						else if(strcmp(text,"roof:levels") == 0){
							xmlFree(text);
							tag_number--;
						}
						else if(strcmp(text,"source") == 0){
							xmlFree(text);
							tag_number--;
						}
						else if(strcmp(text,"source:date") == 0){
							xmlFree(text);
							tag_number--;
						}
						else if(strcmp(text,"source:addr") == 0){
							xmlFree(text);
							tag_number--;
						}
						else if(strcmp(text,"source:building") == 0){
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
						else if(strcmp(text,"nvdb:id") == 0){
							xmlFree(text);
							tag_number--;
						}
						else if(strcmp(text,"wheelchair") == 0){
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
			else if(hasfoundnumber){
				number_onlynumber++;
			}
		} 
	}
	basic_query(db,"create index if not exists file_index_index on existing (file_index ASC);",0);
	basic_query(db,"create index if not exists addr_street_index on existing (addr_street ASC);",0);
	basic_query(db,"create index if not exists addr_housenumber_index on existing (addr_housenumber ASC);",0);

}

void get_corrections(xmlNode * a_node, sqlite3 *db)
{
	xmlNode *cur_node = NULL;
	xmlChar *text;
	static int rowcounter = 0;
	char *querybuffer;
	int ret;
	sqlite3_stmt *stmt;

	char fromname[256];	
	char toname[256];	

	basic_query(db,"create table if not exists corrections (id int auto_increment primary key not null, fromname varchar(100), toname varchar(100));",0);

	for(cur_node = a_node->children; cur_node; cur_node = cur_node->next){
		if(cur_node->type == XML_ELEMENT_NODE && (strcmp(cur_node->name,"spelling") == 0)) {
			text = xmlGetProp(cur_node, "from");
			strncpy(fromname,text,255);
			xmlFree(text);
			text = xmlGetProp(cur_node, "to");
			strncpy(toname,text,255);
			xmlFree(text);
			querybuffer = sqlite3_mprintf("insert into corrections (id,fromname,toname) values (%d,'%q','%q')",rowcounter,fromname,toname);
			if(verbose)
				printf("%s\n",querybuffer);	
			basic_query(db,querybuffer,0);
			sqlite3_free(querybuffer);
			rowcounter++;
		}
	}
	basic_query(db,"create index if not exists fromname_index on corrections (fromname ASC);",0);
}


int parse_cmdline(int argc, char **argv)
{
	int s;
	opterr = 0;
	while((s = getopt(argc, argv, "vso:w:t:d:e:c:")) != -1) {
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
			case 'c':
				correctionsfilename = (char*) malloc(strlen(optarg)+1);
				snprintf(correctionsfilename,strlen(optarg)+1,"%s",optarg);
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
				else if(optopt == 'c')
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

int main(int argc, char **argv)
{

	xmlDoc *doc_corrections = NULL;
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

	if(correctionsfilename != NULL){
		doc_corrections = xmlReadFile(correctionsfilename,NULL,0);
		if(doc_corrections == NULL) {
			printf("error: could not parse file %s\n", correctionsfilename);
		}
		root_element = xmlDocGetRootElement(doc_corrections);
		get_corrections(root_element,db);
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
	if(correctionsfilename != NULL)
		free(correctionsfilename);

	printf("Fixes:\t%d\n",number_fixes);
	printf("Errors:\t%d\n",number_error);
	printf("Onlynumber:\t%d\n",number_onlynumber);
	return 0;

}

