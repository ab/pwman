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
new_options()
{
	Options *new;

	new = malloc(sizeof(Options));
	new->gpg_id = malloc(SHORT_STR);
	new->gpg_path = malloc(LONG_STR);
	new->password_file = malloc(LONG_STR);
	new->passphrase_timeout = 180;

	new->filter = new_filter();

	return new;
}

char*
get_conf_file()
{
	char *conf_file;

	conf_file = malloc(LONG_STR);
	
	if(!getenv("HOME")){
		fprintf(stderr,"PWM-Error: Environment variable \"HOME\" not set\n");
		return NULL;
	} else {
		snprintf(conf_file, LONG_STR, "%s/%s", getenv("HOME"), CONF_FILE);
	}

	return conf_file;
}

int
read_config()
{
	char *file, *text;
	xmlDocPtr doc;
	xmlNodePtr node, root;
	
	file = get_conf_file();
	if(file == NULL){
		fprintf(stderr,"PWM-Error: Couldn't get config filename\n");
		return -1;
	}

	doc = xmlParseFile(file);
	if(!doc){
		fprintf(stderr,"PWM-Error: Couldn't Parse File %s\n", file);
		return -1;
	}

	root = xmlDocGetRootElement(doc);

	if(!root || !root->name || (strcmp((char*)root->name, "pwm_config") != 0) ){
		fprintf(stderr,"PWM-Error: Badly Formed File\n");
		return -1;
	}

	for(node = root->children; node != NULL; node = node->next){
		if(!node || !node->name){
			debug("read_config: Fucked up xml node");
		} else if( strcmp((char*)node->name, "gpg_id") == 0){
			text = (char*)xmlNodeGetContent(node);
			if(text) strncpy(options->gpg_id, text, SHORT_STR);
		} else if( strcmp((char*)node->name, "gpg_path") == 0){
			text = (char*)xmlNodeGetContent(node);
			if(text) strncpy(options->gpg_path, text, LONG_STR);
		} else if( strcmp((char*)node->name, "password_file") == 0){
			text = (char*)xmlNodeGetContent(node);
			if(text) strncpy(options->password_file, text, LONG_STR);
		} else if( strcmp((char*)node->name, "passphrase_timeout") == 0){
			text = (char*)xmlNodeGetContent(node);
			if(text){ options->passphrase_timeout = atoi(text); }
		} else if( strcmp((char*)node->name, "filter") == 0){
			options->filter->field = atoi( (char*)xmlGetProp(node, "field") );
			text = (char*)xmlNodeGetContent(node);
			if(text) strncpy(options->filter->filter, text, SHORT_STR);
		} else {
			debug("read_config: Unrecognised xml node");
		}
	}
	write_options = TRUE;
	xmlFreeDoc(doc);
	return 0;
}

int
write_config()
{
	char *file;
	char text[SHORT_STR];
	xmlDocPtr doc;
	xmlNodePtr node, root;

	if(!write_options){
		return;
	}
	file = get_conf_file();
	if(file == NULL){
		fprintf(stderr, "PWM-Error: Couldn't get config filename\n");
		return -1;
	}

	if(!options){
		fprintf(stderr, "PWM-Error: Options not initialized\n");
		return -1;
	}
	doc = xmlNewDoc((xmlChar*) "1.0");
	root = xmlNewDocNode(doc, NULL, (xmlChar*)"pwm_config", NULL);
	xmlNewChild(root, NULL, (xmlChar*)"gpg_id", (xmlChar*)options->gpg_id);
	xmlNewChild(root, NULL, (xmlChar*)"gpg_path", (xmlChar*)options->gpg_path);
	xmlNewChild(root, NULL, (xmlChar*)"password_file", (xmlChar*)options->password_file);

	snprintf(text, SHORT_STR, "%d", options->passphrase_timeout);
	xmlNewChild(root, NULL, (xmlChar*)"passphrase_timeout", (xmlChar*)text);

	snprintf(text, SHORT_STR, "%d", options->filter->field);
	node = xmlNewChild(root, NULL, (xmlChar*)"filter", (xmlChar*)options->filter->filter);
	xmlSetProp(node, "field", text);

	xmlDocSetRootElement(doc, root);

	if( xmlSaveFormatFile(file, doc, TRUE) != -1 ){
		xmlFreeDoc(doc);
		return 0;
	} else {
		fprintf(stderr, "PWM-Error: Couldn't save config file\n");
		xmlFreeDoc(doc);
		return -1;
	}
}
