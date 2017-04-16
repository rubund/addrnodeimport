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

int get_field(xmlNode *node, char *field_name, char *out, int outmaxlen)
{
    xmlChar * text;
    xmlNode * field_node;
    for(field_node = node->children; field_node ; field_node = field_node->next){
        text = xmlGetProp(field_node, "k");
        if (text == 0)
            continue;
        if(field_node->type == XML_ELEMENT_NODE) {
            if(strcmp(text, field_name) == 0){
                xmlFree(text);
                text = xmlGetProp(field_node, "v");
                if(out != NULL)
                    strncpy(out, text, outmaxlen);
                xmlFree(text);
                return 1;
            }
            else {
                xmlFree(text);
                continue;
            }
        }
        else xmlFree(text);
    }
    return 0;
}

int get_tag_count(xmlNode *node)
{
    int cnt;
    xmlChar * text;
    xmlNode * field_node;
    for(field_node = node->children; field_node ; field_node = field_node->next){
        text = xmlGetProp(field_node, "k");
        if (text == 0)
            continue;
        if(field_node->type == XML_ELEMENT_NODE) {
            cnt++;
        }
        xmlFree(text);
    }
    return cnt;
}

void has_tag(xmlNode *node, const char **field_names, int ntags, int *cnt_how_many)
{
    int i;
    xmlChar * text;
    xmlNode * field_node;
    for(i=0;i<ntags;i++){
        for(field_node = node->children; field_node ; field_node = field_node->next){
            text = xmlGetProp(field_node, "k");
            if (text == 0)
                continue;
            if(field_node->type == XML_ELEMENT_NODE) {
                if(strcmp(text, field_names[i]) == 0){
                    (*cnt_how_many)++;
                    break;
                }
            }
            xmlFree(text);
        }
    }
}

void populate_database(xmlNode * a_node, sqlite3 *db, char isway)
{

    xmlNode *cur_node;
    xmlNode *field_node;
    xmlChar * text;
    char osmid[100];

    char addr_street[256];
    char addr_housenumber[100];
    char addr_postcode[10];
    char addr_city[256];
    char hasfound = 0;
    char abandoned = 0;

    sqlite3_stmt *stmt;
    basic_query(db,"create table if not exists existing (id int auto_increment primary key not null, file_index int, osm_id bigint, addr_housenumber varchar(10), addr_street varchar(255), addr_postcode varchar(10), addr_city varchar(255), isway boolean, tag_number int, building boolean, foundindataset boolean default 0);",0);

    for(cur_node = a_node->children; cur_node; cur_node = cur_node->next){
        addr_housenumber[0] = 0;
        addr_street[0] = 0;
        addr_postcode[0] = 0;
        addr_city[0] = 0;
        if(cur_node->type == XML_ELEMENT_NODE) {
            text = xmlGetProp(cur_node, "id");
            if (text == 0) continue;
            strncpy(osmid, text, 99);
            xmlFree(text);
            printf("OSM-ID: %s\n", osmid);
            char bool_iscorrecttype;
            if(isway)
                bool_iscorrecttype = (strcmp(cur_node->name, "way") == 0);
            else
                bool_iscorrecttype = (strcmp(cur_node->name, "node") == 0);
            int isbuilding = 0;
            if(bool_iscorrecttype) {
                int tag_number = get_tag_count(cur_node);;
                if (get_field(cur_node, "addr:housenumber", addr_housenumber, 99));
                else if (get_field(cur_node, "abandoned:addr:housenumber", addr_housenumber, 99)){
                    abandoned = 1;
                }
                if (get_field(cur_node, "addr:street", addr_street, 255)){
                    hasfound = 1;
                    printf("%s\n", addr_street);
                }
                get_field(cur_node, "addr:postcode", addr_postcode, 9);
                get_field(cur_node, "addr:city", addr_city, 255);
                if(get_field(cur_node, "building", NULL, 0)) isbuilding = 1;
                #define N_IGNORE_TAGS 50
                const char *ignore_tags[N_IGNORE_TAGS];
                int idx = 0;
                ignore_tags[idx++] = "building:levels";
                ignore_tags[idx++] = "building:roof:shape";
                ignore_tags[idx++] = "roof:levels";
                ignore_tags[idx++] = "source";
                ignore_tags[idx++] = "source:date";
                ignore_tags[idx++] = "source:addr";
                ignore_tags[idx++] = "source:building";
                ignore_tags[idx++] = "addr:country";
                ignore_tags[idx++] = "operator";
                ignore_tags[idx++] = "entrance";
                ignore_tags[idx++] = "access";
                ignore_tags[idx++] = "nvdb:id";
                ignore_tags[idx++] = "wheelchair";
                int nfound = 0;
                has_tag(cur_node, ignore_tags, idx, &nfound);
                printf("nfound: %d\n", nfound);
                tag_number -= nfound;
            }
        }
    }

}


int main(int argc, char **argv)
{
    int ret;

    sqlite3 *db = NULL;

    if (argc <= 4) {
        fprintf(stderr, "Usage: %s <existing_nodes.osm> <existing_ways.osm> <new_nodes.osm> <output_dir>\n", argv[0]);
        return -1;
    }
    char *existing_node_filename = argv[1];

    LIBXML_TEST_VERSION
    ret = sqlite3_open(":memory:", &db);
    if(ret){
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return -1;
    }
    else {
        if(verbose)
            printf("Opened database successfully\n");
    }

    xmlDoc *doc_existing_nodes = NULL;
    xmlNode *root_element = NULL;

    doc_existing_nodes = xmlReadFile(existing_node_filename, NULL, 0);
    if (doc_existing_nodes == NULL) {
        printf("error: could not parse file %s\n", existing_node_filename);
    }
    root_element = xmlDocGetRootElement(doc_existing_nodes);

    populate_database(root_element, db, 0);
}
