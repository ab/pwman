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
void free_pw(Pw*);


PWList *
new_pwlist(char *name)
{
	PWList *new;
	new = malloc( sizeof(PWList) );
	new->name = malloc(NAME_LEN);
	strncpy(new->name, name, NAME_LEN);
	new->parent = NULL;
	new->list = NULL;
	new->sublists = NULL;
	debug("new_pwlist: %s", name);

	return new;
}

int
init_database()
{
	pwindex = 0;
	pwlist = NULL;
	current_pw_sublist = NULL;
}

int 
free_pwlist(PWList *old)
{
	Pw *current, *next;
	PWList *curlist, *nlist;

	if(old == NULL){
		return 0;
	}
	for(current = old->list; current != NULL; current = next){
		next = current->next;

		free_pw(current);
	}
	for(curlist = old->sublists; curlist != NULL; curlist = nlist){
		nlist = curlist->next;

		free_pwlist(curlist);
	}
	
	free(old->name);
	free(old);
	old = NULL;
	return 0;
}

int
free_database()
{
	free_pwlist(pwlist);
	return 0;
}

Pw*
new_pw()
{
	Pw *new;
	new = malloc(sizeof(Pw));
	new->id = 0;
	new->name = malloc(NAME_LEN);
	new->host = malloc(HOST_LEN);
	new->user = malloc(USER_LEN);
	new->passwd = malloc(PASS_LEN);
	new->launch = malloc(LAUNCH_LEN);

	memset(new->name, 0, NAME_LEN);
	memset(new->host, 0, HOST_LEN);
	memset(new->user, 0, USER_LEN);
	memset(new->passwd, 0, PASS_LEN);
	memset(new->launch, 0, LAUNCH_LEN);
	
	return new;
}

void
free_pw(Pw *old)
{
	free(old->name);
	free(old->user);
	free(old->host);
	free(old->passwd);
	free(old->launch);
	free(old);
}

int
add_pw(PWList *parent, char* name, char* host, char* user, char* passwd, char* launch)
{
	Pw* new = new_pw();
	
	new->id = pwindex++;
	strncpy(new->name, name, NAME_LEN);
	strncpy(new->host, host, HOST_LEN);
	strncpy(new->user, user, USER_LEN);
	strncpy(new->passwd, passwd, PASS_LEN);
	strncpy(new->launch, launch, LAUNCH_LEN);

	add_pw_ptr(parent, new);

	return 0;
}

int 
add_pw_sublist(PWList *parent, PWList *new)
{
	PWList *current;

	current = parent->sublists;
	new->parent = parent;
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
add_pw_ptr(PWList *list, Pw *new)
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
detach_pw(PWList *list, Pw *pw)
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
delete_pw(PWList *list, Pw *pw)
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
			free_pw(iter);
			break;
		}
		prev = iter;
	}
}

int 
detach_pw_sublist(PWList *parent, PWList *old)
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
delete_pw_sublist(PWList *parent, PWList *old)
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
			free_pwlist(iter);
			break;
		}
		prev = iter;
	}
}

int 
write_password_node(xmlNodePtr root, Pw* pw)
{
	xmlNodePtr node;

	node = xmlNewChild(root, NULL, (xmlChar*)"PwItem", NULL);
	xmlNewChild(node, NULL, (xmlChar*)"name", (xmlChar*)pw->name);
	xmlNewChild(node, NULL, (xmlChar*)"host", (xmlChar*)pw->host);
	xmlNewChild(node, NULL, (xmlChar*)"user", (xmlChar*)pw->user);
	xmlNewChild(node, NULL, (xmlChar*)"passwd", (xmlChar*)pw->passwd);
	xmlNewChild(node, NULL, (xmlChar*)"launch", (xmlChar*)pw->launch);
}

int
write_pwlist(xmlNodePtr parent, PWList *list)
{
	xmlNodePtr node;
	Pw *iter;
	PWList *pwliter;
	
	node = xmlNewChild(parent, NULL, (xmlChar*)"PwList", NULL);
	xmlSetProp(node, (xmlChar*)"name", (xmlChar*)list->name);	
	
	for(iter = list->list; iter != NULL; iter = iter->next){
		write_password_node(node, iter);
	}
	for(pwliter = list->sublists; pwliter != NULL; pwliter = pwliter->next){
		write_pwlist(node, pwliter);
	}
}

int
write_file()
{
	char vers[5];
	xmlDocPtr doc;
	xmlNodePtr root;

	if(!pwlist){
		debug("write_file: bad password file");
		statusline_msg("Bad password list");
		return -1;
	}
	snprintf(vers, 5, "%d", FF_VERSION);
	doc = xmlNewDoc((xmlChar*)"1.0");
	
	root = xmlNewDocNode(doc, NULL, (xmlChar*)"PWMan_PasswordList", NULL);

	xmlSetProp(root, "version", vers);
	write_pwlist(root, pwlist);

	xmlDocSetRootElement(doc, root);

	gnupg_write(doc, options->gpg_id, options->password_file);
	
	xmlFreeDoc(doc);
	return 0;
}

void
read_password_node(xmlNodePtr parent, PWList *list)
{
	Pw* new;
	xmlNodePtr node;
	char *text;
	
	new = new_pw();

	for(node = parent->children; node != NULL; node = node->next){
		if(!node || !node->name){
			debug("Messed up xml node");
		} else if( strcmp((char*)node->name, "name") == 0){
			text = (char*)xmlNodeGetContent(node);
			if(text) strncpy(new->name, text, NAME_LEN);
		} else if( strcmp((char*)node->name, "user") == 0){
			text = (char*)xmlNodeGetContent(node);
			if(text) strncpy(new->user, text, USER_LEN);
		} else if( strcmp((char*)node->name, "passwd") == 0){
			text = (char*)xmlNodeGetContent(node);
			if(text) strncpy(new->passwd, text, PASS_LEN);
		} else if( strcmp((char*)node->name, "host") == 0){
			text = (char*)xmlNodeGetContent(node);
			if(text) strncpy(new->host, text, HOST_LEN);
		} else if( strcmp((char*)node->name, "launch") == 0){
			text = (char*)xmlNodeGetContent(node);
			if(text) strncpy(new->launch, text, LAUNCH_LEN);
		} 
	}
	add_pw_ptr(list, new);
}

int
read_pwlist(xmlNodePtr parent, PWList *parent_list)
{
	xmlNodePtr node;
	PWList *new;

	char name[NAME_LEN];
	if(!parent || !parent->name){
		statusline_msg("Messed up xml node");
		return -1;
	}
	if(strcmp((char*)parent->name,"PwList") == 0){
		strncpy(name, xmlGetProp(parent, (xmlChar*)"name"), NAME_LEN);
		new = new_pwlist(name);

		for(node = parent->children; node != NULL; node = node->next){
			if(!node || !node->name){
				debug("read_pwlist: messed up child node");
			} else if(strcmp(node->name, "PwList") == 0){
				read_pwlist(node, new);
			} else if(strcmp(node->name, "PwItem") == 0){
				read_password_node(node, new);
			}
		}
	}

	if(parent_list == NULL){
		pwlist = new;
		current_pw_sublist = pwlist;
	} else {
		add_pw_sublist(parent_list, new);
	}
	return 0;
}

int
read_file()
{
	char *buf, *cmd, *s, *text;
	int i = 0;
	xmlNodePtr node, root;
	xmlDocPtr doc;

	if(!options->password_file){
		return -1;
	}
	gnupg_read(options->password_file, &doc);	
	
	if(!doc){
		statusline_msg("Bad xml Data");
		return -1;
	}
	root = xmlDocGetRootElement(doc);
	if(!root || !root->name	|| (strcmp((char*)root->name, "PWMan_PasswordList") != 0) ){
		statusline_msg("Badly Formed password data");
		return -1;
	}
	if( buf = xmlGetProp(root, (xmlChar*)"version") ){
		i = atoi( buf );
	} else {
		i = 0;
	}
	if(i < FF_VERSION){
		statusline_msg("Password File in older format, use convert_pwdb");
		return -1;
	}

	for(node = root->children; node != NULL; node = node->next){
		if(strcmp(node->name, "PwList") == 0){
			read_pwlist(node, NULL);

			break;
		}
	}
	xmlFreeDoc(doc);
	return 0;
}
