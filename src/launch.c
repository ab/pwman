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
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

char * 
find_replace(char *haystack, char* needle, char* cow)
{
	char *pos;
	int len,i;
	char *ret, *ret_p;

	pos = strstr(haystack, needle);
	if(pos == NULL){
		return haystack;
	}

	/*
	 * real ugly shit for 
	 * replacing the strings
	 */
	len = (strlen(haystack) - strlen(needle)) + strlen(cow) + 1;
	ret = malloc(len);
	ret_p = ret;
	i = pos - &haystack[0];
	strncpy(ret_p, haystack, i);
	ret_p += i;
	strcpy(ret_p, cow);
	ret_p += strlen(cow);
	pos += strlen(needle);
	strcpy(ret_p, pos);

	return ret;
}

int 
execute(char *cmd) {
	int pid, status;
	char *argv[4];

	if (cmd == NULL){
		return 1;
	}
	
	pid = fork();
	if (pid == -1){
		return -1;
	}
	if (pid == 0) {
		argv[0] = "pwman_exec";
		argv[1] = "-c";
		argv[2] = cmd;
		argv[3] = NULL;

		execv("/bin/sh", argv);
		exit(127);
	}
	do {
		if (waitpid(pid, &status, 0) == -1) {
			if (errno != EINTR){
				return -1;
			} 
		}else {
			return status;
		}
	} while(1);
}

int
launch(Pw *pw)
{
	int i;
	char *cmd;
	char **cmd_array;

	if((pw == NULL) || (pw->launch == NULL)){
		return -1;
	}
	cmd = find_replace(pw->launch, "%h", pw->host);
	cmd = find_replace(cmd, "%u", pw->user);
	cmd = find_replace(cmd, "%p", pw->passwd);

	def_prog_mode();
	end_ui();

	i = execute(cmd);

	puts("Press any key to continue");
	getch();
	
	init_ui();
	reset_prog_mode();

	return i;
}
