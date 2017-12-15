
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  gedit.c:  Olc written for shoplike guildmasters, code by             *
 *             Jason Goodwin                                             *
 *    Made for Circle3.0 bpl11, its copyright applies                    *
 *                                                                       *
 *  Made for Oasis OLC                                                   *
 *  Copyright 1996 Harvey Gilpin.                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"

#include "comm.h"
#include "db.h"
#include "guild.h"
#include "oasis.h"
#include "screen.h"
#include "shop.h"
#include "spells.h"
#include "genolc.h"
#include "genshp.h"
#include "genzon.h"
#include "interpreter.h"
#include "gengld.h"

/*-------------------------------------------------------------------*/
/* external variables */
extern struct guild_data *guild_index;
extern struct spell_info_type spell_info[];
extern int top_guild;

extern zone_rnum real_zone_by_thing(room_vnum vznum);

/*-------------------------------------------------------------------*/
/*. Function prototypes . */

void gedit_setup_new(struct descriptor_data *d);
void gedit_setup_existing(struct descriptor_data *d, int rgm_num);
void gedit_parse(struct descriptor_data *d, char *arg);
void gedit_disp_menu(struct descriptor_data *d);
void gedit_no_train_menu(struct descriptor_data *d);
void gedit_save_internally(struct descriptor_data *d);
void gedit_save_to_disk(int num);
void copy_guild(struct guild_data *tgm, struct guild_data *fgm);
void free_guild_strings(struct guild_data *guild);
void free_guild(struct guild_data *guild);
void gedit_modify_string(char **str, char *new);

extern const char *trade_letters[];

/*. External . */
SPECIAL(guild);

/*
 * Should check more things.
 */
void gedit_save_internally(struct descriptor_data *d)
{
  OLC_GUILD(d)->vnum = OLC_NUM(d);
  add_guild(OLC_GUILD(d));
}

void gedit_save_to_disk(int num)
{
  save_guilds(num);
}


/*-------------------------------------------------------------------*\
  utility functions 
 \*-------------------------------------------------------------------*/

ACMD(do_oasis_gedit)
{
  int number = NOWHERE, save = 0;
  guild_rnum real_num;
  struct descriptor_data *d;
  char *buf3;
  char buf1[MAX_INPUT_LENGTH];
  char buf2[MAX_INPUT_LENGTH];
  
  /****************************************************************************/
  /** Parse any arguments.                                                   **/
  /****************************************************************************/
  buf3 = two_arguments(argument, buf1, buf2);
  
  if (!*buf1) {
    send_to_char(ch, "Specify a guild VNUM to edit.\r\n");
    return;
  } else if (!isdigit(*buf1)) {
    if (str_cmp("save", buf1) != 0) {
      send_to_char(ch, "Yikes!  Stop that, someone will get hurt!\r\n");
      return;
    }
    
    save = TRUE;
    
    if (is_number(buf2))
      number = atoi(buf2);
    else if (GET_OLC_ZONE(ch) > 0) {
      zone_rnum zlok;
      
      if ((zlok = real_zone(GET_OLC_ZONE(ch))) == NOWHERE)
        number = NOWHERE;
      else
        number = genolc_zone_bottom(zlok);
    }
    
    if (number == NOWHERE) {
      send_to_char(ch, "Save which zone?\r\n");
      return;
    }
  }
  
  /****************************************************************************/
  /** If a numeric argument was given, get it.                               **/
  /****************************************************************************/
  if (number == NOWHERE)
    number = atoi(buf1);
  
  /****************************************************************************/
  /** Check that the guild isn't already being edited.                        **/
  /****************************************************************************/
  for (d = descriptor_list; d; d = d->next) {
    if (STATE(d) == CON_GEDIT) {
      if (d->olc && OLC_NUM(d) == number) {
        send_to_char(ch, "That guild is currently being edited by %s.\r\n",
          PERS(d->character, ch));
        return;
      }
    }
  }
  
  /****************************************************************************/
  /** Point d to the builder's descriptor.                                   **/
  /****************************************************************************/
  d = ch->desc;
  
  /****************************************************************************/
  /** Give the descriptor an OLC structure.                                  **/
  /****************************************************************************/
  if (d->olc) {
    mudlog(BRF, LVL_IMMORT, TRUE,
      "SYSERR: do_oasis_gedit: Player already had olc structure.");
    free(d->olc);
  }
  
  CREATE(d->olc, struct oasis_olc_data, 1);
  
  /****************************************************************************/
  /** Find the zone.                                                         **/
  /****************************************************************************/
  if ((OLC_ZNUM(d) = real_zone_by_thing(number)) == NOWHERE) {
    send_to_char(ch, "Sorry, there is no zone for that number!\r\n");
    free(d->olc);
    d->olc = NULL;
    return;
  }
  
  /****************************************************************************/
  /** Everyone but IMPLs can only edit zones they have been assigned.        **/
  /****************************************************************************/
  if (!can_edit_zone(ch, OLC_ZNUM(d))) {
    send_to_char(ch, "You do not have permission to edit this zone.\r\n");
    
    /**************************************************************************/
    /** Free the OLC structure.                                              **/
    /**************************************************************************/
    free(d->olc);
    d->olc = NULL;
    return;
  }
  
  if (save) {
    send_to_char(ch, "Saving all guilds in zone %d.\r\n",
      zone_table[OLC_ZNUM(d)].number);
    mudlog(CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(ch)), TRUE,
      "OLC: %s saves guild info for zone %d.",
      GET_NAME(ch), zone_table[OLC_ZNUM(d)].number);
    
    /**************************************************************************/
    /** Save the guild to the guild file.                                    **/
    /**************************************************************************/
    gedit_save_to_disk(OLC_ZNUM(d));
    
    /**************************************************************************/
    /** Free the OLC structure.                                              **/
    /**************************************************************************/
    free(d->olc);
    d->olc = NULL;
    return;
  }
  
  OLC_NUM(d) = number;
  
  if ((real_num = real_guild(number)) != NOTHING)
    gedit_setup_existing(d, real_num);
  else
    gedit_setup_new(d);
  
  STATE(d) = CON_GEDIT;
  
  act("$n starts using OLC.", TRUE, d->character, 0, 0, TO_ROOM);
  SET_BIT_AR(PLR_FLAGS(ch), PLR_WRITING);
  
  mudlog(BRF, LVL_IMMORT, TRUE, "OLC: %s starts editing zone %d allowed zone %d",
    GET_NAME(ch), zone_table[OLC_ZNUM(d)].number, GET_OLC_ZONE(ch));
}

void gedit_setup_new(struct descriptor_data *d)
{
	int i;
	struct guild_data *guild;

	/*. Alloc some guild shaped space . */
	CREATE(guild, struct guild_data, 1);

	/*. Some default values . */
	G_TRAINER(guild) = -1;
	G_OPEN(guild) = 0;
	G_CLOSE(guild) = 28;
	G_CHARGE(guild) = 1.0;
	G_WITH_WHO(guild) = 0;
	G_FUNC(guild) = NULL;
	G_MINLVL(guild) = 0;

	/*. Some default strings . */
	G_NO_SKILL(guild) = strdup("%s Sorry, but I don't know that one.");
	G_NO_GOLD(guild) = strdup("%s Sorry, but I'm gonna need more gold first.");

	/* init the wasteful skills and spells table */

	for (i = 0; i < MAX_SKILLS + 2; i++)
		G_SK_AND_SP(guild, i) = 0;

	OLC_GUILD(d) = guild;
	gedit_disp_menu(d);
}

/*-------------------------------------------------------------------*/

void gedit_setup_existing(struct descriptor_data *d, int rgm_num)
{
	/*. Alloc some guild shaped space . */
	CREATE(OLC_GUILD(d), struct guild_data, 1);
	copy_guild(OLC_GUILD(d), guild_index + rgm_num);
	gedit_disp_menu(d);
}

/*-------------------------------------------------------------------*/

/**************************************************************************
 Menu functions 
**************************************************************************/

/*-------------------------------------------------------------------*/

void gedit_select_skills_menu(struct descriptor_data *d)
{
	int i, j = 0;
	struct char_data *ch = d->character;
	struct guild_data *guild;

	guild = OLC_GUILD(d);
	get_char_colors(ch);
	clear_screen(d);

	write_to_output(d, "Skills known:\r\n");

	for (i = MAX_SPELLS + 1; i <= MIN_LANGUAGES - 1; i++) {
		if(spell_info[i].name != "!UNUSED!") {
			write_to_output(d, "%s[%s%-3s%s] %-3d %-10.10s  ", QNRM,
				  QCYN, YESNO(G_SK_AND_SP(guild, i)), QNRM,
				  i, spell_info[i].name);
			j++;
		}
		if (!(j % 3))
				write_to_output(d, "\r\n");
			
	}
	  write_to_output(d, "\r\n"
						 "Enter skill num, 0 to quit:  ");

	  OLC_MODE(d) = GEDIT_SELECT_SKILLS;
}

/*-------------------------------------------------------------------*/

void gedit_select_spells_menu(struct descriptor_data *d)
{
	int i, j = 0;
	struct char_data *ch = d->character;
	struct guild_data *guild;

	guild = OLC_GUILD(d);
	get_char_colors(ch);
	clear_screen(d);

	write_to_output(d, "Spells known:\r\n");

	for (i = 1; i <= NUM_SPELLS; i++) {
		if(spell_info[i].name != "!UNUSED!") {
			write_to_output(d, "%s[%s%-3s%s] %-3d %-10.10s  ", QNRM,
				  QCYN, YESNO(G_SK_AND_SP(guild, i)), QNRM,
				  i, spell_info[i].name);
			j++;
		}
		if (!(j % 3))
				write_to_output(d, "\r\n");
			
	}
	  write_to_output(d, "\r\n"
						 "Enter spell num, 0 to quit:  ");

	  OLC_MODE(d) = GEDIT_SELECT_SPELLS;
}

/*-------------------------------------------------------------------*/

void gedit_select_lang_menu(struct descriptor_data *d)
{
	int i, j = 0;
	struct char_data *ch = d->character;
	struct guild_data *guild;

	guild = OLC_GUILD(d);
	get_char_colors(ch);
	clear_screen(d);

	write_to_output(d, "Skills known:\r\n");

	for (i = MIN_LANGUAGES; i <= MAX_LANGUAGES ; i++) {
		if(spell_info[i].name != "!UNUSED!") {
			write_to_output(d, "%s[%s%-3s%s] %-3d %-10.10s  ", QNRM,
				  QCYN, YESNO(G_SK_AND_SP(guild, i)), QNRM,
				  i, spell_info[i].name);
			j++;
		}
		if (!(j % 3))
				write_to_output(d, "\r\n");
			
	}
	  write_to_output(d, "\r\n"
						 "Enter skill num, 0 to quit:  ");

	  OLC_MODE(d) = GEDIT_SELECT_LANGS;
}

/*-------------------------------------------------------------------*/

void gedit_select_wp_menu(struct descriptor_data *d)
{
	int i, j = 0;
	struct char_data *ch = d->character;
	struct guild_data *guild;

	guild = OLC_GUILD(d);
	get_char_colors(ch);
	clear_screen(d);

	write_to_output(d, "Skills known:\r\n");

	for (i = MIN_WP; i <= MAX_WP; i++) {
		if(spell_info[i].name != "!UNUSED!") {
			write_to_output(d, "%s[%s%-3s%s] %-3d %-10.10s  ", QNRM,
				  QCYN, YESNO(G_SK_AND_SP(guild, i)), QNRM,
				  i, spell_info[i].name);
			j++;
		}
		if (!(j % 3))
				write_to_output(d, "\r\n");
			
	}
	  write_to_output(d, "\r\n"
						 "Enter skill num, 0 to quit:  ");

	  OLC_MODE(d) = GEDIT_SELECT_WPS;
}

/*-------------------------------------------------------------------*/

void gedit_no_train_menu(struct descriptor_data *d)
{
	char bits[MAX_STRING_LENGTH];
    int i, count = 0;
	struct char_data *ch = d->character;
	struct guild_data *guild;

	guild = OLC_GUILD(d);
	get_char_colors(ch);
	clear_screen(d);

	for (i = 0; i < NUM_TRADERS; i++) {
    write_to_output(d, "%s%2d%s) %-20.20s   %s", grn, i + 1, nrm, trade_letters[i],
		!(++count % 2) ? "\r\n" : "");
	}
	sprintbit(G_WITH_WHO(guild), trade_letters, bits, sizeof(bits));
  write_to_output(d, "\r\nCurrently won't train: %s%s%s\r\n"
	  "Enter choice, 0 to quit : ", cyn, bits, nrm);
	OLC_MODE(d) = GEDIT_NO_TRAIN;
}

/*-------------------------------------------------------------------*/
/*. Display main menu . */

void gedit_disp_menu(struct descriptor_data *d)
{
	struct guild_data *guild;
	char buf1[MAX_STRING_LENGTH];
	struct char_data *ch = d->character;

	guild = OLC_GUILD(d);
	get_char_colors(ch);

	clear_screen(d);

	sprintbit(G_WITH_WHO(guild), trade_letters, buf1, sizeof(buf1));

	write_to_output(d, 
			  "-- Guild Number: [%s%d%s]\r\n"
			  "%s 0%s) Guild Master : [%s%d%s] %s%s\r\n"
			  "%s 1%s) Doesn't know skill:\r\n %s%s\r\n"
			  "%s 2%s) Player no gold:\r\n %s%s\r\n"
			  "%s 3%s) Open   :  [%s%d%s]\r\n"
			  "%s 4%s) Close  :  [%s%d%s]\r\n"
			  "%s 5%s) Charge :  [%s%3.1f%s]\r\n"
			  "%s 6%s) Minlvl :  [%s%d%s]\r\n"
			  "%s 7%s) Don't Train:  %s%s\r\n"
			  "%s 8%s) Spells Menu\r\n"
			  "%s 9%s) Skills Menu\r\n"
			  "%s A%s) Weapon Profs Menu\r\n"
			  "%s B%s) Languages Menu\r\n"
			  "%s Q%s) Quit\r\n"
			  "Enter Choice : ",

			  QCYN, OLC_NUM(d), QNRM,
			  QGRN, QNRM, QCYN, (G_TRAINER(guild) == -1) ? -1 : mob_index[G_TRAINER(guild)].vnum, QNRM,
			  QYEL, (G_TRAINER(guild) == -1) ? "none" : mob_proto[G_TRAINER(guild)].player.short_descr,
			  QGRN, QNRM, QYEL, G_NO_SKILL(guild),
			  QGRN, QNRM, QYEL, G_NO_GOLD(guild),
			  QGRN, QNRM, QCYN, G_OPEN(guild), QNRM,
			  QGRN, QNRM, QCYN, G_CLOSE(guild), QNRM,
			  QGRN, QNRM, QCYN, G_CHARGE(guild), QNRM,
			  QGRN, QNRM, QCYN, G_MINLVL(guild), QNRM,
			  QGRN, QNRM, QCYN, buf1,
			  QGRN, QNRM, 
			  QGRN, QNRM,
			  QGRN, QNRM, 
			  QGRN, QNRM,			  
			  QGRN, QNRM
			  );

	OLC_MODE(d) = GEDIT_MAIN_MENU;
}

/**************************************************************************
  The GARGANTUAN event handler
**************************************************************************/

void gedit_parse(struct descriptor_data *d, char *arg)
{
	int i;

	if (OLC_MODE(d) > GEDIT_NUMERICAL_RESPONSE) {
		if (!isdigit(arg[0]) && ((*arg == '-') && (!isdigit(arg[1])))) {
			write_to_output(d, "Field must be numerical, try again : ");
			return;
		}
	}
	switch (OLC_MODE(d)) {
/*-------------------------------------------------------------------*/
		case GEDIT_CONFIRM_SAVESTRING:
			switch (*arg) {
				case 'y':
				case 'Y':
					send_to_char(d->character, "Saving Guild to memory.\r\n");
					gedit_save_internally(d);
					mudlog(CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(d->character)), TRUE,
						  "OLC: %s edits guild %d", GET_NAME(d->character), OLC_NUM(d));
					if (CONFIG_OLC_SAVE) {
						gedit_save_to_disk(real_zone_by_thing(OLC_NUM(d)));
						write_to_output(d, "Guild %d saved to disk.\r\n", OLC_NUM(d));
					} else
						write_to_output(d, "Guild %d saved to memory.\r\n", OLC_NUM(d));
						cleanup_olc(d, CLEANUP_STRUCTS);
			        return;
				case 'n':
				case 'N':
					cleanup_olc(d, CLEANUP_ALL);
					return;
				default:
					write_to_output(d, "Invalid choice!\r\nDo you wish to save the guild? : ");
					return;
			}
			break;

/*-------------------------------------------------------------------*/
		case GEDIT_MAIN_MENU:
			i = 0;
			switch (*arg) {
				case 'q':
				case 'Q':
					if (OLC_VAL(d)) {		/*. Anything been changed? . */
						write_to_output(d,  "Do you wish to save the changes to the Guild? (y/n) : ");
						OLC_MODE(d) = GEDIT_CONFIRM_SAVESTRING;
					} else
						cleanup_olc(d, CLEANUP_ALL);
					return;
				case '0':
					OLC_MODE(d) = GEDIT_TRAINER;
					write_to_output(d, "Enter vnum of guild master : ");
					return;
				case '1':
					OLC_MODE(d) = GEDIT_NO_SKILL;
					i--;
					break;
				case '2':
					OLC_MODE(d) = GEDIT_NO_CASH;
					i--;
					break;
				case '3':
					OLC_MODE(d) = GEDIT_OPEN;
					write_to_output(d, "When does this shop open (a day has 28 hours) ? ");
					i++;
					break;
				case '4':
					OLC_MODE(d) = GEDIT_CLOSE;
					write_to_output(d, "When does this shop close (a day has 28 hours) ? ");
					i++;
					break;
				case '5':
					OLC_MODE(d) = GEDIT_CHARGE;
					i++;
					break;
				case '6':
					OLC_MODE(d) = GEDIT_MINLVL;
					write_to_output(d, "Minumum Level will Train: ");
					i++;
					return;
				case '7':
					OLC_MODE(d) = GEDIT_NO_TRAIN;
					gedit_no_train_menu(d);
					return;
				case '8':
					OLC_MODE(d) = GEDIT_SELECT_SPELLS;
					gedit_select_spells_menu(d);
					return;
				case '9':
					OLC_MODE(d) = GEDIT_SELECT_SKILLS;
					gedit_select_skills_menu(d);
					return;
				case 'a':
				case 'A':
					OLC_MODE(d) = GEDIT_SELECT_WPS;
					gedit_select_wp_menu(d);
					return;
				case 'b':
				case 'B':
					OLC_MODE(d) = GEDIT_SELECT_LANGS;
					gedit_select_lang_menu(d);
					return;
				default:
					gedit_disp_menu(d);
					return;
			}

				if (i == 0)
					break;
				else if (i == 1)
					write_to_output(d, "\r\nEnter new value : ");
				else if (i == -1)
					write_to_output(d, "\r\nEnter new text :\r\n] ");
				else
					write_to_output(d, "Oops...\r\n");
			return;
/*-------------------------------------------------------------------*/
			/*. String edits . */
		case GEDIT_NO_SKILL:
			gedit_modify_string(&G_NO_SKILL(OLC_GUILD(d)), arg);
			break;
		case GEDIT_NO_CASH:
			gedit_modify_string(&G_NO_GOLD(OLC_GUILD(d)), arg);
			break;

/*-------------------------------------------------------------------*/
			/*. Numerical responses . */

		case GEDIT_TRAINER:
			i = atoi(arg);
			if ((i = atoi(arg)) != -1)
				if ((i = real_mobile(i)) == NOBODY) {
					write_to_output(d, "That mobile does not exist, try again : ");
					return;
				}
			G_TRAINER(OLC_GUILD(d)) = i;
			if (i == -1)
				break;
			/*. Fiddle with special procs . */
			G_FUNC(OLC_GUILD(d)) = mob_index[i].func;
			mob_index[i].func = guild;
			break;
		case GEDIT_OPEN:
			G_OPEN(OLC_GUILD(d)) = LIMIT(atoi(arg), 0, 28);
			break;
		case GEDIT_CLOSE:
			G_CLOSE(OLC_GUILD(d)) = LIMIT(atoi(arg), 0, 28);;
			break;
		case GEDIT_CHARGE:
			sscanf(arg, "%f", &G_CHARGE(OLC_GUILD(d)));
			break;
		case GEDIT_NO_TRAIN:
			    if ((i = LIMIT(atoi(arg), 0, NUM_TRADERS)) > 0) {
					TOGGLE_BIT(G_WITH_WHO(OLC_GUILD(d)), 1 << (i - 1));
				gedit_no_train_menu(d);
				return;
			}
			break;

		case GEDIT_MINLVL:
			G_MINLVL(OLC_GUILD(d)) = LIMIT(atoi(arg), 0, LVL_IMMORT);;
			break;

		case GEDIT_SELECT_SPELLS:
			i = atoi(arg);
			if (i == 0)
				break;
			i = MAX(1, MIN(i, MAX_SPELLS));
			G_SK_AND_SP(OLC_GUILD(d), i) = !G_SK_AND_SP(OLC_GUILD(d), i);
			gedit_select_spells_menu(d);
			return;

		case GEDIT_SELECT_SKILLS:
			i = atoi(arg);
			if (i == 0)
				break;
			i = MAX(MAX_SPELLS + 1, MIN(i, MIN_LANGUAGES - 1));
			G_SK_AND_SP(OLC_GUILD(d), i) = !G_SK_AND_SP(OLC_GUILD(d), i);
			gedit_select_skills_menu(d);
			return;

		case GEDIT_SELECT_WPS:
			i = atoi(arg);
			if (i == 0)
				break;
			i = MAX(1, MIN(i, MAX_WP));
			G_SK_AND_SP(OLC_GUILD(d), i) = !G_SK_AND_SP(OLC_GUILD(d), i);
			gedit_select_wp_menu(d);
			return;

		case GEDIT_SELECT_LANGS:
			i = atoi(arg);
			if (i == 0)
				break;
			i = MAX(1, MIN(i, MAX_LANGUAGES));
			G_SK_AND_SP(OLC_GUILD(d), i) = !G_SK_AND_SP(OLC_GUILD(d), i);
			gedit_select_lang_menu(d);
			return;

/*-------------------------------------------------------------------*/
		default:
			/*. We should never get here . */
			cleanup_olc(d, CLEANUP_ALL);
			mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: gedit_parse(): "
					 "Reached default case!");
			write_to_output(d, "Oops...\r\n");
			break;
	}
/*-------------------------------------------------------------------*/
/*. END OF CASE 
  If we get here, we have probably changed something, and now want to
  return to main menu.  Use OLC_VAL as a 'has changed' flag . */

	OLC_VAL(d) = 1;
	gedit_disp_menu(d);
}

