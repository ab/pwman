/*
 *  PWDB2CSV - Convert pwman database files into Comma Separated Values
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
#define PWDB2CSV_PACKAGE "PWDB2CSV"
#define PWDB2CSV_VERSION "0.1.0"

#include <libxml/parser.h>
#include <stdlib.h>
#include <stdio.h>
#include <pwman.h>

static void show_version();
static void show_usage(char*);

struct PWDB2CSV_Options {
	char *gpg_id;
	char *infile;
	char *outfile;
} pwdb2csv_options;

void
debug(char *fmt, ... )
{
#ifdef DEBUG
	va_list ap;
	int d, c;
	char *s;

	fputs("PWDB2CSV Debug% ", stderr);
	
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
					c = va_arg(ap, int);
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
	va_end(ap);
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

	debug("free_pwlist: free a password list");
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
	debug("free_pw: free a password");
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
add_pw_sublist(PWList *parent, PWList *new)
{
	PWList *current;

	current = parent->sublists;
	new->parent = parent;
	if(current == NULL){
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

char *
escape_string(char *str)
{
	char *str2, *iter, *iter2, *c;
	int i = 0;
	
	str2 = malloc( strlen(str) + 100 ); /* any string with 100 quotes is a pisstake , it's just here for safety */
	
	for(iter = str, iter2 = str2; iter != 0, i < 100; iter++, iter2++, i++){
		if(*iter != '"'){
			*iter2 = *iter;
		} else {
			*iter2 = '\\'; iter2++;
			*iter2 = *iter;
		}
	}
	
	free(str);
	str = str2;

	return str;
}

int 
write_password_node(FILE *fp, Pw *pw)
{
	fprintf(fp, "\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"\n", escape_string(pw->name), escape_string(pw->host), 
			escape_string(pw->user), escape_string(pw->passwd), escape_string(pw->launch));
}

int
write_pwlist(FILE *fp, PWList *pwlist)
{
	Pw* iter;
	PWList *pwliter;
	
	for(pwliter = pwlist->sublists; pwliter != NULL; pwliter = pwliter->next){
		write_pwlist(fp, pwliter);
	}
	for(iter = pwlist->list; iter != NULL; iter = iter->next){
		write_password_node(fp, iter);
	}

	return 0;
}

void
read_password_node(xmlNodePtr parent, PWList *list)
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
	add_pw_ptr(list, new);
}

int
read_pwlist(xmlNodePtr parent, PWList *parent_list)
{
	xmlNodePtr node;
	PWList *new;

	char name[NAME_LEN];
	if(!parent || !parent->name){
		return -1;
	} 
	if(strcmp((char*)parent->name, "PwList") == 0){
		strncpy(name, xmlGetProp(parent, (xmlChar*)"name"), NAME_LEN);
		new = new_pwlist(name);

		for(node = parent->children; node != NULL; node = node->next){
			if(!node || !node->next){
				fprintf(stderr, "read_pwlist: messed up node\n");
			} else if(strcmp(node->name, "PwList") == 0){
				read_pwlist(node, new);
			} else if(strcmp(node->name, "PwItem") == 0){
				read_password_node(node, new);
			}
		}
	}

	if(parent_list == NULL){
		parent_list = new;
	} else {
		add_pw_sublist(parent_list, new);
	}
	return 0;
}

PWList*
parse_doc(xmlDocPtr doc)
{
	PWList *pwlist = NULL;
	xmlNodePtr root, node;
	char *buf;
	int i;
	
	if(!doc){
		return NULL;
	}

	root = xmlDocGetRootElement(doc);
	if(!root || !root->name || (strcmp((char*)root->name, "PWMan_PasswordList") != 0) ){
		return NULL;
	}
	if(buf = xmlGetProp(root, (xmlChar*)"version")){
		i = atoi( buf );
	} else {
		i = 0;
	}
	if(i < FF_VERSION){
		return NULL;
	}

	pwlist = new_pwlist("Main");
	for(node = root->children; node != NULL; node = node->next){
		if(strcmp(node->name, "PwList") == 0){
			read_pwlist(node, pwlist);

			break;
		}
	}
	xmlFreeDoc(doc);

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
	snprintf(cmd, STR_LEN, "gpg -d %s", pwdb2csv_options.infile);
	debug(cmd);
	fp = popen(cmd, "r");

	while( fgets(buf, STR_LEN, fp) != NULL ){
		data = add_to_buf(data, buf);
	}
	pclose(fp);

	if(!data){
		exit(-1);
	}

	doc = xmlParseMemory(data, strlen(data));

	return doc;
}

void
put_data(PWList *pwlist)
{
	FILE *fp;

	fp = fopen(pwdb2csv_options.outfile, "w");

	if(!fp){
		fprintf(stderr, "write_pwlist: couldn't open file \"%s\"\n", pwdb2csv_options.outfile);
		return;
	}

	write_pwlist(fp, pwlist);

	fclose(fp);
}

void
free_pwdb2csv_options()
{
	debug("free_pwdb2csv_options: free options");
	free(pwdb2csv_options.infile);
	free(pwdb2csv_options.outfile);
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

void
get_options(int argc, char *argv[])
{
	int i;

	for(i = 0; i < argc; i++){
		if( !strcmp(argv[i], "--help") || !strcmp(argv[i], "-h") ){
			show_usage(argv[0]);
			exit(1);
		} else if( !strcmp(argv[i], "--version") || !strcmp(argv[i], "-v") ){
			show_version();
			exit(1);
		}
	}
	
	if(argc > 1){
		pwdb2csv_options.infile = malloc(STR_LEN);
		strncpy(pwdb2csv_options.infile, argv[1], STR_LEN);
	} else {
		pwdb2csv_options.infile = ask("Password Database:");
	}
	if(argc > 2){
		pwdb2csv_options.outfile = malloc(STR_LEN);
		strncpy(pwdb2csv_options.outfile, argv[2], STR_LEN);
	} else {
		pwdb2csv_options.outfile = ask("CSV file:");
	}
}

int
main(int argc, char *argv[])
{
	xmlDocPtr doc;
	PWList *pwlist;
	
	get_options(argc, argv);

	doc = get_data();
	pwlist = parse_doc(doc);
	
	put_data(pwlist);

	free_pwdb2csv_options();
	return 0;
}

static void
show_version()
{
	puts(PWDB2CSV_PACKAGE " v " PWDB2CSV_VERSION);
	puts("Written by Ivan Kelly <ivan@ivankelly.net>\n");
	puts("Copyright (C) 2002 Ivan Kelly");
	puts("This program is free software; you can redistribute it and/or modify");
	puts("it under the terms of the GNU General Public License as published by");
	puts("the Free Software Foundation; either version 2 of the License, or");
	puts("(at your option) any later version.\n");

	puts("This program is distributed in the hope that it will be useful,");
	puts("but WITHOUT ANY WARRANTY; without even the implied warranty of");
	puts("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the");
	puts("GNU General Public License for more details.\n");

	puts("You should have received a copy of the GNU General Public License");
	puts("along with this program; if not, write to the Free Software");
	puts("Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.\n");
}

static void
show_usage(char *argv_0)
{
	printf("Usage: %s [<pwdatabase> [<csvfile>]]\n", argv_0);
	puts("Convert Password Database from PWMan Encrypted Format to Comma Separated Values\n");
	puts("  --help                 show usage");
	puts("  --version              display version information");
	puts("  <pwdatabase>           password database file in encrypted");
	puts("  <csvfile>              file to write to with comma separated values\n\n");
	puts("Report bugs to <ivan@ivankelly.net>");
}
