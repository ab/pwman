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
pwman_get_options()
{
	char pw_file[LONG_STR];
	char text[SHORT_STR];
	
	puts("No .pwmanrc found. Manual config");
	
	printf("Path to GnuPG [/usr/bin/gpg]: ");
	fgets(options->gpg_path, LONG_STR, stdin);
	if( strcmp(options->gpg_path, "\n") == 0 ){
		strncpy(options->gpg_path, "/usr/bin/gpg", LONG_STR);
	} else {
		options->gpg_path[ strlen(options->gpg_path) - 1] = 0;
	}
	
	printf("GnuPG ID [you@yourdomain.com]: ");
	fgets(options->gpg_id, SHORT_STR, stdin);
	if( strcmp(options->gpg_id, "\n") == 0 ){
		strncpy(options->gpg_id, "you@yourdomain.com", SHORT_STR);
	} else {
		options->gpg_id[ strlen(options->gpg_id) - 1] = 0;
	}
	
	snprintf(pw_file, LONG_STR, "%s/.pwman.enc", getenv("HOME") );
	printf("Password Database File [%s]: ", pw_file );
	fgets(options->password_file, LONG_STR, stdin);
	if( strcmp(options->password_file, "\n") == 0){
		strncpy(options->password_file, pw_file, LONG_STR);
	} else {
		options->password_file[ strlen(options->password_file) - 1] = 0;
	}
	
	printf("Passphrase Timeout(in minutes) [180]: ");
	fgets(text, SHORT_STR, stdin);
	if( strcmp(text, "\n") == 0){
		options->passphrase_timeout = 180;
	} else {
		options->passphrase_timeout = atoi(text);
	}

	write_options = TRUE;
	write_config();
}

static void
init_pwman(int argc, char *argv[])
{
	signal(SIGKILL, quit_pwman);
	signal(SIGTERM, quit_pwman);

	umask(DEFAULT_UMASK);

	/* get options from .pwmanrc */
	options = options_new();
	if(read_config() == -1){
		pwman_get_options();
	}

	/* parse command line options */
	parse_command_line(argc, argv);

	/* check to see if another instance of pwman is open */
	if(check_lock_file()){
		fprintf(stderr, "It seems another %s is already opened by an instance of pwman\n",
				options->password_file);
		fprintf(stderr, "Two instances of pwman cannot open the same file at the same time\n");
		fprintf(stderr, "If you do not have another instance of pwman open delete the file %s.lock\n",
				options->password_file);
		exit(-1);
	}
	
	/* get pw database */
	init_database();
	read_file();
	create_lock_file();

	if( init_ui() ){
		exit(1);
	}
	
	refresh_windows();
}

static void
quit_pwman()
{

	end_ui();

	write_file();
	free_database();
	delete_lock_file();
	
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

char *
trim_ws(char *str)
{
	int i;

	for(i = (strlen(str) - 1); i >= 0; i--){
		if(str[i] != ' '){
			return str;
		} else {
			str[i] = 0;
		}
	}
	statusline_msg(str);
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
		} else if( !strcmp(argv[i], "--gpg-id") ){
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
	printf("Usage: %s [OPTION]...\n", argv_0);
	puts("Store you passwords securely using public key encryption\n");
	puts("  --help                 show usage");
	puts("  --version              display version information");
	puts("  --gpg-path [path]      path to GnuPG executable");
	puts("  --gpg-id [id]          GnuPG ID to use");
	puts("  --file [file]          file to read passwords from");
	puts("  --passphrase-timeout   time before app forgets passphrase\n\n");
	puts("Report bugs to <ivan@ivankelly.net>");
}
