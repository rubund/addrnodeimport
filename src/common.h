#ifndef INC_COMMON_H
#define INC_COMMON_H

void get_corrections(xmlNode * a_node, sqlite3 *db);
xmlNode *get_xml_node(xmlDoc *doc,int index,int isway);
void correct_name(sqlite3 *db, char *in, char *out, int out_max_len);


#endif
