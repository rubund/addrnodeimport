/*
 *    getmissingandreport.c
 *
 *    (c) Ruben Undheim 2017
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

static int sql_callback(void *info, int argc, char **argv, char **azColName)
{
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

void populate_database(xmlNode * a_node, sqlite3 *db, char isway)
{

    sqlite3_stmt *stmt;
    basic_query(db,"create table if not exists existing (id int auto_increment primary key not null, file_index int, osm_id bigint, addr_housenumber varchar(10), addr_street varchar(255), addr_postcode varchar(10), addr_city varchar(255), isway boolean, tag_number int, building boolean, foundindataset boolean default 0);",0);


}


int main(int argc, char **argv)
{
    int ret;

    sqlite3 *db = NULL;

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

    populate_database(NULL, db, 0);
}
