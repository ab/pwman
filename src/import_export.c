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
#include <libxml/tree.h>
#include <libxml/parser.h>

int
export_passwd(Pw *pw)
{
	char *filename, *text, *cmd;
	char id[MED_STR];
	FILE *fp;
	xmlDocPtr doc;
	xmlNodePtr root;
	
	if(pw == NULL){
		return -1;
	}
	
	filename = malloc(V_LONG_STR);
	cmd = malloc(V_LONG_STR);
	text = malloc(LONG_STR);
	
	statusline_ask_str("Export password to file:", filename, LONG_STR);
	statusline_ask_str("GnuPG ID to export to:", id, MED_STR);

	/*
	 * do writing bit
	 */
	doc = xmlNewDoc((xmlChar*)"1.0");

	root = xmlNewDocNode(doc, NULL, (xmlChar*)"PWMan_Export", NULL);

	xmlDocSetRootElement(doc, root);

	write_password_node(root, pw);

	gnupg_write(doc, id, filename);

	xmlFreeDoc(doc);
	
	free(filename);
	free(cmd);
	free(text);
}

int
import_passwd()
{
	char *filename;
	xmlDocPtr doc;
	xmlNodePtr node, root;

	filename = malloc(LONG_STR);
	statusline_ask_str("Enter file to import password from:", filename, LONG_STR);
	
	gnupg_read(filename, &doc);
	/*doc = xmlParseFile(options->password_file);*/
	if(!doc){
		statusline_msg("Bad password data or incorrect passphrase\n");
		return -1;
	}
	root = xmlDocGetRootElement(doc);
	if(!root || !root->name	|| (strcmp((char*)root->name, "PWMan_Export") != 0) ){
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
