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

static char*
store_in_buffer(char *buf, char *s)
{
	size_t size;

	size = strlen(buf) + strlen(s) + 1;
	buf = (char*)realloc(buf, size);

	strncat(buf, s, size);
	return buf;
}

static void
check_gnupg_id()
{
	char cmd[V_LONG_STR];
	int i;
	
	while(1){
		if( strlen(options->gpg_id) > 6 ){
			snprintf(cmd, V_LONG_STR, "%s --list-keys %s > /dev/null", options->gpg_path, options->gpg_id);
		}
		if( (execute(cmd) == 512) ){
			fprintf(stderr, "Public Key not found: \"%s\"\n", options->gpg_id);
			fprintf(stderr, "Please enter a new one:\t");
			fgets(options->gpg_id, SHORT_STR, stdin);
			options->gpg_id[ (strlen(options->gpg_id) - 1) ] = 0;
		} else {
			break;
		}
	}
}

static void
check_file()
{
	FILE *fp;

	while(1){
		fp = fopen(options->password_file, "w");
		if(fp == NULL){
			fprintf(stderr, "Cannot open password file \"%s\"\n", options->password_file);
			fprintf(stderr, "Please enter a new one:\t");
			fgets(options->password_file, LONG_STR, stdin);
			options->password_file[ strlen(options->password_file) - 1] = 0;
		} else {
			fclose(fp);
			break;
		}
	}
}

static void
check_gnupg()
{
	while(1){
		if(access(options->gpg_path, F_OK) != 0){
			fprintf(stderr, "GnuPG not found at \"%s\"\n", options->gpg_path);
			fprintf(stderr, "Please enter new path:\t");
			fgets(options->gpg_path, LONG_STR, stdin);
			options->gpg_path[ strlen(options->gpg_path) - 1] = 0;
		} else {
			break;
		}
	}
}

int
write_file()
{
	Pw *iter;
	char *cmd;
	xmlDocPtr doc;
	xmlNodePtr node, root;
	FILE *fp;

	check_gnupg_id();
	check_gnupg();
	check_file();

	if(!options->password_file){
		fprintf(stderr,"PWM-error: password file not set\n");
		return -1;
	}
	if(!pwlist){
		fprintf(stderr,"PWM-error: bad password list\n");
		return -1;
	}
	doc = xmlNewDoc((xmlChar*)"1.0");
	
	root = xmlNewDocNode(doc, NULL, (xmlChar*)"PWMan_List", NULL);
	for(iter = pwlist; iter != NULL; iter = iter->next){
		node = xmlNewChild(root, NULL, (xmlChar*)"PW_Item", NULL);
		xmlNewChild(node, NULL, (xmlChar*)"name", (xmlChar*)iter->name);
		xmlNewChild(node, NULL, (xmlChar*)"host", (xmlChar*)iter->host);
		xmlNewChild(node, NULL, (xmlChar*)"user", (xmlChar*)iter->user);
		xmlNewChild(node, NULL, (xmlChar*)"passwd", (xmlChar*)iter->passwd);
		xmlNewChild(node, NULL, (xmlChar*)"launch", (xmlChar*)iter->launch);
	}

	xmlDocSetRootElement(doc, root);

	cmd = malloc(V_LONG_STR);
	snprintf(cmd, V_LONG_STR, "%s -e -a -r %s > %s", options->gpg_path, options->gpg_id, options->password_file);
	fp = popen(cmd, "w");
	free(cmd);

	if(!fp){
		fprintf(stderr,"PWM-Error: Couldn't execute encryption software\n");
		xmlFreeDoc(doc);
		pclose(fp);
		return 0;
	}
	/*xmlSaveFile(options->password_file, doc);*/
	
	xmlDocDump(fp, doc);
	xmlFreeDoc(doc);
	pclose(fp);
	return 0;
}

static void
read_pw_node(xmlNodePtr parent)
{
	Pw* new;
	xmlNodePtr node;
	char *text;
	
	new = new_pw();

	for(node = parent->children; node != NULL; node = node->next){
		if(!node || !node->name){
			fprintf(stderr,"PWM-Warning: Fucked up xml node\n");
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
		} else {
			fprintf(stderr,"PWM-Warning: Unrecognised Node, %s\n", node->name);
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

	check_gnupg();

	buf = malloc(1); buf[0] = 0;
	s = malloc(V_LONG_STR);
	cmd = malloc(V_LONG_STR);
	snprintf(cmd, V_LONG_STR, "%s -d %s", options->gpg_path, options->password_file);
	fp = popen(cmd, "r");
	while( fgets(s, V_LONG_STR, fp) ){
		buf = store_in_buffer(buf, s);
	}
	pclose(fp);

	free(s);
	free(cmd);
	cmd = s = NULL;

	doc = xmlParseMemory(buf, strlen(buf));
	/*doc = xmlParseFile(options->password_file);*/
	if(!doc){
		fprintf(stderr,"PWM-Error: Bad password data\n");
		return -1;
	}
	root = xmlDocGetRootElement(doc);
	if(!root || !root->name	|| (strcmp((char*)root->name, "PWMan_List") != 0) ){
		fprintf(stderr,"PWM-Error: Badly Formed password data\n");
		return -1;
	}
	for(node = root->children; node != NULL; node = node->next){
		if(!node || !node->name){
			fprintf(stderr,"PWM-Warning: Fucked up xml node\n");
		} else if( strcmp((char*)node->name, "PW_Item") == 0){
			read_pw_node(node);
		} else {
			fprintf(stderr,"PWM-Warning: Unrecognised xml node\n");
		}
	}
	xmlFreeDoc(doc);
}
