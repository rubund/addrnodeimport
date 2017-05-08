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

// TODO:
//  If not correct tag_number, it is an additional object not managed by scripts.
//  Report duplicates
//  If no exact match found. Change "vei" to "veg" and "veg" to "vei" (lower/upper case) for every and see if any matches. Then add to changeto
//  If exact same position, but different data: add to changeto
//  If near same position, but exact data: add to changeto

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <sqlite3.h>
#include <math.h>
#include "common.h"

char verbose = 0;

const char *ignore_tags[] = {             "building:levels"
                                         ,"building:roof:shape"
                                         ,"roof:levels"
                                         ,"source"
                                         ,"source:date"
                                         ,"source:addr"
                                         ,"source:building"
                                         ,"addr:country"
                                         ,"operator"
                                         ,"entrance"
                                         ,"access"
                                         ,"nvdb:id"
                                         ,"wheelchair"};

int changetorownumber = 0;
int global_new_node_id = -1;

char *existing_node_filename;
char *existing_ways_filename;
char *new_nodes_filename;
char *outdir_path;
char *correctionsfilename = NULL;

xmlDoc *doc_changes = NULL;
xmlDoc *doc_notmatched_manual = NULL;
xmlDoc *doc_newnodes_manual = NULL;
xmlDoc *doc_existing_nodes = NULL;
xmlDoc *doc_existing_ways = NULL;

double meter_to_latitude(double meter)
{
    return meter / 111111.0; // Crude estimate
}

double meter_to_longitude(double meter, double latitude)
{
    return meter / (111111.0 * cos(latitude*3.141592654/180.0));
}

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
            if (text == 0){
                xmlFree(text);
                continue;
            }
            if(field_node->type == XML_ELEMENT_NODE) {
                if(strcmp(text, field_names[i]) == 0){
                    (*cnt_how_many)++;
                    xmlFree(text);
                    break;
                }
            }
            xmlFree(text);
        }
    }
}

// Fill in the latitude/longitude columns in the existing db table for ways.
void find_lat_lon_for_ways(xmlNode *a_node, sqlite3 *db)
{
    int ret;
    sqlite3_stmt *stmt;
    char *querybuffer;
    xmlChar * text;
    //char osmid[100];
    xmlNode *cur_node;
    xmlNode *child_node;
    char found;
    char latitude_str[20];
    char longitude_str[20];
    char osmid[100];

    for(cur_node = a_node->children; cur_node; cur_node = cur_node->next){
        found = 0;
        if(cur_node->type == XML_ELEMENT_NODE) {
            text = xmlGetProp(cur_node, "id");
            if (text == 0) continue;
            strncpy(osmid,text,99);
            xmlFree(text);
            if(strcmp(cur_node->name, "way") == 0) {
                for(child_node = cur_node->children; child_node ; child_node = child_node->next){
                    if(child_node->type == XML_ELEMENT_NODE){
                        if(strcmp(child_node->name,"nd") == 0){
                            text = xmlGetProp(child_node, "ref");
                            //printf("Found reference: %s\n", text);
                            querybuffer = sqlite3_mprintf("select id, osm_id, latitude, longitude from waynodes where osm_id = '%q';", text);
                            xmlFree(text);
                            ret = sqlite3_prepare_v2(db, querybuffer, -1, &stmt, 0);
                            sqlite3_free(querybuffer);
                            if((ret = sqlite3_step(stmt)) == SQLITE_ROW){
                                querybuffer = sqlite3_mprintf("update existing set latitude='%q', longitude='%q' where osm_id='%q'", sqlite3_column_text(stmt, 2), sqlite3_column_text(stmt, 3), osmid);
                                basic_query(db,querybuffer, 0);
                                sqlite3_free(querybuffer);
                                found = 1;
                            }
                            sqlite3_finalize(stmt);
                        }
                    }
                    if(found) break;
                }
            }
        }
    }


}

void populate_database_existing(xmlNode * a_node, sqlite3 *db, char isway)
{

    xmlNode *cur_node;
    xmlNode *field_node;
    xmlChar * text;
    char *querybuffer;

    char osmid[100];

    char addr_street[256];
    char addr_housenumber[100];
    char addr_postcode[10];
    char addr_city[256];
    char latitude_str[20];
    char longitude_str[20];
    char hasfound = 0;
    char abandoned = 0;
    static int rowcounter = 0;
    static int rowcounterwaynodes = 0;
    int file_index = 0;

    sqlite3_stmt *stmt;
    basic_query(db,"create table if not exists existing (id int auto_increment primary key not null, file_index int, osm_id bigint, addr_housenumber varchar(10) collate nocase, addr_street varchar(255), addr_postcode varchar(10), addr_city varchar(255), latitude double, longitude double, isway boolean, tag_number int, building boolean, changeto_id int, foundindataset boolean default 0);",0);

    basic_query(db,"create table if not exists changeto (id int auto_increment primary key not null, change_type varchar(50), existing_id int, addr_housenumber varchar(10), addr_street varchar(255), addr_postcode varchar(10), addr_city varchar(255), latitude double, longitude double);",0);

    basic_query(db,"create table if not exists waynodes (id int auto_increment primary key not null, osm_id bigint, latitude double, longitude double);",0);

    for(cur_node = a_node->children; cur_node; cur_node = cur_node->next){
        addr_housenumber[0] = 0;
        addr_street[0] = 0;
        addr_postcode[0] = 0;
        addr_city[0] = 0;
        latitude_str[0] = 0;
        longitude_str[0] = 0;
        if(cur_node->type == XML_ELEMENT_NODE) {
            text = xmlGetProp(cur_node, "id");
            if (text == 0) continue;
            strncpy(osmid, text, 99);
            xmlFree(text);
            //printf("OSM-ID: %s\n", osmid);
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
                    //printf("%s\n", addr_street);
                    //printf("%s\n", addr_housenumber);
                }
                get_field(cur_node, "addr:postcode", addr_postcode, 9);
                get_field(cur_node, "addr:city", addr_city, 255);
                if(get_field(cur_node, "building", NULL, 0)) isbuilding = 1;
                int nfound = 0;
                has_tag(cur_node, ignore_tags, sizeof(ignore_tags)/sizeof(void*), &nfound);
                //printf("nfound: %d\n", nfound);
                tag_number -= nfound;

                if (!isway) { // Only nodes have latitude and longitude on the main XML element
                    text = xmlGetProp(cur_node, "lat");
                    strncpy(latitude_str, text, 15);
                    xmlFree(text);
                    text = xmlGetProp(cur_node, "lon");
                    strncpy(longitude_str, text, 15);
                    xmlFree(text);
                }

                if(hasfound) {
                    querybuffer = sqlite3_mprintf("insert into existing (id,file_index,osm_id,addr_housenumber,addr_street,addr_postcode,addr_city,isway,tag_number,building,latitude,longitude) values (%d,%d,'%q','%q','%q','%q','%q','%d','%d','%d','%q','%q');",rowcounter,file_index,osmid,addr_housenumber,addr_street,addr_postcode,addr_city,(int)isway,tag_number,isbuilding, latitude_str, longitude_str);
                    basic_query(db,querybuffer,0);
                    sqlite3_free(querybuffer);
                    rowcounter++;
                    file_index++;
                }
            }
            else if(isway && (strcmp(cur_node->name, "node") == 0)) { // Add single nodes for ways to a separate table
                text = xmlGetProp(cur_node, "lat");
                //printf("%s, ", text);
                strncpy(latitude_str, text, 15);
                xmlFree(text);
                text = xmlGetProp(cur_node, "lon");
                //printf("%s\n", text);
                strncpy(longitude_str, text, 15);
                xmlFree(text);
                querybuffer = sqlite3_mprintf("insert into waynodes (id, osm_id, latitude, longitude) values (%d,'%q','%q','%q');",rowcounterwaynodes, osmid, latitude_str, longitude_str);
                basic_query(db,querybuffer,0);
                sqlite3_free(querybuffer);
                rowcounterwaynodes++;
            }
        }
    }

    basic_query(db,"create index if not exists file_index_index on existing (file_index ASC);",0);
    basic_query(db,"create index if not exists addr_street_index on existing (addr_street ASC);",0);
    basic_query(db,"create index if not exists addr_housenumber_index on existing (addr_housenumber ASC);",0);

    basic_query(db,"create index if not exists osmid_index on waynodes (osm_id ASC);",0);

    if(isway){
        find_lat_lon_for_ways(a_node, db);
    }
}

void correct_name(sqlite3 *db, char *in, char *out, int out_max_len)
{
    char *querybuffer;
    sqlite3_stmt *stmt;
    int ret;
    char *tmp_int = malloc(out_max_len);
    querybuffer = sqlite3_mprintf("select toname from corrections where fromname='%q';",in);
    ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt,0);
    sqlite3_free(querybuffer);
    if (ret){
        fprintf(stderr,"SQL Error");
        return;
    }
    if((ret = sqlite3_step(stmt)) == SQLITE_ROW){
        if(verbose)
            fprintf(stdout,"Found correction: %s %s",in, sqlite3_column_text(stmt,0));
        strncpy(out,sqlite3_column_text(stmt,0),out_max_len-1);
    }
    else {
        strncpy(out,in,out_max_len-1);
    }
    sqlite3_finalize(stmt);

    char *searchres;

    // Replace '' with correct ’
    searchres = strstr(out, "''");
    while (searchres != 0){
        strncpy(tmp_int, out, (searchres - out));
        strncpy(tmp_int + (searchres - out), "’", 3);
        strncpy(tmp_int + (searchres - out) + 3, searchres + 2, out_max_len-1-(int)(searchres-out)-2);
        tmp_int[strlen(out)+1] = 0;
        strncpy(out,tmp_int,out_max_len-1);
        //printf("Replace apostrophe: %s\n", out);
        searchres = strstr(out, "''");
    }

    // Replace ´ with correct ’
    searchres = strstr(out, "´");
    while (searchres != 0){
        strncpy(tmp_int, out, (searchres - out));
        strncpy(tmp_int + (searchres - out), "’", 3);
        strncpy(tmp_int + (searchres - out) + 3, searchres + 2, out_max_len-1-(int)(searchres-out)-2);
        tmp_int[strlen(out)+1] = 0;
        strncpy(out,tmp_int,out_max_len-1);
        //printf("Replace apostrophe: %s\n", out);
        searchres = strstr(out, "´");
    }

    free(tmp_int);
}

void populate_database_newnodes(xmlNode * a_node, sqlite3 *db)
{
    static int rowcounter = 0;
    xmlNode *cur_node;
    sqlite3_stmt *stmt;
    char addr_street[256];
    char addr_housenumber[100];
    char addr_postcode[10];
    char addr_postcode_tmp[10];
    char addr_city[256];
    char tmp[256];
    char latitude_str[20];
    char longitude_str[20];
    char *querybuffer;

    xmlChar *text;

    basic_query(db,"create table if not exists newnodes (id int auto_increment primary key not null, addr_housenumber varchar(10) collate nocase, addr_street varchar(255), addr_postcode varchar(10), addr_city varchar(255), latitude double, longitude double, foundindataset boolean default 0);",0);

    for(cur_node = a_node->children; cur_node; cur_node = cur_node->next){
        addr_housenumber[0] = 0;
        addr_street[0] = 0;
        addr_postcode[0] = 0;
        addr_city[0] = 0;
        latitude_str[0] = 0;
        longitude_str[0] = 0;
        if(cur_node->type == XML_ELEMENT_NODE) {
            text = xmlGetProp(cur_node, "id");
            if (text == 0) continue;
            xmlFree(text);
            get_field(cur_node, "addr:postcode", addr_postcode, 9);
            if(strlen(addr_postcode) == 3){
                strncpy(addr_postcode_tmp+1,addr_postcode,9);
                addr_postcode_tmp[0] = '0';
                strncpy(addr_postcode,addr_postcode_tmp,9);
            }

            get_field(cur_node, "addr:city", tmp, 255);
            correct_name(db, tmp, addr_city, 255);
            get_field(cur_node, "addr:street", tmp, 255);
            correct_name(db, tmp, addr_street, 255);
            get_field(cur_node, "addr:housenumber", addr_housenumber, 99);

            text = xmlGetProp(cur_node, "lat");
            strncpy(latitude_str, text, 15);
            xmlFree(text);
            text = xmlGetProp(cur_node, "lon");
            strncpy(longitude_str, text, 15);
            xmlFree(text);
            //printf("addr:steet: %s\n", addr_street);
            querybuffer = sqlite3_mprintf("insert into newnodes (id,addr_housenumber,addr_street,addr_postcode,addr_city,latitude,longitude) values (%d,'%q','%q','%q','%q','%q','%q');",rowcounter,addr_housenumber,addr_street,addr_postcode,addr_city, latitude_str, longitude_str);
            basic_query(db,querybuffer,0);
            sqlite3_free(querybuffer);
            rowcounter++;
        }

    }
}


void match_exact(sqlite3 *db)
{
    int ret;
    sqlite3_stmt *stmt, *stmt2;
    char *querybuffer;
    double meter_margin = 40;

    double latitude, latmargin, longitude, lonmargin;
    latmargin = meter_to_latitude(meter_margin);

    querybuffer = sqlite3_mprintf("select id, addr_street, addr_housenumber, addr_city, addr_postcode, latitude, longitude from newnodes;");
    ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt,0);
    sqlite3_free(querybuffer);
    while((ret = sqlite3_step(stmt)) == SQLITE_ROW){
        latitude = sqlite3_column_double(stmt, 5);
        longitude = sqlite3_column_double(stmt, 6);
        lonmargin = meter_to_longitude(meter_margin, latitude);
        //printf("Iterating: %s\n", sqlite3_column_text(stmt,0));
        querybuffer = sqlite3_mprintf("select id, osm_id, latitude, longitude from existing where foundindataset=0 and addr_street='%q' and addr_housenumber='%q' and addr_city='%q' and addr_postcode='%q' and ((tag_number <= 4) or (tag_number <= 5 and building = 1))", sqlite3_column_text(stmt, 1),sqlite3_column_text(stmt, 2),sqlite3_column_text(stmt, 3),sqlite3_column_text(stmt, 4));
        ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt2,0);
        //printf("querybuffer: %s\n", querybuffer);
        sqlite3_free(querybuffer);
        while((ret = sqlite3_step(stmt2)) == SQLITE_ROW){
            double latex;
            double longex;
            latex  = sqlite3_column_double(stmt2, 2);
            longex = sqlite3_column_double(stmt2, 3);
            if (latex > (latitude - latmargin) && latex < (latitude + latmargin) && longex > (longitude - lonmargin) && longex < (longitude + lonmargin)) {
                querybuffer = sqlite3_mprintf("update existing set foundindataset=1 where id='%q'", sqlite3_column_text(stmt2,0));
                basic_query(db, querybuffer, 0);
                sqlite3_free(querybuffer);
                querybuffer = sqlite3_mprintf("update newnodes set foundindataset=1 where id='%q'", sqlite3_column_text(stmt,0));
                basic_query(db, querybuffer, 0);
                sqlite3_free(querybuffer);
            }
            else {
                //printf("Warning: The position is not EXACT: %s %s, %s %s (%f %f vs %f %f)\n", sqlite3_column_text(stmt,1), sqlite3_column_text(stmt,2), sqlite3_column_text(stmt,4), sqlite3_column_text(stmt,3), latitude, longitude, latex, longex);
            }
        }
        sqlite3_finalize(stmt2);
    }
    sqlite3_finalize(stmt);
}


void match_corrected(sqlite3 *db)
{
    int ret;
    sqlite3_stmt *stmt, *stmt2, *stmt3;
    char *querybuffer;
    double meter_margin = 40;

    double latitude, latmargin, longitude, lonmargin;
    latmargin = meter_to_latitude(meter_margin);

    querybuffer = sqlite3_mprintf("select id, addr_street, addr_housenumber, addr_city, addr_postcode, latitude, longitude from newnodes where foundindataset=0;");
    ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt,0);
    sqlite3_free(querybuffer);
    while((ret = sqlite3_step(stmt)) == SQLITE_ROW){
        latitude = sqlite3_column_double(stmt, 5);
        longitude = sqlite3_column_double(stmt, 6);
        lonmargin = meter_to_longitude(meter_margin, latitude);
        //printf("Iterating: %s\n", sqlite3_column_text(stmt,0));
        querybuffer = sqlite3_mprintf("select fromname from corrections where toname='%q';",sqlite3_column_text(stmt, 1));
        //printf("%s\n", querybuffer);
        ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt3,0);
        sqlite3_free(querybuffer);
        //printf("her %s\n", sqlite3_column_text(stmt, 1));
        if((ret = sqlite3_step(stmt3)) == SQLITE_ROW){
            //printf("checking %s instead of %s\n", sqlite3_column_text(stmt3, 0), sqlite3_column_text(stmt, 1));
            querybuffer = sqlite3_mprintf("select id, osm_id, latitude, longitude from existing where foundindataset=0 and addr_street='%q' and addr_housenumber='%q' and addr_city='%q' and addr_postcode='%q' and ((tag_number <= 4) or (tag_number <= 5 and building = 1))", sqlite3_column_text(stmt3, 0),sqlite3_column_text(stmt, 2),sqlite3_column_text(stmt, 3),sqlite3_column_text(stmt, 4));
            ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt2,0);
            //printf("querybuffer: %s\n", querybuffer);
            sqlite3_free(querybuffer);
            while((ret = sqlite3_step(stmt2)) == SQLITE_ROW){
                double latex;
                double longex;
                latex  = sqlite3_column_double(stmt2, 2);
                longex = sqlite3_column_double(stmt2, 3);
                if (latex > (latitude - latmargin) && latex < (latitude + latmargin) && longex > (longitude - lonmargin) && longex < (longitude + lonmargin)) {
                    querybuffer = sqlite3_mprintf("update existing set foundindataset=1 where id='%q'", sqlite3_column_text(stmt2,0));
                    basic_query(db, querybuffer, 0);
                    sqlite3_free(querybuffer);
                    querybuffer = sqlite3_mprintf("update newnodes set foundindataset=1 where id='%q'", sqlite3_column_text(stmt,0));
                    basic_query(db, querybuffer, 0);
                    sqlite3_free(querybuffer);

                    querybuffer = sqlite3_mprintf("insert into changeto (id, change_type, existing_id, addr_street, addr_housenumber, addr_postcode, addr_city, latitude, longitude) values (%d, 'samepos_new_data', '%q', '%q', '%q', '%q', '%q', NULL, NULL)", changetorownumber, sqlite3_column_text(stmt2, 0), sqlite3_column_text(stmt,1), sqlite3_column_text(stmt,2), sqlite3_column_text(stmt,4), sqlite3_column_text(stmt,3));
                    basic_query(db, querybuffer, 0);
                    sqlite3_free(querybuffer);
                    changetorownumber++;
                    int lastid = sqlite3_last_insert_rowid(db);
                    querybuffer = sqlite3_mprintf("update existing set changeto_id='%d'  where id = '%q'", lastid, sqlite3_column_text(stmt2, 0));
                    basic_query(db, querybuffer, 0);
                    sqlite3_free(querybuffer);
                }
                else {
                    //printf("Warning: The position is not EXACT: %s %s, %s %s (%f %f vs %f %f)\n", sqlite3_column_text(stmt,1), sqlite3_column_text(stmt,2), sqlite3_column_text(stmt,4), sqlite3_column_text(stmt,3), latitude, longitude, latex, longex);
                }
            }
            sqlite3_finalize(stmt2);
        }
        sqlite3_finalize(stmt3);
    }
    sqlite3_finalize(stmt);
}


void find_exact_data_but_moved_around(sqlite3 *db)
{
    int ret;
    sqlite3_stmt *stmt, *stmt2;
    char *querybuffer;
    double meter_margin = 300;

    double latitude, latmargin, longitude, lonmargin;
    latmargin = meter_to_latitude(meter_margin);

    querybuffer = sqlite3_mprintf("select id, addr_street, addr_housenumber, addr_city, addr_postcode, latitude, longitude from newnodes where foundindataset=0;");
    ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt,0);
    sqlite3_free(querybuffer);
    while((ret = sqlite3_step(stmt)) == SQLITE_ROW){
        latitude = sqlite3_column_double(stmt, 5);
        longitude = sqlite3_column_double(stmt, 6);
        lonmargin = meter_to_longitude(meter_margin, latitude);
        //printf("Iterating: %s\n", sqlite3_column_text(stmt,0));
        querybuffer = sqlite3_mprintf("select id, osm_id, latitude, longitude from existing where addr_street='%q' and addr_housenumber='%q' and addr_city='%q' and addr_postcode='%q' and ((tag_number <= 4) or (tag_number <= 5 and building = 1)) and foundindataset=0 and isway=0", sqlite3_column_text(stmt, 1),sqlite3_column_text(stmt, 2),sqlite3_column_text(stmt, 3),sqlite3_column_text(stmt, 4));
        ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt2,0);
        //printf("querybuffer: %s\n", querybuffer);
        sqlite3_free(querybuffer);
        while((ret = sqlite3_step(stmt2)) == SQLITE_ROW){
            double latex;
            double longex;
            latex  = sqlite3_column_double(stmt2, 2);
            longex = sqlite3_column_double(stmt2, 3);
            //if (latex > (latitude - latmargin) && latex < (latitude + latmargin) && longex > (longitude - lonmargin) && longex < (longitude + lonmargin)) {
            //    printf("SHOULD NOT BE ANY HERE!\n");
            //}
            //else
            querybuffer = sqlite3_mprintf("update existing set foundindataset=1 where id='%q'", sqlite3_column_text(stmt2,0));
            basic_query(db, querybuffer, 0);
            sqlite3_free(querybuffer);
            querybuffer = sqlite3_mprintf("update newnodes set foundindataset=1 where id='%q'", sqlite3_column_text(stmt,0));
            basic_query(db, querybuffer, 0);
            sqlite3_free(querybuffer);
            if (latex > (latitude - latmargin) && latex < (latitude + latmargin) && longex > (longitude - lonmargin) && longex < (longitude + lonmargin)) {

                querybuffer = sqlite3_mprintf("insert into changeto (id, change_type, existing_id, addr_street, addr_housenumber, addr_postcode, addr_city, latitude, longitude) values (%d, 'moved_exact', '%q', NULL, NULL, NULL, NULL, %f, %f)", changetorownumber, sqlite3_column_text(stmt2, 0), latitude, longitude);
                basic_query(db, querybuffer, 0);
                sqlite3_free(querybuffer);
                changetorownumber++;
                int lastid = sqlite3_last_insert_rowid(db);
                querybuffer = sqlite3_mprintf("update existing set changeto_id='%d'  where id = '%q'", lastid, sqlite3_column_text(stmt2, 0));
                basic_query(db, querybuffer, 0);
                sqlite3_free(querybuffer);
                //printf("Warning: The position is not EXACT: %s %s, %s %s (%f %f vs %f %f)\n", sqlite3_column_text(stmt,1), sqlite3_column_text(stmt,2), sqlite3_column_text(stmt,4), sqlite3_column_text(stmt,3), latitude, longitude, latex, longex);
            }
            else {
                querybuffer = sqlite3_mprintf("insert into changeto (id, change_type, existing_id, addr_street, addr_housenumber, addr_postcode, addr_city, latitude, longitude) values (%d, 'moved_too_far', '%q', NULL, NULL, NULL, NULL, %f, %f)", changetorownumber, sqlite3_column_text(stmt2, 0), latitude, longitude);
                basic_query(db, querybuffer, 0);
                sqlite3_free(querybuffer);
                changetorownumber++;
                int lastid = sqlite3_last_insert_rowid(db);
                querybuffer = sqlite3_mprintf("update existing set changeto_id='%d'  where id = '%q'", lastid, sqlite3_column_text(stmt2, 0));
                basic_query(db, querybuffer, 0);
                sqlite3_free(querybuffer);
            }
        }
        sqlite3_finalize(stmt2);
    }
    sqlite3_finalize(stmt);
}


void find_exact_data_but_building(sqlite3 *db)
{
    int ret;
    sqlite3_stmt *stmt, *stmt2;
    char *querybuffer;
    double meter_margin = 300;

    double latitude, latmargin, longitude, lonmargin;
    latmargin = meter_to_latitude(meter_margin);

    querybuffer = sqlite3_mprintf("select id, addr_street, addr_housenumber, addr_city, addr_postcode, latitude, longitude from newnodes where foundindataset=0;");
    ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt,0);
    sqlite3_free(querybuffer);
    while((ret = sqlite3_step(stmt)) == SQLITE_ROW){
        latitude = sqlite3_column_double(stmt, 5);
        longitude = sqlite3_column_double(stmt, 6);
        lonmargin = meter_to_longitude(meter_margin, latitude);
        //printf("Iterating: %s\n", sqlite3_column_text(stmt,0));
        querybuffer = sqlite3_mprintf("select id, osm_id, latitude, longitude from existing where addr_street='%q' and addr_housenumber='%q' and addr_city='%q' and addr_postcode='%q' and ((tag_number <= 4) or (tag_number <= 5 and building = 1)) and foundindataset=0 and isway=1", sqlite3_column_text(stmt, 1),sqlite3_column_text(stmt, 2),sqlite3_column_text(stmt, 3),sqlite3_column_text(stmt, 4));
        ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt2,0);
        //printf("querybuffer: %s\n", querybuffer);
        sqlite3_free(querybuffer);
        while((ret = sqlite3_step(stmt2)) == SQLITE_ROW){
            double latex;
            double longex;
            latex  = sqlite3_column_double(stmt2, 2);
            longex = sqlite3_column_double(stmt2, 3);
            //if (latex > (latitude - latmargin) && latex < (latitude + latmargin) && longex > (longitude - lonmargin) && longex < (longitude + lonmargin)) {
            //    printf("SHOULD NOT BE ANY HERE!\n");
            //}
            //else
            querybuffer = sqlite3_mprintf("update existing set foundindataset=1 where id='%q'", sqlite3_column_text(stmt2,0));
            basic_query(db, querybuffer, 0);
            sqlite3_free(querybuffer);
            querybuffer = sqlite3_mprintf("update newnodes set foundindataset=1 where id='%q'", sqlite3_column_text(stmt,0));
            basic_query(db, querybuffer, 0);
            sqlite3_free(querybuffer);
            if (latex > (latitude - latmargin) && latex < (latitude + latmargin) && longex > (longitude - lonmargin) && longex < (longitude + lonmargin)) {
                querybuffer = sqlite3_mprintf("insert into changeto (id, change_type, existing_id, addr_street, addr_housenumber, addr_postcode, addr_city, latitude, longitude) values (%d, 'building_moved_exact', '%q', NULL, NULL, NULL, NULL, %f, %f)", changetorownumber, sqlite3_column_text(stmt2, 0), latitude, longitude);
                basic_query(db, querybuffer, 0);
                sqlite3_free(querybuffer);
                changetorownumber++;
                int lastid = sqlite3_last_insert_rowid(db);
                querybuffer = sqlite3_mprintf("update existing set changeto_id='%d'  where id = '%q'", lastid, sqlite3_column_text(stmt2, 0));
                basic_query(db, querybuffer, 0);
                sqlite3_free(querybuffer);
            }
            else {
                querybuffer = sqlite3_mprintf("insert into changeto (id, change_type, existing_id, addr_street, addr_housenumber, addr_postcode, addr_city, latitude, longitude) values (%d, 'building_moved_too_far', '%q', NULL, NULL, NULL, NULL, %f, %f)", changetorownumber, sqlite3_column_text(stmt2, 0), latitude, longitude);
                basic_query(db, querybuffer, 0);
                sqlite3_free(querybuffer);
                changetorownumber++;
                int lastid = sqlite3_last_insert_rowid(db);
                querybuffer = sqlite3_mprintf("update existing set changeto_id='%d'  where id = '%q'", lastid, sqlite3_column_text(stmt2, 0));
                basic_query(db, querybuffer, 0);
                sqlite3_free(querybuffer);
            }
        }
        sqlite3_finalize(stmt2);
    }
    sqlite3_finalize(stmt);
}


void find_corrected_data_but_moved_around(sqlite3 *db)
{
    int ret;
    sqlite3_stmt *stmt, *stmt2, *stmt3;
    char *querybuffer;
    double meter_margin = 300;

    double latitude, latmargin, longitude, lonmargin;
    latmargin = meter_to_latitude(meter_margin);

    querybuffer = sqlite3_mprintf("select id, addr_street, addr_housenumber, addr_city, addr_postcode, latitude, longitude from newnodes where foundindataset=0;");
    ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt,0);
    sqlite3_free(querybuffer);
    while((ret = sqlite3_step(stmt)) == SQLITE_ROW){
        latitude = sqlite3_column_double(stmt, 5);
        longitude = sqlite3_column_double(stmt, 6);
        lonmargin = meter_to_longitude(meter_margin, latitude);
        //printf("Iterating: %s\n", sqlite3_column_text(stmt,0));
        querybuffer = sqlite3_mprintf("select fromname from corrections where toname='%q';",sqlite3_column_text(stmt, 1));
        ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt3,0);
        sqlite3_free(querybuffer);
        if((ret = sqlite3_step(stmt3)) == SQLITE_ROW){
            querybuffer = sqlite3_mprintf("select id, osm_id, latitude, longitude from existing where addr_street='%q' and addr_housenumber='%q' and addr_city='%q' and addr_postcode='%q' and ((tag_number <= 4) or (tag_number <= 5 and building = 1)) and foundindataset=0 and isway=0", sqlite3_column_text(stmt3, 0),sqlite3_column_text(stmt, 2),sqlite3_column_text(stmt, 3),sqlite3_column_text(stmt, 4));
            ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt2,0);
            //printf("querybuffer: %s\n", querybuffer);
            sqlite3_free(querybuffer);
            while((ret = sqlite3_step(stmt2)) == SQLITE_ROW){
                double latex;
                double longex;
                latex  = sqlite3_column_double(stmt2, 2);
                longex = sqlite3_column_double(stmt2, 3);
                //if (latex > (latitude - latmargin) && latex < (latitude + latmargin) && longex > (longitude - lonmargin) && longex < (longitude + lonmargin)) {
                //    printf("SHOULD NOT BE ANY HERE!\n");
                //}
                //else
                querybuffer = sqlite3_mprintf("update existing set foundindataset=1 where id='%q'", sqlite3_column_text(stmt2,0));
                basic_query(db, querybuffer, 0);
                sqlite3_free(querybuffer);
                querybuffer = sqlite3_mprintf("update newnodes set foundindataset=1 where id='%q'", sqlite3_column_text(stmt,0));
                basic_query(db, querybuffer, 0);
                sqlite3_free(querybuffer);
                if (latex > (latitude - latmargin) && latex < (latitude + latmargin) && longex > (longitude - lonmargin) && longex < (longitude + lonmargin)) {

                    querybuffer = sqlite3_mprintf("insert into changeto (id, change_type, existing_id, addr_street, addr_housenumber, addr_postcode, addr_city, latitude, longitude) values (%d, 'moved_exact', '%q', NULL, NULL, NULL, NULL, %f, %f)", changetorownumber, sqlite3_column_text(stmt2, 0), latitude, longitude);
                    basic_query(db, querybuffer, 0);
                    sqlite3_free(querybuffer);
                    changetorownumber++;
                    int lastid = sqlite3_last_insert_rowid(db);
                    querybuffer = sqlite3_mprintf("update existing set changeto_id='%d'  where id = '%q'", lastid, sqlite3_column_text(stmt2, 0));
                    basic_query(db, querybuffer, 0);
                    sqlite3_free(querybuffer);
                    //printf("Warning: The position is not EXACT: %s %s, %s %s (%f %f vs %f %f)\n", sqlite3_column_text(stmt,1), sqlite3_column_text(stmt,2), sqlite3_column_text(stmt,4), sqlite3_column_text(stmt,3), latitude, longitude, latex, longex);
                }
                else {
                    querybuffer = sqlite3_mprintf("insert into changeto (id, change_type, existing_id, addr_street, addr_housenumber, addr_postcode, addr_city, latitude, longitude) values (%d, 'moved_too_far', '%q', NULL, NULL, NULL, NULL, %f, %f)", changetorownumber, sqlite3_column_text(stmt2, 0), latitude, longitude);
                    basic_query(db, querybuffer, 0);
                    sqlite3_free(querybuffer);
                    changetorownumber++;
                    int lastid = sqlite3_last_insert_rowid(db);
                    querybuffer = sqlite3_mprintf("update existing set changeto_id='%d'  where id = '%q'", lastid, sqlite3_column_text(stmt2, 0));
                    basic_query(db, querybuffer, 0);
                    sqlite3_free(querybuffer);
                }
            }
            sqlite3_finalize(stmt2);
        }
        sqlite3_finalize(stmt3);
    }
    sqlite3_finalize(stmt);
}


void find_same_pos_but_new_data(sqlite3 *db)
{
    int ret;
    sqlite3_stmt *stmt, *stmt2;
    char *querybuffer;
    double latitude, latmargin, longitude, lonmargin;
    double meter_margin = 5;
    latmargin = meter_to_latitude(meter_margin);

    querybuffer = sqlite3_mprintf("select id, addr_street, addr_housenumber, addr_city, addr_postcode, latitude, longitude from newnodes where foundindataset = 0;");
    ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt,0);
    sqlite3_free(querybuffer);
    while((ret = sqlite3_step(stmt)) == SQLITE_ROW){
        latitude = sqlite3_column_double(stmt,5);
        longitude = sqlite3_column_double(stmt,6);
        lonmargin = meter_to_longitude(meter_margin, latitude);
        //printf("latitude: %f\n", latitude);
        querybuffer = sqlite3_mprintf("select id, addr_street, addr_housenumber, addr_city, addr_postcode from existing where foundindataset = 0 and latitude > '%f' and latitude < '%f' and longitude > '%f' and longitude < '%f' and ((tag_number <= 4) or (tag_number <= 5 and building = 1));", latitude - latmargin, latitude + latmargin, longitude - lonmargin, longitude + lonmargin);
        ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt2,0);
        sqlite3_free(querybuffer);
        while((ret = sqlite3_step(stmt2)) == SQLITE_ROW){
            querybuffer = sqlite3_mprintf("update existing set foundindataset=1 where id='%q'", sqlite3_column_text(stmt2,0));
            basic_query(db, querybuffer, 0);
            sqlite3_free(querybuffer);
            querybuffer = sqlite3_mprintf("update newnodes set foundindataset=1 where id='%q'", sqlite3_column_text(stmt,0));
            basic_query(db, querybuffer, 0);
            sqlite3_free(querybuffer);

            querybuffer = sqlite3_mprintf("insert into changeto (id, change_type, existing_id, addr_street, addr_housenumber, addr_postcode, addr_city, latitude, longitude) values (%d, 'samepos_new_data', '%q', '%q', '%q', '%q', '%q', NULL, NULL)", changetorownumber, sqlite3_column_text(stmt2, 0), sqlite3_column_text(stmt,1), sqlite3_column_text(stmt,2), sqlite3_column_text(stmt,4), sqlite3_column_text(stmt,3));

            basic_query(db, querybuffer, 0);
            sqlite3_free(querybuffer);
            changetorownumber++;
            int lastid = sqlite3_last_insert_rowid(db);
            querybuffer = sqlite3_mprintf("update existing set changeto_id='%d'  where id = '%q'", lastid, sqlite3_column_text(stmt2, 0));
            basic_query(db, querybuffer, 0);
            sqlite3_free(querybuffer);
            break;
        }
        sqlite3_finalize(stmt2);
    }
    sqlite3_finalize(stmt);
}

void print_new_nodes_not_found(sqlite3 *db)
{
    int ret;
    sqlite3_stmt *stmt;
    char *querybuffer;

    querybuffer = sqlite3_mprintf("select id, addr_street, addr_housenumber, addr_city, addr_postcode from newnodes where foundindataset = 0 order by addr_street, addr_housenumber, addr_postcode;");
    ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt,0);
    sqlite3_free(querybuffer);
    while((ret = sqlite3_step(stmt)) == SQLITE_ROW){
        printf("%s %s, %s %s\n", sqlite3_column_text(stmt,1), sqlite3_column_text(stmt,2), sqlite3_column_text(stmt,4), sqlite3_column_text(stmt,3));
    }
    sqlite3_finalize(stmt);
}

void dump_new_nodes_not_found(sqlite3 *db) {
    int ret;
    sqlite3_stmt *stmt, *stmt2;
    char *querybuffer;
    char cntBuf[32];
    xmlNode *newNode;
    xmlNode *newTagNode;

    querybuffer = sqlite3_mprintf("select id, addr_street, addr_housenumber, addr_city, addr_postcode, latitude, longitude from newnodes where foundindataset=0 order by addr_street, addr_housenumber;");
    ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt,0);
    sqlite3_free(querybuffer);
    while((ret = sqlite3_step(stmt)) == SQLITE_ROW){
        newNode = xmlNewNode(NULL, "node");
        snprintf(cntBuf, 31, "%d", global_new_node_id);
        xmlSetProp(newNode, "id", cntBuf);
        xmlSetProp(newNode, "version", "1");
        xmlSetProp(newNode, "lat", sqlite3_column_text(stmt,5));
        xmlSetProp(newNode, "lon", sqlite3_column_text(stmt,6));

        newTagNode = xmlNewNode(NULL, "tag");
        xmlSetProp(newTagNode, "k", "addr:street");
        xmlSetProp(newTagNode, "v", sqlite3_column_text(stmt, 1));
        xmlAddChild(newNode, newTagNode);

        newTagNode = xmlNewNode(NULL, "tag");
        xmlSetProp(newTagNode, "k", "addr:housenumber");
        xmlSetProp(newTagNode, "v", sqlite3_column_text(stmt, 2));
        xmlAddChild(newNode, newTagNode);

        newTagNode = xmlNewNode(NULL, "tag");
        xmlSetProp(newTagNode, "k", "addr:city");
        xmlSetProp(newTagNode, "v", sqlite3_column_text(stmt, 3));
        xmlAddChild(newNode, newTagNode);

        newTagNode = xmlNewNode(NULL, "tag");
        xmlSetProp(newTagNode, "k", "addr:postcode");
        xmlSetProp(newTagNode, "v", sqlite3_column_text(stmt, 4));
        xmlAddChild(newNode, newTagNode);

        xmlNode *root_element = xmlDocGetRootElement(doc_newnodes_manual);
        xmlAddChild(root_element, newNode);
        global_new_node_id--;
    }
    sqlite3_finalize(stmt);
}


void print_not_yet_matched(sqlite3 *db)
{
    int ret;
    sqlite3_stmt *stmt;
    char *querybuffer;

    querybuffer = sqlite3_mprintf("select id, addr_street, addr_housenumber, addr_city, addr_postcode from existing where foundindataset = 0 and ((tag_number <= 4) or (tag_number <= 5 and building = 1)) order by addr_street, addr_housenumber, addr_postcode;");
    ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt,0);
    sqlite3_free(querybuffer);
    while((ret = sqlite3_step(stmt)) == SQLITE_ROW){
        printf("%s %s, %s %s\n", sqlite3_column_text(stmt,1), sqlite3_column_text(stmt,2), sqlite3_column_text(stmt,4), sqlite3_column_text(stmt,3));
    }
    sqlite3_finalize(stmt);
}


void dump_not_yet_matched(sqlite3 *db) {
    int ret;
    sqlite3_stmt *stmt;
    char *querybuffer;
    xmlNode *tmp_node;

    querybuffer = sqlite3_mprintf("select id, addr_street, addr_housenumber, addr_city, addr_postcode, latitude, longitude, isway, file_index from existing where foundindataset = 0 and ((tag_number <= 4) or (tag_number <= 5 and building = 1)) order by addr_street, addr_housenumber;");
    ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt,0);
    sqlite3_free(querybuffer);
    while((ret = sqlite3_step(stmt)) == SQLITE_ROW){

        if(sqlite3_column_int(stmt, 7)) // if isway
            tmp_node = get_xml_node(doc_existing_ways, sqlite3_column_int(stmt, 8), 1);
        else
            tmp_node = get_xml_node(doc_existing_nodes, sqlite3_column_int(stmt, 8), 0);
        //replace_tag_value(tmp_node, "addr:street", sqlite3_column_text(stmt, 1));
        //replace_tag_value(tmp_node, "addr:housenumber", sqlite3_column_text(stmt, 2));
        //replace_tag_value(tmp_node, "addr:city", sqlite3_column_text(stmt, 3));
        //replace_tag_value(tmp_node, "addr:postcode", sqlite3_column_text(stmt, 4));
        xmlNode *newNode = xmlCopyNode(tmp_node, 1);
        xmlNode *root_element = xmlDocGetRootElement(doc_notmatched_manual);
        xmlAddChild(root_element, newNode);
    }
    sqlite3_finalize(stmt);
}


void print_new_nodes_which_are_not_close_to_any_old_ones(sqlite3 *db, double meter_margin)
{
    int ret;
    sqlite3_stmt *stmt, *stmt2;
    char *querybuffer;
    double latitude, latmargin, longitude, lonmargin;
    char found;
    latmargin = meter_to_latitude(meter_margin);

    querybuffer = sqlite3_mprintf("select id, addr_street, addr_housenumber, addr_city, addr_postcode, latitude, longitude from newnodes where foundindataset = 0;");
    ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt,0);
    sqlite3_free(querybuffer);
    while((ret = sqlite3_step(stmt)) == SQLITE_ROW){
        found = 0;
        latitude = sqlite3_column_double(stmt,5);
        longitude = sqlite3_column_double(stmt,6);
        lonmargin = meter_to_longitude(meter_margin, latitude);
        //printf("latitude: %f\n", latitude);
        querybuffer = sqlite3_mprintf("select id, addr_street, addr_housenumber, addr_city, addr_postcode from existing where foundindataset = 0 and latitude > '%f' and latitude < '%f' and longitude > '%f' and longitude < '%f';", latitude - latmargin, latitude + latmargin, longitude - lonmargin, longitude + lonmargin);
        //printf("%s\n", querybuffer);
        ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt2,0);
        sqlite3_free(querybuffer);
        while((ret = sqlite3_step(stmt2)) == SQLITE_ROW){
            found = 1;
        }
        sqlite3_finalize(stmt2);

        if(!found)
            printf("%s %s, %s %s\n", sqlite3_column_text(stmt,1), sqlite3_column_text(stmt,2), sqlite3_column_text(stmt,4), sqlite3_column_text(stmt,3));
    }
    sqlite3_finalize(stmt);
}


void add_new_nodes_which_are_not_close_to_any_old_ones(sqlite3 *db, double meter_margin)
{
    int ret;
    sqlite3_stmt *stmt, *stmt2;
    char *querybuffer;
    double latitude, latmargin, longitude, lonmargin;
    char found;
    latmargin = meter_to_latitude(meter_margin);

    querybuffer = sqlite3_mprintf("select id, addr_street, addr_housenumber, addr_city, addr_postcode, latitude, longitude from newnodes where foundindataset = 0;");
    ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt,0);
    sqlite3_free(querybuffer);
    while((ret = sqlite3_step(stmt)) == SQLITE_ROW){
        found = 0;
        latitude = sqlite3_column_double(stmt,5);
        longitude = sqlite3_column_double(stmt,6);
        lonmargin = meter_to_longitude(meter_margin, latitude);
        //printf("latitude: %f\n", latitude);
        querybuffer = sqlite3_mprintf("select id, addr_street, addr_housenumber, addr_city, addr_postcode from existing where foundindataset = 0 and latitude > '%f' and latitude < '%f' and longitude > '%f' and longitude < '%f';", latitude - latmargin, latitude + latmargin, longitude - lonmargin, longitude + lonmargin);
        //printf("%s\n", querybuffer);
        ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt2,0);
        sqlite3_free(querybuffer);
        while((ret = sqlite3_step(stmt2)) == SQLITE_ROW){
            found = 1;
        }
        sqlite3_finalize(stmt2);

        if(!found) {
            //querybuffer = sqlite3_mprintf("update newnodes set foundindataset = 1 where id = '%q'", sqlite3_column_text(stmt, 0)); // FIXME: rename "foundindataset" to "handled"
            querybuffer = sqlite3_mprintf("update newnodes set foundindataset = 1 where id = %d", sqlite3_column_int(stmt, 0)); // FIXME: rename "foundindataset" to "handled"
            basic_query(db, querybuffer, 0);
            sqlite3_free(querybuffer);
            // FIXME: Actually add to a separate table for adding
            querybuffer = sqlite3_mprintf("insert into changeto (id, change_type, existing_id, addr_street, addr_housenumber, addr_postcode, addr_city, latitude, longitude) values (%d, 'new_nodes_far_away', NULL, '%q', '%q', '%q', '%q', %f, %f)", changetorownumber, sqlite3_column_text(stmt,1), sqlite3_column_text(stmt,2), sqlite3_column_text(stmt,4), sqlite3_column_text(stmt,3), latitude, longitude);
            basic_query(db, querybuffer, 0);
            sqlite3_free(querybuffer);
            changetorownumber++;

        }
    }
    sqlite3_finalize(stmt);
}


void print_new_nodes_and_suggested_existing_nearby(sqlite3 *db, double meter_margin)
{
    int ret;
    sqlite3_stmt *stmt, *stmt2;
    char *querybuffer;
    double latitude, latmargin, longitude, lonmargin;
    char found;
    latmargin = meter_to_latitude(meter_margin);

    querybuffer = sqlite3_mprintf("select id, addr_street, addr_housenumber, addr_city, addr_postcode, latitude, longitude from newnodes where foundindataset = 0;");
    ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt,0);
    sqlite3_free(querybuffer);
    while((ret = sqlite3_step(stmt)) == SQLITE_ROW){
        found = 0;
        latitude = sqlite3_column_double(stmt,5);
        longitude = sqlite3_column_double(stmt,6);
        lonmargin = meter_to_longitude(meter_margin, latitude);
        //printf("latitude: %f\n", latitude);
        querybuffer = sqlite3_mprintf("select id, addr_street, addr_housenumber, addr_city, addr_postcode from existing where foundindataset = 0 and ((tag_number <= 4) or (tag_number <= 5 and building = 1)) and latitude > '%f' and latitude < '%f' and longitude > '%f' and longitude < '%f';", latitude - latmargin, latitude + latmargin, longitude - lonmargin, longitude + lonmargin);
        //printf("%s\n", querybuffer);
        ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt2,0);
        sqlite3_free(querybuffer);
        while((ret = sqlite3_step(stmt2)) == SQLITE_ROW){
            printf("%s %s, %s %s    is close to existing %s %s, %s %s\n", sqlite3_column_text(stmt,1), sqlite3_column_text(stmt,2), sqlite3_column_text(stmt,4), sqlite3_column_text(stmt,3),  sqlite3_column_text(stmt2,1), sqlite3_column_text(stmt2,2), sqlite3_column_text(stmt2,4), sqlite3_column_text(stmt2,3));
            found = 1;
        }
        sqlite3_finalize(stmt2);

        //if(!found)
        //    printf("%s %s, %s %s\n", sqlite3_column_text(stmt,1), sqlite3_column_text(stmt,2), sqlite3_column_text(stmt,4), sqlite3_column_text(stmt,3));
    }
    sqlite3_finalize(stmt);
}

void print_nodes_where_data_will_be_changed(sqlite3 *db) {
    int ret;
    sqlite3_stmt *stmt, *stmt2;
    char *querybuffer;

    querybuffer = sqlite3_mprintf("select id, addr_street, addr_housenumber, addr_city, addr_postcode, latitude, longitude, existing_id from changeto where change_type='samepos_new_data' order by addr_street, addr_housenumber, addr_postcode;");
    ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt,0);
    sqlite3_free(querybuffer);
    while((ret = sqlite3_step(stmt)) == SQLITE_ROW){
        querybuffer = sqlite3_mprintf("select id, addr_street, addr_housenumber, addr_city, addr_postcode, latitude, longitude from existing where id=%d;", sqlite3_column_int(stmt, 7));
        ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt2,0);
        sqlite3_free(querybuffer);
        if((ret = sqlite3_step(stmt2)) == SQLITE_ROW){
            printf("%s %s, %s %s  ->  %s %s, %s %s\n", sqlite3_column_text(stmt2,1), sqlite3_column_text(stmt2,2), sqlite3_column_text(stmt2,4), sqlite3_column_text(stmt2,3), sqlite3_column_text(stmt,1), sqlite3_column_text(stmt,2), sqlite3_column_text(stmt,4), sqlite3_column_text(stmt,3)); 
        }
        sqlite3_finalize(stmt2);
    }
    sqlite3_finalize(stmt);
}

void replace_tag_value(xmlNode *node, char *key, const char *new_val)
{
    xmlChar *text;
    xmlNode *tag_node;
    for(tag_node = node->children; tag_node ; tag_node = tag_node->next) {
        if(tag_node->type == XML_ELEMENT_NODE) {
            text = xmlGetProp(tag_node, "k");
            if(text != 0) {
                if(strcmp(text, key) == 0) {
                    xmlSetProp(tag_node, "v", new_val);
                }
            }
            xmlFree(text);
        }
    }
}

void dump_osm_nodes_where_data_will_be_changed(sqlite3 *db) {
    int ret;
    sqlite3_stmt *stmt, *stmt2;
    char *querybuffer;
    xmlNode *tmp_node;

    querybuffer = sqlite3_mprintf("select id, addr_street, addr_housenumber, addr_city, addr_postcode, latitude, longitude, existing_id from changeto where change_type='samepos_new_data' order by addr_street, addr_housenumber;");
    ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt,0);
    sqlite3_free(querybuffer);
    while((ret = sqlite3_step(stmt)) == SQLITE_ROW){
        querybuffer = sqlite3_mprintf("select id, addr_street, addr_housenumber, addr_city, addr_postcode, latitude, longitude, isway, file_index from existing where id=%d;", sqlite3_column_int(stmt, 7));
        ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt2,0);
        sqlite3_free(querybuffer);
        if((ret = sqlite3_step(stmt2)) == SQLITE_ROW){

            if(sqlite3_column_int(stmt2, 7)) // if isway
                tmp_node = get_xml_node(doc_existing_ways, sqlite3_column_int(stmt2, 8), 1);
            else
                tmp_node = get_xml_node(doc_existing_nodes, sqlite3_column_int(stmt2, 8), 0);
            replace_tag_value(tmp_node, "addr:street", sqlite3_column_text(stmt, 1));
            replace_tag_value(tmp_node, "addr:housenumber", sqlite3_column_text(stmt, 2));
            replace_tag_value(tmp_node, "addr:city", sqlite3_column_text(stmt, 3));
            replace_tag_value(tmp_node, "addr:postcode", sqlite3_column_text(stmt, 4));
            xmlSetProp(tmp_node,"action", "modify");
            xmlNode *newNode = xmlCopyNode(tmp_node, 1);
            xmlNode *root_element = xmlDocGetRootElement(doc_changes);
            xmlAddChild(root_element, newNode);
        }
        sqlite3_finalize(stmt2);
    }
    sqlite3_finalize(stmt);
}


void print_nodes_where_coordinates_will_be_changed(sqlite3 *db) {
    int ret;
    sqlite3_stmt *stmt, *stmt2;
    char *querybuffer;

    querybuffer = sqlite3_mprintf("select id, addr_street, addr_housenumber, addr_city, addr_postcode, latitude, longitude, existing_id from changeto where change_type='moved_exact' order by addr_street, addr_housenumber, addr_postcode;");
    ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt,0);
    sqlite3_free(querybuffer);
    while((ret = sqlite3_step(stmt)) == SQLITE_ROW){
        querybuffer = sqlite3_mprintf("select id, addr_street, addr_housenumber, addr_city, addr_postcode, latitude, longitude from existing where id=%d;", sqlite3_column_int(stmt, 7));
        ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt2,0);
        sqlite3_free(querybuffer);
        if((ret = sqlite3_step(stmt2)) == SQLITE_ROW){
            printf("%s %s, %s %s:  %f,%f -> %f,%f\n", sqlite3_column_text(stmt2,1), sqlite3_column_text(stmt2,2), sqlite3_column_text(stmt2,4), sqlite3_column_text(stmt2,3), sqlite3_column_double(stmt2,5), sqlite3_column_double(stmt2,6), sqlite3_column_double(stmt,5), sqlite3_column_double(stmt,6)); 
        }
        sqlite3_finalize(stmt2);
    }
    sqlite3_finalize(stmt);
}


void dump_osm_nodes_where_coordinates_will_be_changed(sqlite3 *db) {
    int ret;
    sqlite3_stmt *stmt, *stmt2;
    char *querybuffer;
    xmlNode *tmp_node;

    querybuffer = sqlite3_mprintf("select id, addr_street, addr_housenumber, addr_city, addr_postcode, latitude, longitude, existing_id from changeto where change_type='moved_exact' order by addr_street, addr_housenumber;");
    ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt,0);
    sqlite3_free(querybuffer);
    while((ret = sqlite3_step(stmt)) == SQLITE_ROW){
        querybuffer = sqlite3_mprintf("select id, addr_street, addr_housenumber, addr_city, addr_postcode, latitude, longitude, isway, file_index from existing where id=%d;", sqlite3_column_int(stmt, 7));
        ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt2,0);
        sqlite3_free(querybuffer);
        if((ret = sqlite3_step(stmt2)) == SQLITE_ROW){

            if(sqlite3_column_int(stmt2, 7)) // if isway
                tmp_node = get_xml_node(doc_existing_ways, sqlite3_column_int(stmt2, 8), 1);
            else
                tmp_node = get_xml_node(doc_existing_nodes, sqlite3_column_int(stmt2, 8), 0);
            xmlSetProp(tmp_node, "lat", sqlite3_column_text(stmt,5));
            xmlSetProp(tmp_node, "lon", sqlite3_column_text(stmt,6));
            xmlSetProp(tmp_node,"action", "modify");
            xmlNode *newNode = xmlCopyNode(tmp_node, 1);
            xmlNode *root_element = xmlDocGetRootElement(doc_changes);
            xmlAddChild(root_element, newNode);
        }
        sqlite3_finalize(stmt2);
    }
    sqlite3_finalize(stmt);
}

void print_nodes_which_have_moved_too_far(sqlite3 *db) {
    int ret;
    sqlite3_stmt *stmt, *stmt2;
    char *querybuffer;

    querybuffer = sqlite3_mprintf("select id, addr_street, addr_housenumber, addr_city, addr_postcode, latitude, longitude, existing_id from changeto where change_type='moved_too_far' order by addr_street, addr_housenumber;");
    ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt,0);
    sqlite3_free(querybuffer);
    while((ret = sqlite3_step(stmt)) == SQLITE_ROW){
        querybuffer = sqlite3_mprintf("select id, addr_street, addr_housenumber, addr_city, addr_postcode, latitude, longitude from existing where id=%d;", sqlite3_column_int(stmt, 7));
        ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt2,0);
        sqlite3_free(querybuffer);
        if((ret = sqlite3_step(stmt2)) == SQLITE_ROW){
            printf("%s %s, %s %s:  %f,%f -> %f,%f\n", sqlite3_column_text(stmt2,1), sqlite3_column_text(stmt2,2), sqlite3_column_text(stmt2,4), sqlite3_column_text(stmt2,3), sqlite3_column_double(stmt2,5), sqlite3_column_double(stmt2,6), sqlite3_column_double(stmt,5), sqlite3_column_double(stmt,6)); 
        }
        sqlite3_finalize(stmt2);
    }
    sqlite3_finalize(stmt);
}

void print_buildings_which_are_too_far_away(sqlite3 *db) {
    int ret;
    sqlite3_stmt *stmt, *stmt2;
    char *querybuffer;

    querybuffer = sqlite3_mprintf("select id, addr_street, addr_housenumber, addr_city, addr_postcode, latitude, longitude, existing_id from changeto where change_type='building_moved_too_far' order by addr_street, addr_housenumber;");
    ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt,0);
    sqlite3_free(querybuffer);
    while((ret = sqlite3_step(stmt)) == SQLITE_ROW){
        querybuffer = sqlite3_mprintf("select id, addr_street, addr_housenumber, addr_city, addr_postcode, latitude, longitude from existing where id=%d;", sqlite3_column_int(stmt, 7));
        ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt2,0);
        sqlite3_free(querybuffer);
        if((ret = sqlite3_step(stmt2)) == SQLITE_ROW){
            printf("%s %s, %s %s:  %f,%f -> %f,%f\n", sqlite3_column_text(stmt2,1), sqlite3_column_text(stmt2,2), sqlite3_column_text(stmt2,4), sqlite3_column_text(stmt2,3), sqlite3_column_double(stmt2,5), sqlite3_column_double(stmt2,6), sqlite3_column_double(stmt,5), sqlite3_column_double(stmt,6)); 
        }
        sqlite3_finalize(stmt2);
    }
    sqlite3_finalize(stmt);
}


void print_new_nodes_which_will_be_added(sqlite3 *db) {
    int ret;
    sqlite3_stmt *stmt, *stmt2;
    char *querybuffer;

    querybuffer = sqlite3_mprintf("select id, addr_street, addr_housenumber, addr_city, addr_postcode, latitude, longitude, existing_id from changeto where change_type='new_nodes_far_away' order by addr_street, addr_housenumber, addr_postcode;");
    ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt,0);
    sqlite3_free(querybuffer);
    while((ret = sqlite3_step(stmt)) == SQLITE_ROW){
        printf("%s %s, %s %s (%f,%f)\n", sqlite3_column_text(stmt,1), sqlite3_column_text(stmt,2), sqlite3_column_text(stmt,4), sqlite3_column_text(stmt,3), sqlite3_column_double(stmt, 5), sqlite3_column_double(stmt,6));
    }
    sqlite3_finalize(stmt);
}


void dump_new_nodes_which_will_be_added(sqlite3 *db) {
    int ret;
    sqlite3_stmt *stmt, *stmt2;
    char *querybuffer;
    char cntBuf[32];
    xmlNode *newNode;
    xmlNode *newTagNode;

    querybuffer = sqlite3_mprintf("select id, addr_street, addr_housenumber, addr_city, addr_postcode, latitude, longitude, existing_id from changeto where change_type='new_nodes_far_away' order by addr_street, addr_housenumber;");
    ret = sqlite3_prepare_v2(db,querybuffer,-1,&stmt,0);
    sqlite3_free(querybuffer);
    while((ret = sqlite3_step(stmt)) == SQLITE_ROW){
        newNode = xmlNewNode(NULL, "node");
        snprintf(cntBuf, 31, "%d", global_new_node_id);
        xmlSetProp(newNode, "id", cntBuf);
        xmlSetProp(newNode, "version", "1");
        xmlSetProp(newNode, "lat", sqlite3_column_text(stmt,5));
        xmlSetProp(newNode, "lon", sqlite3_column_text(stmt,6));

        newTagNode = xmlNewNode(NULL, "tag");
        xmlSetProp(newTagNode, "k", "addr:street");
        xmlSetProp(newTagNode, "v", sqlite3_column_text(stmt, 1));
        xmlAddChild(newNode, newTagNode);

        newTagNode = xmlNewNode(NULL, "tag");
        xmlSetProp(newTagNode, "k", "addr:housenumber");
        xmlSetProp(newTagNode, "v", sqlite3_column_text(stmt, 2));
        xmlAddChild(newNode, newTagNode);

        newTagNode = xmlNewNode(NULL, "tag");
        xmlSetProp(newTagNode, "k", "addr:city");
        xmlSetProp(newTagNode, "v", sqlite3_column_text(stmt, 3));
        xmlAddChild(newNode, newTagNode);

        newTagNode = xmlNewNode(NULL, "tag");
        xmlSetProp(newTagNode, "k", "addr:postcode");
        xmlSetProp(newTagNode, "v", sqlite3_column_text(stmt, 4));
        xmlAddChild(newNode, newTagNode);

        xmlNode *root_element = xmlDocGetRootElement(doc_changes);
        xmlAddChild(root_element, newNode);
        global_new_node_id--;
    }
    sqlite3_finalize(stmt);
}


void remove_all_subnodes_from_xml(xmlDoc *doc)
{
    xmlNode *root_element = xmlDocGetRootElement(doc);
    xmlNode *cur_node;
    xmlNode *tmp_node;
    for(cur_node = root_element->children;cur_node;){
        xmlNode *tmp_node;
        tmp_node = cur_node;
        cur_node = cur_node->next;
        xmlUnlinkNode(tmp_node);
        xmlFreeNode(tmp_node);
    }
}

void export_osm(xmlDoc *doc, char *file_name)
{
    char fullpath [256];
    strncpy(fullpath, outdir_path, 255);
    strncat(fullpath, "/", 255 - strlen(fullpath));
    strncat(fullpath, file_name, 255 - strlen(fullpath) - 1);
    xmlSaveFileEnc(fullpath, doc, "UTF-8");
}

int parse_cmdline(int argc, char **argv)
{
    int s;
    opterr = 0;
    while((s = getopt(argc, argv, "vc:")) != -1) {
        switch (s) {
            //case 's':
            //    only_info = 1;
            //    break;
            case 'v':
                verbose = 1;
                break;
            case 'c':
                correctionsfilename = (char*) malloc(strlen(optarg)+1);
                snprintf(correctionsfilename,strlen(optarg)+1,"%s",optarg);
                break;
            case '?':
                if(optopt == 'c')
                    fprintf(stderr, "Option -%c requires an argument.\n",optopt);
                else if(isprint(optopt)) 
                    fprintf(stderr, "Unknown option '-%c'.\n",optopt);
                return -1;
            default:
                abort();
        }
    }

    if(argc != (optind + 4)){
        fprintf(stderr, "Usage: %s <existing_nodes.osm> <existing_ways.osm> <new_nodes.osm> <output_dir>\n", argv[0]);
        return -1;
    }
    existing_node_filename = (char*) malloc(strlen(argv[optind])+1);
    snprintf(existing_node_filename,strlen(argv[optind])+1,"%s",argv[optind]);
    existing_ways_filename = (char*) malloc(strlen(argv[optind+1])+1);
    snprintf(existing_ways_filename,strlen(argv[optind+1])+1,"%s",argv[optind+1]);
    new_nodes_filename = (char*) malloc(strlen(argv[optind+2])+1);
    snprintf(new_nodes_filename,strlen(argv[optind+2])+1,"%s",argv[optind+2]);
    outdir_path = (char*) malloc(strlen(argv[optind+3])+1);
    snprintf(outdir_path,strlen(argv[optind+3])+1,"%s",argv[optind+3]);
    return 0;
}

int main(int argc, char **argv)
{
    int ret;

    sqlite3 *db = NULL;

    if(parse_cmdline(argc, argv) != 0){
         return -1;
    }

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

    xmlDoc *doc_new_nodes = NULL;
    xmlDoc *doc_corrections = NULL;
    xmlNode *root_element = NULL;

    basic_query(db,"create table if not exists corrections (id int auto_increment primary key not null, fromname varchar(100), toname varchar(100));",0); // Need to create table in any case since it is looked up later
    if(correctionsfilename != NULL) {
        doc_corrections = xmlReadFile(correctionsfilename,NULL,0);
        if(doc_corrections == NULL) {
            printf("error: could not parse file %s\n", correctionsfilename);
        }
        root_element = xmlDocGetRootElement(doc_corrections);
        get_corrections(root_element,db);
        xmlFreeDoc(doc_corrections);
    }

    doc_new_nodes = xmlReadFile(new_nodes_filename, NULL, 0);
    if (doc_new_nodes == NULL) {
        printf("error: could not parse file %s\n", new_nodes_filename);
    }
    root_element = xmlDocGetRootElement(doc_new_nodes);


    populate_database_newnodes(root_element, db);
    xmlFreeDoc(doc_new_nodes);


    // Used to get empty XML to fill with new fields:
    doc_existing_nodes = xmlReadFile(existing_node_filename, NULL, 0);
    remove_all_subnodes_from_xml(doc_existing_nodes);
    doc_changes = doc_existing_nodes;
    doc_notmatched_manual = xmlCopyDoc(doc_changes,1);
    doc_newnodes_manual = xmlCopyDoc(doc_changes,1);
    //

    doc_existing_nodes = xmlReadFile(existing_node_filename, NULL, 0);
    if (doc_existing_nodes == NULL) {
        printf("error: could not parse file %s\n", existing_node_filename);
    }
    root_element = xmlDocGetRootElement(doc_existing_nodes);

    populate_database_existing(root_element, db, 0);


    doc_existing_ways = xmlReadFile(existing_ways_filename, NULL, 0);
    if (doc_existing_ways == NULL) {
        printf("error: could not parse file %s\n", existing_ways_filename);
    }
    root_element = xmlDocGetRootElement(doc_existing_ways);

    populate_database_existing(root_element, db, 1);



    // Do the actual processing
    match_exact(db);
    match_corrected(db);
    find_exact_data_but_moved_around(db);
    find_same_pos_but_new_data(db);
    find_corrected_data_but_moved_around(db);
    find_exact_data_but_building(db);

    add_new_nodes_which_are_not_close_to_any_old_ones(db, 100); // All new nodes which are at least 100 metres from
                                                                // any existing ones, can be added automatically !
    //print_new_nodes_which_are_not_close_to_any_old_ones(db, 100);
    //print_new_nodes_and_suggested_existing_nearby(db, 20);
    printf("Not matched existing nodes:\n");
    print_not_yet_matched(db);
    printf("\nNew nodes which needs manual inspection:\n");
    print_new_nodes_not_found(db);
    printf("\nNodes with correct existing data which have moved too far. Needs manual inspection:\n");
    print_nodes_which_have_moved_too_far(db);
    printf("\nBuildings with correct existing data which have moved too far. Needs manual inspection:\n");
    print_buildings_which_are_too_far_away(db);
    printf("\nData will be changed:\n");
    print_nodes_where_data_will_be_changed(db);
    printf("\nCoordinates will be changed:\n");
    print_nodes_where_coordinates_will_be_changed(db);
    printf("\nNew nodes which will be added:\n");
    print_new_nodes_which_will_be_added(db);


    dump_osm_nodes_where_data_will_be_changed(db);
    dump_osm_nodes_where_coordinates_will_be_changed(db);
    dump_new_nodes_which_will_be_added(db);
    dump_new_nodes_not_found(db);
    dump_not_yet_matched(db);
    export_osm(doc_changes, "changes.osm");
    export_osm(doc_newnodes_manual, "newnodes_manual.osm");
    export_osm(doc_notmatched_manual, "notmatched_manual.osm");

    sqlite3_close(db);

    xmlFreeDoc(doc_existing_nodes);
    xmlFreeDoc(doc_existing_ways);
    xmlFreeDoc(doc_newnodes_manual);
    xmlFreeDoc(doc_notmatched_manual);
    xmlFreeDoc(doc_changes);

    xmlCleanupParser();

    free(existing_node_filename);
    free(existing_ways_filename);
    free(new_nodes_filename);
    if(correctionsfilename != NULL)
        free(correctionsfilename);

    return 0;
}
