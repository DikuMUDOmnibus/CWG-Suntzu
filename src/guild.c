/* ************************************************************************
 *   File: Guild.c                                                         *
 *  Usage: GuildMaster's: loading files, assigning spec_procs, and handling*
 *                        practicing.                                      *
 *                                                                         *
 * Based on shop.c.  As such, the CircleMud License applies                *
 * Written by Jason Goodwin.   jgoodwin@expert.cc.purdue.edu               *
 ************************************************************************ */


#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"

#include "comm.h"
#include "constants.h"
#include "db.h"
#include "guild.h"
#include "handler.h"
#include "interpreter.h"
#include "spells.h"
#include "screen.h"
#include "gengld.h"
#include "shop.h"


/* extern variables */
extern struct spell_info_type spell_info[];
extern struct time_info_data time_info;
extern const char *trade_letters[];
extern int cmd_say, cmd_tell;

/* extern function prototypes */
ACMD(do_tell);
ACMD(do_say);

/* Local variables */
struct guild_data *guild_index;
int spell_sort_info[MAX_SKILLS + 1];
int top_guild = -1;
char *guild_customer_string(int guild_nr, int detailed);

const char *how_good(int percent)
{
  if (percent < 0)
    return " error)";
  if (percent == 0)
    return "(@Mnot@n)";
  if (percent <= 10)
    return "(@rawful@n)";
  if (percent <= 20)
    return "(@Rbad@n)";
  if (percent <= 40)
    return "(@ypoor@n)";
  if (percent <= 55)
    return "(@Yaverage@n)";
  if (percent <= 70)
    return "(@gfair@n)";
  if (percent <= 80)
    return "(@Ggood@n)";
  if (percent <= 85)
    return "(@bgreat@n)";
  if (percent <= 100)
    return "(@Bsuperb@n)";

  return "(@rinate@n)";
}

const char *prac_types[] = {
  "spell",
  "skill"
};


int compare_spells(const void *x, const void *y)
{
  int	a = *(const int *)x,
	b = *(const int *)y;

  return strcmp(spell_info[a].name, spell_info[b].name);
}


void list_skills_perct(struct char_data *ch, struct char_data *vict)
{
  const char *overflow = "\r\n**OVERFLOW**\r\n";
  int i, sortpos;
  size_t len = 0, nlen;
  char buf2[MAX_STRING_LENGTH];

  if (!GET_PRACTICES(vict)) {
    send_to_char(ch, "They have no practice sessions remaining.\r\n");
    return;
  }

  len = snprintf(buf2, sizeof(buf2), 
					  "%s has %d practice session%s remaining.\r\n"
					  "They know of the following skills:\r\n", 
					  GET_NAME(vict), GET_PRACTICES(vict),
					  GET_PRACTICES(vict) == 1 ? "" : "s");
  
  for (sortpos = MAX_SPELLS + 1; sortpos <= MIN_LANGUAGES - 1; sortpos++) {
    i = sortpos; /* spell_sort_info[sortpos]; */
    if (GET_LEVEL(vict) >= spell_info[i].min_level[(int) GET_CLASS(vict)] || GET_LEVEL(ch) >= spell_info[i].min_rlevel[(int) GET_RACE(ch)]) {
      nlen = snprintf(buf2 + len, sizeof(buf2) - len, 
							 "%-20s %3d\r\n", 
							 spell_info[i].name, GET_SKILL(vict, i));
      if (len + nlen >= sizeof(buf2) || nlen < 0)
        break;
      len += nlen;
    }
  }

  len += snprintf(buf2 + len, sizeof(buf2) - len,
						"\r\nand the following spells:\r\n");
  for (sortpos = 1; sortpos <= MAX_SPELLS; sortpos++) {
    i = sortpos; /* spell_sort_info[sortpos]; */
    if (GET_LEVEL(vict) >= spell_info[i].min_level[(int) GET_CLASS(vict)] || GET_LEVEL(ch) >= spell_info[i].min_rlevel[(int) GET_RACE(ch)]) {
      nlen = snprintf(buf2 + len, sizeof(buf2) - len, 
							 "%-20s %3d\r\n", 
							 spell_info[i].name, GET_SKILL(vict, i));
      if (len + nlen >= sizeof(buf2) || nlen < 0)
        break;
      len += nlen;
    }
  }

  	len += snprintf(buf2 + len, sizeof(buf2) - len,
						 "\r\nand the following weapon types:\r\n");
	for (sortpos = MIN_WP ; sortpos <= MAX_WP ; sortpos++) {
		i = sortpos; /* spell_sort_info[sortpos]; */
		if (GET_LEVEL(ch) >= spell_info[i].min_level[(int) GET_CLASS(ch)] || GET_LEVEL(ch) >= spell_info[i].min_rlevel[(int) GET_RACE(ch)]) {
			nlen = snprintf(buf2 + len, sizeof(buf2) - len, 
								 "%-20s %s\r\n", 
								 spell_info[i].name, how_good(GET_SKILL(ch, i)));
			if (len + nlen >= sizeof(buf2) || nlen < 0)
				break;
			len += nlen;
		}
	}


        if (CONFIG_ENABLE_LANGUAGES) {
  	len += snprintf(buf2 + len, sizeof(buf2) - len,
						 "\r\nand the following languages:\r\n");
	for (sortpos = MIN_LANGUAGES ; sortpos <= MAX_LANGUAGES ; sortpos++) {
		i = sortpos; /* spell_sort_info[sortpos]; */
		if (GET_LEVEL(ch) >= spell_info[i].min_level[(int) GET_CLASS(ch)] || GET_LEVEL(ch) >= spell_info[i].min_rlevel[(int) GET_RACE(ch)]) {
			nlen = snprintf(buf2 + len, sizeof(buf2) - len, 
								 "%-20s %s\r\n", 
								 spell_info[i].name, how_good(GET_SKILL(ch, i)));
			if (len + nlen >= sizeof(buf2) || nlen < 0)
				break;
			len += nlen;
		}
	}
        }

  if (len >= sizeof(buf2))
    strcpy(buf2 + sizeof(buf2) - strlen(overflow) - 1, overflow); /* strcpy: OK */

  page_string(ch->desc, buf2, TRUE);
}


void list_skills(struct char_data *ch)
{
  const char *overflow = "\r\n**OVERFLOW**\r\n";
  int i, sortpos;
  size_t len = 0, nlen;
  char buf2[MAX_STRING_LENGTH];

  /* if (!GET_PRACTICES(ch)) {
    send_to_char(ch, "You have no practice sessions remaining.\r\n");
    return;
  } */

  len = snprintf(buf2, sizeof(buf2), 
					  "You have %d practice session%s remaining.\r\n"
					  "You know of the following skills:\r\n", GET_PRACTICES(ch),
					  GET_PRACTICES(ch) == 1 ? "" : "s");
  
  for (sortpos = MAX_SPELLS + 1; sortpos <= MIN_LANGUAGES - 1; sortpos++) {
    i = sortpos; /* spell_sort_info[sortpos]; */
    if (GET_LEVEL(ch) >= spell_info[i].min_level[(int) GET_CLASS(ch)] || GET_LEVEL(ch) >= spell_info[i].min_rlevel[(int) GET_RACE(ch)]) {
      nlen = snprintf(buf2 + len, sizeof(buf2) - len, 
							 "%-20s %s\r\n", 
							 spell_info[i].name, how_good(GET_SKILL(ch, i)));
      if (len + nlen >= sizeof(buf2) || nlen < 0)
        break;
      len += nlen;
    }
  }

  len += snprintf(buf2 + len, sizeof(buf2) - len,
						"\r\n\r\nand the following spells:\r\n");
  for (sortpos = 1; sortpos <= MAX_SPELLS; sortpos++) {
    i = sortpos; /* spell_sort_info[sortpos]; */
    if (GET_LEVEL(ch) >= spell_info[i].min_level[(int) GET_CLASS(ch)] || GET_LEVEL(ch) >= spell_info[i].min_rlevel[(int) GET_RACE(ch)]) {
      nlen = snprintf(buf2 + len, sizeof(buf2) - len, 
							 "%-20s %s\r\n", 
							 spell_info[i].name, how_good(GET_SKILL(ch, i)));
      if (len + nlen >= sizeof(buf2) || nlen < 0)
        break;
      len += nlen;
    }
  }

  	len += snprintf(buf2 + len, sizeof(buf2) - len,
						 "\r\nand the following weapon types:\r\n");
	for (sortpos = MIN_WP ; sortpos <= MAX_WP ; sortpos++) {
		i = sortpos; /* spell_sort_info[sortpos]; */
		if (GET_LEVEL(ch) >= spell_info[i].min_level[(int) GET_CLASS(ch)] || GET_LEVEL(ch) >= spell_info[i].min_rlevel[(int) GET_RACE(ch)]) {
			nlen = snprintf(buf2 + len, sizeof(buf2) - len, 
								 "%-20s %s\r\n", 
								 spell_info[i].name, how_good(GET_SKILL(ch, i)));
			if (len + nlen >= sizeof(buf2) || nlen < 0)
				break;
			len += nlen;
		}
	}

        if (CONFIG_ENABLE_LANGUAGES) {
  	len += snprintf(buf2 + len, sizeof(buf2) - len,
						 "\r\nand the following languages:\r\n");
	for (sortpos = MIN_LANGUAGES ; sortpos <= MAX_LANGUAGES ; sortpos++) {
		i = sortpos; /* spell_sort_info[sortpos]; */
		if (GET_LEVEL(ch) >= spell_info[i].min_level[(int) GET_CLASS(ch)] || GET_LEVEL(ch) >= spell_info[i].min_rlevel[(int) GET_RACE(ch)]) {
			nlen = snprintf(buf2 + len, sizeof(buf2) - len, 
								 "%-20s %s\r\n", 
								 spell_info[i].name, how_good(GET_SKILL(ch, i)));
			if (len + nlen >= sizeof(buf2) || nlen < 0)
				break;
			len += nlen;
		}
	}
        }

  if (len >= sizeof(buf2))
    strcpy(buf2 + sizeof(buf2) - strlen(overflow) - 1, overflow); /* strcpy: OK */

  page_string(ch->desc, buf2, TRUE);
}


int is_guild_open(struct char_data *keeper, int guild_nr, int msg)
{
	char buf[200];
	*buf = 0;

	if (GM_OPEN(guild_nr) > time_info.hours &&
		 GM_CLOSE(guild_nr) < time_info.hours)
		strlcpy(buf, MSG_TRAINER_NOT_OPEN, sizeof(buf));

	if (!*buf)
		return (TRUE);
	if (msg)
		do_say(keeper, buf, cmd_tell, 0);

	return (FALSE);
}


int is_guild_ok_char(struct char_data * keeper, struct char_data * ch, int guild_nr)
{
	char buf[200];

	if (!(CAN_SEE(keeper, ch))) {
		do_say(keeper, MSG_TRAINER_NO_SEE_CH, cmd_say, 0);
		return (FALSE);
	}

	if (GET_LEVEL(ch) >= LVL_IMMORT) {
		snprintf(buf, sizeof(buf), "%s %s", 
					GET_NAME(ch), "You should be training me!");
		do_tell(keeper, buf, cmd_tell, 0); 
		return (FALSE);
	}
		
	
	if (GET_LEVEL(ch) < GM_MINLVL(guild_nr)) {
				snprintf(buf, sizeof(buf), "%s %s", 
					GET_NAME(ch), MSG_TRAINER_MINLVL);
		do_tell(keeper, buf, cmd_tell, 0); 
		return (FALSE);
	}


	if ((IS_GOOD(ch) && NOTRAIN_GOOD(guild_nr)) ||
		 (IS_EVIL(ch) && NOTRAIN_EVIL(guild_nr)) ||
		 (IS_NEUTRAL(ch) && NOTRAIN_NEUTRAL(guild_nr))) {
		snprintf(buf, sizeof(buf), "%s %s", 
					GET_NAME(ch), MSG_TRAINER_DISLIKE_ALIGN);
		do_tell(keeper, buf, cmd_tell, 0);
		return (FALSE);
	}

	if (IS_NPC(ch))
		return (FALSE);

	if ((IS_MAGIC_USER(ch) && NOTRAIN_MAGIC_USER(guild_nr)) ||
		(IS_CLERIC(ch) && NOTRAIN_CLERIC(guild_nr)) ||
		(IS_THIEF(ch) && NOTRAIN_THIEF(guild_nr)) ||
		(IS_WARRIOR(ch) && NOTRAIN_WARRIOR(guild_nr))) {
		snprintf(buf, sizeof(buf), "%s %s", 
					GET_NAME(ch), MSG_TRAINER_DISLIKE_CLASS);
		do_tell(keeper, buf, cmd_tell, 0);
		return (FALSE);
	}

	if ((IS_HUMAN(ch) && NOTRAIN_HUMAN(guild_nr)) ||
		 (IS_ELF(ch)   && NOTRAIN_ELF(guild_nr)) ||
		 (IS_GNOME(ch) && NOTRAIN_GNOME(guild_nr)) ||
		 (IS_DWARF(ch) && NOTRAIN_DWARF(guild_nr)) ||
		 (IS_HALF_ELF(ch) && NOTRAIN_HALF_ELF(guild_nr)) ||
		 (IS_HALFLING(ch) && NOTRAIN_HALFLING(guild_nr)) ||
		 (IS_DROW_ELF(ch) && NOTRAIN_DROW_ELF(guild_nr)) ||
 		 (IS_ANIMAL(ch) && NOTRAIN_ANIMAL(guild_nr)) ||
		 (IS_CONSTRUCT(ch) && NOTRAIN_CONSTRUCT(guild_nr)) ||
		 (IS_DEMON(ch) && NOTRAIN_DEMON(guild_nr)) ||
 		 (IS_DRAGON(ch) && NOTRAIN_DRAGON(guild_nr)) ||
 		 (IS_FISH(ch) && NOTRAIN_FISH(guild_nr)) ||
 		 (IS_GIANT(ch) && NOTRAIN_GIANT(guild_nr)) ||
 		 (IS_GOBLIN(ch) && NOTRAIN_GOBLIN(guild_nr)) ||
 		 (IS_INSECT(ch) && NOTRAIN_INSECT(guild_nr)) ||
 		 (IS_ORC(ch) && NOTRAIN_ORC(guild_nr)) ||
 		 (IS_SNAKE(ch) && NOTRAIN_SNAKE(guild_nr)) ||
		 (IS_TROLL(ch) && NOTRAIN_TROLL(guild_nr)) ||
 		 (IS_HALF_ORC(ch) && NOTRAIN_HALF_ORC(guild_nr)) ||
 		 (IS_MINOTAUR(ch) && NOTRAIN_MINOTAUR(guild_nr)) ||
 		 (IS_KOBOLD(ch) && NOTRAIN_KOBOLD(guild_nr)) ||
		 (IS_MINDFLAYER(ch) && NOTRAIN_MINDFLAYER(guild_nr))) {
		snprintf(buf, sizeof(buf), "%s %s", 
					GET_NAME(ch), MSG_TRAINER_DISLIKE_RACE);
		do_tell(keeper, buf, cmd_tell, 0);
		return (FALSE);
	}
	return (TRUE);
}


int is_guild_ok(struct char_data * keeper, struct char_data * ch, int guild_nr)
{
	if (is_guild_open(keeper, guild_nr, TRUE))
		return (is_guild_ok_char(keeper, ch, guild_nr));

	return (FALSE);
}


int does_guild_know(int guild_nr, int i)
{
	return ((int)(guild_index[guild_nr].skills_and_spells[i]));
}


void sort_spells(void)
{
  int a;

  /* initialize array, avoiding reserved. */
  for (a = 1; a <= MAX_SKILLS; a++)
    spell_sort_info[a] = a;

  qsort(&spell_sort_info[1], MAX_SKILLS, sizeof(int), compare_spells);
}


/* this and list skills should probally be combined.  perhaps in the
 * next release?  */
void what_does_guild_know(int guild_nr, struct char_data * ch)
{
  const char *overflow = "\r\n**OVERFLOW**\r\n";
  char buf2[MAX_STRING_LENGTH];
  int i, sortpos;
  size_t nlen = 0, len = 0;

  /*if (!GET_PRACTICES(ch)) {
    send_to_char(ch, "You have no practice sessions remaining.\r\n");
    return;
  }*/

  len = snprintf(buf2, sizeof(buf2), "You have %d practice session%s remaining.\r\n", GET_PRACTICES(ch), GET_PRACTICES(ch) == 1 ? "" : "s"); // ???

  nlen = snprintf(buf2 + len, sizeof(buf2) - len, "I can teach you the following skills:\r\n");
  len += nlen;
  
  /* Need to check if trainer can train doesnt do it now ??? */
  for (sortpos = MAX_SPELLS + 1; sortpos <= MIN_LANGUAGES - 1; sortpos++) {
    i = sortpos; /* spell_sort_info[sortpos]; */
    if (does_guild_know(guild_nr, sortpos)) {
      if (GET_LEVEL(ch) >= spell_info[i].min_level[(int) GET_CLASS(ch)] || GET_LEVEL(ch) >= spell_info[i].min_rlevel[(int) GET_RACE(ch)]) {
        if (GET_SKILL(ch, i) < LEARNED(ch)) {
          nlen = snprintf(buf2 + len, sizeof(buf2) - len, "%-20s %s\r\n", spell_info[i].name, how_good(GET_SKILL(ch, i)));
          if (len + nlen >= sizeof(buf2) || nlen < 0)
            break;
          len += nlen;
        }
      }
    }
  }

  len += snprintf(buf2 + len, sizeof(buf2) - len, "\r\n\r\nand the following spells:\r\n");

  for (sortpos = 1; sortpos <= MAX_SPELLS; sortpos++) {
    i = sortpos; /* spell_sort_info[sortpos]; */
      if (does_guild_know(guild_nr, sortpos)) {
        if (GET_LEVEL(ch) >= spell_info[i].min_level[(int) GET_CLASS(ch)] || GET_LEVEL(ch) >= spell_info[i].min_rlevel[(int) GET_RACE(ch)]) {
          if (GET_SKILL(ch, i) < LEARNED(ch)) {
            nlen = snprintf(buf2 + len, sizeof(buf2) - len, "%-20s %s\r\n", spell_info[i].name, how_good(GET_SKILL(ch, i)));
            if (len + nlen >= sizeof(buf2) || nlen < 0)
              break;
            len += nlen;
          }
        }
      }
  }

  len += snprintf(buf2 + len, sizeof(buf2) - len, "\r\nand the following weapon types:\r\n");

  for (sortpos = MIN_WP ; sortpos <= MAX_WP ; sortpos++) {
    i = sortpos; /* spell_sort_info[sortpos]; */
    if (does_guild_know(guild_nr, sortpos)) {
      if (GET_LEVEL(ch) >= spell_info[i].min_level[(int) GET_CLASS(ch)] || GET_LEVEL(ch) >= spell_info[i].min_rlevel[(int) GET_RACE(ch)]) {
        if (GET_SKILL(ch, i) < LEARNED(ch)) {
          nlen = snprintf(buf2 + len, sizeof(buf2) - len, "%-20s %s\r\n", spell_info[i].name, how_good(GET_SKILL(ch, i)));
          if (len + nlen >= sizeof(buf2) || nlen < 0)
            break;
          len += nlen;
        }
      }
    }
  }

  if (CONFIG_ENABLE_LANGUAGES) {
  len += snprintf(buf2 + len, sizeof(buf2) - len, "\r\nand the following languages:\r\n");

  for (sortpos = MIN_LANGUAGES ; sortpos <= MAX_LANGUAGES ; sortpos++) {
    i = sortpos; /* spell_sort_info[sortpos]; */
    if (does_guild_know(guild_nr, sortpos)) {
      if (GET_LEVEL(ch) >= spell_info[i].min_level[(int) GET_CLASS(ch)] || GET_LEVEL(ch) >= spell_info[i].min_rlevel[(int) GET_RACE(ch)]) {
        if (GET_SKILL(ch, i) < LEARNED(ch)) {
          nlen = snprintf(buf2 + len, sizeof(buf2) - len, "%-20s %s\r\n", spell_info[i].name, how_good(GET_SKILL(ch, i)));
          if (len + nlen >= sizeof(buf2) || nlen < 0)
            break;
          len += nlen;
        }
      }
    }
  }
  }
  if (len >= sizeof(buf2))
    strcpy(buf2 + sizeof(buf2) - strlen(overflow) - 1, overflow); /* strcpy: OK */

  page_string(ch->desc, buf2, TRUE);
}


SPECIAL(guild)
{
	char arg[MAX_INPUT_LENGTH];
	char buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
	int skill_num, percent;
	int guild_nr;
	struct char_data *keeper = (struct char_data *) me;

	for (guild_nr = 0; guild_nr <= top_guild; guild_nr++)
		if (GM_TRAINER(guild_nr) == keeper->nr)
			break;

	if (guild_nr > top_guild)
		return (FALSE);

	if (GM_FUNC(guild_nr))
		if ((GM_FUNC(guild_nr)) (ch, me, cmd, arg))
			return (TRUE);


	if (/*IS_NPC(ch) || */!CMD_IS("practice"))
		return (FALSE);

	skip_spaces(&argument);

/*** Is the GM able to train?    ****/
	if (!AWAKE(keeper))
		return (FALSE);

	if (!(is_guild_ok(keeper, ch, guild_nr)))
      return (TRUE);

	if (!*argument) {
		what_does_guild_know(guild_nr, ch);
		return (TRUE);
	}

	if (GET_PRACTICES(ch) <= 0) {
		send_to_char(ch, "You do not seem to be able to practice now.\r\n");
		return (TRUE);
	}

	skill_num = find_skill_num(argument);

/****  Does the GM know the skill the player wants to learn?  ****/
	if (!(does_guild_know(guild_nr, skill_num))) {
		snprintf(buf2, sizeof(buf2), 
					guild_index[guild_nr].no_such_skill, GET_NAME(ch));
		do_tell(keeper, buf2, cmd_tell, 0);
		return (TRUE);
	}

/**** Can the player learn the skill if the GM knows it?  ****/ 
	if (skill_num < 1 ||
		 GET_LEVEL(ch) < spell_info[skill_num].min_level[(int) GET_CLASS(ch)]) {
		send_to_char(ch, "You do not know of that %s.\r\n", SPLSKL(ch));
		return (TRUE);
	}

/****  Is the player maxxed out with the skill?  ****/
	if (GET_SKILL(ch, skill_num) >= LEARNED(ch)) {
		send_to_char(ch, "You are already learned in that area.\r\n");
		return (TRUE);
	}

/****  Does the Player have enough gold to train?  ****/
	if (GET_GOLD(ch) < (GM_COST(guild_nr,skill_num,ch))) {
		snprintf(buf1, sizeof(buf1), 
					guild_index[guild_nr].not_enough_gold, GET_NAME(ch));
		do_tell(keeper, buf1, cmd_tell, 0);
		return (TRUE);
	}

/****  If we've made it this far, then its time to practice  ****/
	send_to_char(ch, "You practice for a while...\r\n");
	GET_PRACTICES(ch)--;

	GET_GOLD(ch) -= GM_COST(guild_nr,skill_num,ch);

	percent = GET_SKILL(ch, skill_num);
	percent += MIN(MAXGAIN(ch), MAX(MINGAIN(ch), int_app[GET_INT(ch)].learn));

	SET_SKILL(ch, skill_num, MIN(LEARNED(ch), percent));

	if (GET_SKILL(ch, skill_num) >= LEARNED(ch))
		send_to_char(ch, "You are now learned in that area.\r\n");

	return (TRUE);
}


/**** This function is here just because I'm extremely paranoid.  Take
      it out if you aren't ;)  ****/
void clear_skills(int index)
{
	int i;

	for  (i = 0; i < MAX_SKILLS + 2; i++) 
		guild_index[index].skills_and_spells[i] = 0;
}


/****  This is ripped off of read_line from shop.c.  They could be
 *  combined. But why? ****/

void read_guild_line(FILE * gm_f, char *string, void *data, char *type)
{
	char buf[MAX_STRING_LENGTH];
	
	if (!get_line(gm_f, buf) || !sscanf(buf, string, data)) {
		fprintf(stderr, "Error in guild #%d, Could not get %s\n", GM_NUM(top_guild), type);
		exit(1);
	}
}


void boot_the_guilds(FILE * gm_f, char *filename, int rec_count)
{
	char *buf, buf2[256];
	int temp;
	int done = FALSE;

	snprintf(buf2, sizeof(buf2), "beginning of GM file %s", filename);

	while (!done) {
		buf = fread_string(gm_f, buf2);
		if (*buf == '#') {		/* New Trainer */
			sscanf(buf, "#%d\n", &temp);
			snprintf(buf2, sizeof(buf2), "GM #%d in GM file %s", temp, filename);
			free(buf);		/* Plug memory leak! */
			top_guild++;
			if (!top_guild)
				CREATE(guild_index, struct guild_data, rec_count);
			GM_NUM(top_guild) = temp;

			clear_skills(top_guild);    
			read_guild_line(gm_f, "%d", &temp, "TEMP1");
			while( temp > -1) {
				guild_index[top_guild].skills_and_spells[(int)temp] = 1;
				read_guild_line(gm_f, "%d", &temp, "TEMP2");
			}
                   
			read_guild_line(gm_f, "%f", &GM_CHARGE(top_guild), "GM_CHARGE");

			guild_index[top_guild].no_such_skill = fread_string(gm_f, buf2);
			guild_index[top_guild].not_enough_gold = fread_string(gm_f, buf2);

			read_guild_line(gm_f, "%d", &GM_MINLVL(top_guild), "GM_MINLVL");
			read_guild_line(gm_f, "%d", &GM_TRAINER(top_guild), "GM_TRAINER");

			GM_TRAINER(top_guild) = real_mobile(GM_TRAINER(top_guild));
			read_guild_line(gm_f, "%d", &GM_WITH_WHO(top_guild), "GM_WITH_WHO");

			read_guild_line(gm_f, "%d", &GM_OPEN(top_guild), "GM_OPEN");
			read_guild_line(gm_f, "%d", &GM_CLOSE(top_guild), "GM_CLOSE");

			GM_FUNC(top_guild) = NULL;
		} else {
			if (*buf == '$')		/* EOF */
				done = TRUE;
			free(buf);		/* Plug memory leak! */
		}
	}
}


void assign_the_guilds(void)
{
	int index;

	cmd_say = find_command("say");
	cmd_tell = find_command("tell");

	for (index = 0; index <= top_guild; index++) {
		if (GM_TRAINER(index) == NOBODY)
			continue;
		if (mob_index[GM_TRAINER(index)].func)
			GM_FUNC(index) = mob_index[GM_TRAINER(index)].func;
		mob_index[GM_TRAINER(index)].func = guild;
	}
}

char *guild_customer_string(int guild_nr, int detailed)
{
  int gindex = 0, flag = 1, nlen;
  size_t len = 0;
  static char buf[MAX_STRING_LENGTH];

  while (*trade_letters[gindex] != '\n' && len + 1 < sizeof(buf)) {
    if (detailed) {
      if (!IS_SET(flag, GM_WITH_WHO(guild_nr))) {
	nlen = snprintf(buf + len, sizeof(buf) - len, ", %s", trade_letters[gindex]);

        if (len + nlen >= sizeof(buf) || nlen < 0)
          break;

        len += nlen;
      }
    } else {
      buf[len++] = (IS_SET(flag, GM_WITH_WHO(guild_nr)) ? '_' : *trade_letters[gindex]);
      buf[len] = '\0';

      if (len >= sizeof(buf))
        break;
    }

    gindex++;
    flag <<= 1;
  }

  buf[sizeof(buf) - 1] = '\0';
  return (buf);
}

void list_all_guilds(struct char_data *ch)
{
  const char *list_all_guilds_header =
  "Virtual   G.Master	Charge   Members\r\n"
  "-------------------------------------------------------------\r\n";
  int gm_nr, headerlen = strlen(list_all_guilds_header);
  size_t len = 0;
  char buf[MAX_STRING_LENGTH], buf1[16];

  *buf = '\0';
  for (gm_nr = 0; gm_nr <= top_guild && len < sizeof(buf); gm_nr++) {
  /* New page in page_string() mechanism, print the header again. */
    if (!(gm_nr % (PAGE_LENGTH - 2))) {
    /*
     * If we don't have enough room for the header, or all we have room left
     * for is the header, then don't add it and just quit now.
     */
    if (len + headerlen + 1 >= sizeof(buf))
      break;
    strcpy(buf + len, list_all_guilds_header);	/* strcpy: OK (length checked above) */
    len += headerlen;
    }

    if (GM_TRAINER(gm_nr) == NOBODY)
      strcpy(buf1, "<NONE>");  /* strcpy: OK (for 'buf1 >= 7') */
    else
      sprintf(buf1, "%6d", mob_index[GM_TRAINER(gm_nr)].vnum);  /* sprintf: OK (for 'buf1 >= 11', 32-bit int) */	

    len += snprintf(buf + len, sizeof(buf) - len, "%6d	%s		%5.2f	%s\r\n",
      GM_NUM(gm_nr), buf1, GM_CHARGE(gm_nr), guild_customer_string(gm_nr, FALSE));
  }

  page_string(ch->desc, buf, TRUE);
}


void list_detailed_guild(struct char_data * ch, int gm_nr)
{
  int i;
  char buf[MAX_STRING_LENGTH];
  char buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];

  if (GM_TRAINER(gm_nr) < 0)
    strcpy(buf1, "<NONE>");
  else
    sprintf(buf1, "%6d   ", mob_index[GM_TRAINER(gm_nr)].vnum);

  sprintf(buf, " Guild Master: %s\r\n", buf1);
  sprintf(buf, "%s Hours: %4d to %4d,  Surcharge: %5.2f\r\n", buf,
			  GM_OPEN(gm_nr), GM_CLOSE(gm_nr), GM_CHARGE(gm_nr));
  sprintf(buf, "%s Min Level will train: %d\r\n", buf, GM_MINLVL(gm_nr));
  sprintf(buf, "%s Whom will train: %s\r\n", buf, guild_customer_string(gm_nr, TRUE));

   /* now for the REAL reason why someone would want to see a Guild :) */

  sprintf(buf, "%s The GM can teach the following:\r\n", buf);

  *buf2 = '\0';
  for (i = 0; i <= MAX_SKILLS; i++) {
    if (does_guild_know(gm_nr, i))
      sprintf(buf2, "%s %s \r\n", buf2, spell_info[i].name);
  }
 
  strcat(buf, buf2);

  page_string(ch->desc, buf, 1);
}
  

void show_guild(struct char_data * ch, char *arg)
{
  int gm_nr, gm_num;

  if (!*arg)
    list_all_guilds(ch);
  else {
    if (is_number(arg))
      gm_num = atoi(arg);
    else
      gm_num = -1;

    if (gm_num > 0) {
      for (gm_nr = 0; gm_nr <= top_guild; gm_nr++) {
      if (gm_num == GM_NUM(gm_nr))
        break; 
      }

      if (gm_num < 0 || gm_nr > top_guild) {
        send_to_char(ch, "Illegal guild master number.\n\r");
        return;
      }
      list_detailed_guild(ch, gm_nr);
    }
  }
}

/*
 * List all guilds in a zone.                              
 */                                                                           
void list_guilds(struct char_data *ch, zone_rnum rnum, guild_vnum vmin, guild_vnum vmax)
{
  int i, bottom, top, counter = 0;
  
  if (rnum != NOWHERE) {
    bottom = zone_table[rnum].bot;
    top    = zone_table[rnum].top;
  } else {
    bottom = vmin;
    top    = vmax;
  }
  
  /****************************************************************************/
  /** Store the header for the guild listing.                                 **/
  /****************************************************************************/
  send_to_char (ch,
  "Index VNum    Guild Master\r\n"
  "----- ------- ---------------------------------------------\r\n");
  
  for (i = 0; i <= top_guild; i++) {
    if (GM_NUM(i) >= bottom && GM_NUM(i) <= top) {
      counter++;
      
      send_to_char(ch, "%s%4d%s) [%s%-5d%s]",
        QGRN, counter, QNRM, QGRN, GM_NUM(i), QNRM);

      /************************************************************************/
      /** Retrieve the list of rooms for this guild.                          **/
      /************************************************************************/

		send_to_char(ch, " %s[%s%d%s]%s %s%s", 
			
			QCYN, QYEL, 
			(GM_TRAINER(i) == -1) ?
			  -1 : mob_index[GM_TRAINER(i)].vnum, QCYN, QYEL,
			(GM_TRAINER(i) == -1) ?
			  "" : mob_proto[GM_TRAINER(i)].player.short_descr, QNRM); 
      
      send_to_char(ch, "\r\n");
    }
  }
  
  if (counter == 0)
    send_to_char(ch, "None found.\r\n");
}

void destroy_guilds(void)
{
  ssize_t cnt/*, itr*/;

  if (!guild_index)
    return;

  for (cnt = 0; cnt <= top_guild; cnt++) {
    if (guild_index[cnt].no_such_skill)
      free(guild_index[cnt].no_such_skill);
    if (guild_index[cnt].not_enough_gold)
      free(guild_index[cnt].not_enough_gold);

    /*if (guild_index[cnt].type) {
      for (itr = 0; BUY_TYPE(guild_index[cnt].type[itr]) != NOTHING; itr++)
        if (BUY_WORD(guild_index[cnt].type[itr]))
          free(BUY_WORD(guild_index[cnt].type[itr]));
      free(shop_index[cnt].type);
    } */
  }

  free(guild_index);
  guild_index = NULL;
  top_guild = -1;
}


int count_guilds(guild_vnum low, guild_vnum high)
{
  int i, j;
  
  for (i = j = 0; GM_NUM(i) <= high; i++)
    if (GM_NUM(i) >= low)
      j++;
 
  return j;
}

