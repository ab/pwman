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

#include <ui.h>
#include <pwman.h>

WINDOW *list = NULL;
int lines = 0;
int curitem = 0;

int
init_list()
{
	char str[80];
	list = newwin(LIST_LINES, COLS, LIST_TOP, 0);
	
	scrollok(list, TRUE);
}
/*
int
resize_list()
{
	wresize(list, LIST_LINES, COLS);
}
*/

int
free_list()
{
	delwin(list);
	list = NULL;
}

int 
highlight_line(int line)
{
	wstandout(list);
/*	mvwchgat(list, i, 0, -1, A_STANDOUT, 0, NULL);*/
        scrollok(list, FALSE);
	        {
                int i;
                wmove(list, line, 0);
                for(i = 0; i < COLS; i++)
                        waddch(list, ' ');
	        /*wattrset(win, 0);*/
        }
        scrollok(list, TRUE);
}

int
refresh_list()
{
	Pw *iter;
	int i = 0;
	static int first_list_item = 0;
	int num_shown = 0;

	if(list == NULL){
		init_list();
	}

	clear_list();;
	lines = 0;

	list_headerline();
	if(curitem < 0){
		curitem = 0;
	}
	if(curitem < first_list_item){
		first_list_item = curitem;		
	} else if((curitem > LAST_LIST_ITEM)){
		first_list_item++;
	}

	for(iter = pwlist; (iter != NULL); iter = iter->next){
		/*
		 * if line satifies filter criteria increment i and lines
		 */
		if( apply_filter(iter, options->filter) ){
			if((i >= first_list_item) && (i <= LAST_LIST_ITEM)){
				if(lines == curitem){
					highlight_line(num_shown);
				}
				mvwaddnstr(list, num_shown, NAMEPOS, iter->name, NAMELEN);
				mvwaddnstr(list, num_shown, HOSTPOS, iter->host, HOSTLEN);
				mvwaddnstr(list, num_shown, USERPOS, iter->user, USERLEN);
				wstandend(list);
				num_shown++;
			}
			i++;	
			lines++;
		}
		
	}
	wrefresh(list);
	hide_cursor();
}

int
clear_list()
{
	int i;
	
	werase(list);
	for(i = 0; i < COLS; i++){
		mvaddch(2, i, ' ');
	}
	wrefresh(list);
}

int
list_headerline()
{
	show_cursor();
	attrset(A_BOLD);

	mvaddnstr(2, NAMEPOS, "Name", NAMELEN);
	mvaddnstr(2, HOSTPOS, "Host", HOSTLEN);
	mvaddnstr(2, USERPOS, "Username", USERLEN);

	attrset(A_NORMAL);
	hide_cursor();
}

int
list_up()
{
	if(curitem < 1){
		return;
	}

	curitem--;
	
	refresh_list();
}

int
list_down()
{
	if(curitem >= (lines-1)){
		return;
	}

	curitem++;

	refresh_list();
}
