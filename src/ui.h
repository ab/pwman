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

#ifndef UI_H
#define UI_H

#include <curses.h>
#include <signal.h>
#include <ctype.h>

#ifdef HAVE_TERMIOS_H
#       include <termios.h>
#else   
#       ifdef HAVE_LINUX_TERMIOS_H
#               include <linux/termios.h>
#       endif
#endif
#ifdef HAVE_SYS_IOCTL_H
#       include <sys/ioctl.h>
#endif

#define MIN_LINES 22
#define MIN_COLS 60

#define LIST_TOP 3 
#define LIST_BOTTOM (LINES -3)
#define LIST_LINES (LIST_BOTTOM-LIST_TOP)

#define LAST_LIST_ITEM (first_list_item + LIST_LINES - 1)
#define NAMELEN (COLS/3)-1
#define NAMEPOS 2

#define HOSTLEN (COLS/3)-1
#define HOSTPOS (NAMEPOS + NAMELEN + 1)

#define USERLEN (COLS/3)-1
#define USERPOS (NAMEPOS + NAMELEN + HOSTLEN + 2)
#define hide_cursor() curs_set(0)
#define show_cursor() curs_set(1)

#define NUM_TO_CHAR(n) (n + 48)
#define CHAR_TO_NUM(n) (n - 49)

typedef enum {
	STRING,
	INT
} TYPE;

typedef struct {
	char *name;
	void *value;
	int max_length;
	TYPE type;
	char *(*autogen)(char*);
} InputField;

int init_list();
int free_list();
int refresh_list();

int view_pw(int i);

int statusline_yes_no(char *, int);
int statusline_msg(char *msg);
char * statusline_ask_str(char *, char *, int len);

#endif


