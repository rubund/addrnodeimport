/*
 *	common.c
 *
 *	(c) Ruben Undheim 2017
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
#include "common.h"


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
			//if(verbose)
			//	printf("%s\n",querybuffer);	
			basic_query(db,querybuffer,0);
			sqlite3_free(querybuffer);
			rowcounter++;
		}
	}
	basic_query(db,"create index if not exists fromname_index on corrections (fromname ASC);",0);
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
