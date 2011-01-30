/*
 *  PWMan - password manager application
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
 * all code in this file is taken from pwgen
 * (c) 2001 Theodore Ts'o
 */

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <pwman.h>
#include <ui.h>

struct pwgen_element {
	char	*str;
	int	flags;
};

/*
 * Flags for the pw_element
 */
#define CONSONANT	0x0001
#define VOWEL		0x0002
#define DIPTHONG	0x0004
#define NOT_FIRST	0x0008


struct pwgen_element elements[] = {
	{ "a",	VOWEL },
	{ "ae", VOWEL | DIPTHONG },
	{ "ah",	VOWEL | DIPTHONG },
	{ "ai", VOWEL | DIPTHONG },
	{ "b",  CONSONANT },
	{ "c",	CONSONANT },
	{ "ch", CONSONANT | DIPTHONG },
	{ "d",	CONSONANT },
	{ "e",	VOWEL },
	{ "ee", VOWEL | DIPTHONG },
	{ "ei",	VOWEL | DIPTHONG },
	{ "f",	CONSONANT },
	{ "g",	CONSONANT },
	{ "gh", CONSONANT | DIPTHONG | NOT_FIRST },
	{ "h",	CONSONANT },
	{ "i",	VOWEL },
	{ "ie", VOWEL | DIPTHONG },
	{ "j",	CONSONANT },
	{ "k",	CONSONANT },
	{ "l",	CONSONANT },
	{ "m",	CONSONANT },
	{ "n",	CONSONANT },
	{ "ng",	CONSONANT | DIPTHONG | NOT_FIRST },
	{ "o",	VOWEL },
	{ "oh",	VOWEL | DIPTHONG },
	{ "oo",	VOWEL | DIPTHONG},
	{ "p",	CONSONANT },
	{ "ph",	CONSONANT | DIPTHONG },
	{ "qu",	CONSONANT | DIPTHONG},
	{ "r",	CONSONANT },
	{ "s",	CONSONANT },
	{ "sh",	CONSONANT | DIPTHONG},
	{ "t",	CONSONANT },
	{ "th",	CONSONANT | DIPTHONG},
	{ "u",	VOWEL },
	{ "v",	CONSONANT },
	{ "w",	CONSONANT },
	{ "x",	CONSONANT },
	{ "y",	CONSONANT },
	{ "z",	CONSONANT }
};

#define NUM_ELEMENTS (sizeof(elements) / sizeof (struct pwgen_element))

char *pwgen(char *buf, int size)
{
	int	c, i, len, flags, feature_flags;
	int	prev, should_be, first;
	char	*str;

	if(buf == NULL){
		buf = malloc(size);
	}

	c = 0;
	prev = 0;
	should_be = 0;
	first = 1;

	should_be = pwgen_random_number(1) ? VOWEL : CONSONANT;
	
	while (c < size) {
		i = pwgen_random_number(NUM_ELEMENTS);
		str = elements[i].str;
		len = strlen(str);
		flags = elements[i].flags;
		/* Filter on the basic type of the next element */
		if ((flags & should_be) == 0)
			continue;
		/* Handle the NOT_FIRST flag */
		if (first && (flags & NOT_FIRST))
			continue;
		/* Don't allow VOWEL followed a Vowel/Dipthong pair */
		if ((prev & VOWEL) && (flags & VOWEL) &&
		    (flags & DIPTHONG))
			continue;
		/* Don't allow us to overflow the buffer */
		if (len > size-c)
			continue;
		/*
		 * OK, we found an element which matches our criteria,
		 * let's do it!
		 */
		strcpy(buf+c, str);

		/* Handle PW_ONE_CASE */
		if ((first || flags & CONSONANT) && (pwgen_random_number(10) < 3)) {
			buf[c] = toupper(buf[c]);
		}
		
		c += len;
		
		/* Time to stop? */
		if (c >= size)
			break;
		
		/*
		 * Handle PW_ONE_NUMBER
		 */
		if (!first && (pwgen_random_number(10) < 3)) {
			buf[c++] = pwgen_random_number(9)+'0';
			buf[c] = 0;
				
			first = 1;
			prev = 0;
			should_be = pwgen_random_number(1) ? VOWEL : CONSONANT;
			continue;
		}
				
		/*
		 * OK, figure out what the next element should be
		 */
		if (should_be == CONSONANT) {
			should_be = VOWEL;
		} else { /* should_be == VOWEL */
			if ((prev & VOWEL) ||
			    (flags & DIPTHONG) ||
			    (pwgen_random_number(10) > 3))
				should_be = CONSONANT;
			else
				should_be = VOWEL;
		}
		prev = flags;
		first = 0;
	}

	return buf;
}

#ifdef HAVE_DRAND48
extern double drand48();
#endif

/* Borrowed/adapted from e2fsprogs's UUID generation code */
static int pwgen_get_random_fd(void)
{
	struct timeval	tv;
	static int	fd = -2;
	int		i;

	if (fd == -2) {
		gettimeofday(&tv, 0);
		fd = open("/dev/urandom", O_RDONLY);
		if (fd == -1)
			fd = open("/dev/random", O_RDONLY | O_NONBLOCK);
#ifdef HAVE_DRAND48
		srand48((tv.tv_sec<<9) ^ (getpgrp()<<15) ^
			(getpid()) ^ (tv.tv_usec>>11));
#else
		srandom((getpid() << 16) ^ (getpgrp() << 8) ^ getuid() 
		      ^ tv.tv_sec ^ tv.tv_usec);
#endif
	}
	/* Crank the random number generator a few times */
	gettimeofday(&tv, 0);
	for (i = (tv.tv_sec ^ tv.tv_usec) & 0x1F; i > 0; i--)
#ifdef HAVE_DRAND48
		drand48();
#else
		rand();
#endif
	return fd;
}

/*
 * Generate a random number n, where 0 <= n < max_num, using
 * /dev/urandom if possible.
 */
int pwgen_random_number(int max_num)
{
	int i, fd = pwgen_get_random_fd();
	int lose_counter = 0, nbytes=4;
	unsigned int rand;
	char *cp = (char *) &rand;

	if (fd >= 0) {
		while (nbytes > 0) {
			i = read(fd, cp, nbytes);
			if ((i < 0) &&
			    ((errno == EINTR) || (errno == EAGAIN)))
				continue;
			if (i <= 0) {
				if (lose_counter++ == 8)
					break;
				continue;
			}
			nbytes -= i;
			cp += i;
			lose_counter = 0;
		}
	}
	if (nbytes == 0)
		return (rand % max_num);

	/* OK, we weren't able to use /dev/random, fall back to rand/rand48 */

#ifdef RAND48
	return ((int) ((drand48() * max_num)));
#else
	return ((int) (random() / ((float) RAND_MAX) * max_num));
#endif
}

char 
*pwgen_ask(char *pw)
{
	int i;
	ui_statusline_ask_num("Length of Password(default 5):\t", &i);

	if(i == 0){
		i = 5;
	} else if(i > STRING_SHORT) {
		i = STRING_SHORT;
	}
	pw = pwgen(pw, i);

	return pw;
}

int 
pwgen_indep()
{
	char pass[STRING_SHORT], text[STRING_LONG];

	pwgen_ask(pass);

	snprintf(text, STRING_LONG, "Generated Password: %s", pass);
	ui_statusline_msg(text);
}
