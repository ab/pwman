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


/*
 * define for strings to check for with regex
 * should allow for internationalization
 */

#define GPG_ERR_CANTWRITE	"file create error"
#define GPG_ERR_CANTOPEN 	"can't open"
#define GPG_ERR_BADPASSPHRASE	"bad passphrase"
#define GPG_ERR_NOSECRETKEY	"secret key not available"

/* end defines */

#include <sys/stat.h>
#include <pwman.h>
#include <ui.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <regex.h>
#include <fcntl.h>

#define STDOUT 0
#define STDIN 1
#define STDERR 2

int passphrase_good = 0;
extern int errno;
extern int write_options;

int 
gnupg_exec(char *path, char *args[], FILE *stream[3])
{
	int stdin_fd[2];
	int stdout_fd[2];
	int stderr_fd[2];
	int pid;

	pipe(stdin_fd);
	pipe(stdout_fd);
	pipe(stderr_fd);

	pid = fork();

	if( (path == NULL) || (args == NULL) ){
		return -1;
	}
	if(pid == 0){
		close(stdout_fd[0]);
		close(stdin_fd[1]);
		close(stderr_fd[0]);

		dup2(stdout_fd[1], fileno(stdout));
		dup2(stdin_fd[0], fileno(stdin));
		dup2(stderr_fd[1], fileno(stderr));

		execv(path, args);
	} else {
		close(stdout_fd[1]);
		close(stdin_fd[0]);
		close(stderr_fd[1]);

		if(stream != NULL){
			stream[STDOUT] = fdopen(stdout_fd[0], "r");
			stream[STDIN] = fdopen(stdin_fd[1], "w");
			stream[STDERR] = fdopen(stderr_fd[0], "r");
		}
		
		return pid;
	}
}

void
gnupg_exec_end(int pid)
{
	waitpid(pid, NULL, 0);
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

int
str_in_buf(char *buf, char *check)
{
	regex_t reg;

	debug("str_in_buf: start checking");
	
	if( (buf == NULL) || (check == NULL) ){
		return 0;
	}
	regcomp(&reg, check ,0);
	if(regexec(&reg, buf, 0, NULL , 0) == 0){
		regfree(&reg);
		return 1;
	} else {
		regfree(&reg);
		return 0;
	}
}

int
gnupg_find_recp(char *str, char *user)
{
	regex_t reg;
	regmatch_t pmatch;
	int i[3];

	regcomp(&reg, "\"", 0);
	regexec(&reg, str, 1, &pmatch, 0);
	i[0] = pmatch.rm_eo;
	regexec(&reg, str, 1, &pmatch, 0);
	i[1] = pmatch.rm_so;

	i[2] = i[1] - i[0];
	user = malloc(i[2]+1);
	strncpy(user, str+i[0], i[2]);
}

void
get_gnupg_id(char *id)
{
	while(1){
		id = statusline_ask_str("GnuPG Recipient ID:", id, MED_STR);
		if(id[0] == 0){
			return;
		}
		if(check_gnupg_id(id) == 0){
			return;
		}
		debug("get_gnupg_id: if here is reached id is bad");
		statusline_msg("Bad Recipient, Try again"); getch();
	}
}

char *
expand_filename(char *filename)
{
	char *buf = malloc(LONG_STR);
	
	if((filename[0] == '~') && getenv("HOME")){
		snprintf(buf, LONG_STR, "%s%s\0", getenv("HOME"), filename+1);
		strncpy(filename, buf, LONG_STR);
	}
	free(buf);

	return filename;
}

char *
get_filename(char *filename, char rw)
{
	if(rw == 'r'){
		filename = statusline_ask_str("File to read from:", filename, LONG_STR);
	} else if(rw == 'w'){
		filename = statusline_ask_str("File to write to:", filename, LONG_STR);
	} else {
		return;
	}
}

const char *
get_passphrase()
{
	static char *passphrase = NULL;

/*	passphrase == malloc(V_LONG_STR);*/
	if((time_base >= (time(NULL) - (options->passphrase_timeout*60))) && (passphrase_good == 1)){
		return passphrase;
	}
	
	passphrase_good = 0;

	passphrase = statusline_ask_passwd("Enter passphrase(^G to Cancel):", passphrase, 
			V_LONG_STR, 0x07); /* 0x07 == ^G */
	passphrase_good = 1;

	return passphrase;
}

void
forget_passphrase()
{
	passphrase_good = 0;

	debug("forget_passphrase: passphrase forgotten");
	statusline_msg("Passphrase forgotten");
}

int
check_gnupg_id(char *id)
{	
	regex_t reg;
	int pid;
	char text[V_LONG_STR], idstr[LONG_STR], *args[3];
	FILE *streams[3];
	
	debug("check_gnupg_id: check gnupg id");	

	if( strchr(id, '%') ){ /* hmm, could be format string bug so tell fuck off */
		return -1;
	} else {
		snprintf(idstr, LONG_STR, "[^<]*<%s>", id);
	}
	args[0] = "gpg";
	args[1] = "--list-keys";
	args[2] = NULL;

	pid = gnupg_exec(options->gpg_path, args, streams);

	while( fgets(text, V_LONG_STR, streams[STDOUT]) ){
		regcomp(&reg, idstr,0);
		if(regexec(&reg, text, 0, NULL , 0) == 0){
			gnupg_exec_end(pid);
			return 0; 
		}
	}
	
	gnupg_exec_end(pid);

	return -1;
}

int 
check_gnupg()
{
	FILE *streams[3];
	char *args[3];
	int pid, count, version[3];
	
	debug("check_gnupg: start");
	if(options->gpg_path == NULL){
		return -1;
	}

	args[0] = "gpg";
	args[1] = "--version";
	args[2] = NULL;
	
	pid = gnupg_exec(options->gpg_path, args, streams);

	/* this might do version checking someday if needed */
	count = fscanf(streams[STDOUT], "gpg (GnuPG) %d.%d.%d", 
			&version[0], &version[1], &version[2]);
	gnupg_exec_end(pid);

	if(count != 3){
		statusline_msg("WARNING! GnuPG Executable not found"); getch();
		return -1;
	} else {
		debug("check_gnupg: Version %d.%d.%d", version[0], version[1], version[2]);	
		return 0;
	}
}

int
gnupg_write(xmlDocPtr doc, char* id, char* filename)
{
	FILE *streams[3];
	char cmd[V_LONG_STR], buf[V_LONG_STR];
	char *args[10];
	char *err;
	int pid;

	debug("gnupg_write: do some checks");	
	if((check_gnupg() != 0) || !filename || (filename[0] == 0)){
		debug("gnupg_write: no gnupg or filename not set");
		return -1;
	}
	if(check_gnupg_id(id) != 0){
		get_gnupg_id(id);

		if(id[0] == 0){
			return -1;
		}
	}

	debug("gnupg_write: start writing");

	err = NULL;
	args[0] = "gpg";
	args[1] = "-e";
	args[2] = "-a";
	args[3] = "--always-trust"; /* gets rid of error when moving 
				       keys from other machines */
	args[4] = "--yes";
	args[5] = "-r";
	args[6] = id;
	args[7] = "-o";
	args[8] = expand_filename( filename );
	args[9] = NULL;

	while(1){
		/* clear err buffer */
		if(err != NULL){ free(err); err = NULL; }

		pid = gnupg_exec(options->gpg_path, args, streams);

#if XML_VERSION >= 20423
		xmlDocFormatDump(streams[STDIN], doc, TRUE);
#else
		xmlDocDump(streams[STDIN], doc);
#endif
		close( fileno(streams[STDIN]) );
		
		while( fgets(buf, V_LONG_STR - 1, streams[STDERR]) != NULL ){
			err = add_to_buf(err, buf);
		}
		gnupg_exec_end(pid);
		
		debug("gnupg_write: start error checking");
		/*
		 * check for errors(no key, bad pass, no file etc etc)
		 */
		if( str_in_buf(err, GPG_ERR_CANTWRITE) ){
			debug("gnupg_write: cannot write to %s", filename);

			snprintf(buf, V_LONG_STR, "Cannot write to %s", filename);
			statusline_msg(buf); getch();

			filename = get_filename(filename, 'w');
			if(filename[0] == 0){
				return -1;
			}
			continue;
		}
		break;
	}

	statusline_msg("List Saved");
	debug("gnupg_write: file write sucessful");
	
	return 0;
}

int
gnupg_read(char *filename, xmlDocPtr *doc)
{
	char *args[9], *passphrase, *data, *err, buf[V_LONG_STR], *user;
	FILE *streams[3];
	int pid;
	
	if(check_gnupg() != 0){
		*doc = xmlNewDoc("1.0");
		return -1;
	}
/*
	if(check_gnupg_passphrase(filename) == -1){
		return -1;
	}*/

	data = NULL;
	err = NULL;
	user = NULL;
	args[0] = "gpg";
	args[1] = "--passphrase-fd";
	args[2] = "0";
	args[3] = "--no-verbose";
	args[4] = "--batch";
	args[5] = "--output";
	args[6] = "-";
	args[7] = expand_filename( filename );
	args[8] = NULL;
	
	while(1){
		/* clear buffers */
		if(err != NULL){ free(err); err = NULL; }
		if(data != NULL){ free(data); data = NULL; }
		
		passphrase = (char*)get_passphrase();
		if(passphrase == NULL){
			write_options = 0;
			filename[0] = 0;
			break;
		}
		pid = gnupg_exec(options->gpg_path, args, streams);

		fputs( passphrase, streams[STDIN]);
		fputc( '\n' , streams[STDIN]);
		fclose(streams[STDIN]);

		debug("gnupg_read: start reading data");
		while( fgets(buf, V_LONG_STR - 1, streams[STDOUT]) != NULL ){
			data = add_to_buf(data, buf);
		}
		while( fgets(buf, V_LONG_STR - 1, streams[STDERR]) != NULL ){
			err = add_to_buf(err, buf);
		}
	
		gnupg_exec_end(pid);

		debug("gnupg_read: start error checking");
		/*
		 * check for errors(no key, bad pass, no file etc etc)
		 */
		if( str_in_buf(err, GPG_ERR_BADPASSPHRASE) ){
			debug("gnupg_read: bad passphrase");
			statusline_msg("Bad Passphrase, please re-enter");
			getch();

			passphrase_good = 0;
			continue;
		}
		if( str_in_buf(err, GPG_ERR_CANTOPEN) ){
			debug("gnupg_read: cannot open %s", filename);
			snprintf(buf, V_LONG_STR, "Cannot open file \"%s\"", filename);
			statusline_msg(buf); getch();
			break;
		}
		if( str_in_buf(err, GPG_ERR_NOSECRETKEY) ){
			gnupg_find_recp(err, user);
			debug("gnupg_read: bad user %s", user);
			snprintf(buf, V_LONG_STR, "You do not have the secret key for %s", user);
			statusline_msg(buf); getch();
			break;
		}
		break;
	}


	debug("gnupg_read: finished read");
	if(data != NULL){
		debug("gnupg_read: data != NULL");
		*doc = xmlParseMemory(data, strlen(data));
		free(data);
	} else {
		debug("gnupg_read: data is null");
		*doc = xmlNewDoc("1.0");
	}
	if(err != NULL){ free(err); }
	if(user != NULL){ free(user); }

	debug("gnupg_read: finished all");

	return 0;
}
