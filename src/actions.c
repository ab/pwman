/*
 *  PWman - Password management application
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

extern Pw * get_highlighted_item();
extern PWList *get_highlighted_sublist();
extern PWList * new_pwlist(char*);
extern char *pwgen_ask();
int disp_h = 15, disp_w = 60;
extern int curitem;
extern WINDOW *bottom;

int
list_add_pw()
{
	Pw *pw;
	InputField fields[] = {
		{"Name:\t", NULL, STRING_MEDIUM, STRING},
		{"Host:\t", NULL, STRING_MEDIUM, STRING},
		{"User:\t", NULL, STRING_MEDIUM, STRING},
		{"Password:\t", NULL, STRING_SHORT, STRING, pwgen_ask},
		{"Launch Command:\t", NULL, STRING_LONG, STRING}
	};
	int i, x;

	pw = new_pw(); 
	statusline_ask_str(fields[0].name, pw->name, STRING_MEDIUM);
	statusline_ask_str(fields[1].name, pw->host, STRING_MEDIUM);
	statusline_ask_str(fields[2].name, pw->user, STRING_MEDIUM);
/*
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

	statusline_clear();*/

	statusline_ask_str_with_autogen(fields[3].name, pw->passwd, STRING_SHORT, fields[3].autogen, 0x07);
/*	statusline_msg(fields[3].name);
	x = strlen(fields[3].name) + 5;

	if((i = getch()) == 0x07){
		pw->passwd = fields[3].autogen();
	} else {
		echo();
		show_cursor();
		mvwgetnstr(bottom, 1, x, pw->passwd, PASS_LEN);
		mvwaddch(bottom, 1, x, i);
		wmove(bottom, 1, x+1);
		noecho();
		hide_cursor();
		statusline_clear();
	}*/
	
	statusline_ask_str(fields[4].name, pw->launch, STRING_LONG);
	
	fields[0].value = pw->name;
	fields[1].value = pw->host;
	fields[2].value = pw->user;
	fields[3].value = pw->passwd;
	fields[4].value = pw->launch;

	i = yes_no_dialog(fields, (sizeof(fields)/sizeof(InputField)), NULL, "Add this entry");

	if(i){
		add_pw_ptr(current_pw_sublist, pw);
		statusline_msg("New password added");
	} else {
		free_pw(pw);
		statusline_msg("New password cancelled");
	}

	refresh_list();
}

int
edit_pw(Pw *pw)
{
	InputField fields[] = {
		{"Name:\t", NULL, STRING_MEDIUM, STRING},
		{"Host:\t", NULL, STRING_MEDIUM, STRING},
		{"User:\t", NULL, STRING_MEDIUM, STRING},
		{"Password:\t", NULL, STRING_SHORT, STRING, pwgen_ask},
		{"Launch Command:\t", NULL, STRING_LONG, STRING}
	};

	if(pw == NULL){
		return -1;
	}
	/*
	 * Get specified password
	 */

	fields[0].value = pw->name;
	fields[1].value = pw->host;
	fields[2].value = pw->user;
	fields[3].value = pw->passwd;
	fields[4].value = pw->launch;

	/*
	 * initialize the info window
	 */

	input_dialog(fields, (sizeof(fields)/sizeof(InputField)), "Edit Password");
}

int
edit_options()
{
	InputField fields[] = {
		{"GnuPG Path:\t", options->gpg_path, STRING_LONG, STRING},
		{"GnuPG ID:\t", options->gpg_id, STRING_LONG, STRING},
		{"Password File:\t", options->password_file, STRING_LONG, STRING},
		{"Passphrase Timeout(in minutes):\t", &options->passphrase_timeout, STRING_SHORT, INT}
	};

	input_dialog(fields, (sizeof(fields)/sizeof(InputField)), "Edit Preferences");

	write_options = TRUE;
}

int 
input_dialog_draw_items(WINDOW* dialog_win, InputField *fields, 
		int num_fields, char *title, char *msg)
{
	int i, h = 0;

	wclear(dialog_win);
	
	box(dialog_win, 0, 0);

	if(title){
		wattron(dialog_win, A_BOLD);
		i = strlen(title);
		h += 2;
		mvwaddstr(dialog_win, h, (disp_w - i)/2, title);
		wattroff(dialog_win, A_BOLD);
	}

	/* loop through fields */
	for(i = 0; i < num_fields; i++){
		h += 2;
		if(fields[i].type == STRING){
			mvwprintw(dialog_win, h, 3,
				"%d - %s %s", (i+1), fields[i].name, (char*)fields[i].value);
		} else if(fields[i].type == INT){
			mvwprintw(dialog_win, h, 3,
				"%d - %s %d", (i+1), fields[i].name, *((int*)fields[i].value) );
		}
	}

	wattron(dialog_win, A_BOLD);

	if(msg){
		i = strlen(msg);
		h += 2;
		mvwaddstr(dialog_win, h, (disp_w - i)/2, msg);
	}

	wattroff(dialog_win, A_BOLD);

	/*
	 * do final stuff the put all together
	 */
	wrefresh(dialog_win);
}

int 
input_dialog(InputField *fields, int num_fields, char *title)
{
	int ch, i;
	char *ret;
	WINDOW *dialog_win;
	char msg[] = "(press 'q' to return to list)";
	char msg2[80];
	/*
	 * initialize the info window
	 */
	if(title){
		disp_h = ((num_fields+2) * 2) + 3;
	} else {
		disp_h = ((num_fields+1) * 2) + 3;
	}
	
	dialog_win = newwin(disp_h, disp_w, (LINES - disp_h)/2, (COLS - disp_w)/2);
	keypad(dialog_win, TRUE);

	input_dialog_draw_items(dialog_win, fields, num_fields, title, msg);
	/*
	 * actions loop
	 */
	while((ch = wgetch(dialog_win)) != 'q'){
		if( (ch >= '1') && (ch <= NUM_TO_CHAR(num_fields)) ){
			i = CHAR_TO_NUM(ch);
			if(fields[i].autogen != NULL){
				fields[i].value = (void*)statusline_ask_str_with_autogen(fields[i].name, (char*)fields[i].value, fields[i].max_length, fields[i].autogen, 0x07); 
			} else if(fields[i].type == STRING){
				fields[i].value = (void*)statusline_ask_str(fields[i].name, (char*)fields[i].value, fields[i].max_length);
			} else if(fields[i].type == INT){
				statusline_ask_num(fields[i].name, (int*)fields[i].value);
			}
			input_dialog_draw_items(dialog_win, fields, num_fields, title, msg);
		} else if(ch == 'l'){
			delwin(dialog_win);
			list_launch();
			break;
		}	
	}
	/*
	 * clean up
	 */
	delwin(dialog_win);
	refresh_list();
}

int yes_no_dialog(InputField *fields, int num_fields, char *title, char *question)
{
	int ch, i;
	char *ret;
	WINDOW *dialog_win;

	/*
	 * initialize the info window
	 */
	if(title){
		disp_h = ((num_fields+2) * 2) + 3;
	} else {
		disp_h = ((num_fields+1) * 2) + 3;
	}
	
	dialog_win = newwin(disp_h, disp_w, (LINES - disp_h)/2, (COLS - disp_w)/2);
	keypad(dialog_win, TRUE);

	input_dialog_draw_items(dialog_win, fields, num_fields, title, NULL);
	
	i = statusline_yes_no(question, 1);

	/*
	 * clean up
	 */
	delwin(dialog_win);
	refresh_list();

	return i;
}

int
list_add_sublist()
{
	char *name;
	PWList *sublist, *iter;

	name = malloc(STRING_MEDIUM);
	statusline_ask_str("Sublist Name:", name, STRING_MEDIUM);
	for(iter = current_pw_sublist->sublists; iter != NULL; iter = iter->next){
		if( strcmp(iter->name, name) == 0){
			free(name);
			return -1;
		}
	}
	sublist = new_pwlist(name);

	add_pw_sublist(current_pw_sublist, sublist);
	refresh_list();
}

int
list_select_item()
{
	Pw* curpw;
	PWList* curpwl;

	switch(get_highlighted_type()){
		case PW_ITEM:
			curpw = get_highlighted_item();
			if(curpw){
				edit_pw(curpw);
			}
			break;
		case PW_SUBLIST:
			curpwl = get_highlighted_sublist();
			if(curpwl){
				current_pw_sublist = curpwl;
				refresh_list();
			}
			break;
		case PW_UPLEVEL:
			list_up_one_level();
			break;
		case PW_NULL:
		default:
			/* do nothing */
			break;
	}
}

int
list_delete_item()
{
	Pw* curpw;
	PWList* curpwl;
	int i;
	char str[STRING_LONG];
	
	switch(get_highlighted_type()){
		case PW_ITEM:
			curpw = get_highlighted_item();
			if(curpw){
				snprintf(str, STRING_LONG, "Really delete \"%s\"", curpw->name);
				i = statusline_yes_no(str, 0);
				if(i){
					delete_pw(current_pw_sublist, curpw);
					statusline_msg("Password deleted");
				} else {
					statusline_msg("Password not deleted");
				}	
			}
			break;
		case PW_SUBLIST:
			curpwl = get_highlighted_sublist();
			if(curpwl){
				snprintf(str, STRING_LONG, "Really delete Sublist \"%s\"", curpwl->name);
				i = statusline_yes_no(str, 0);
				if(i){
					delete_pw_sublist(curpwl->parent, curpwl);
					statusline_msg("Password Sublist deleted");
				} else {
					statusline_msg("Password not deleted");
				}
			}
			break;
		case PW_UPLEVEL:
		case PW_NULL:
		default:
			/* do nothing */
			break;
	}
	refresh_list();
}

int
list_move_item()
{
	Pw* curpw;
	PWList *curpwl, *iter;
	int i;
	char str[STRING_LONG];
	char answer[STRING_MEDIUM];

	switch(get_highlighted_type()){
		case PW_ITEM:
			curpw = get_highlighted_item();
			if(curpw){
				while(1){
					snprintf(str, STRING_LONG, "Move \"%s\" to where?", curpw->name);
					statusline_ask_str(str, answer, STRING_MEDIUM);
					
					/* if user just enters nothing do nothing */
					if(answer[0] == 0){
						return 0;
					}
					
					for(iter = current_pw_sublist->sublists; iter != NULL; iter = iter->next){
						if( strcmp(iter->name, answer) == 0 ){
							detach_pw(current_pw_sublist, curpw);
							add_pw_ptr(iter, curpw);
							refresh_list();
							return 0;
						}
					}
					statusline_msg("Sublist does not exist, try again");
					getch();
				}
			}
			break;
		case PW_SUBLIST:
			curpwl = get_highlighted_sublist();
			if(curpwl){
				while(1){
					snprintf(str, STRING_LONG, "Move sublist \"%s\" to where?", curpwl->name);
					statusline_ask_str(str, answer, STRING_MEDIUM);
					
					/* if user just enters nothing, do nothing */
					if(answer[0] == 0){
						return 0;
					}
					if( strcmp(answer, curpwl->name) == 0 ){
						return 0;
					}

					for(iter = current_pw_sublist->sublists; iter != NULL; iter = iter->next){
						if( strcmp(iter->name, answer) == 0 ){
							detach_pw_sublist(current_pw_sublist, curpwl);
							add_pw_sublist(iter, curpwl);
							refresh_list();
							return 0;
						}
					}
					statusline_msg("Sublist does not exist, try again");
					getch();
				}
			}
			break;
		case PW_UPLEVEL:
		case PW_NULL:
		default:
			/* do nothing */
			break;
	}
}

int
list_move_item_up_level()
{
	Pw* curpw;
	PWList *curpwl, *iter;
	int i;
	char str[STRING_LONG];
	char answer[STRING_MEDIUM];

	switch(get_highlighted_type()){
		case PW_ITEM:
			curpw = get_highlighted_item();
			if(curpw && current_pw_sublist->parent){
				detach_pw(current_pw_sublist, curpw);
				add_pw_ptr(current_pw_sublist->parent, curpw);
				refresh_list();
			}
			break;
		case PW_SUBLIST:
			curpwl = get_highlighted_sublist();
			if(curpwl && current_pw_sublist->parent){
				detach_pw_sublist(current_pw_sublist, curpwl);
				add_pw_sublist(current_pw_sublist->parent, curpwl);
				refresh_list();
			}
			break;
		case PW_UPLEVEL:
		case PW_NULL:
		default:
			/* do nothing */
			break;
	}
}

int
list_up_one_level()
{
	/* move up one sublist */
	if(current_pw_sublist->parent){
		current_pw_sublist = current_pw_sublist->parent;
		refresh_list();
	}
}
	
int
list_export()
{
	Pw* curpw;
	PWList *curpwl;

	debug("list_export: enter switch");
	switch(get_highlighted_type()){
		case PW_ITEM:
			debug("list_export: is a pw");
			curpw = get_highlighted_item();
			if(curpw){
				export_passwd(curpw);
			}
			break;
		case PW_SUBLIST:
			debug("list_export: is a pwlist");
			curpwl = get_highlighted_sublist();
			if(curpwl){
				export_passwd_list(curpwl);
			}
			break;
		case PW_UPLEVEL:
		case PW_NULL:
		default:
			/* do nothing */
			break;
	}
}

int
list_launch()
{
	int i;
	Pw* curpw;
	char msg[STRING_LONG];

	switch(get_highlighted_type()){
		case PW_ITEM:
			debug("list_launch: is a pw");
			curpw = get_highlighted_item();
			if(curpw){
				i = launch(curpw);
				snprintf(msg, STRING_LONG, "Application exited with code %d", i);
				statusline_msg(msg);
			}
			break;
		case PW_SUBLIST:
		case PW_UPLEVEL:
		case PW_NULL:
		default:
			/* do nothing */
			break;
	}
}

int
list_read_file()
{
	free_database();
	if(read_file() != 0){
		pwlist = new_pwlist("Main");
		current_pw_sublist = pwlist;
	}
	refresh_list();
	return -1;
}
