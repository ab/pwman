/*
 *  PWMan - password management application
 *
 *  Copyright (C) 2002  Ivan Kelly <ivan@ivankelly.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <pwman.h>
#include <errno.h>
#include <unistd.h>
#include <libxml/tree.h>
#include <libxml/parser.h>

int pwindex = 0;
extern int errno;
void pwlist_free_pw(Pw*);


PWList *
pwlist_new(char *name)
{
	PWList *new;
	new = malloc( sizeof(PWList) );
	new->name = malloc(STRING_MEDIUM);
	strncpy(new->name, name, STRING_MEDIUM);
	new->parent = NULL;
	new->list = NULL;
	new->sublists = NULL;
	debug("new_pwlist: %s", name);

	return new;
}

int
pwlist_init()
{
	pwindex = 0;
	pwlist = NULL;
	current_pw_sublist = NULL;
}

int 
pwlist_free(PWList *old)
{
	Pw *current, *next;
	PWList *curlist, *nlist;

	if(old == NULL){
		return 0;
	}
	for(current = old->list; current != NULL; current = next){
		next = current->next;

		pwlist_free_pw(current);
	}
	for(curlist = old->sublists; curlist != NULL; curlist = nlist){
		nlist = curlist->next;

		pwlist_free(curlist);
	}
	
	free(old->name);
	free(old);
	old = NULL;
	return 0;
}

int
pwlist_free_all()
{
	pwlist_free(pwlist);
	return 0;
}

Pw*
pwlist_new_pw()
{
	Pw *new;
	new = malloc(sizeof(Pw));
	new->id = 0;
	new->name = malloc(STRING_MEDIUM);
	new->host = malloc(STRING_MEDIUM);
	new->user = malloc(STRING_MEDIUM);
	new->passwd = malloc(STRING_SHORT);
	new->launch = malloc(STRING_LONG);

	memset(new->name, 0, STRING_MEDIUM);
	memset(new->host, 0, STRING_MEDIUM);
	memset(new->user, 0, STRING_MEDIUM);
	memset(new->passwd, 0, STRING_SHORT);
	memset(new->launch, 0, STRING_LONG);
	
	return new;
}

void
pwlist_free_pw(Pw *old)
{
	free(old->name);
	free(old->user);
	free(old->host);
	free(old->passwd);
	free(old->launch);
	free(old);
}

int
pwlist_change_item_order(Pw* pw, PWList *parent, int moveUp) {
	Pw *iter = NULL;
	Pw *pprev = NULL;
	Pw *prev = NULL;
	Pw *next = NULL;
	Pw *nnext = NULL;

	// Find us, in our parents list of children
	for(iter = parent->list; iter != NULL; iter = iter->next){
		if(iter == pw) {
			// Grab the next one, and the one after
			next = pw->next;
			if(next != NULL) {
				nnext = next->next;
			} else {
				nnext = NULL;
			}

			// Which way do we need to shuffle?
			if(moveUp) {
				// Up the list, if we can
				if(prev == NULL) { break; }

				// Are we going to the top?
				if(prev == parent->list) {
					parent->list = pw;
					pw->next = prev;
					prev->next = next;
				} else {
					pprev->next = pw;
					pw->next = prev;
					prev->next = next;
				}
				return 1;
			} else {
				// Down the list, if we can
				if(next == NULL) { break; }

				// Were we at the top?
				if(pw == parent->list) {
					parent->list = next;
					next->next = pw;
					pw->next = nnext;
				} else {
					prev->next = next;
					next->next = pw;
					pw->next = nnext;
				}
				return 1;
			}
		} else {
			// Update the running list of prev and pprev
			pprev = prev;
			prev = iter;
		}
	}

	return 0;
}

int
pwlist_change_list_order(PWList *pw, int moveUp) {
	// Grab the parent, assuming there is one
	PWList *parent = pw->parent;
	if(parent==NULL) { return 0; }


	// Find us
	PWList *iter = NULL;
	PWList *pprev = NULL;
	PWList *prev = NULL;
	PWList *next = NULL;
	PWList *nnext = NULL;

	// Find us, in our parents list of children
	for(iter = parent->sublists; iter != NULL; iter = iter->next){
		if(iter == pw) {
			// Grab the next one, and the one after
			next = pw->next;
			if(next != NULL) {
				nnext = next->next;
			} else {
				nnext = NULL;
			}

			// Which way do we need to shuffle?
			if(moveUp) {
				// Up the list, if we can
				if(prev == NULL) { break; }

				// Are we going to the top?
				if(prev == parent->sublists) {
					parent->sublists = pw;
					pw->next = prev;
					prev->next = next;
				} else {
					pprev->next = pw;
					pw->next = prev;
					prev->next = next;
				}
				return 1;
			} else {
				// Down the list, if we can
				if(next == NULL) { break; }

				// Were we at the top?
				if(pw == parent->sublists) {
					parent->sublists = next;
					next->next = pw;
					pw->next = nnext;
				} else {
					prev->next = next;
					next->next = pw;
					pw->next = nnext;
				}
				return 1;
			}
		} else {
			// Update the running list of prev and pprev
			pprev = prev;
			prev = iter;
		}
	}

	return 0;
}

int
pwlist_rename_item(Pw* pwitem, char* new_name) {
	strncpy(pwitem->name, new_name, STRING_MEDIUM);
}
int
pwlist_rename_sublist(PWList *pwlist, char* new_name) {
	strncpy(pwlist->name, new_name, STRING_MEDIUM);
}

int
pwlist_add(PWList *parent, char* name, char* host, char* user, char* passwd, char* launch)
{
	Pw* new = pwlist_new_pw();
	
	new->id = pwindex++;
	strncpy(new->name, name, STRING_MEDIUM);
	strncpy(new->host, host, STRING_MEDIUM);
	strncpy(new->user, user, STRING_MEDIUM);
	strncpy(new->passwd, passwd, STRING_SHORT);
	strncpy(new->launch, launch, STRING_LONG);

	pwlist_add_ptr(parent, new);

	return 0;
}

int 
pwlist_add_sublist(PWList *parent, PWList *new)
{
	PWList *current;

	current = parent->sublists;
	new->parent = parent;
	new->current_item = 1;

	if(current == NULL){
		debug("add_pw_sublist: current = NULL");
		parent->sublists = new;
		new->next = NULL;
		return 0;
	}
	while(current->next != NULL){
		current = current->next;
	}
	current->next = new;
	new->next = NULL;

	return 0;
}

int
pwlist_add_ptr(PWList *list, Pw *new)
{
	Pw *current;
	
	if(list == NULL){
		debug("add_pw_ptr : Bad PwList");
		return -1;
	}
	if(new == NULL){
		debug("add_pw_ptr : Bad Pw");
		return -1;
	}
	if(list->list == NULL){
		list->list = new;
		new->next = NULL;
		return 0;
	}

	debug("add_pw_ptr: add to list");
	current = list->list;
	while(current->next != NULL){
		current = current->next;
	}
	current->next = new;
	new->next = NULL;

	return 0;
}

int 
pwlist_detach_pw(PWList *list, Pw *pw)
{
	Pw *iter, *prev;

	prev = NULL;
	for(iter = list->list; iter != NULL; iter = iter->next){
		
		if(iter == pw){
			if(prev == NULL){
				list->list = iter->next;
			} else {
				prev->next = iter->next;
			}
			break;
		}
		prev = iter;
	}
}

int 
pwlist_delete_pw(PWList *list, Pw *pw)
{
	Pw *iter, *prev;

	prev = NULL;
	for(iter = list->list; iter != NULL; iter = iter->next){
		
		if(iter == pw){
			if(prev == NULL){
				list->list = iter->next;
			} else {
				prev->next = iter->next;
			}
			pwlist_free_pw(iter);
			break;
		}
		prev = iter;
	}
}

int 
pwlist_detach_sublist(PWList *parent, PWList *old)
{
	PWList *iter, *prev;

	prev = NULL;
	for(iter = parent->sublists; iter != NULL; iter = iter->next){

		if(iter == old){
			if(prev == NULL){
				parent->sublists = iter->next;
			} else {
				prev->next = iter->next;
			}
			break;
		}
		prev = iter;
	}
}

int
pwlist_delete_sublist(PWList *parent, PWList *old)
{
	PWList *iter, *prev;

	prev = NULL;
	for(iter = parent->sublists; iter != NULL; iter = iter->next){

		if(iter == old){
			if(prev == NULL){
				parent->sublists = iter->next;
			} else {
				prev->next = iter->next;
			}
			pwlist_free(iter);
			break;
		}
		prev = iter;
	}
}

int 
pwlist_write_node(xmlNodePtr root, Pw* pw)
{
	xmlNodePtr node;
	xmlChar *escaped;

	// Take the inbound strings. Treat as an xmlChar, and escape
	// Need to free the result of escape every time

	node = xmlNewChild(root, NULL, (xmlChar*)"PwItem", NULL);

	escaped = xmlEncodeSpecialChars(root->doc, (xmlChar*)pw->name);
	xmlNewChild(node, NULL, (xmlChar*)"name", escaped);
	xmlFree(escaped);
	
	escaped = xmlEncodeSpecialChars(root->doc, (xmlChar*)pw->host);
	xmlNewChild(node, NULL, (xmlChar*)"host", escaped);
	xmlFree(escaped);

	escaped = xmlEncodeSpecialChars(root->doc, (xmlChar*)pw->user);
	xmlNewChild(node, NULL, (xmlChar*)"user", escaped);
	xmlFree(escaped);

	escaped = xmlEncodeSpecialChars(root->doc, (xmlChar*)pw->passwd);
	xmlNewChild(node, NULL, (xmlChar*)"passwd", escaped);
	xmlFree(escaped);

	escaped = xmlEncodeSpecialChars(root->doc, (xmlChar*)pw->launch);
	xmlNewChild(node, NULL, (xmlChar*)"launch", escaped);
	xmlFree(escaped);
}

int
pwlist_write(xmlNodePtr parent, PWList *list)
{
	xmlNodePtr node;
	Pw *iter;
	PWList *pwliter;
	
	node = xmlNewChild(parent, NULL, (xmlChar*)"PwList", NULL);
	xmlSetProp(node, (xmlChar*)"name", (xmlChar*)list->name);	
	
	for(iter = list->list; iter != NULL; iter = iter->next){
		pwlist_write_node(node, iter);
	}
	for(pwliter = list->sublists; pwliter != NULL; pwliter = pwliter->next){
		pwlist_write(node, pwliter);
	}
}

int
pwlist_write_file()
{
	char vers[5];
	xmlDocPtr doc;
	xmlNodePtr root;

	if(options->readonly){
		return 0;
	}

	if(!pwlist){
		debug("write_file: bad password file");
		ui_statusline_msg("Bad password list");
		return -1;
	}
	snprintf(vers, 5, "%d", FF_VERSION);
	doc = xmlNewDoc((xmlChar*)"1.0");
	
	root = xmlNewDocNode(doc, NULL, (xmlChar*)"PWMan_PasswordList", NULL);

	xmlSetProp(root, "version", vers);
	pwlist_write(root, pwlist);

	xmlDocSetRootElement(doc, root);

	gnupg_write(doc, options->gpg_id, options->password_file);
	
	xmlFreeDoc(doc);
	return 0;
}

void
pwlist_read_node(xmlNodePtr parent, PWList *list)
{
	Pw* new;
	xmlNodePtr node;
	char *text;
	
	new = pwlist_new_pw();

	for(node = parent->children; node != NULL; node = node->next){
		if(!node || !node->name){
			debug("Messed up xml node");
		} else if( strcmp((char*)node->name, "name") == 0){
			text = (char*)xmlNodeGetContent(node);
			if(text) strncpy(new->name, text, STRING_MEDIUM);
		} else if( strcmp((char*)node->name, "user") == 0){
			text = (char*)xmlNodeGetContent(node);
			if(text) strncpy(new->user, text, STRING_MEDIUM);
		} else if( strcmp((char*)node->name, "passwd") == 0){
			text = (char*)xmlNodeGetContent(node);
			if(text) strncpy(new->passwd, text, STRING_SHORT);
		} else if( strcmp((char*)node->name, "host") == 0){
			text = (char*)xmlNodeGetContent(node);
			if(text) strncpy(new->host, text, STRING_MEDIUM);
		} else if( strcmp((char*)node->name, "launch") == 0){
			text = (char*)xmlNodeGetContent(node);
			if(text) strncpy(new->launch, text, STRING_LONG);
		} 
	}
	pwlist_add_ptr(list, new);
}

int
pwlist_read(xmlNodePtr parent, PWList *parent_list)
{
	xmlNodePtr node;
	PWList *new;

	char name[STRING_MEDIUM];
	if(!parent || !parent->name){
		ui_statusline_msg("Messed up xml node");
		return -1;
	}
	if(strcmp((char*)parent->name,"PwList") == 0){
		strncpy(name, xmlGetProp(parent, (xmlChar*)"name"), STRING_MEDIUM);
		new = pwlist_new(name);

		for(node = parent->children; node != NULL; node = node->next){
			if(!node || !node->name){
				debug("read_pwlist: messed up child node");
			} else if(strcmp(node->name, "PwList") == 0){
				pwlist_read(node, new);
			} else if(strcmp(node->name, "PwItem") == 0){
				pwlist_read_node(node, new);
			}
		}
	}

	if(parent_list == NULL){
		pwlist = new;
		current_pw_sublist = pwlist;

		pwlist->current_item = 0;
	} else {
		pwlist_add_sublist(parent_list, new);
	}
	return 0;
}

int
pwlist_read_file()
{
    char fn[STRING_LONG];
	char *buf, *cmd, *s, *text;
	int i = 0;
	int gnupg_worked = 0;
	xmlNodePtr node, root;
	xmlDocPtr doc;

	// Have the defined a file yet?
	if(!options->password_file){
		return -1;
	}
	// Do we need to create a new file?
	snprintf(fn, STRING_LONG, "%s", options->password_file);
	if(access(fn, F_OK) != 0){
		ui_statusline_msg("Database not found, created. Press any key to begin  "); getch();
		return -1;
	}

	// Try to load the file
	gnupg_worked = gnupg_read(options->password_file, &doc);	

	// Did it work?
	if(gnupg_worked != 0) {
		return gnupg_worked;
	}
	
	if(!doc){
		ui_statusline_msg("Bad xml Data");
		return -1;
	}
	root = xmlDocGetRootElement(doc);
	if(!root || !root->name	|| (strcmp((char*)root->name, "PWMan_PasswordList") != 0) ){
		ui_statusline_msg("Badly Formed password data");
		return -1;
	}
	if( buf = xmlGetProp(root, (xmlChar*)"version") ){
		i = atoi( buf );
	} else {
		i = 0;
	}
	if(i < FF_VERSION){
		ui_statusline_msg("Password File in older format, use convert_pwdb");
		return -1;
	}

	for(node = root->children; node != NULL; node = node->next){
		if(strcmp(node->name, "PwList") == 0){
			pwlist_read(node, NULL);

			break;
		}
	}
	xmlFreeDoc(doc);
	return 0;
}


int
pwlist_export_passwd(Pw *pw)
{
	char vers[5];
	char file[STRING_LONG];
	int max_id_num = 5, i=0, valid_ids = 0;
	char **ids; // max_id_num by STRING_LONG
	
	xmlDocPtr doc;
	xmlNodePtr root;

	if(!pw){
		debug("export_passwd: bad password");
		ui_statusline_msg("Bad password");
		return -1;
	}


	// We need the IDs to default to empty strings
	ids = malloc(5);
	for(i=0; i<max_id_num; i++) {
		ids[i] = malloc(STRING_LONG);
		snprintf(ids[i], STRING_LONG, "");
	}

	// Fetch the IDs
	gnupg_get_ids(ids,max_id_num);

	// Check we really got one
	for(i=0; i<max_id_num; i++) {
		if(ids[i][0] != 0) {
			valid_ids++;
		}
	}

	if(valid_ids == 0){
		debug("export_passwd: cancel because id is blank");
		return -1;
	}
	
	gnupg_get_filename(file, 'w');

	debug("export_passwd: construct xml doc");
	snprintf(vers, 5, "%d", FF_VERSION);
	doc = xmlNewDoc((xmlChar*)"1.0");
	
	root = xmlNewDocNode(doc, NULL, (xmlChar*)"PWMan_Export", NULL);

	xmlSetProp(root, "version", vers);
	
	pwlist_write_node(root, pw);

	xmlDocSetRootElement(doc, root);

	gnupg_write_many(doc, ids, max_id_num, file);
	
	xmlFreeDoc(doc);

	// All done.
	// Free our ID structures
	for(i=0; i<max_id_num; i++) {
		free(ids[i]);
	}
	free(ids);
	
	return 0;
}

int
pwlist_export(PWList *pwlist)
{
	char vers[5];
	char file[STRING_LONG];
	int max_id_num = 5, i=0, valid_ids = 0;
	char **ids; // max_id_num by STRING_LONG
	
	xmlDocPtr doc;
	xmlNodePtr root;

	// We need the IDs to default to empty strings
	ids = malloc(5);
	for(i=0; i<max_id_num; i++) {
		ids[i] = malloc(STRING_LONG);
		snprintf(ids[i], STRING_LONG, "");
	}

	// Fetch the IDs
	gnupg_get_ids(ids,max_id_num);

	// Check we really got one
	for(i=0; i<max_id_num; i++) {
		if(ids[i][0] != 0) {
			valid_ids++;
		}
	}

	if(valid_ids == 0) {
		debug("export_passwd_list: cancel because id is blank");
		return -1;
	} else {
		debug("exporting to %d ids", valid_ids);
	}

	// Get the filename to export to
	gnupg_get_filename(file, 'w');

	// Do the export
	debug("export_passwd_list: construct xml doc");
	snprintf(vers, 5, "%d", FF_VERSION);
	doc = xmlNewDoc((xmlChar*)"1.0");
	
	root = xmlNewDocNode(doc, NULL, (xmlChar*)"PWMan_Export", NULL);

	xmlSetProp(root, "version", vers);
	
	pwlist_write(root, pwlist);

	xmlDocSetRootElement(doc, root);

	gnupg_write_many(doc, ids, max_id_num, file);
	
	xmlFreeDoc(doc);

	// All done.
	// Free our ID structures
	for(i=0; i<max_id_num; i++) {
		free(ids[i]);
	}
	free(ids);
	
	return 0;
}

int
pwlist_import_passwd()
{
	char file[STRING_LONG], *buf, *cmd, *s, *text;
	int i = 0;
	xmlNodePtr node, root;
	xmlDocPtr doc;

	gnupg_get_filename(file, 'r');

	gnupg_read(file, &doc);	
	
	if(!doc){
		debug("import_passwd: bad data");
		return -1;
	}
	root = xmlDocGetRootElement(doc);
	if(!root || !root->name	|| (strcmp((char*)root->name, "PWMan_Export") != 0) ){
		ui_statusline_msg("Badly Formed password data");
		return -1;
	}
	if( buf = xmlGetProp(root, (xmlChar*)"version") ){
		i = atoi( buf );
	} else {
		i = 0;
	}
	if(i < FF_VERSION){
		ui_statusline_msg("Password Export File in older format, use convert_pwdb");
		return -1;
	}
	
	for(node = root->children; node != NULL; node = node->next){
		if(strcmp(node->name, "PwList") == 0){
			pwlist_read(node, current_pw_sublist);
			break;
		} else if(strcmp(node->name, "PwItem") == 0){
			pwlist_read_node(node, current_pw_sublist);
			break;
		}
	}
	xmlFreeDoc(doc);
	return 0;
}
