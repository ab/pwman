/*
 *  PWMan - Password Management Software
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

void
debug(char *fmt, ... )
{
#ifdef DEBUG
	va_list ap;
	int d, c;
	char *s;

	fputs("PWMan Debug% ", stderr);
	
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

void
get_options()
{
	char pw_file[V_LONG_STR];
	char text[SHORT_STR];
	
	puts("Hmm... can't open ~/.pwmanrc, we'll create one manually now.");
	
	printf("GnuPG ID [you@yourdomain.com]: ");
	fgets(options->gpg_id, SHORT_STR, stdin);
	if( strcmp(options->gpg_id, "\n") == 0 ){
		strncpy(options->gpg_id, "you@yourdomain.com", SHORT_STR);
	} else {
		options->gpg_id[ strlen(options->gpg_id) - 1] = 0;
	}
	
	printf("Path to GnuPG [/usr/bin/gpg]: ");
	fgets(options->gpg_path, V_LONG_STR, stdin);
	if( strcmp(options->gpg_path, "\n") == 0){
		strncpy(options->gpg_path, "/usr/bin/gpg", V_LONG_STR);
	} else {
		options->gpg_path[ strlen(options->gpg_path) - 1] = 0;
	}
	
	snprintf(pw_file, V_LONG_STR, "%s/.pwman.db", getenv("HOME") );
	printf("Password Database File [%s]: ", pw_file );
	fgets(options->password_file, V_LONG_STR, stdin);
	if( strcmp(options->password_file, "\n") == 0){
		strncpy(options->password_file, pw_file, V_LONG_STR);
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

