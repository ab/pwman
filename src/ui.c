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
#include <ui.h>
#include <help.h>
#include <time.h>

char * statusline_ask_str(char *, char*, int);
Pw *get_current_item();

int should_resize = FALSE;
int can_resize = FALSE;


WINDOW *top = NULL, *bottom = NULL;
extern int curitem, lines;
extern WINDOW *list;

int
draw_top()
{
	werase(top);
	mvwhline(top, 1, 0, ACS_HLINE, COLS);
	mvwprintw(top, 0, 0, "%s | %s", PACKAGE " " VERSION, MAIN_HELPLINE);
	
	wrefresh(top);
}

int
draw_bottom()
{
	werase(bottom);
	mvwhline(bottom, 0, 0, ACS_HLINE, COLS);
	mvwhline(bottom, 2, 0, ACS_HLINE, COLS);

	wrefresh(bottom);
}

int 
refresh_windows()
{
	draw_top();
	draw_bottom();
	refresh_list();

	refresh();
}

int
resize_windows()
{
	wresize(top, 2, COLS);
}

static void
too_small_warning()
{
	clear();
	attron(A_BOLD);
	mvprintw(((LINES-2)/2),0,"Your window is too small\n\n"
			"Minimum size is %dx%d\n\n"
			"Please resize and press any key", MIN_LINES, MIN_COLS);
	attroff(A_BOLD);
	getch();
}

#ifdef SIGWINCH
static void
resize()
{
	struct winsize winsz;
	ioctl (0, TIOCGWINSZ, &winsz);

	resizeterm(winsz.ws_row, winsz.ws_col);
	if((winsz.ws_col < MIN_COLS) || (winsz.ws_row < MIN_LINES)) {
		
		/*
		 * if window is too small notify user 
		 * until he changes it
		 */
		do {
			too_small_warning();
			ioctl (0, TIOCGWINSZ, &winsz);
			resizeterm(winsz.ws_row, winsz.ws_col);
		} while ((winsz.ws_col < MIN_COLS) || (winsz.ws_row < MIN_LINES));

	} else {
		should_resize = FALSE;
		/*	resize_windows();
		resize_list();*/
		free_windows();
		init_windows();
		refresh_windows();
	}
}
#endif

static void
win_changed(int i)
{
	if( can_resize ){
		resize();
		refresh_windows(); /* dunno why i need this but it wont work without it */
	} else {
		should_resize = TRUE;
	}
}


int
init_windows()
{
	top = newwin(2, COLS,0,0);
	bottom = newwin(3, COLS, LINES - 3, 0);

	init_list();
}

int 
free_windows()
{
	free_list();

	erase();
	delwin(top);
	delwin(bottom);
}

int
init_ui()
{
	initscr();
	cbreak();
	noecho();
	nonl();

	intrflush(stdscr, FALSE);
	keypad(stdscr, TRUE);

	if((LINES < MIN_LINES) || (COLS < MIN_COLS)){
		clear(); refresh(); endwin();
		fprintf(stderr, "Your terminal is to small\n");
		fprintf(stderr, "Min size is %dx%d\n", MIN_COLS, MIN_LINES);

		return 1;
	}

#ifdef SIGWINCH
	signal(SIGWINCH, win_changed);
#endif

	init_windows();
	refresh_windows();
}

int
run_ui()
{
	Pw *current_item;
	int ch;
	int i = 0;
	char msg[80];

	time_base = time(NULL);

	while(1){
		can_resize = TRUE;
		if( should_resize ){
			resize();
		}
		ch = getch();
		statusline_clear();
		can_resize = FALSE;

		if((time_base < (time(NULL) - (options->passphrase_timeout*60)))
				&& options->passphrase_timeout != 0 && tolower(ch) != 'q'){
			write_file();
			free_database();

			statusline_msg("Passphrase has timed out and you must enter it again.");
			getch();
			
			read_file();

			time_base = time(NULL);
		}
		
		switch(ch){
			case 'Q':
			case 'q':
				return;
				break;
			case '?':
				display_help();
				break;
			case KEY_UP:
			case 'j':
				list_up();
				break;
			case KEY_DOWN:
			case 'k':
				list_down();
				break;
			case 'a':
				add_pw_ui();
				break;
			case 'e':
			case ' ':
			case 13:
				current_item = get_current_item();
				edit_pw(current_item);
				break;
			case 'd':
			case 0x14A:
				current_item = get_current_item();
				delete_pw_ui(current_item);
				break;
			case 'h':
				hide_cursor();
				break;
			case 's':
				show_cursor();
				break;
			case 'o':
				edit_options();
				break;
			case 0x17:
				write_file();
				break;
			case 0x12:
				free_database();
				read_file();
				refresh_list();
				break;
			case 0x07:
				pwgen_indep();
				break;
			case 0x06:
				forget_passphrase();
				break;
			case 0x0C:
				refresh_windows();
				break;
			case '/':
				get_filter();
				curitem = 1;
				break;
			case 'E':
				current_item = get_current_item();
				export_passwd(current_item);
				break;
			case 'I':
				import_passwd();
				refresh_list();
				break;
			case 'l':
				i = launch(curitem);
				snprintf(msg, 80, "Application exited with code %d", i);
				statusline_msg(msg);
				break;
			default:
				break;
		}
	}
}

int
end_ui()
{
	free_windows();
	clear();
	refresh();
	endwin();
	echo();
}

int 
statusline_msg(char * msg)
{
	statusline_clear();
	mvwaddstr(bottom, 1, 0, msg);
	refresh();
	wrefresh(bottom);
}

int 
statusline_clear()
{
	wmove(bottom, 1, 0);
	wclrtoeol(bottom);
	wrefresh(bottom);
	refresh();
}

void
statusline_ask_num(char *msg, int *i)
{
	int x = strlen(msg) + 5;
	char input[SHORT_STR];

	statusline_clear();
	statusline_msg(msg);

	echo();
	show_cursor();

	mvwgetnstr(bottom, 1, x, input, SHORT_STR);
	*i = atoi(input);
	
	noecho();
	hide_cursor();

	statusline_clear();
}

void
statusline_ask_char(char *msg, char *c, char* valid)
{
	int x = strlen(msg) + 5;
	char input[SHORT_STR];

	*c = 0;
	do {
		statusline_clear();
		if(*c != 0){
			statusline_msg("Bad choice, press any key to try again");
			getch();
			statusline_clear();
		}
		statusline_msg(msg);

		echo();
		show_cursor();

		*c = mvwgetch(bottom, 1, x);

		noecho();
		hide_cursor();
		
	} while ( !strchr(valid, *c) );
	
	statusline_clear();
}

char *
statusline_ask_str(char *msg, char *input, int len)
{
	int x = strlen(msg) + 5;

	if(input == NULL){
		input = malloc(len);
	}
	statusline_clear();
	statusline_msg(msg);

	echo();
	show_cursor();
	mvwgetnstr(bottom, 1, x, input, len);
	noecho();
	hide_cursor();

	statusline_clear();
	
	return input;
}

char *
statusline_ask_str_with_autogen(char *msg, char *input, int len, char *(*autogen)(char *), int ch)
{
	int i = 0;
	int c;
	char *text[2], *s;
	int x;

	if(input == NULL){
		input = malloc(len);
	}
	text[0] = malloc(LONG_STR);
	text[1] = malloc(LONG_STR);
	
	strncpy(text[1], msg, LONG_STR);
	if(s = strrchr(text[1], ':')){
		*s = 0;
	}
	snprintf(text[0], LONG_STR, "%s(%c for autogen):\t", text[1],ch);
	x = strlen(text[0]) + 5;

	statusline_clear();
	statusline_msg(text[0]);

	show_cursor();
	noecho();

	wmove(bottom, 1, x);

	while(i < len){
		c = wgetch(bottom);
		if(c == 0x7f){
			if(i){
				i--;
				mvwaddch(bottom, 1, x+i, ' ');
				wmove(bottom, 1, x+i);
			}
		} else if(c == 0xd){
			input[i] = 0;
			break;
		} else if(c == ch){
			input = autogen(input);
			break;
		} else {
			input[i] = c;
			mvwaddch(bottom, 1, x + i, c);
			i++;
		}
	}
	
	hide_cursor();
	
	statusline_clear();

	free(text[0]);
	free(text[1]);

	return input;
}

char *
statusline_ask_passwd(char *msg, char *input, int len)
{
	int i = 0;
	int c;
	int x = strlen(msg) + 5;

	if(!input){
		input = malloc(len);
	}
	statusline_clear();
	statusline_msg(msg);

	show_cursor();
	noecho();

	wmove(bottom, 1, x);

	while(i < len){
		c = wgetch(bottom);
		if(c == 0x7f){
			if(i){
				i--;
				mvwaddch(bottom, 1, x+i, ' ');
				wmove(bottom, 1, x+i);
			}
		} else if(c == 0xd){
			input[i] = 0;
			break;
		} else {
			input[i] = c;
			mvwaddch(bottom, 1, x + i, '*');
			i++;
		}
	}
	
	hide_cursor();
	
	statusline_clear();
	
	return input;
}

int 
statusline_yes_no(char *msg, int def)
{
	int ret = -1, len;
	char *msg2;
	int ch;
	
	len = strlen(msg) + 10;
	msg2 = malloc(len);

	snprintf(msg2, len,  "%s%s", msg, def ? " (Y/n)?" : " (y/N)?", NULL);

	while(ret == -1){
		statusline_msg(msg2);
		
		ch = getch();
		switch( ch ){
			case 'n':
			case 'N':
				ret = FALSE;
				break;
			case 'y':
			case 'Y':
				ret = TRUE;
				break;
			case 13:
				ret = def;
				break;
			default:
				statusline_msg("Bad option, try again.");
				getch();
				break;
		}
	}

	free(msg2);
	statusline_clear();

	return ret;
}

int 
display_help()
{
	int i;
	WINDOW *helpwin;

	helpwin = newwin(LINES - 5, COLS - 6, 2, 3);
	clear_list();

	for(i = 0; help[i] != NULL; i++){
		waddstr(helpwin, help[i]);
		if( !((i+1) % (LINES - 8)) || (help[i+1] == NULL) ){
	/*		refresh();*/
			wrefresh(helpwin);
			statusline_msg("Press any key to continue...");
			getch();
			wclear(helpwin);
		}
	}
	refresh_list();
	statusline_clear();
	delwin(helpwin);
}
