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

#include "pwman.h"
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>

static void pwman_parse_command_line(int argc, char **argv);
static void pwman_show_usage();
static void pwman_show_version();
static void pwman_quit();


static int
pwman_check_lock_file()
{
	char fn[STRING_LONG];
	FILE *fp;
	
	snprintf(fn, STRING_LONG, "%s.lock", options->password_file);
	if(access(fn, F_OK) == 0){
		return 1;
	} else {
		return 0;
	}
}

static int
pwman_create_lock_file()
{
	char fn[STRING_LONG];
		
	snprintf(fn, STRING_LONG, "%s.lock", options->password_file);
	creat(fn, S_IRWXU);
}

static int
pwman_delete_lock_file()
{
	char fn[STRING_LONG];
	
	snprintf(fn, STRING_LONG, "%s.lock", options->password_file);
	unlink(fn);
}

static void
pwman_init(int argc, char *argv[])
{
	char c;
	int load_worked, gpg_id_valid;

	signal(SIGKILL, pwman_quit);
	signal(SIGTERM, pwman_quit);

	umask(DEFAULT_UMASK);

	/* get options from .pwmanrc */
	options = options_new();
	if(options_read() == -1){
		options_get();
	}

	/* parse command line options */
	pwman_parse_command_line(argc, argv);

	/* check to see if another instance of pwman is open */
	if(!options->readonly && pwman_check_lock_file()){
		fprintf(stderr, "It seems %s is already opened by an instance of pwman\n",
				options->password_file);
		fprintf(stderr, "Two instances of pwman should not be open the same file at the same time\n");
		fprintf(stderr, "If you are sure pwman is not already open you can delete the file.\n");
		fprintf(stderr,"Alternatively, you can open the file readonly by answering 'r'\n");
		fprintf(stderr,"Delete file %s.lock? [y/n/r]\n",
				options->password_file);
		c = getchar();
		fprintf(stderr,"\n");
		switch (tolower(c)) {
			case 'y':
				pwman_delete_lock_file();
				break;
			case 'r':
				options->readonly = TRUE;
				break;
			default:
				exit(-1);
		}
	}

	// Check that the gpg id is valid, if given
	if(strlen(options->gpg_id)) {
		gpg_id_valid = gnupg_check_id(options->gpg_id);
		if(gpg_id_valid == -1) {
			fprintf(stderr, "Your GPG key with id of '%s' could not be found\n", options->gpg_id);
			fprintf(stderr, "You will be prompted for the correct key when saving\n");
			fprintf(stderr, "\n(press any key to continue)\n");
			c = getchar();
		}
		if(gpg_id_valid == -2) {
			fprintf(stderr, "Your GPG key with id of '%s' has expired!\n", options->gpg_id);
			fprintf(stderr, "Please change the expiry date of your key, or switch to a new one\n");
			exit(-1);
		}
	}
	
	// Start up our UI
	if( ui_init() ){
		exit(1);
	}

	ui_refresh_windows();

	/* get pw database */
	pwlist_init();
	load_worked = pwlist_read_file();
	if(load_worked != 0) {
		debug("Failed to load the database, error was %d", load_worked);
		// Did they cancel out, or is it a new file?
		if(load_worked < 0) {
			pwlist = pwlist_new("Main");
			current_pw_sublist = pwlist;
		} else {
			// Quit, hard!
			ui_end();
			fprintf(stderr, "\n\nGPG read cancelled, exiting\n");
			exit(1);
		}
	}
	if (!options->readonly){
		pwman_create_lock_file();
	}

	ui_refresh_windows();
}

static void
pwman_quit()
{
	pwlist_write_file();
	pwlist_free_all();
	pwman_delete_lock_file();
	
	ui_end();
	options_write();
	
	exit(0);
}

int
main(int argc, char *argv[])
{
	pwman_init(argc, argv);

	ui_run();

	pwman_quit();
	return 0;
}

static void
pwman_parse_command_line(int argc, char **argv)
{
	int i;

	for(i = 1; i < argc; i++){
		if( !strcmp(argv[i], "--help") || !strcmp(argv[i], "-h") ){
			pwman_show_usage(argv[0]);
			exit(1);
		} else if( !strcmp(argv[i], "--version") || !strcmp(argv[i], "-v") ){
			pwman_show_version();
			exit(1);
		} else if( !strcmp(argv[i], "--gpg-path") ){
			write_options = FALSE;
			strncpy(options->gpg_path, argv[i + 1], STRING_LONG);
			i++;
		}else if( !strcmp(argv[i], "--gpg-id") ){
			write_options = FALSE;
			strncpy(options->gpg_id, argv[i + 1], STRING_LONG);
			i++;
		} else if( !strcmp(argv[i], "--file") || !strcmp(argv[i], "-f") ){
			write_options = FALSE;
			strncpy(options->password_file, argv[i + 1], STRING_LONG);
			i++;
		} else if( !strcmp(argv[i], "--passphrase-timeout") || !strcmp(argv[i], "-t") ){
			write_options = FALSE;
			options->passphrase_timeout = atoi(argv[i + 1]);
			i++;
		} else if ( !strcmp(argv[i], "--readonly") || !strcmp(argv[i], "-r") ){
			write_options = FALSE;
			options->readonly = TRUE;
		} else {
			printf("option %s not recognised\n", argv[i]);
			printf("try %s --help for more info\n", argv[0]);
			exit(1);
		}
	}
}

static void
pwman_show_version()
{
	puts(PACKAGE " v " VERSION);
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
pwman_show_usage(char *argv_0)
{
	printf("Usage: %s [OPTIONS]...\n", argv_0);
	puts("Store you passwords securely using public key encryption\n");
	puts("  --help                 show usage");
	puts("  --version              display version information");
	puts("  --gpg-path <path>      Path to GnuPG executable");
	puts("  --gpg-id <id>          GnuPG ID to use");
	puts("  --file <file>          file to read passwords from");
	puts("  --passphrase-timeout <mins>    time before app forgets passphrase(in minutes)");
	puts("  --readonly             open the database readonly\n\n");
	puts("Report bugs to <ivan@ivankelly.net>");
}
