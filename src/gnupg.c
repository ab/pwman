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
#include <gpgme.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <unistd.h>
#include <time.h>

GpgmeCtx ctx;
int passphrase_good = 0;

const char *
get_passphrase(void *HOOK, const char *DESC, void **R_HD)
{
	static char passphrase[V_LONG_STR];

/*	passphrase == malloc(V_LONG_STR);*/
	if((time_base >= (time(NULL) - (options->passphrase_timeout*60))) && (passphrase_good == 1)){
		return passphrase;
	}
	if(DESC){
		passphrase_good = 0;
		statusline_ask_passwd("Please enter your passphrase:", passphrase, V_LONG_STR);
	}
	return passphrase;
}

void
forget_passphrase()
{
	passphrase_good = 0;
	statusline_msg("Passphrase forgotten");
}

int
init_gpgme()
{
	gpgme_new(&ctx);
	gpgme_set_armor(ctx, 1);
	gpgme_set_passphrase_cb(ctx, get_passphrase, NULL);
}

int
quit_gpgme()
{
	gpgme_release(ctx);
}

static void
check_gnupg_id(char *id)
{
	int i;
	char *text;
	GpgmeKey key;

	text = malloc(LONG_STR);
	
	do {
		gpgme_op_keylist_start(ctx, id, 0);
		
		i = gpgme_op_keylist_next(ctx, &key);
		if(i == GPGME_EOF){
			snprintf(text, LONG_STR, "Cannot find public key for %s", id);
			statusline_msg(text);
			getch();
			statusline_ask_str("Please enter another GnuPG ID:", id, SHORT_STR);
		}
		gpgme_op_keylist_end(ctx);
	} while (i == GPGME_EOF);

	free(text);
}

void
check_file(char *filename)
{	
	FILE *fp;
	char dir[LONG_STR];
	
	while(1) {
		if((filename[0] == '~') && getenv("HOME") ){
			snprintf(filename, LONG_STR, "%s%s", getenv("HOME"), filename+1);					
		}
		fp = fopen(filename, "a");
		if(!fp){
			statusline_msg("Cannot write to specified file, please enter a new filename");
			getch();
			statusline_ask_str("File to write to:", filename, LONG_STR);
		} else {
			fclose(fp);
			break;
		}
	} 

}

int
gnupg_write(xmlDocPtr doc, char* id, char* filename)
{
	char *dump, *buf;
	FILE *fp;
	int i;
	GpgmeData plain, cipher;
	GpgmeRecipients recp;
	GpgmeError error;

	check_gnupg_id(id);
	check_file(filename);

	xmlDocDumpMemory(doc, (xmlChar**)&dump, &i);
	gpgme_data_new_from_mem(&plain, dump, i, -1);
	gpgme_data_new(&cipher);
	gpgme_recipients_new(&recp);
	gpgme_recipients_add_name(recp, id);
	gpgme_op_encrypt(ctx, recp, plain, cipher);
	
	gpgme_data_read(cipher, NULL, 0, &i);
	buf = malloc(i);
	gpgme_data_read(cipher, buf, i, &i);

	fp = fopen(filename, "w");
	if(!fp){
		statusline_msg("Couldn't open file for writing");
		getch();
		return -1;
	}
	fputs(buf, fp);
	fclose(fp);

	gpgme_data_release(plain);
	gpgme_data_release(cipher);
	gpgme_recipients_release(recp);

	free(dump);
	free(buf);

	statusline_msg("File wrote sucessfully");
}

int
gnupg_read(char *filename, xmlDocPtr *doc)
{
	int i;
	char *buf;
	char text[LONG_STR];
	GpgmeData plain, cipher;
	GpgmeError error;
	
	gpgme_data_new(&plain);
	error = gpgme_data_new_from_file(&cipher, filename, -1);
	if(error != GPGME_No_Error){
		return error;
	}

	error = gpgme_op_decrypt(ctx, cipher, plain);
	if(error != GPGME_No_Error){
		return error;
	}
	
	gpgme_data_read(plain, NULL, 0, &i);
	buf = malloc(i);
	error = gpgme_data_read(plain, buf, i, &i);

	if(error == GPGME_No_Error){
		*doc = xmlParseMemory(buf, i);
	} else {
		return -1;
	}

	passphrase_good = 1;
	return 0;
}
