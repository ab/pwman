/*
 *  Convert_PWDB - Convert old pwman files into the new format
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
#define STR_LEN 255

#include <libxml/parser.h>
#include <stdlib.h>
#include <stdio.h>
#include <pwman.h>

struct ConvertDB_Options {
	char *gpg;
	char *gpg_id;
	char *infile;
	char *outfile;
} convertdb_options;

int export = 0;

void
debug(char *fmt, ... )
{
#if 1
	va_list ap;
	int d;
	char *s, c;

	fputs("Convert_PWDB Debug% ", stderr);
	
	va_start(ap, fmt);
	while(*fmt){
		if(*fmt == '%'){
			switch(*++fmt){
				case 's': 	/* string */
					s = va_arg(ap, char*);
					fputs(s, stderr);
					break;
				case 'd':	/* int */
					d = va_arg(ap, int);
					fprintf(stderr, "%d", d);
					break;
				case 'c':	/* char */
					c = va_arg(ap, char);
					fputc(c, stderr);
					break;
				default:
					fputc('%', stderr);
					fputc(*fmt, stderr);
					break;
			}
		} else {
			fputc(*fmt, stderr);
		}
		*fmt++;
	}
	va_end(end);
	fputc('\n', stderr);
#endif
}

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

int
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
write_password_node(xmlNodePtr root, Pw *pw)
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
write_pwlist(xmlNodePtr parent, PWList *pwlist)
{
	xmlNodePtr node;
	Pw* iter;

	node = xmlNewChild(parent, NULL, (xmlChar*)"PwList", NULL);
	xmlSetProp(node, (xmlChar*)"name", (xmlChar*)pwlist->name);

	for(iter = pwlist->list; iter != NULL; iter = iter->next){
		write_password_node(node, iter);
	}
}

xmlDocPtr
write_new_doc(PWList *pwlist)
{
	char vers[5];
	xmlDocPtr doc;
	xmlNodePtr root;

	if(!pwlist){
		puts("write_new_file: bad password data");
		exit(-1);
	}
	snprintf(vers, 5, "%d", FF_VERSION);
	doc = xmlNewDoc((xmlChar*)"1.0");

	if(!export){
		root = xmlNewDocNode(doc, NULL, (xmlChar*)"PWMan_PasswordList", NULL);
		xmlSetProp(root, "version", vers);
		write_pwlist(root, pwlist);
	} else {
		root = xmlNewDocNode(doc, NULL, (xmlChar*)"PWMan_Export", NULL);
		xmlSetProp(root, "version", vers);
		write_password_node(root, pwlist->list);	
	}

	xmlDocSetRootElement(doc, root);

	return doc;
}

Pw *
read_pw_node(xmlNodePtr parent)
{
	Pw *new;
	xmlNodePtr node;
	char *text;

	new = new_pw();

	for(node = parent->children; node != NULL; node = node->next){
		if(!node || !node->name){
			debug("read_pw_node: fucked node");
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
			debug("read_pw_node: unrecognised node \"%s\"", node->name);
		}
	}
	return new;
}

PWList*
parse_old_doc(xmlDocPtr doc)
{
	PWList *pwlist;
	Pw *pw;
	xmlNodePtr root, node;

	pwlist = new_pwlist("Main");

	if(!doc){
		debug("parse_old_doc: Bad xmlDocPtr");
		return NULL;
	}
	root = xmlDocGetRootElement(doc);
	
	if(!root || !root->name){
		debug("parse_old_doc: Badly formed data");
		return NULL;
	}
	if( strcmp(root->name, "PWMan_Export") == 0){
		export = 1;
	} else if( strcmp(root->name, "PWMan_List") == 0){
		export = 0;
	} else {
		debug("parse_old_doc: Not a pwman file");
		return NULL;
	}
	for(node = root->children; node != NULL; node = node->next){
		if(!node || !node->name){
			debug("parse_old_doc: Bad xml Node");
		} else if( strcmp((char*)node->name, "PW_Item") == 0){
			pw = read_pw_node(node);
			add_pw_ptr(pwlist, pw);
		} else {
			debug("parse_old_doc: Unrecognised xml Node - \"%s\"", node->name);
		}
	}

	return pwlist;
}

char *
add_to_buf(char *buf, char *new)
{
	size_t size;

	if(new == NULL){
		return buf;
	} 
	if(buf == NULL){
		buf = malloc(strlen(new)+1);
		strncpy(buf, new, strlen(new)+1);
		return buf;
	}

	size = strlen(buf) + strlen(new) + 1;
	buf = (char*)realloc(buf, size);
	strncat(buf, new, size);

	return buf;
}

xmlDocPtr
get_data()
{
	FILE *fp;
	char *cmd;
	char *data;
	char buf[STR_LEN];
	xmlDocPtr doc;

	data = NULL;
	cmd = malloc(STR_LEN);
	snprintf(cmd, STR_LEN, "%s -d %s", convertdb_options.gpg, convertdb_options.infile);
	debug(cmd);
	fp = popen(cmd, "r");

	while( fgets(buf, STR_LEN, fp) != NULL ){
		data = add_to_buf(data, buf);
	}
	pclose(fp);

	doc = xmlParseMemory(data, strlen(data));

	return doc;
}

void
put_data(xmlDocPtr doc)
{
	FILE *fp;
	char *cmd;

	cmd = malloc(STR_LEN);
	snprintf(cmd, STR_LEN, "%s -e -r %s -o %s", convertdb_options.gpg, convertdb_options.gpg_id, convertdb_options.outfile);
	debug(cmd);
	fp = popen(cmd, "w");

	xmlDocFormatDump(fp, doc, TRUE);
	
	pclose(fp);
}

void
free_convertdb_options()
{
	free(convertdb_options.gpg);
	free(convertdb_options.infile);
	free(convertdb_options.outfile);
	free(convertdb_options.gpg_id);
}

char *
ask(char *msg)
{
	char * input;

	input = malloc(STR_LEN);

	fputs(msg, stdout);
	fputc('\t', stdout);
	fgets(input, STR_LEN, stdin);

	input[ strlen(input) - 1] = 0;

	return input;
}

int
main(int argc, char *argv[])
{
	xmlDocPtr doc;
	PWList *pwlist;
	
	convertdb_options.gpg = ask("Path to GnuPG Binary:");
	convertdb_options.infile = ask("File in old format:");
	convertdb_options.outfile = ask("File to write new format to:");
	convertdb_options.gpg_id = ask("ID to encrypt new file to:");
	
	doc = get_data();
	pwlist = parse_old_doc(doc);
	xmlFreeDoc(doc);
	
	doc = write_new_doc(pwlist);
	put_data(doc);
	xmlFreeDoc(doc);

	free_convertdb_options();
	return 0;
}
