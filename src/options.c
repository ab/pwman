/*
 *  PWman - password management application
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
#include <libxml/tree.h>
#include <libxml/parser.h>

Options*
options_new()
{
	Options *new;

	new = malloc(sizeof(Options));
	new->gpg_id = malloc(STRING_LONG);
	new->gpg_path = malloc(STRING_LONG);
	new->password_file = malloc(STRING_LONG);
	new->passphrase_timeout = 180;
	new->readonly = FALSE;

	new->filter = filter_new();

	return new;
}

char*
options_get_file()
{
	char *conf_file;

	conf_file = malloc(STRING_LONG);
	
	if(!getenv("HOME")){
		return NULL;
	} else {
		snprintf(conf_file, STRING_LONG, "%s/%s", getenv("HOME"), CONF_FILE);
	}

	return conf_file;
}

int
options_read()
{
	char *file, *text;
	xmlDocPtr doc;
	xmlNodePtr node, root;
	
	file = options_get_file();
	if(file == NULL){
		return -1;
	}

	doc = xmlParseFile(file);
	if(!doc){
		return -1;
	}

	root = xmlDocGetRootElement(doc);

	if(!root || !root->name || (strcmp((char*)root->name, "pwm_config") != 0) ){
		fprintf(stderr,"PWM-Warning: Badly Formed .pwmanrc\n");
		return -1;
	}

	for(node = root->children; node != NULL; node = node->next){
		if(!node || !node->name){
			debug("read_config: Fucked up xml node");
		} else if( strcmp((char*)node->name, "gpg_id") == 0){
			text = (char*)xmlNodeGetContent(node);
			if(text) strncpy(options->gpg_id, text, STRING_LONG);
		} else if( strcmp((char*)node->name, "gpg_path") == 0){
			text = (char*)xmlNodeGetContent(node);
			if(text) strncpy(options->gpg_path, text, STRING_LONG);
		} else if( strcmp((char*)node->name, "password_file") == 0){
			text = (char*)xmlNodeGetContent(node);
			if(text) strncpy(options->password_file, text, STRING_LONG);
		} else if( strcmp((char*)node->name, "passphrase_timeout") == 0){
			text = (char*)xmlNodeGetContent(node);
			if(text){ options->passphrase_timeout = atoi(text); }
		} else if( strcmp((char*)node->name, "filter") == 0){
			options->filter->field = atoi( (char*)xmlGetProp(node, "field") );
			text = (char*)xmlNodeGetContent(node);
			if(text) strncpy(options->filter->filter, text, STRING_LONG);
		} else if( strcmp((char*)node->name, "readonly") == 0){
			options->readonly = TRUE;
		} else {
			debug("read_config: Unrecognised xml node");
		}
	}
	write_options = TRUE;
	xmlFreeDoc(doc);
	return 0;
}

int
options_write()
{
	char *file;
	char text[STRING_SHORT];
	xmlDocPtr doc;
	xmlNodePtr node, root;

	if(!write_options){
		return 0;
	}
	file = options_get_file();
	if(file == NULL){
		return -1;
	}

	if(!options){
		return -1;
	}
	doc = xmlNewDoc((xmlChar*) "1.0");
	root = xmlNewDocNode(doc, NULL, (xmlChar*)"pwm_config", NULL);
	xmlNewChild(root, NULL, (xmlChar*)"gpg_id", (xmlChar*)options->gpg_id);
	xmlNewChild(root, NULL, (xmlChar*)"gpg_path", (xmlChar*)options->gpg_path);
	xmlNewChild(root, NULL, (xmlChar*)"password_file", (xmlChar*)options->password_file);

	snprintf(text, STRING_SHORT, "%d", options->passphrase_timeout);
	xmlNewChild(root, NULL, (xmlChar*)"passphrase_timeout", (xmlChar*)text);

	snprintf(text, STRING_SHORT, "%d", options->filter->field);
	node = xmlNewChild(root, NULL, (xmlChar*)"filter", (xmlChar*)options->filter->filter);
	xmlSetProp(node, "field", text);

	xmlDocSetRootElement(doc, root);

	if( xmlSaveFormatFile(file, doc, TRUE) != -1 ){
		xmlFreeDoc(doc);
		return 0;
	} else {
		debug("write_options: couldn't write config file");
		xmlFreeDoc(doc);
		return -1;
	}
}

void
options_get()
{
	char pw_file[STRING_LONG];
	char text[STRING_SHORT];
	
	puts("Hmm... can't open ~/.pwmanrc, we'll create one manually now.");
	
	printf("GnuPG ID [you@yourdomain.com] or [012345AB]: ");
	fgets(options->gpg_id, STRING_LONG, stdin);
	if( strcmp(options->gpg_id, "\n") == 0 ){
		strncpy(options->gpg_id, "you@yourdomain.com", STRING_SHORT);
	} else {
		options->gpg_id[ strlen(options->gpg_id) - 1] = 0;
	}
	
	printf("Path to GnuPG [/usr/bin/gpg]: ");
	fgets(options->gpg_path, STRING_LONG, stdin);
	if( strcmp(options->gpg_path, "\n") == 0){
		strncpy(options->gpg_path, "/usr/bin/gpg", STRING_LONG);
	} else {
		options->gpg_path[ strlen(options->gpg_path) - 1] = 0;
	}
	
	snprintf(pw_file, STRING_LONG, "%s/.pwman.db", getenv("HOME") );
	printf("Password Database File [%s]: ", pw_file );
	fgets(options->password_file, STRING_LONG, stdin);
	if( strcmp(options->password_file, "\n") == 0){
		strncpy(options->password_file, pw_file, STRING_LONG);
	} else {
		options->password_file[ strlen(options->password_file) - 1] = 0;
	}
	
	printf("Passphrase Timeout(in minutes) [180]: ");
	fgets(text, STRING_SHORT, stdin);
	if( strcmp(text, "\n") == 0){
		options->passphrase_timeout = 180;
	} else {
		options->passphrase_timeout = atoi(text);
	}

	write_options = TRUE;
	options_write();
}

