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
xmlDocPtr gnupg_read(char*);

int
init_database()
{
	pwindex = 0;
	pwlist = NULL;
}

int
free_database()
{
	Pw *current, *next;

	current = pwlist;
	while(current != NULL){
		next = current->next;

		free_pw(current);

		current = next;
	}
	pwlist = NULL;
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
add_pw(char* name, char* host, char* user, char* passwd, char* launch)
{
	Pw* new = new_pw();
	Pw* current;
	new->id = pwindex++;
	strncpy(new->name, name, NAME_LEN);
	strncpy(new->host, host, HOST_LEN);
	strncpy(new->user, user, USER_LEN);
	strncpy(new->passwd, passwd, PASS_LEN);
	strncpy(new->launch, launch, LAUNCH_LEN);

	current = pwlist;
	if(!pwlist){
		pwlist = new;
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
add_pw_ptr(Pw *new)
{
	Pw *current;
	
	if(pwlist == NULL){
		pwlist = new;
		new->next = NULL;
		return 0;
	}

	current = pwlist;
	while(current->next != NULL){
		current = current->next;
	}
	current->next = new;
	new->next = NULL;

	return 0;
}

int 
delete_pw(Pw *pw)
{
	Pw *iter, *prev;

	prev = NULL;
	for(iter = pwlist; iter != NULL; iter = iter->next){
		
		if(iter == pw){
			if(prev == NULL){
				pwlist = iter->next;
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
write_password_node(xmlNodePtr root, Pw* pw)
{
	xmlNodePtr node;

	node = xmlNewChild(root, NULL, (xmlChar*)"PW_Item", NULL);
	xmlNewChild(node, NULL, (xmlChar*)"name", (xmlChar*)pw->name);
	xmlNewChild(node, NULL, (xmlChar*)"host", (xmlChar*)pw->host);
	xmlNewChild(node, NULL, (xmlChar*)"user", (xmlChar*)pw->user);
	xmlNewChild(node, NULL, (xmlChar*)"passwd", (xmlChar*)pw->passwd);
	xmlNewChild(node, NULL, (xmlChar*)"launch", (xmlChar*)pw->launch);
}

int
write_file()
{
	Pw *iter;
	char *cmd;
	xmlDocPtr doc;
	xmlNodePtr node, root;
	FILE *fp;

	if(!options->password_file){
		statusline_msg("Password file not set\n");
		getch();
		return -1;
	}
	if(!pwlist){
		statusline_msg("Bad password list\n");
		getch();
		return -1;
	}
	doc = xmlNewDoc((xmlChar*)"1.0");
	
	root = xmlNewDocNode(doc, NULL, (xmlChar*)"PWMan_List", NULL);
	for(iter = pwlist; iter != NULL; iter = iter->next){
		write_password_node(root, iter);
	}

	xmlDocSetRootElement(doc, root);

	gnupg_write(doc, options->gpg_id, options->password_file);
	
	xmlFreeDoc(doc);
	return 0;
}

void
read_password_node(xmlNodePtr parent)
{
	Pw* new;
	xmlNodePtr node;
	char *text;
	
	new = new_pw();

	for(node = parent->children; node != NULL; node = node->next){
		if(!node || !node->name){
			statusline_msg("Messed up xml node\n");
			getch();
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
	add_pw_ptr(new);
}

int
read_file()
{
	char *buf, *cmd, *s, *text;
	FILE *fp;
	xmlNodePtr node, root;
	xmlDocPtr doc;
	doc = gnupg_read(options->password_file);
	/*doc = xmlParseFile(options->password_file);*/
	if(!doc){
		statusline_msg("Bad password data in file\n");
		getch();
		return -1;
	}
	root = xmlDocGetRootElement(doc);
	if(!root || !root->name	|| (strcmp((char*)root->name, "PWMan_List") != 0) ){
		statusline_msg("Badly Formed password data\n");
		getch();
		return -1;
	}
	for(node = root->children; node != NULL; node = node->next){
		if(!node || !node->name){
			statusline_msg("Messed up xml node\n");
		} else if( strcmp((char*)node->name, "PW_Item") == 0){
			read_password_node(node);
		}
	}
	xmlFreeDoc(doc);
}
