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

static void parse_command_line(int argc, char **argv);
static void show_usage();
static void show_version();
static void quit_pwman();


static int
check_lock_file()
{
	char fn[V_LONG_STR];
	FILE *fp;
	
	snprintf(fn, V_LONG_STR, "%s.lock", options->password_file);
	if(access(fn, F_OK) == 0){
		return 1;
	} else {
		return 0;
	}
}

static int
create_lock_file()
{
	char fn[V_LONG_STR];
		
	snprintf(fn, V_LONG_STR, "%s.lock", options->password_file);
	creat(fn, S_IRWXU);
}

static int
delete_lock_file()
{
	char fn[V_LONG_STR];
	
	snprintf(fn, V_LONG_STR, "%s.lock", options->password_file);
	unlink(fn);
}

static void
init_pwman(int argc, char *argv[])
{
	char c;
	signal(SIGKILL, quit_pwman);
	signal(SIGTERM, quit_pwman);

	umask(DEFAULT_UMASK);

	/* get options from .pwmanrc */
	options = new_options();
	if(read_config() == -1){
		get_options();
	}

	/* parse command line options */
	parse_command_line(argc, argv);

	/* check to see if another instance of pwman is open */
	if(check_lock_file()){
		fprintf(stderr, "It seems %s is already opened by an instance of pwman\n",
				options->password_file);
		fprintf(stderr, "Two instances of pwman should not be open the same file at the same time\n");
		fprintf(stderr, "If you are sure pwman is not already open you can delete the file. Delete file %s.lock? [y/n]\n",
				options->password_file);
		c = getchar();
		if(tolower(c) == 'y'){
			delete_lock_file();
		} else {
			exit(-1);
		}
	}
	
	if( init_ui() ){
		exit(1);
	}

	refresh_windows();

	/* get pw database */
	init_database();
	if(read_file() != 0){
		pwlist = new_pwlist("Main");
		current_pw_sublist = pwlist;
	}
	create_lock_file();

	refresh_windows();
}

static void
quit_pwman()
{
	write_file();
	free_database();
	delete_lock_file();
	
	end_ui();
	write_config();
	
	exit(0);
}

int
main(int argc, char *argv[])
{
	init_pwman(argc, argv);

	run_ui();

	quit_pwman();
	return 0;
}

static void
parse_command_line(int argc, char **argv)
{
	int i;

	for(i = 1; i < argc; i++){
		if( !strcmp(argv[i], "--help") || !strcmp(argv[i], "-h") ){
			show_usage(argv[0]);
			exit(1);
		} else if( !strcmp(argv[i], "--version") || !strcmp(argv[i], "-v") ){
			show_version();
			exit(1);
		} else if( !strcmp(argv[i], "--gpg-path") ){
			write_options = FALSE;
			strncpy(options->gpg_path, argv[i + 1], LONG_STR);
			i++;
		}else if( !strcmp(argv[i], "--gpg-id") ){
			write_options = FALSE;
			strncpy(options->gpg_id, argv[i + 1], SHORT_STR);
			i++;
		} else if( !strcmp(argv[i], "--file") || !strcmp(argv[i], "-f") ){
			write_options = FALSE;
			strncpy(options->password_file, argv[i + 1], LONG_STR);
			i++;
		} else if( !strcmp(argv[i], "--passphrase-timeout") || !strcmp(argv[i], "-t") ){
			write_options = FALSE;
			options->passphrase_timeout = atoi(argv[i + 1]);
			i++;
		} else {
			printf("option %s not recognised\n", argv[i]);
			printf("try %s --help for more info\n", argv[0]);
			exit(1);
		}
	}
}

static void
show_version()
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
show_usage(char *argv_0)
{
	printf("Usage: %s [OPTIONS]...\n", argv_0);
	puts("Store you passwords securely using public key encryption\n");
	puts("  --help                 show usage");
	puts("  --version              display version information");
	puts("  --gpg-path <path>      Path to GnuPG executable");
	puts("  --gpg-id <id>          GnuPG ID to use");
	puts("  --file <file>          file to read passwords from");
	puts("  --passphrase-timeout <mins>    time before app forgets passphrase(in minutes)\n\n");
	puts("Report bugs to <ivan@ivankelly.net>");
}
