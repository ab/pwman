/*
 *  PWMan - password application manager
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
#include <ui.h>
#include <libxml/tree.h>
#include <libxml/parser.h>

int
export_passwd(Pw *pw)
{
	char vers[5];
	char id[MED_STR], file[LONG_STR];
	
	xmlDocPtr doc;
	xmlNodePtr root;

	if(check_gnupg() != 0){
		debug("export_passwd: gnupg not found");
		return -1;
	}
	if(!pw){
		debug("export_passwd: bad password");
		statusline_msg("Bad password");
		return -1;
	}

	get_gnupg_id(id);
	if(id[0] == 0){
		debug("export_passwd: cancel because id is blank");
		return -1;
	}
	
	get_filename(file, 'w');

	debug("export_passwd: construct xml doc");
	snprintf(vers, 5, "%d", FF_VERSION);
	doc = xmlNewDoc((xmlChar*)"1.0");
	
	root = xmlNewDocNode(doc, NULL, (xmlChar*)"PWMan_Export", NULL);

	xmlSetProp(root, "version", vers);
	
	write_password_node(root, pw);

	xmlDocSetRootElement(doc, root);

	gnupg_write(doc, id, file);
	
	xmlFreeDoc(doc);
	return 0;
}

int
export_passwd_list(PWList *pwlist)
{
	char vers[5];
	char id[MED_STR], file[LONG_STR];
	
	xmlDocPtr doc;
	xmlNodePtr root;

	if(check_gnupg() != 0){
		debug("export_passwd_list: gnupg not found");
		return -1;
	}
	if(!pwlist){
		debug("export_passwd_list: bad password");
		statusline_msg("Bad password");
		return -1;
	}

	get_gnupg_id(id);
	if(id[0] == 0){
		debug("export_passwd_list: cancel because id is blank");
		return -1;
	}
	
	get_filename(file, 'w');

	debug("export_passwd_list: construct xml doc");
	snprintf(vers, 5, "%d", FF_VERSION);
	doc = xmlNewDoc((xmlChar*)"1.0");
	
	root = xmlNewDocNode(doc, NULL, (xmlChar*)"PWMan_Export", NULL);

	xmlSetProp(root, "version", vers);
	
	write_pwlist(root, pwlist);

	xmlDocSetRootElement(doc, root);

	gnupg_write(doc, id, file);
	
	xmlFreeDoc(doc);
	return 0;
}

int
import_passwd()
{
	char file[LONG_STR], *buf, *cmd, *s, *text;
	int i = 0;
	xmlNodePtr node, root;
	xmlDocPtr doc;

	get_filename(file, 'r');

	gnupg_read(file, &doc);	
	
	if(!doc){
		debug("import_passwd: bad data");
		return -1;
	}
	root = xmlDocGetRootElement(doc);
	if(!root || !root->name	|| (strcmp((char*)root->name, "PWMan_Export") != 0) ){
		statusline_msg("Badly Formed password data");
		return -1;
	}
	i = atoi( xmlGetProp(root, (xmlChar*)"version") );
	if(i < FF_VERSION){
		statusline_msg("Password Export File in older format, use convert_pwdb");
		return -1;
	}
	
	for(node = root->children; node != NULL; node = node->next){
		if(strcmp(node->name, "PwList") == 0){
			read_pwlist(node, current_pw_sublist);
			break;
		} else if(strcmp(node->name, "PwItem") == 0){
			read_password_node(node, current_pw_sublist);
			break;
		}
	}
	xmlFreeDoc(doc);
	return 0;
}
