/* ************************************************************************
*   File: act.informative.c                             Part of CircleMUD *
*  Usage: Player-level commands of an informative nature                  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "constants.h"
#include "dg_scripts.h"
#include "boards.h"

/* extern variables */
extern int top_of_helpt;
extern struct help_index_element *help_table;
extern char *help;
extern struct time_info_data time_info;

extern char *credits;
extern char *news;
extern char *info;
extern char *motd;
extern char *imotd;
extern char *wizlist;
extern char *immlist;
extern char *policies;
extern char *handbook;
extern char *class_abbrevs[];
extern char *race_abbrevs[];
extern char *pc_race_types[];
extern const char *material_names[];
extern int show_mob_stacking;
extern int show_obj_stacking;
extern char *pc_class_types[];

/* extern functions */
ACMD(do_action);
ACMD(do_insult);
int level_exp(int chclass, int level);
char *title_male(int chclass, int level);
char *title_female(int chclass, int level);
struct time_info_data *real_time_passed(time_t t2, time_t t1);
int compute_armor_class(struct char_data *ch);
struct obj_data *find_vehicle_by_vnum(int vnum);
extern struct obj_data *get_obj_in_list_type(int type, struct obj_data *list);
void view_room_by_rnum(struct char_data * ch, int is_in);
int check_disabled(const struct command_info *command);

/* local functions */
int sort_commands_helper(const void *a, const void *b);
void print_object_location(int num, struct obj_data *obj, struct char_data *ch, int recur);
void show_obj_to_char(struct obj_data *obj, struct char_data *ch, int mode);
void list_obj_to_char(struct obj_data *list, struct char_data *ch, int mode, int show);
int show_obj_modifiers(struct obj_data *obj, struct char_data *ch);
ACMD(do_look);
ACMD(do_examine);
ACMD(do_gold);
ACMD(do_score);
ACMD(do_inventory);
ACMD(do_equipment);
ACMD(do_time);
ACMD(do_weather);
ACMD(do_help);
ACMD(do_who);
ACMD(do_users);
ACMD(do_gen_ps);
void perform_mortal_where(struct char_data *ch, char *arg);
void perform_immort_where(struct char_data *ch, char *arg);
ACMD(do_where);
ACMD(do_levels);
ACMD(do_consider);
ACMD(do_diagnose);
ACMD(do_color);
ACMD(do_toggle);
void sort_commands(void);
ACMD(do_commands);
void diag_char_to_char(struct char_data *i, struct char_data *ch);
void diag_obj_to_char(struct obj_data *obj, struct char_data *ch);
void look_at_char(struct char_data *i, struct char_data *ch);
void list_one_char(struct char_data *i, struct char_data *ch);
void list_char_to_char(struct char_data *list, struct char_data *ch);
ACMD(do_exits);
void look_in_direction(struct char_data *ch, int dir);
void look_in_obj(struct char_data *ch, char *arg);
void look_out_window(struct char_data *ch, char *arg);
char *find_exdesc(char *word, struct extra_descr_data *list);
void look_at_target(struct char_data *ch, char *arg, int read);
void search_in_direction(struct char_data * ch, int dir);
ACMD(do_autoexit);
void do_auto_exits(room_rnum target_room, struct char_data *ch, int exit_mode);


/* local globals */
int *cmd_sort_info;

/* Portal appearance types */
const char *portal_appearance[] = {
    "All you can see is the glow of the portal.",
    "You see an image of yourself in the room - my, you are looking attractive today.",
    "All you can see is a swirling grey mist.",
    "The scene is of the surrounding countryside, but somehow blurry and lacking focus.",
    "The blackness appears to stretch on forever.",
    "Suddenly, out of the blackness a flaming red eye appears and fixes its gaze upon you.",
    "\n"
};
#define MAX_PORTAL_TYPES        6

/* For show_obj_to_char 'mode'.	/-- arbitrary */
#define SHOW_OBJ_LONG		0
#define SHOW_OBJ_SHORT		1
#define SHOW_OBJ_ACTION		2


void show_obj_to_char(struct obj_data *obj, struct char_data *ch, int mode)
{
  if (!obj || !ch) {
    log("SYSERR: NULL pointer in show_obj_to_char(): obj=%p ch=%p", obj, ch);
    return;
  }

  switch (mode) {
  case SHOW_OBJ_LONG:
    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_ROOMFLAGS)) 
      send_to_char(ch, "[%d] %s", GET_OBJ_VNUM(obj), SCRIPT(obj) ? "[TRIG] " : "");
    
    /*
     * hide objects starting with . from non-holylighted people 
     * Idea from Elaseth of TBA 
     */
    if (*obj->description == '.' && (IS_NPC(ch) || !PRF_FLAGGED(ch, PRF_HOLYLIGHT))) 
      return;

    send_to_char(ch, "%s", CCGRN(ch, C_NRM));
    send_to_char(ch, "%s", obj->description);
    break;

  case SHOW_OBJ_SHORT:
    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_ROOMFLAGS)) 
      send_to_char(ch, "[%d] %s", GET_OBJ_VNUM(obj), SCRIPT(obj) ? "[TRIG] " : "");
    
    send_to_char(ch, "%s", obj->short_description);
    break;

  case SHOW_OBJ_ACTION:
    switch (GET_OBJ_TYPE(obj)) {
    case ITEM_NOTE:
      if (obj->action_description) {
        char notebuf[MAX_NOTE_LENGTH + 64];

        snprintf(notebuf, sizeof(notebuf), "There is something written on it:\r\n\r\n%s", obj->action_description);
        page_string(ch->desc, notebuf, TRUE);
      } else
	send_to_char(ch, "It's blank.\r\n");
      return;

    case ITEM_BOARD:
      show_board(GET_OBJ_VNUM(obj),ch);
      break;
      
    case ITEM_DRINKCON:
      send_to_char(ch, "It looks like a drink container.\r\n");
      break;

    default:
      send_to_char(ch, "You see nothing special..\r\n");
      break;
    }
      diag_obj_to_char(obj, ch);
      send_to_char(ch, "It appears to be made of %s.\r\n",
       material_names[GET_OBJ_MATERIAL(obj)]);
      break;

  default:
    log("SYSERR: Bad display mode (%d) in show_obj_to_char().", mode);
    return;
  }

  if ((show_obj_modifiers(obj, ch) || (mode != SHOW_OBJ_ACTION)))
  send_to_char(ch, "\r\n");
}


int show_obj_modifiers(struct obj_data *obj, struct char_data *ch)
{
  int found = FALSE;

  if (OBJ_FLAGGED(obj, ITEM_INVISIBLE)) {
    send_to_char(ch, " (invisible)");
    found++;
  }
  if (OBJ_FLAGGED(obj, ITEM_BLESS) && AFF_FLAGGED(ch, AFF_DETECT_ALIGN)) {
    send_to_char(ch, " ..It glows blue!");
    found++;
  }
  if (OBJ_FLAGGED(obj, ITEM_MAGIC) && AFF_FLAGGED(ch, AFF_DETECT_MAGIC)) {
    send_to_char(ch, " ..It glows yellow!");
    found++;
  }
  if (OBJ_FLAGGED(obj, ITEM_GLOW)) {
    send_to_char(ch, " ..It has a soft glowing aura!");
    found++;
  }
  if (OBJ_FLAGGED(obj, ITEM_HUM)) {
    send_to_char(ch, " ..It emits a faint humming sound!");
    found++;
  }
  if (OBJ_FLAGGED(obj, ITEM_BROKEN)) {
    send_to_char(ch, " ..It appears to be broken.");
    found++;
  }
  return(found);
}

void list_obj_to_char(struct obj_data *list, struct char_data *ch, int mode, int show)
{
  struct obj_data *i, *j;
  bool found = FALSE;
  int num;

  for (i = list; i; i = i->next_content) {
    if (i->description == NULL)
      continue;
    if (str_cmp(i->description, "undefined") == 0)
      continue;
    num = 0;
    if (CONFIG_STACK_OBJS) {
      for (j = list; j != i; j = j->next_content)
        if (!str_cmp(j->short_description, i->short_description) &&
            (j->item_number == i->item_number))
          break;
      if (j!=i)
        continue;
      for (j = i; j; j = j->next_content)
        if (!str_cmp(j->short_description, i->short_description) &&
            (j->item_number == i->item_number))
          num++;
    }
    if (CAN_SEE_OBJ(ch, i) || (GET_OBJ_TYPE(i) == ITEM_LIGHT)) {
      if (num > 1)
        send_to_char(ch, "(%2i) ", num);
      show_obj_to_char(i, ch, mode);
      found = TRUE;
    }
  } /* Loop through all objects in the list */
  if (!found && show)
  send_to_char(ch, " Nothing.\r\n");
}

void diag_obj_to_char(struct obj_data *obj, struct char_data *ch)
{
  struct {
    int percent;
    const char *text;
  } diagnosis[] = {
    { 100, "is in excellent condition."                 },
    {  90, "has a few scuffs."                          },
    {  75, "has some small scuffs and scratches."       },
    {  50, "has quite a few scratches."                 },
    {  30, "has some big nasty scrapes and scratches."  },
    {  15, "looks pretty damaged."                      },
    {   0, "is in awful condition."                     },
    {  -1, "is in need of repair."                      },
  };
  int percent, ar_index;
  const char *objs = OBJS(obj, ch);

  if (GET_OBJ_VAL(obj, VAL_ALL_MAXHEALTH) > 0)
    percent = (100 * GET_OBJ_VAL(obj, VAL_ALL_HEALTH)) / GET_OBJ_VAL(obj, VAL_ALL_MAXHEALTH);
  else
    percent = 0;               /* How could MAX_HIT be < 1?? */

  for (ar_index = 0; diagnosis[ar_index].percent >= 0; ar_index++)
    if (percent >= diagnosis[ar_index].percent)
      break;

  send_to_char(ch, "\r\n%c%s %s\r\n", UPPER(*objs), objs + 1, diagnosis[ar_index].text);
}

void diag_char_to_char(struct char_data *i, struct char_data *ch)
{
  struct {
    int percent;
    const char *text;
  } diagnosis[] = {
    { 100, "is in excellent condition."			},
    {  90, "has a few scratches."			},
    {  75, "has some small wounds and bruises."		},
    {  50, "has quite a few wounds."			},
    {  30, "has some big nasty wounds and scratches."	},
    {  15, "looks pretty hurt."				},
    {   0, "is in awful condition."			},
    {  -1, "is bleeding awfully from big wounds."	},
  };
  int percent, ar_index;
  const char *pers = PERS(i, ch);

  if (GET_MAX_HIT(i) > 0)
    percent = (100 * GET_HIT(i)) / GET_MAX_HIT(i);
  else
    percent = 0;		/* How could MAX_HIT be < 1?? */

  for (ar_index = 0; diagnosis[ar_index].percent >= 0; ar_index++)
    if (percent >= diagnosis[ar_index].percent)
      break;

  send_to_char(ch, "%c%s %s\r\n", UPPER(*pers), pers + 1, diagnosis[ar_index].text);
}


void look_at_char(struct char_data *i, struct char_data *ch)
{
  int j, found;
  struct obj_data *tmp_obj;

  if (!ch->desc)
    return;

   if (i->player.description)
    send_to_char(ch, "%s", i->player.description);
  else
    act("You see nothing special about $m.", FALSE, i, 0, ch, TO_VICT);

  if (GET_SEX(i) == SEX_NEUTRAL) 
    send_to_char(ch, "%c%s appears to be %s %s.\r\n", UPPER(*GET_NAME(i)), GET_NAME(i) + 1, AN(RACE(i)), RACE(i));
  else
    send_to_char(ch, "%c%s appears to be %s %s %s.\r\n", UPPER(*GET_NAME(i)), GET_NAME(i) + 1, AN(MAFE(i)), MAFE(i), RACE(i));

  diag_char_to_char(i, ch);

  found = FALSE;
  for (j = 0; !found && j < NUM_WEARS; j++)
    if (GET_EQ(i, j) && CAN_SEE_OBJ(ch, GET_EQ(i, j)))
      found = TRUE;

  if (found) {
    send_to_char(ch, "\r\n");	/* act() does capitalization. */
    act("$n is using:", FALSE, i, 0, ch, TO_VICT);
    for (j = 0; j < NUM_WEARS; j++)
      if (GET_EQ(i, j) && CAN_SEE_OBJ(ch, GET_EQ(i, j))) {
	send_to_char(ch, "%s", wear_where[j]);
	show_obj_to_char(GET_EQ(i, j), ch, SHOW_OBJ_SHORT);
      }
  }
  if (ch != i && (IS_THIEF(ch) || GET_LEVEL(ch) >= LVL_IMMORT)) {
    found = FALSE;
    act("\r\nYou attempt to peek at $s inventory:", FALSE, i, 0, ch, TO_VICT);
    for (tmp_obj = i->carrying; tmp_obj; tmp_obj = tmp_obj->next_content) {
      if (CAN_SEE_OBJ(ch, tmp_obj) && (rand_number(0, 20) < GET_LEVEL(ch))) {
	show_obj_to_char(tmp_obj, ch, SHOW_OBJ_SHORT);
	found = TRUE;
      }
    }

    if (!found)
      send_to_char(ch, "You can't see anything.\r\n");
  }
}


void list_one_char(struct char_data *i, struct char_data *ch)
{
  const char *positions[] = {
    " is lying here, dead.",
    " is lying here, mortally wounded.",
    " is lying here, incapacitated.",
    " is lying here, stunned.",
    " is sleeping here.",
    " is resting here.",
    " is sitting here.",
    "!FIGHTING!",
    " is standing here."
  };

  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_ROOMFLAGS) && IS_NPC(i)) 
     send_to_char(ch, "[%d] %s", GET_MOB_VNUM(i), SCRIPT(i) ? "[TRIG] " : "");
  
  if (IS_NPC(i) && i->player.long_descr && GET_POS(i) == GET_DEFAULT_POS(i)) {
    if (AFF_FLAGGED(i, AFF_INVISIBLE))
      send_to_char(ch, "*");

    if (AFF_FLAGGED(ch, AFF_DETECT_ALIGN)) {
      if (IS_EVIL(i))
	send_to_char(ch, "(Red Aura) ");
      else if (IS_GOOD(i))
	send_to_char(ch, "(Blue Aura) ");
    }
    send_to_char(ch, "%s", i->player.long_descr);

    if (AFF_FLAGGED(i, AFF_SANCTUARY))
      act("...$e glows with a bright light!", FALSE, i, 0, ch, TO_VICT);
    if (AFF_FLAGGED(i, AFF_BLIND))
      act("...$e is groping around blindly!", FALSE, i, 0, ch, TO_VICT);

    return;
  }

  if (IS_NPC(i))
    send_to_char(ch, "%c%s", UPPER(*i->player.short_descr), i->player.short_descr + 1);
  else
    send_to_char(ch, "%s %s", i->player.name, GET_TITLE(i));

  if (AFF_FLAGGED(i, AFF_INVISIBLE))
    send_to_char(ch, " (invisible)");
  if (AFF_FLAGGED(i, AFF_HIDE))
    send_to_char(ch, " (hidden)");
  if (!IS_NPC(i) && !i->desc)
    send_to_char(ch, " (linkless)");
  if (!IS_NPC(i) && PLR_FLAGGED(i, PLR_WRITING))
    send_to_char(ch, " (writing)");
  if (!IS_NPC(i) && PRF_FLAGGED(i, PRF_BUILDWALK))
    send_to_char(ch, " (buildwalk)");

  if (GET_POS(i) != POS_FIGHTING)
    send_to_char(ch, "%s", positions[(int) GET_POS(i)]);
  else {
    if (FIGHTING(i)) {
      send_to_char(ch, " is here, fighting ");
      if (FIGHTING(i) == ch)
	send_to_char(ch, "YOU!");
      else {
	if (IN_ROOM(i) == IN_ROOM(FIGHTING(i)))
	  send_to_char(ch, "%s!", PERS(FIGHTING(i), ch));
	else
	  send_to_char(ch,  "someone who has already left!");
      }
    } else			/* NIL fighting pointer */
      send_to_char(ch, " is here struggling with thin air.");
  }

  if (AFF_FLAGGED(ch, AFF_DETECT_ALIGN)) {
    if (IS_EVIL(i))
      send_to_char(ch, " (Red Aura)");
    else if (IS_GOOD(i))
      send_to_char(ch, " (Blue Aura)");
  }
  send_to_char(ch, "\r\n");

  if (AFF_FLAGGED(i, AFF_SANCTUARY))
    act("...$e glows with a bright light!", FALSE, i, 0, ch, TO_VICT);
}

void list_char_to_char(struct char_data *list, struct char_data *ch)
{
  struct char_data *i, *j;
  int num;

  for (i = list; i; i = i->next_in_room) {
    if (ch == i) 
      continue;
    if (CAN_SEE(ch, i)) {
      num = 0;
      if (CONFIG_STACK_MOBS) {
        /* How many other occurences of this mob are there? */
        for (j = list; j != i; j = j->next_in_room)
          if ( (i->nr           == j->nr            ) &&
               (GET_POS(i)      == GET_POS(j)       ) &&
               (AFF_FLAGS(i)[0]    == AFF_FLAGS(j)[0]     ) &&
               (AFF_FLAGS(i)[1]    == AFF_FLAGS(j)[1]     ) &&
               (AFF_FLAGS(i)[2]    == AFF_FLAGS(j)[2]     ) &&
               (AFF_FLAGS(i)[3]    == AFF_FLAGS(j)[3]     ) &&
               !strcmp(GET_NAME(i), GET_NAME(j))         ) 
            break;
     	if (j!=i)
          /* This will be true where we have already found this
 	  * mob for an earlier "i".  The continue pops us out of
 	  * the main "i" for loop.
 	  */
          continue;
 	for (j = i; j; j = j->next_in_room)
          if ( (i->nr           == j->nr            ) &&
               (GET_POS(i)      == GET_POS(j)       ) &&
               (AFF_FLAGS(i)[0]    == AFF_FLAGS(j)[0]     ) &&
               (AFF_FLAGS(i)[1]    == AFF_FLAGS(j)[1]     ) &&
               (AFF_FLAGS(i)[2]    == AFF_FLAGS(j)[2]     ) &&
               (AFF_FLAGS(i)[3]    == AFF_FLAGS(j)[3]     ) &&
               !strcmp(GET_NAME(i), GET_NAME(j))         ) 
            num++;
      }
      /* Now show this mob's name and other stuff */
      if (num > 1)
        send_to_char(ch, "(%2i) ", num);
	list_one_char(i, ch);
    } /* processed a character we can see */
      else if (IS_DARK(IN_ROOM(ch)) && !CAN_SEE_IN_DARK(ch) &&
	       AFF_FLAGGED(i, AFF_INFRAVISION))
	send_to_char(ch, "You see a pair of glowing red eyes looking your way.\r\n");
  } /* loop through all characters in room */
}

void do_auto_exits(room_rnum target_room, struct char_data *ch, int exit_mode)
{
  int door, door_found = 0, has_light = FALSE;

  if (exit_mode == EXIT_NORMAL) {
    /* Standard behaviour - just list the available exit directions */
    send_to_char(ch, "%s[ Exits: ", CCCYN(ch, C_NRM));
    for (door = 0; door < NUM_OF_DIRS; door++) {
      if (!W_EXIT(target_room, door) ||
           W_EXIT(target_room, door)->to_room == NOWHERE)
        continue;
      if (EXIT_FLAGGED(W_EXIT(target_room, door), EX_CLOSED) &&
          !CONFIG_DISP_CLOSED_DOORS)
        continue;
      if (EXIT_FLAGGED(W_EXIT(target_room, door), EX_SECRET) && 
          EXIT_FLAGGED(W_EXIT(target_room, door), EX_CLOSED))
        continue;
      if (EXIT_FLAGGED(W_EXIT(target_room, door), EX_CLOSED))
        send_to_char(ch, "%s(%s)%s ",
                     CCRED(ch, C_NRM), abbr_dirs[door], CCCYN(ch, C_NRM));
      else
        send_to_char(ch, "%s ", abbr_dirs[door]);
      door_found++;
    }
    send_to_char(ch, "%s]%s\r\n", door_found ? "" : "None!", CCNRM(ch, C_NRM));
  }
  if (exit_mode == EXIT_COMPLETE) {
    send_to_char(ch, "%sObvious Exits:%s\r\n", CCCYN(ch, C_NRM), CCNRM(ch,C_NRM));
    if (IS_AFFECTED(ch, AFF_BLIND)) {
      send_to_char(ch, "You can't see a damned thing, you're blind!\r\n");
      return;
    }
    if (IS_DARK(IN_ROOM(ch)) && !CAN_SEE_IN_DARK(ch)) {
      send_to_char(ch, "It is pitch black...\r\n");
      return;
    }

    /* Is the character using a working light source? */
    if (GET_EQ(ch, WEAR_LIGHT))
      if (GET_OBJ_TYPE(GET_EQ(ch, WEAR_LIGHT)) == ITEM_LIGHT)
        if (GET_OBJ_VAL(GET_EQ(ch, WEAR_LIGHT),VAL_LIGHT_HOURS))
          has_light = TRUE;

    for (door = 0; door < NUM_OF_DIRS; door++) {
      if (W_EXIT(target_room, door) &&
          W_EXIT(target_room, door)->to_room != NOWHERE) {
        /* We have a door that leads somewhere */
        if (GET_LEVEL(ch) >= LVL_IMMORT) {
          /* Immortals see everything */
          door_found++;
          send_to_char(ch, "%-9s - [%5d] %s.\r\n", dirs[door],
                world[W_EXIT(target_room, door)->to_room].number,
                world[W_EXIT(target_room, door)->to_room].name);
          if (IS_SET(W_EXIT(target_room, door)->exit_info, EX_ISDOOR) ||
              IS_SET(W_EXIT(target_room, door)->exit_info, EX_SECRET)   ) {
            /* This exit has a door - tell all about it */
            send_to_char(ch,"                    The %s%s is %s %s%s.\r\n",
                IS_SET(W_EXIT(target_room, door)->exit_info, EX_SECRET) ?
                    "secret " : "",
                (W_EXIT(target_room, door)->keyword &&
                 str_cmp(fname(W_EXIT(target_room, door)->keyword), "undefined")) ?
                    fname(W_EXIT(target_room, door)->keyword) : "opening",
                IS_SET(W_EXIT(target_room, door)->exit_info, EX_CLOSED) ?
                    "closed" : "open",
                IS_SET(W_EXIT(target_room, door)->exit_info, EX_LOCKED) ?
                    "and locked" : "but unlocked",
                IS_SET(W_EXIT(target_room, door)->exit_info, EX_PICKPROOF) ?
                    " (pickproof)" : "");
          }
        }
        else { /* This is what mortal characters see */
          if (!IS_SET(W_EXIT(target_room, door)->exit_info, EX_CLOSED)) {
            /* And the door is open */
            door_found++;
            send_to_char(ch, "%-9s - %s\r\n", dirs[door],
                IS_DARK(W_EXIT(target_room, door)->to_room) &&
                !CAN_SEE_IN_DARK(ch) && !has_light ?
                "Too dark to tell." :
                world[W_EXIT(target_room, door)->to_room].name);
          } else if (CONFIG_DISP_CLOSED_DOORS &&
              !IS_SET(W_EXIT(target_room, door)->exit_info, EX_SECRET)) {
              /* But we tell them the door is closed */
              door_found++;
              send_to_char(ch, "%-9s - The %s is closed.\r\n", dirs[door],
                  (W_EXIT(target_room, door)->keyword) ?
                  fname(W_EXIT(target_room,door)->keyword) : "opening" );
            }
        }
      }
    }
    if (!door_found)
    send_to_char(ch, " None.\r\n");
  }
}

ACMD(do_exits)
{
  /* Why duplicate code? */
  do_auto_exits(IN_ROOM(ch), ch, EXIT_COMPLETE);
}

const char *exitlevels[] = {
  "off", "normal", "n/a", "complete", "\n"};

ACMD(do_autoexit)
{
  char arg[MAX_INPUT_LENGTH];
  int tp;

  if (IS_NPC(ch))
    return;

  one_argument(argument, arg);


  if (!*arg) {
    send_to_char(ch, "Your current autoexit level is %s.\r\n", exitlevels[EXIT_LEV(ch)]);
    return;
  }
  if (((tp = search_block(arg, exitlevels, FALSE)) == -1)) {
    send_to_char(ch, "Usage: Autoexit { Off | Normal | Complete }\r\n");
    return;
  }
  switch (tp) {
    case EXIT_OFF:
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_AUTOEXIT);
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_FULL_EXIT);
      break;
    case EXIT_NORMAL:
      SET_BIT_AR(PRF_FLAGS(ch), PRF_AUTOEXIT);
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_FULL_EXIT);
      break;
    case EXIT_COMPLETE:
      SET_BIT_AR(PRF_FLAGS(ch), PRF_AUTOEXIT);
      SET_BIT_AR(PRF_FLAGS(ch), PRF_FULL_EXIT);
      break;
  }
  send_to_char(ch, "Your %sautoexit level%s is now %s.\r\n",
               CCRED(ch, C_BRI), CCNRM(ch, C_OFF), exitlevels[EXIT_LEV(ch)]);
}

void look_at_room(room_rnum target_room, struct char_data *ch, int ignore_brief)
{
  struct room_data *rm = &world[IN_ROOM(ch)];
  if (!ch->desc)
    return;

  if (IS_DARK(target_room) && !CAN_SEE_IN_DARK(ch)) {
    send_to_char(ch, "It is pitch black...\r\n");
    return;
  } else if (AFF_FLAGGED(ch, AFF_BLIND)) {
    send_to_char(ch, "You see nothing but infinite darkness...\r\n");
    return;
  }
  send_to_char(ch, "%s", CCCYN(ch, C_NRM));
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_ROOMFLAGS)) {
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];

    sprintbitarray(ROOM_FLAGS(target_room), room_bits, RF_ARRAY_MAX, buf);
    sprinttype(rm->sector_type, sector_types, buf2, sizeof(buf2));
    send_to_char(ch, "[%5d] ", GET_ROOM_VNUM(IN_ROOM(ch)));

    send_to_char(ch, "%s%s [ %s] [ %s ]",
                     SCRIPT(rm) ? "[TRIG] " : "",
                     world[IN_ROOM(ch)].name, buf, buf2);
  } else
    send_to_char(ch, "%s", world[target_room].name);

  send_to_char(ch, "%s\r\n", CCNRM(ch, C_NRM));

  if ((!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_BRIEF)) || ignore_brief ||
      ROOM_FLAGGED(target_room, ROOM_DEATH))
    send_to_char(ch, "%s", world[target_room].description);

  /* autoexits */
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOEXIT))
    do_auto_exits(target_room, ch, EXIT_LEV(ch));

  /* now list characters & objects */
  send_to_char(ch, "%s", CCGRN(ch, C_NRM));
  list_obj_to_char(world[target_room].contents, ch, SHOW_OBJ_LONG, FALSE);
  send_to_char(ch, "%s", CCYEL(ch, C_NRM));
  list_char_to_char(world[target_room].people, ch);
  send_to_char(ch, "%s", CCNRM(ch, C_NRM));
}

void look_in_direction(struct char_data *ch, int dir)
{
  if (EXIT(ch, dir)) {
    if (EXIT(ch, dir)->general_description)
      send_to_char(ch, "%s", EXIT(ch, dir)->general_description);
    else
      send_to_char(ch, "You see nothing special.\r\n");

    if (EXIT_FLAGGED(EXIT(ch, dir), EX_ISDOOR) && EXIT(ch, dir)->keyword) {
      if (!EXIT_FLAGGED(EXIT(ch, dir), EX_SECRET) &&
           EXIT_FLAGGED(EXIT(ch, dir), EX_CLOSED) )
        send_to_char(ch, "The %s is closed.\r\n", fname(EXIT(ch, dir)->keyword));
      else if (!EXIT_FLAGGED(EXIT(ch, dir), EX_CLOSED))
        send_to_char(ch, "The %s is open.\r\n", fname(EXIT(ch, dir)->keyword));
    }
  } else
    send_to_char(ch, "Nothing special there...\r\n");
}

void look_in_obj(struct char_data *ch, char *arg)
{
 struct obj_data *obj = NULL;
 struct char_data *dummy = NULL;
 int amt, bits;

  if (!*arg)
    send_to_char(ch, "Look in what?\r\n");
  else if (!(bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch, &dummy, &obj))) {
    send_to_char(ch, "There doesn't seem to be %s %s here.\r\n", AN(arg), arg);
  } else if (find_exdesc(arg, obj->ex_description) != NULL)
      send_to_char(ch, "There's nothing inside that!\r\n");
    else if ((GET_OBJ_TYPE(obj) == ITEM_PORTAL) && !OBJVAL_FLAGGED(obj, CONT_CLOSEABLE)) { 
   if (GET_OBJ_VAL(obj, VAL_PORTAL_APPEAR) < 0) {
     /* You can look through the portal to the destination */
     /* where does this lead to? */
     room_rnum portal_dest = real_room(GET_OBJ_VAL(obj, VAL_PORTAL_DEST));
     if (portal_dest == NOWHERE) {
       send_to_char(ch, "You see nothing but infinite darkness...\r\n");
     } else if (IS_DARK(portal_dest) && !CAN_SEE_IN_DARK(ch)) {
       send_to_char(ch, "You see nothing but infinite darkness...\r\n");
     } else {
       send_to_char(ch, "After seconds of concentration you see the image of %s.\r\n", world[portal_dest].name);
     }
   } else if (GET_OBJ_VAL(obj, VAL_PORTAL_APPEAR) < MAX_PORTAL_TYPES) {
     /* display the appropriate description from the list of descriptions
*/
     send_to_char(ch, "%s\r\n", portal_appearance[GET_OBJ_VAL(obj, VAL_PORTAL_APPEAR)]);
   } else {
     /* We shouldn't really get here, so give a default message */
     send_to_char(ch, "All you can see is the glow of the portal.\r\n");
   }
 } else if (GET_OBJ_TYPE(obj) == ITEM_VEHICLE) {
   if (OBJVAL_FLAGGED(obj, CONT_CLOSED))
     send_to_char(ch, "It is closed.\r\n");
   else if (GET_OBJ_VAL(obj, VAL_VEHICLE_APPEAR) < 0) {
     /* You can look inside the vehicle */
     /* where does this lead to? */
     room_rnum vehicle_inside = real_room(GET_OBJ_VAL(obj, VAL_VEHICLE_ROOM));
     if (vehicle_inside == NOWHERE) {
       send_to_char(ch, "You cannot see inside that.\r\n");
     } else if (IS_DARK(vehicle_inside) && !CAN_SEE_IN_DARK(ch)) {
       send_to_char(ch, "It is pitch black...\r\n");
     } else {
       send_to_char(ch, "You look inside and see:\r\n");
       look_at_room(vehicle_inside, ch, 0);
     }
   } else {
     send_to_char(ch, "You cannot see inside that.\r\n");
   }
 } else if (GET_OBJ_TYPE(obj) == ITEM_WINDOW) {
   look_out_window(ch, arg);
 } else if ((GET_OBJ_TYPE(obj) != ITEM_DRINKCON) &&
            (GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN) &&
            (GET_OBJ_TYPE(obj) != ITEM_CONTAINER)&&
            (GET_OBJ_TYPE(obj) != ITEM_PORTAL))     {
   send_to_char(ch, "There's nothing inside that!\r\n");
 } else if ((GET_OBJ_TYPE(obj) == ITEM_CONTAINER) ||
            (GET_OBJ_TYPE(obj) == ITEM_PORTAL)) {
     if (OBJVAL_FLAGGED(obj, CONT_CLOSED))
       send_to_char(ch, "It is closed.\r\n");
     else {
       send_to_char(ch, "%s", fname(obj->name));
       switch (bits) {
       case FIND_OBJ_INV:
         send_to_char(ch, " (carried): \r\n");
         break;
       case FIND_OBJ_ROOM:
         send_to_char(ch, " (here): \r\n");
         break;
       case FIND_OBJ_EQUIP:
         send_to_char(ch, " (used): \r\n");
         break;
       }

       list_obj_to_char(obj->contains, ch, SHOW_OBJ_SHORT, TRUE);
     }
   } else {            /* item must be a fountain or drink container */
       if (GET_OBJ_VAL(obj, VAL_DRINKCON_HOWFULL) <= 0)
          send_to_char(ch, "It is empty.\r\n");
          else {
        if (GET_OBJ_VAL(obj, VAL_DRINKCON_CAPACITY) < 0)
        {
         char buf2[MAX_STRING_LENGTH];
         sprinttype(GET_OBJ_VAL(obj, VAL_DRINKCON_LIQUID), color_liquid, buf2, sizeof(buf2));
         send_to_char(ch, "It's full of a %s liquid.\r\n", buf2);
        }
       else if (GET_OBJ_VAL(obj,VAL_DRINKCON_HOWFULL) > GET_OBJ_VAL(obj,VAL_DRINKCON_CAPACITY)) {
         send_to_char(ch, "Its contents seem somewhat murky.\r\n"); /* BUG */
       } else {
          char buf2[MAX_STRING_LENGTH];
          amt = (GET_OBJ_VAL(obj, VAL_DRINKCON_HOWFULL) * 3) / GET_OBJ_VAL(obj, VAL_DRINKCON_CAPACITY);
          sprinttype(GET_OBJ_VAL(obj, VAL_DRINKCON_LIQUID), color_liquid, buf2, sizeof(buf2));
          send_to_char(ch, "It's %sfull of a %s liquid.\r\n", fullness[amt], buf2);
       }
          }
   }
}


char *find_exdesc(char *word, struct extra_descr_data *list)
{
  struct extra_descr_data *i;

  for (i = list; i; i = i->next)
    if (isname(word, i->keyword))
      return (i->description);

  return (NULL);
}


/*
 * Given the argument "look at <target>", figure out what object or char
 * matches the target.  First, see if there is another char in the room
 * with the name.  Then check local objs for exdescs.
 *
 * Thanks to Angus Mezick <angus@EDGIL.CCMAIL.COMPUSERVE.COM> for the
 * suggested fix to this problem.
 */
void look_at_target(struct char_data *ch, char *arg, int read)
{
  int bits, found = FALSE, j, fnum, i = 0, msg = 1;
  struct char_data *found_char = NULL;
  struct obj_data *obj, *found_obj = NULL;
  char *desc;
  char number[MAX_STRING_LENGTH];

  if (!ch->desc)
    return;

  if (!*arg) {
    send_to_char(ch, "Look at what?\r\n");
    return;
  }

  if (read) {
    for (obj = ch->carrying; obj;obj=obj->next_content) {
      if(GET_OBJ_TYPE(obj) == ITEM_BOARD) {
	found = TRUE;
	break;
      }
    }
    if(!obj) {
      for (obj = world[ch->in_room].contents; obj;obj=obj->next_content) {
	if(GET_OBJ_TYPE(obj) == ITEM_BOARD) {
	  found = TRUE;
	  break;
	}
      }
    }
    if (obj && found) {
      arg = one_argument(arg, number);
      if (!*number) {
	send_to_char(ch,"Read what?\r\n");
	return;
      }
      
      /* Okay, here i'm faced with the fact that the person could be
	 entering in something like 'read 5' or 'read 4.mail' .. so, whats the
	 difference between the two?  Well, there's a period in the second,
	 so, we'll just stick with that basic difference */
      
      if (isname(number, obj->name)) {
	show_board(GET_OBJ_VNUM(obj), ch);
      } else if ((!isdigit(*number) || (!(msg = atoi(number)))) ||
		 (strchr(number,'.'))) {
	sprintf(arg,"%s %s", number,arg);
	look_at_target(ch, arg, 0);
      } else {
	board_display_msg(GET_OBJ_VNUM(obj), ch, msg);
      }
      return;
      }
    }
  bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP |
		      FIND_CHAR_ROOM, ch, &found_char, &found_obj);

  /* Is the target a character? */
  if (found_char != NULL) {
    look_at_char(found_char, ch);
    if (ch != found_char) {
      if (CAN_SEE(found_char, ch))
	act("$n looks at you.", TRUE, ch, 0, found_char, TO_VICT);
      act("$n looks at $N.", TRUE, ch, 0, found_char, TO_NOTVICT);
    }
    return;
  }

  /* Strip off "number." from 2.foo and friends. */
  if (!(fnum = get_number(&arg))) {
    send_to_char(ch, "Look at what?\r\n");
    return;
  }

  /* Does the argument match an extra desc in the room? */
  if ((desc = find_exdesc(arg, world[IN_ROOM(ch)].ex_description)) != NULL && ++i == fnum) {
    page_string(ch->desc, desc, FALSE);
    return;
  } 

  /* Does the argument match an extra desc in the char's equipment? */
  for (j = 0; j < NUM_WEARS && !found; j++)
    if (GET_EQ(ch, j) && CAN_SEE_OBJ(ch, GET_EQ(ch, j)))
      if ((desc = find_exdesc(arg, GET_EQ(ch, j)->ex_description)) != NULL && ++i == fnum) {
	send_to_char(ch, "%s", desc);
        diag_obj_to_char(GET_EQ(ch, j), ch);
        send_to_char(ch, "It appears to be made of %s.\r\n",
         material_names[GET_OBJ_MATERIAL(GET_EQ(ch, j))]);
	found = TRUE;
      }

  /* Does the argument match an extra desc in the char's inventory? */
  for (obj = ch->carrying; obj && !found; obj = obj->next_content) {
    if (CAN_SEE_OBJ(ch, obj))
      if ((desc = find_exdesc(arg, obj->ex_description)) != NULL && ++i == fnum) {
	if(GET_OBJ_TYPE(obj) == ITEM_BOARD) {
	  show_board(GET_OBJ_VNUM(obj), ch);
	} else {
	send_to_char(ch, "%s", desc);
        diag_obj_to_char(obj, ch);
        send_to_char(ch, "It appears to be made of %s.\r\n",
         material_names[GET_OBJ_MATERIAL(obj)]);
	}
	found = TRUE;
      }
  }

  /* Does the argument match an extra desc of an object in the room? */
  for (obj = world[IN_ROOM(ch)].contents; obj && !found; obj = obj->next_content)
    if (CAN_SEE_OBJ(ch, obj))
      if ((desc = find_exdesc(arg, obj->ex_description)) != NULL && ++i == fnum) {
	if(GET_OBJ_TYPE(obj) == ITEM_BOARD) {
	  show_board(GET_OBJ_VNUM(obj), ch);
	} else {
	send_to_char(ch, "%s", desc);
        diag_obj_to_char(obj, ch);
        send_to_char(ch, "It appears to be made of %s.\r\n",
         material_names[GET_OBJ_MATERIAL(obj)]);
	}
	found = TRUE;
      }

  /* If an object was found back in generic_find */
  if (bits) {
    if (!found)
      show_obj_to_char(found_obj, ch, SHOW_OBJ_ACTION);
    else {
      if (show_obj_modifiers(found_obj, ch))
      send_to_char(ch, "\r\n");
    }
  } else if (!found)
    send_to_char(ch, "You do not see that here.\r\n");
}

void look_out_window(struct char_data *ch, char *arg)
{
  struct obj_data *i, *viewport = NULL, *vehicle = NULL;
  struct char_data *dummy = NULL;
  room_rnum target_room = NOWHERE;
  int bits, door;

  /* First, lets find something to look out of or through. */
  if (*arg) {
    /* Find this object and see if it is a window */
    if (!(bits = generic_find(arg,
              FIND_OBJ_ROOM | FIND_OBJ_INV | FIND_OBJ_EQUIP,
              ch, &dummy, &viewport))) {
      send_to_char(ch, "You don't see that here.\r\n");
      return;
    } else if (GET_OBJ_TYPE(viewport) != ITEM_WINDOW) {
      send_to_char(ch, "You can't look out that!\r\n");
    return;
  }
  } else if (OUTSIDE(ch)) {
      /* yeah, sure stupid */
      send_to_char(ch, "But you are already outside.\r\n");
    return;
  } else {
    /* Look for any old window in the room */
    for (i = world[IN_ROOM(ch)].contents; i; i = i->next_content)
      if ((GET_OBJ_TYPE(i) == ITEM_WINDOW) &&
           isname("window", i->name)) {
        viewport = i;
      continue;
      }
  }
  if (!viewport) {
    /* Nothing suitable to look through */
    send_to_char(ch, "You don't seem to be able to see outside.\r\n");
  } else if (OBJVAL_FLAGGED(viewport, CONT_CLOSEABLE) &&
             OBJVAL_FLAGGED(viewport, CONT_CLOSED)) {
    /* The window is closed */
    send_to_char(ch, "It is closed.\r\n");
  } else {
    if (GET_OBJ_VAL(viewport, 0) < 0) {
      /* We are looking out of the room */
      if (GET_OBJ_VAL(viewport, 3) < 0) {
        /* Look for the default "outside" room */
        for (door = 0; door < NUM_OF_DIRS; door++)
          if (EXIT(ch, door))
            if (EXIT(ch, door)->to_room != NOWHERE)
              if (!ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_INDOORS)) {
                target_room = EXIT(ch, door)->to_room;
                continue;
              }
      } else {
        target_room = real_room(GET_OBJ_VAL(viewport, 3));
      }
    } else {
      /* We are looking out of a vehicle */
      if ( (vehicle = find_vehicle_by_vnum(GET_OBJ_VAL(viewport, 0))) );
        target_room = vehicle->in_room;
    }
    if (target_room == NOWHERE) {
      send_to_char(ch, "You don't seem to be able to see outside.\r\n");
    } else {
      if (viewport->action_description)
        act(viewport->action_description, TRUE, ch, viewport, 0, TO_CHAR);
      else
        send_to_char(ch, "You look outside and see:\r\n");
      look_at_room(target_room, ch, 0);
    }
  }
}

ACMD(do_look)
{
  int look_type;

  if (!ch->desc)
    return;

  if (GET_POS(ch) < POS_SLEEPING)
    send_to_char(ch, "You can't see anything but stars!\r\n");
  else if (AFF_FLAGGED(ch, AFF_BLIND))
    send_to_char(ch, "You can't see a damned thing, you're blind!\r\n");
  else if (IS_DARK(IN_ROOM(ch)) && !CAN_SEE_IN_DARK(ch)) {
    send_to_char(ch, "It is pitch black...\r\n");
    list_char_to_char(world[IN_ROOM(ch)].people, ch);	/* glowing red eyes */
  } else {
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    if (subcmd == SCMD_READ) {
      one_argument(argument, arg);
      if (!*arg)
	send_to_char(ch, "Read what?\r\n");
      else
	look_at_target(ch, arg, 1);
      return;
    }
    argument = any_one_arg(argument, arg);
    one_argument(argument, arg2);
    if (!*arg) {
      if (subcmd == SCMD_SEARCH)
        send_to_char(ch, "You need to search in a particular direction.\r\n");
      else
      look_at_room(IN_ROOM(ch), ch, 1);
    } else if (is_abbrev(arg, "inside")   && EXIT(ch, INDIR) && !*arg2) {
      if (subcmd == SCMD_SEARCH)
        search_in_direction(ch, INDIR);
      else
        look_in_direction(ch, INDIR);
    } else if (is_abbrev(arg, "inside") && (subcmd == SCMD_SEARCH) && !*arg2) {
      search_in_direction(ch, INDIR);
    } else if (is_abbrev(arg, "inside")   ||
               is_abbrev(arg, "into")       )  { 
      look_in_obj(ch, arg2);
    } else if ((is_abbrev(arg, "outside") || 
                is_abbrev(arg, "through") ||
	        is_abbrev(arg, "thru")      ) && 
               (subcmd == SCMD_LOOK) && *arg2) {
      look_out_window(ch, arg2);
    } else if (is_abbrev(arg, "outside") && 
               (subcmd == SCMD_LOOK) && !EXIT(ch, OUTDIR)) {
      look_out_window(ch, arg2);
    } else if ((look_type = search_block(arg, dirs, FALSE)) >= 0 ||
               (look_type = search_block(arg, abbr_dirs, FALSE)) >= 0) {
      if (subcmd == SCMD_SEARCH)
        search_in_direction(ch, look_type);
      else
        look_in_direction(ch, look_type);
    } else if ((is_abbrev(arg, "towards")) &&
               ((look_type = search_block(arg2, dirs, FALSE)) >= 0 ||
                (look_type = search_block(arg2, abbr_dirs, FALSE)) >= 0 )) {
      if (subcmd == SCMD_SEARCH)
        search_in_direction(ch, look_type);
      else
      look_in_direction(ch, look_type);
    } else if (is_abbrev(arg, "at")) {
      if (subcmd == SCMD_SEARCH)
        send_to_char(ch, "That is not a direction!\r\n");
      else
      look_at_target(ch, arg2, 0);
    } else if (find_exdesc(arg, world[IN_ROOM(ch)].ex_description) != NULL) {
      look_at_target(ch, arg, 0);
    } else {
      if (subcmd == SCMD_SEARCH)
        send_to_char(ch, "That is not a direction!\r\n");
    else
      look_at_target(ch, arg, 0);
  }
  }
}



ACMD(do_examine)
{
  struct char_data *tmp_char;
  struct obj_data *tmp_object;
  char tempsave[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Examine what?\r\n");
    return;
  }

  /* look_at_target() eats the number. */
  look_at_target(ch, strcpy(tempsave, arg),0);	/* strcpy: OK */

  generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_CHAR_ROOM |
		      FIND_OBJ_EQUIP, ch, &tmp_char, &tmp_object);

  if (tmp_object) {
    if ((GET_OBJ_TYPE(tmp_object) == ITEM_DRINKCON) ||
	(GET_OBJ_TYPE(tmp_object) == ITEM_FOUNTAIN) ||
	(GET_OBJ_TYPE(tmp_object) == ITEM_CONTAINER)) {
      send_to_char(ch, "When you look inside, you see:\r\n");
      look_in_obj(ch, arg);
    }
  }
}



ACMD(do_gold)
{
  if (GET_GOLD(ch) == 0)
    send_to_char(ch, "You're broke!\r\n");
  else if (GET_GOLD(ch) == 1)
    send_to_char(ch, "You have one miserable little gold coin.\r\n");
  else
    send_to_char(ch, "You have %d gold coins.\r\n", GET_GOLD(ch));
}


ACMD(do_score)
{
  struct time_info_data playing_time;

  if (IS_NPC(ch))
    return;

  send_to_char(ch, "You are %d years old.\r\n", GET_AGE(ch));

  if (age(ch)->month == 0 && age(ch)->day == 0)
    send_to_char(ch, "  It's your birthday today.\r\n");
  else
    send_to_char(ch, "\r\n");

  send_to_char(ch, "You have %d(%d) hit, %d(%d) mana and %d(%d) movement points.\r\n",
	  GET_HIT(ch), GET_MAX_HIT(ch), GET_MANA(ch), GET_MAX_MANA(ch),
	  GET_MOVE(ch), GET_MAX_MOVE(ch));

  send_to_char(ch, "Your armor class is %d/10, and your alignment is %d.\r\n",
	  compute_armor_class(ch), GET_ALIGNMENT(ch));

  send_to_char(ch, "You have scored %d exp, and have %d gold coins.\r\n",
	  GET_EXP(ch), GET_GOLD(ch));

  if (GET_LEVEL(ch) < LVL_IMMORT)
    send_to_char(ch, "You need %d exp to reach your next level.\r\n",
	level_exp(GET_CLASS(ch), GET_LEVEL(ch) + 1) - GET_EXP(ch));

  playing_time = *real_time_passed((time(0) - ch->player.time.logon) +
				  ch->player.time.played, 0);
  send_to_char(ch, "You have been playing for %d day%s and %d hour%s.\r\n",
     playing_time.day, playing_time.day == 1 ? "" : "s",
     playing_time.hours, playing_time.hours == 1 ? "" : "s");

  send_to_char(ch, "This ranks you as %s %s (level %d).\r\n",
	  GET_NAME(ch), GET_TITLE(ch), GET_LEVEL(ch));

  send_to_char(ch, "Your race is %s, and your class is %s.\r\n",
          pc_race_types[(int)GET_RACE(ch)], pc_class_types[(int)GET_CLASS(ch)]);

  switch (GET_POS(ch)) {
  case POS_DEAD:
    send_to_char(ch, "You are DEAD!\r\n");
    break;
  case POS_MORTALLYW:
    send_to_char(ch, "You are mortally wounded!  You should seek help!\r\n");
    break;
  case POS_INCAP:
    send_to_char(ch, "You are incapacitated, slowly fading away...\r\n");
    break;
  case POS_STUNNED:
    send_to_char(ch, "You are stunned!  You can't move!\r\n");
    break;
  case POS_SLEEPING:
    send_to_char(ch, "You are sleeping.\r\n");
    break;
  case POS_RESTING:
    send_to_char(ch, "You are resting.\r\n");
    break;
  case POS_SITTING:
    send_to_char(ch, "You are sitting.\r\n");
    break;
  case POS_FIGHTING:
    send_to_char(ch, "You are fighting %s.\r\n", FIGHTING(ch) ? PERS(FIGHTING(ch), ch) : "thin air");
    break;
  case POS_STANDING:
    send_to_char(ch, "You are standing.\r\n");
    break;
  default:
    send_to_char(ch, "You are floating.\r\n");
    break;
  }

  if (GET_COND(ch, DRUNK) > 10)
    send_to_char(ch, "You are intoxicated.\r\n");

  if (GET_COND(ch, FULL) == 0)
    send_to_char(ch, "You are hungry.\r\n");

  if (GET_COND(ch, THIRST) == 0)
    send_to_char(ch, "You are thirsty.\r\n");

  if (AFF_FLAGGED(ch, AFF_BLIND))
    send_to_char(ch, "You have been blinded!\r\n");

  if (AFF_FLAGGED(ch, AFF_INVISIBLE))
    send_to_char(ch, "You are invisible.\r\n");

  if (AFF_FLAGGED(ch, AFF_DETECT_INVIS))
    send_to_char(ch, "You are sensitive to the presence of invisible things.\r\n");

  if (AFF_FLAGGED(ch, AFF_SANCTUARY))
    send_to_char(ch, "You are protected by Sanctuary.\r\n");

  if (AFF_FLAGGED(ch, AFF_POISON))
    send_to_char(ch, "You are poisoned!\r\n");

  if (AFF_FLAGGED(ch, AFF_CHARM))
    send_to_char(ch, "You have been charmed!\r\n");

  if (affected_by_spell(ch, SPELL_ARMOR))
    send_to_char(ch, "You feel protected.\r\n");

  if (AFF_FLAGGED(ch, AFF_INFRAVISION))
    send_to_char(ch, "You can see in darkness with infravision.\r\n");

  if (PRF_FLAGGED(ch, PRF_SUMMONABLE))
    send_to_char(ch, "You are summonable by other players.\r\n");

  if (AFF_FLAGGED(ch, AFF_DETECT_ALIGN))
    send_to_char(ch, "You see into the hearts of others.\r\n");

  if (AFF_FLAGGED(ch, AFF_DETECT_MAGIC))
    send_to_char(ch, "You are sensitive to the magical nature of things.\r\n");

}


ACMD(do_inventory)
{
  send_to_char(ch, "You are carrying:\r\n");
  list_obj_to_char(ch->carrying, ch, SHOW_OBJ_SHORT, TRUE);
}


ACMD(do_equipment)
{
  int i, found = 0;

  send_to_char(ch, "You are using:\r\n");
  for (i = 0; i < NUM_WEARS; i++) {
    if (GET_EQ(ch, i)) {
      if (CAN_SEE_OBJ(ch, GET_EQ(ch, i))) {
	send_to_char(ch, "%s", wear_where[i]);
	show_obj_to_char(GET_EQ(ch, i), ch, SHOW_OBJ_SHORT);
	found = TRUE;
      } else {
	send_to_char(ch, "%s", wear_where[i]);
	send_to_char(ch, "Something.\r\n");
	found = TRUE;
      }
    }
  }
  if (!found)
    send_to_char(ch, " Nothing.\r\n");
}


ACMD(do_time)
{
  const char *suf;
  int weekday, day;

  /* day in [1..35] */
  day = time_info.day + 1;

  /* 35 days in a month, 7 days a week */
  weekday = ((35 * time_info.month) + day) % 7;

  send_to_char(ch, "It is %d o'clock %s, on %s.\r\n",
	  (time_info.hours % 12 == 0) ? 12 : (time_info.hours % 12),
	  time_info.hours >= 12 ? "pm" : "am", weekdays[weekday]);

  /*
   * Peter Ajamian <peter@PAJAMIAN.DHS.ORG> supplied the following as a fix
   * for a bug introduced in the ordinal display that caused 11, 12, and 13
   * to be incorrectly displayed as 11st, 12nd, and 13rd.  Nate Winters
   * <wintersn@HOTMAIL.COM> had already submitted a fix, but it hard-coded a
   * limit on ordinal display which I want to avoid.	-dak
   */

  suf = "th";

  if (((day % 100) / 10) != 1) {
    switch (day % 10) {
    case 1:
      suf = "st";
      break;
    case 2:
      suf = "nd";
      break;
    case 3:
      suf = "rd";
      break;
    }
  }

  send_to_char(ch, "The %d%s Day of the %s, Year %d.\r\n",
	  day, suf, month_name[time_info.month], time_info.year);
}


ACMD(do_weather)
{
  const char *sky_look[] = {
    "cloudless",
    "cloudy",
    "rainy",
    "lit by flashes of lightning"
  };

  if (OUTSIDE(ch))
    {
    send_to_char(ch, "The sky is %s and %s.\r\n", sky_look[weather_info.sky],
	    weather_info.change >= 0 ? "you feel a warm wind from south" :
	     "your foot tells you bad weather is due");
    if (GET_LEVEL(ch) >= LVL_GOD)
      send_to_char(ch, "Pressure: %d (change: %d), Sky: %d (%s)\r\n",
                 weather_info.pressure,
                 weather_info.change,
                 weather_info.sky,
                 sky_look[weather_info.sky]);
    }
  else
    send_to_char(ch, "You have no feeling about the weather at all.\r\n");
}


ACMD(do_help)
{
  int chk, bot, top, mid, minlen;

  if (!ch->desc)
    return;

  skip_spaces(&argument);

  if (!*argument) {
    page_string(ch->desc, help, 0);
    return;
  }
  if (!help_table) {
    send_to_char(ch, "No help available.\r\n");
    return;
  }

  bot = 0;
  top = top_of_helpt;
  minlen = strlen(argument);

  for (;;) {
    mid = (bot + top) / 2;

    if (bot > top) {
      send_to_char(ch, "There is no help on that word.\r\n");
      mudlog(NRM, MAX(LVL_GRGOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s tried to get help on %s", GET_NAME(ch), argument);
      return;
    } else if (!(chk = strn_cmp(argument, help_table[mid].keyword, minlen))) {
      /* trace backwards to find first matching entry. Thanks Jeff Fink! */
      while ((mid > 0) &&
	 (!(chk = strn_cmp(argument, help_table[mid - 1].keyword, minlen))))
	mid--;
      page_string(ch->desc, help_table[mid].entry, 0);
      return;
    } else {
      if (chk > 0)
        bot = mid + 1;
      else
        top = mid - 1;
    }
  }
}



#define WHO_FORMAT \
"format: who [minlev[-maxlev]] [-n name] [-s] [-o] [-q] [-r] [-z]\r\n"

/* FIXME: This whole thing just needs rewritten. */
ACMD(do_who)
{
  struct descriptor_data *d;
  struct char_data *tch;
  char name_search[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
  char mode;
  int low = 0, high = LVL_IMPL, localwho = 0, questwho = 0;
  int showclass = 0, short_list = 0, outlaws = 0, num_can_see = 0;
  int who_room = 0, showrace = 0;


  skip_spaces(&argument);
  strcpy(buf, argument);	/* strcpy: OK (sizeof: argument == buf) */
  name_search[0] = '\0';

  while (*buf) {
    char arg[MAX_INPUT_LENGTH], buf1[MAX_INPUT_LENGTH];

    half_chop(buf, arg, buf1);
    if (isdigit(*arg)) {
      sscanf(arg, "%d-%d", &low, &high);
      strcpy(buf, buf1);	/* strcpy: OK (sizeof: buf1 == buf) */
    } else if (*arg == '-') {
      mode = *(arg + 1);       /* just in case; we destroy arg in the switch */
      switch (mode) {
      case 'o':
      case 'k':
	outlaws = 1;
	strcpy(buf, buf1);	/* strcpy: OK (sizeof: buf1 == buf) */
	break;
      case 'z':
	localwho = 1;
	strcpy(buf, buf1);	/* strcpy: OK (sizeof: buf1 == buf) */
	break;
      case 's':
	short_list = 1;
	strcpy(buf, buf1);	/* strcpy: OK (sizeof: buf1 == buf) */
	break;
      case 'q':
	questwho = 1;
	strcpy(buf, buf1);	/* strcpy: OK (sizeof: buf1 == buf) */
	break;
      case 'l':
	half_chop(buf1, arg, buf);
	sscanf(arg, "%d-%d", &low, &high);
	break;
      case 'n':
	half_chop(buf1, name_search, buf);
	break;
      case 'r':
	who_room = 1;
	strcpy(buf, buf1);	/* strcpy: OK (sizeof: buf1 == buf) */
	break;
      default:
	send_to_char(ch, "%s", WHO_FORMAT);
	return;
      }				/* end of switch */

    } else {			/* endif */
      send_to_char(ch, "%s", WHO_FORMAT);
      return;
    }
  }				/* end while (parser) */

  send_to_char(ch, "Players\r\n-------\r\n");

  for (d = descriptor_list; d; d = d->next) {
    if (!IS_PLAYING(d))
      continue;

    if (d->original)
      tch = d->original;
    else if (!(tch = d->character))
      continue;

    if (*name_search && str_cmp(GET_NAME(tch), name_search) &&
	!strstr(GET_TITLE(tch), name_search))
      continue;
    if (!CAN_SEE(ch, tch) || GET_LEVEL(tch) < low || GET_LEVEL(tch) > high)
      continue;
    if (outlaws && !PLR_FLAGGED(tch, PLR_KILLER) &&
	!PLR_FLAGGED(tch, PLR_THIEF))
      continue;
    if (questwho && !PRF_FLAGGED(tch, PRF_QUEST))
      continue;
    if (localwho && world[IN_ROOM(ch)].zone != world[IN_ROOM(tch)].zone)
      continue;
    if (who_room && (IN_ROOM(tch) != IN_ROOM(ch)))
      continue;
    if (showclass && !(showclass & (1 << GET_CLASS(tch))))
      continue;
     if (showrace && !(showrace & (1 << GET_RACE(tch))))
       continue;
    if (short_list) {
      send_to_char(ch, "%s[%2d %s %s] %-12.12s%s%s",
	      (GET_LEVEL(tch) >= LVL_IMMORT ? CCYEL(ch, C_BRI) : ""),
	      GET_LEVEL(tch), CLASS_ABBR(tch), RACE_ABBR(tch), GET_NAME(tch),
	      (GET_LEVEL(tch) >= LVL_IMMORT ? CCNRM(ch, C_BRI) : ""),
	      ((!(++num_can_see % 4)) ? "\r\n" : ""));
    } else {
      num_can_see++;
      send_to_char(ch, "%s[%2d %s %s] %s%s%s",
	      (GET_LEVEL(tch) >= LVL_IMMORT ? CCYEL(ch, C_BRI) : ""),
	      GET_LEVEL(tch), CLASS_ABBR(tch), RACE_ABBR(tch), GET_NAME(tch),
              *GET_TITLE(tch) ? " " : "", GET_TITLE(tch));

      if (GET_INVIS_LEV(tch))
	send_to_char(ch, " (i%d)", GET_INVIS_LEV(tch));
      else if (AFF_FLAGGED(tch, AFF_INVISIBLE))
	send_to_char(ch, " (invis)");

      if (PLR_FLAGGED(tch, PLR_MAILING))
	send_to_char(ch, " (mailing)");
      else if (d->olc)
        send_to_char(ch, " (OLC)");
      else if (PLR_FLAGGED(tch, PLR_WRITING))
	send_to_char(ch, " (writing)");

      if (d->original)
        send_to_char(ch, " (out of body)");

      if (d->connected == CON_OEDIT)
        send_to_char(ch, " (Object Edit)");
      if (d->connected == CON_MEDIT)
        send_to_char(ch, " (Mobile Edit)");
      if (d->connected == CON_ZEDIT)
        send_to_char(ch, " (Zone Edit)");
      if (d->connected == CON_SEDIT)
        send_to_char(ch, " (Shop Edit)");
      if (d->connected == CON_REDIT)
        send_to_char(ch, " (Room Edit)");
      if (d->connected == CON_TEDIT)
        send_to_char(ch, " (Text Edit)");
      if (d->connected == CON_TRIGEDIT)
        send_to_char(ch, " (Trigger Edit)");
      if (d->connected == CON_AEDIT)
        send_to_char(ch, " (Social Edit)");
      if (d->connected == CON_CEDIT)
        send_to_char(ch, " (Configuration Edit)");

      if (PRF_FLAGGED(tch, PRF_BUILDWALK))
      send_to_char(ch, " (Buildwalking)");

      if (PRF_FLAGGED(tch, PRF_DEAF))
	send_to_char(ch, " (deaf)");
      if (PRF_FLAGGED(tch, PRF_NOTELL))
	send_to_char(ch, " (notell)");
      if (PRF_FLAGGED(tch, PRF_NOGOSS))
        send_to_char(ch, " (nogos)");
      if (PRF_FLAGGED(tch, PRF_QUEST))
	send_to_char(ch, " (quest)");
      if (PLR_FLAGGED(tch, PLR_THIEF))
	send_to_char(ch, " (THIEF)");
      if (PLR_FLAGGED(tch, PLR_KILLER))
	send_to_char(ch, " (KILLER)");
      if (PRF_FLAGGED(tch, PRF_BUILDWALK))
	send_to_char(ch, " (Buildwalking)");
      if (PRF_FLAGGED(tch, PRF_AFK))
        send_to_char(ch, " (AFK)");
      if (PRF_FLAGGED(tch, PRF_NOWIZ))
        send_to_char(ch, " (nowiz)");
      if (GET_LEVEL(tch) >= LVL_IMMORT)
	send_to_char(ch, CCNRM(ch, C_BRI));
      send_to_char(ch, "\r\n");
    }				/* endif shortlist */
  }				/* end of for */
  if (short_list && (num_can_see % 4))
    send_to_char(ch, "\r\n");
  if (num_can_see == 0)
    send_to_char(ch, "\r\nNobody at all!\r\n");
  else if (num_can_see == 1)
    send_to_char(ch, "\r\nOne lonely character displayed.\r\n");
  else
    send_to_char(ch, "\r\n%d characters displayed.\r\n", num_can_see);
}


#define USERS_FORMAT \
"format: users [-l minlevel[-maxlevel]] [-n name] [-h host] [-o] [-p]\r\n"

/* BIG OL' FIXME: Rewrite it all. Similar to do_who(). */
ACMD(do_users)
{
  char line[200], line2[220], idletime[10];
  char state[30], *timeptr, mode;
  char name_search[MAX_INPUT_LENGTH], host_search[MAX_INPUT_LENGTH];
  struct char_data *tch;
  struct descriptor_data *d;
  int low = 0, high = LVL_IMPL, num_can_see = 0;
  int showclass = 0, outlaws = 0, playing = 0, deadweight = 0, showrace = 0;
  char buf[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];

  host_search[0] = name_search[0] = '\0';

  strcpy(buf, argument);	/* strcpy: OK (sizeof: argument == buf) */
  while (*buf) {
    char buf1[MAX_INPUT_LENGTH];

    half_chop(buf, arg, buf1);
    if (*arg == '-') {
      mode = *(arg + 1);  /* just in case; we destroy arg in the switch */
      switch (mode) {
      case 'o':
      case 'k':
	outlaws = 1;
	playing = 1;
	strcpy(buf, buf1);	/* strcpy: OK (sizeof: buf1 == buf) */
	break;
      case 'p':
	playing = 1;
	strcpy(buf, buf1);	/* strcpy: OK (sizeof: buf1 == buf) */
	break;
      case 'd':
	deadweight = 1;
	strcpy(buf, buf1);	/* strcpy: OK (sizeof: buf1 == buf) */
	break;
      case 'l':
	playing = 1;
	half_chop(buf1, arg, buf);
	sscanf(arg, "%d-%d", &low, &high);
	break;
      case 'n':
	playing = 1;
	half_chop(buf1, name_search, buf);
	break;
      case 'h':
	playing = 1;
	half_chop(buf1, host_search, buf);
	break;
      default:
	send_to_char(ch, "%s", USERS_FORMAT);
	return;
      }				/* end of switch */

    } else {			/* endif */
      send_to_char(ch, "%s", USERS_FORMAT);
      return;
    }
  }				/* end while (parser) */
  send_to_char(ch,
         "Num Name         State          Idl Login    C Site\r\n"
         "--- ------------ -------------- --- -------- - -----------------------\r\n");

  one_argument(argument, arg);

  for (d = descriptor_list; d; d = d->next) {
    if (STATE(d) != CON_PLAYING && playing)
      continue;
    if (STATE(d) == CON_PLAYING && deadweight)
      continue;
    if (IS_PLAYING(d)) {
      if (d->original)
	tch = d->original;
      else if (!(tch = d->character))
	continue;

      if (*host_search && !strstr(d->host, host_search))
	continue;
      if (*name_search && str_cmp(GET_NAME(tch), name_search))
	continue;
      if (!CAN_SEE(ch, tch) || GET_LEVEL(tch) < low || GET_LEVEL(tch) > high)
	continue;
      if (outlaws && !PLR_FLAGGED(tch, PLR_KILLER) &&
	  !PLR_FLAGGED(tch, PLR_THIEF))
	continue;
      if (showclass && !(showclass & (1 << GET_CLASS(tch))))
	continue;
      if (showrace && !(showrace & (1 << GET_RACE(tch))))
        continue;
      if (GET_INVIS_LEV(ch) > GET_LEVEL(ch))
	continue;
    }

    timeptr = asctime(localtime(&d->login_time));
    timeptr += 11;
    *(timeptr + 8) = '\0';

    if (STATE(d) == CON_PLAYING && d->original)
      strcpy(state, "Switched");
    else
      strcpy(state, connected_types[STATE(d)]);

    if (d->character && STATE(d) == CON_PLAYING && GET_LEVEL(d->character) < LVL_GOD)
      sprintf(idletime, "%3d", d->character->char_specials.timer *
	      SECS_PER_MUD_HOUR / SECS_PER_REAL_MIN);
    else
      strcpy(idletime, "");

    sprintf(line, "%3d %-12s %-14s %-3s %-8s %1s ", d->desc_num,
	d->original && d->original->player.name ? d->original->player.name :
	d->character && d->character->player.name ? d->character->player.name :
	"UNDEFINED", state, idletime, timeptr,
        d->comp->state ? d->comp->state == 1 ? "?" : "Y" : "N");

    if (d->host && *d->host)
      sprintf(line + strlen(line), "[%s]\r\n", d->host);
    else
      strcat(line, "[Hostname unknown]\r\n");

    if (STATE(d) != CON_PLAYING) {
      sprintf(line2, "%s%s%s", CCGRN(ch, C_BRI), line, CCNRM(ch, C_BRI));
      strcpy(line, line2);
    }
    if (STATE(d) != CON_PLAYING ||
		(STATE(d) == CON_PLAYING && CAN_SEE(ch, d->character))) {
      send_to_char(ch, "%s", line);
      num_can_see++;
    }
  }

  send_to_char(ch, "\r\n%d visible sockets connected.\r\n", num_can_see);
}


/* Generic page_string function for displaying text */
ACMD(do_gen_ps)
{
  int patch_num, show_patches;
  char arg[MAX_INPUT_LENGTH];
  const char *version_args[] = {"full", "complete", "\n"}; 
  one_argument(argument, arg);
  
  switch (subcmd) {
  case SCMD_CREDITS:
    page_string(ch->desc, credits, 0);
    break;
  case SCMD_NEWS:
    page_string(ch->desc, news, 0);
    break;
  case SCMD_INFO:
    page_string(ch->desc, info, 0);
    break;
  case SCMD_WIZLIST:
    page_string(ch->desc, wizlist, 0);
    break;
  case SCMD_IMMLIST:
    page_string(ch->desc, immlist, 0);
    break;
  case SCMD_HANDBOOK:
    page_string(ch->desc, handbook, 0);
    break;
  case SCMD_POLICIES:
    page_string(ch->desc, policies, 0);
    break;
  case SCMD_MOTD:
    page_string(ch->desc, motd, 0);
    break;
  case SCMD_IMOTD:
    page_string(ch->desc, imotd, 0);
    break;
  case SCMD_CLEAR:
    send_to_char(ch, "\033[H\033[J");
    break;
  case SCMD_VERSION:
    if (!*arg)
      show_patches = FALSE;
    else
      if (search_block(arg, version_args, FALSE ) != -1)
        show_patches = TRUE;
      else
      show_patches = FALSE;
    send_to_char(ch, "%s\r\n", circlemud_version);
    send_to_char(ch, "%s\r\n", oasisolc_version);
    send_to_char(ch, "%s\r\n", DG_SCRIPT_VERSION);
    send_to_char(ch, "%s\r\n", CWG_VERSION);
    if ((GET_LEVEL(ch) >= LVL_IMMORT) && (show_patches == TRUE)) {
      send_to_char(ch, "The following patches have been installed:\r\n");
      for (patch_num = 0; **(patch_list + patch_num) != '\n'; patch_num++)
        send_to_char(ch, "%2d: %s\r\n", patch_num, patch_list[patch_num]);
    }
    break;
  case SCMD_WHOAMI:
    send_to_char(ch, "%s\r\n", GET_NAME(ch));
    break;
  default:
    log("SYSERR: Unhandled case in do_gen_ps. (%d)", subcmd);
    return;
  }
}


void perform_mortal_where(struct char_data *ch, char *arg)
{
  struct char_data *i;
  struct descriptor_data *d;

  if (!*arg) {
    send_to_char(ch, "Players in your Zone\r\n--------------------\r\n");
    for (d = descriptor_list; d; d = d->next) {
      if (STATE(d) != CON_PLAYING || d->character == ch)
	continue;
      if ((i = (d->original ? d->original : d->character)) == NULL)
	continue;
      if (IN_ROOM(i) == NOWHERE || !CAN_SEE(ch, i))
	continue;
      if (world[IN_ROOM(ch)].zone != world[IN_ROOM(i)].zone)
	continue;
      send_to_char(ch, "%-20s - %s\r\n", GET_NAME(i), world[IN_ROOM(i)].name);
    }
  } else {			/* print only FIRST char, not all. */
    for (i = character_list; i; i = i->next) {
      if (IN_ROOM(i) == NOWHERE || i == ch)
	continue;
      if (!CAN_SEE(ch, i) || world[IN_ROOM(i)].zone != world[IN_ROOM(ch)].zone)
	continue;
      if (!isname(arg, i->player.name))
	continue;
      send_to_char(ch, "%-25s - %s\r\n", GET_NAME(i), world[IN_ROOM(i)].name);
      return;
    }
    send_to_char(ch, "Nobody around by that name.\r\n");
  }
}


void print_object_location(int num, struct obj_data *obj, struct char_data *ch,
			        int recur)
{
  if (num > 0)
    send_to_char(ch, "O%3d. %-25s - ", num, obj->short_description);
  else
    send_to_char(ch, "%33s", " - ");

  if (obj->proto_script)
    send_to_char(ch, "[TRIG]");

  if (IN_ROOM(obj) != NOWHERE)
    send_to_char(ch, "[%5d] %s\r\n", GET_ROOM_VNUM(IN_ROOM(obj)), world[IN_ROOM(obj)].name);
  else if (obj->carried_by)
    send_to_char(ch, "carried by %s\r\n", PERS(obj->carried_by, ch));
  else if (obj->worn_by)
    send_to_char(ch, "worn by %s\r\n", PERS(obj->worn_by, ch));
  else if (obj->in_obj) {
    send_to_char(ch, "inside %s%s\r\n", obj->in_obj->short_description, (recur ? ", which is" : " "));
    if (recur)
      print_object_location(0, obj->in_obj, ch, recur);
  } else
    send_to_char(ch, "in an unknown location\r\n");
}



void perform_immort_where(struct char_data *ch, char *arg)
{
  struct char_data *i;
  struct obj_data *k;
  struct descriptor_data *d;
  int num = 0, found = 0;

  if (!*arg) {
    send_to_char(ch, "Players\r\n-------\r\n");
    for (d = descriptor_list; d; d = d->next)
      if (IS_PLAYING(d)) {
	i = (d->original ? d->original : d->character);
	if (i && CAN_SEE(ch, i) && (IN_ROOM(i) != NOWHERE)) {
	  if (d->original)
	    send_to_char(ch, "%-20s - [%5d] %s (in %s)\r\n",
		GET_NAME(i), GET_ROOM_VNUM(IN_ROOM(d->character)),
		world[IN_ROOM(d->character)].name, GET_NAME(d->character));
	  else
	    send_to_char(ch, "%-20s - [%5d] %s\r\n", GET_NAME(i), GET_ROOM_VNUM(IN_ROOM(i)), world[IN_ROOM(i)].name);
	}
      }
  } else {
    for (i = character_list; i; i = i->next)
      if (CAN_SEE(ch, i) && IN_ROOM(i) != NOWHERE && isname(arg, i->player.name)) {
	found = 1;
	send_to_char(ch, "M%3d. %-25s - [%5d] %-25s %s\r\n", ++num, GET_NAME(i),
		GET_ROOM_VNUM(IN_ROOM(i)), world[IN_ROOM(i)].name,
		(IS_NPC(i) && i->proto_script) ? "[TRIG]" : "");
      }
    for (num = 0, k = object_list; k; k = k->next)
      if (CAN_SEE_OBJ(ch, k) && isname(arg, k->name)) {
	found = 1;
	print_object_location(++num, k, ch, TRUE);
      }
    if (!found)
      send_to_char(ch, "Couldn't find any such thing.\r\n");
  }
}



ACMD(do_where)
{
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (GET_LEVEL(ch) >= LVL_IMMORT)
    perform_immort_where(ch, arg);
  else
    perform_mortal_where(ch, arg);
}



ACMD(do_levels)
{
  char buf[MAX_STRING_LENGTH];
  size_t i, len = 0, nlen;

  if (IS_NPC(ch)) {
    send_to_char(ch, "You ain't nothin' but a hound-dog.\r\n");
    return;
  }

  for (i = 1; i < LVL_IMMORT; i++) {
    nlen = snprintf(buf + len, sizeof(buf) - len, "[%2d] %8d-%-8d : ", i,
		level_exp(GET_CLASS(ch), i), level_exp(GET_CLASS(ch), i + 1) - 1);
    if (len + nlen >= sizeof(buf) || nlen < 0)
      break;
    len += nlen;

    switch (GET_SEX(ch)) {
    case SEX_MALE:
    case SEX_NEUTRAL:
      nlen = snprintf(buf + len, sizeof(buf) - len, "%s\r\n", title_male(GET_CLASS(ch), i));
      break;
    case SEX_FEMALE:
      nlen = snprintf(buf + len, sizeof(buf) - len, "%s\r\n", title_female(GET_CLASS(ch), i));
      break;
    default:
      nlen = snprintf(buf + len, sizeof(buf) - len, "Oh dear.  You seem to be sexless.\r\n");
      break;
    }
    if (len + nlen >= sizeof(buf) || nlen < 0)
      break;
    len += nlen;
  }

  if (len < sizeof(buf))
    snprintf(buf + len, sizeof(buf) - len, "[%2d] %8d          : Immortality\r\n",
		LVL_IMMORT, level_exp(GET_CLASS(ch), LVL_IMMORT));
  page_string(ch->desc, buf, TRUE);
}



ACMD(do_consider)
{
  char buf[MAX_INPUT_LENGTH];
  struct char_data *victim;
  int diff;

  one_argument(argument, buf);

  if (!(victim = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "Consider killing who?\r\n");
    return;
  }
  if (victim == ch) {
    send_to_char(ch, "Easy!  Very easy indeed!\r\n");
    return;
  }
  if (!IS_NPC(victim)) {
    send_to_char(ch, "Would you like to borrow a cross and a shovel?\r\n");
    return;
  }
  diff = (GET_LEVEL(victim) - GET_LEVEL(ch));

  if (diff <= -10)
    send_to_char(ch, "Now where did that chicken go?\r\n");
  else if (diff <= -5)
    send_to_char(ch, "You could do it with a needle!\r\n");
  else if (diff <= -2)
    send_to_char(ch, "Easy.\r\n");
  else if (diff <= -1)
    send_to_char(ch, "Fairly easy.\r\n");
  else if (diff == 0)
    send_to_char(ch, "The perfect match!\r\n");
  else if (diff <= 1)
    send_to_char(ch, "You would need some luck!\r\n");
  else if (diff <= 2)
    send_to_char(ch, "You would need a lot of luck!\r\n");
  else if (diff <= 3)
    send_to_char(ch, "You would need a lot of luck and great equipment!\r\n");
  else if (diff <= 5)
    send_to_char(ch, "Do you feel lucky, punk?\r\n");
  else if (diff <= 10)
    send_to_char(ch, "Are you mad!?\r\n");
  else if (diff <= 100)
    send_to_char(ch, "You ARE mad!\r\n");
}



ACMD(do_diagnose)
{
  char buf[MAX_INPUT_LENGTH];
  struct char_data *vict;

  one_argument(argument, buf);

  if (*buf) {
    if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM)))
      send_to_char(ch, "%s", CONFIG_NOPERSON);
    else
      diag_char_to_char(vict, ch);
  } else {
    if (FIGHTING(ch))
      diag_char_to_char(FIGHTING(ch), ch);
    else
      send_to_char(ch, "Diagnose who?\r\n");
  }
}


const char *ctypes[] = {
  "off", "brief", "normal", "complete", "\n"
};

ACMD(do_color)
{
  char arg[MAX_INPUT_LENGTH];
  int tp;

  if (IS_NPC(ch))
    return;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Your current color level is %s.\r\n", ctypes[COLOR_LEV(ch)]);
    return;
  }
  if (((tp = search_block(arg, ctypes, FALSE)) == -1)) {
    send_to_char(ch, "Usage: color { Off | BRIEF | Normal | Complete }\r\n");
    return;
  }
  switch (tp) {
    case C_OFF:
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_COLOR_1);
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_COLOR_2);
      break;
    case C_BRI:
      SET_BIT_AR(PRF_FLAGS(ch), PRF_COLOR_1);
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_COLOR_2);
      break;
    case C_NRM:
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_COLOR_1);
      SET_BIT_AR(PRF_FLAGS(ch), PRF_COLOR_2);
      break;
    case C_CMP:
      SET_BIT_AR(PRF_FLAGS(ch), PRF_COLOR_1);
      SET_BIT_AR(PRF_FLAGS(ch), PRF_COLOR_2);
      break;
  }
  send_to_char(ch, "Your %scolor%s is now %s.\r\n", CCRED(ch, C_BRI), CCNRM(ch, C_OFF), ctypes[tp]);
}


ACMD(do_toggle)
{
  char buf2[4];

  if (IS_NPC(ch))
    return;

  if (GET_WIMP_LEV(ch) == 0)
    strcpy(buf2, "OFF");	/* strcpy: OK */
  else
    sprintf(buf2, "%-3.3d", GET_WIMP_LEV(ch));	/* sprintf: OK */

  if (GET_LEVEL(ch) >= LVL_IMMORT) {
    send_to_char(ch,
          "      Buildwalk: %-3s    "
          "Clear Screen in OLC: %-3s\r\n",
        ONOFF(PRF_FLAGGED(ch, PRF_BUILDWALK)),
        ONOFF(PRF_FLAGGED(ch, PRF_CLS))
    );

    send_to_char(ch,
	  "      No Hassle: %-3s    "
	  "      Holylight: %-3s    "
	  "     Room Flags: %-3s\r\n",
	ONOFF(PRF_FLAGGED(ch, PRF_NOHASSLE)),
	ONOFF(PRF_FLAGGED(ch, PRF_HOLYLIGHT)),
	ONOFF(PRF_FLAGGED(ch, PRF_ROOMFLAGS))
    );
  }

  send_to_char(ch,
	  "Hit Pnt Display: %-3s    "
	  "     Brief Mode: %-3s    "
	  " Summon Protect: %-3s\r\n"

	  "   Move Display: %-3s    "
	  "   Compact Mode: %-3s    "
	  "       On Quest: %-3s\r\n"

	  "   Mana Display: %-3s    "
	  "         NoTell: %-3s    "
	  "   Repeat Comm.: %-3s\r\n"

	  " Auto Show Exit: %-3s    "
	  "           Deaf: %-3s    "
	  "     Wimp Level: %-3s\r\n"

	  " Gossip Channel: %-3s    "
	  "Auction Channel: %-3s    "
	  "  Grats Channel: %-3s\r\n"

          "      Auto Loot: %-3s    "
          "      Auto Gold: %-3s    "
	  "    Color Level: %s\r\n"

          "     Auto Split: %-3s    "
          "       Auto Sac: %-3s    "
          "     View Order: %-3s\r\n"

          "    Auto Assist: %-3s\r\n",

	  ONOFF(PRF_FLAGGED(ch, PRF_DISPHP)),
	  ONOFF(PRF_FLAGGED(ch, PRF_BRIEF)),
	  ONOFF(!PRF_FLAGGED(ch, PRF_SUMMONABLE)),

	  ONOFF(PRF_FLAGGED(ch, PRF_DISPMOVE)),
	  ONOFF(PRF_FLAGGED(ch, PRF_COMPACT)),
	  YESNO(PRF_FLAGGED(ch, PRF_QUEST)),

	  ONOFF(PRF_FLAGGED(ch, PRF_DISPMANA)),
	  ONOFF(PRF_FLAGGED(ch, PRF_NOTELL)),
	  YESNO(!PRF_FLAGGED(ch, PRF_NOREPEAT)),

	  ONOFF(PRF_FLAGGED(ch, PRF_AUTOEXIT)),
	  YESNO(PRF_FLAGGED(ch, PRF_DEAF)),
	  buf2,

	  ONOFF(!PRF_FLAGGED(ch, PRF_NOGOSS)),
	  ONOFF(!PRF_FLAGGED(ch, PRF_NOAUCT)),
	  ONOFF(!PRF_FLAGGED(ch, PRF_NOGRATZ)),

          ONOFF(PRF_FLAGGED(ch, PRF_AUTOLOOT)),
          ONOFF(PRF_FLAGGED(ch, PRF_AUTOGOLD)),
	  ctypes[COLOR_LEV(ch)],

          ONOFF(PRF_FLAGGED(ch, PRF_AUTOSPLIT)),
          ONOFF(PRF_FLAGGED(ch, PRF_AUTOSAC)),
          ONOFF(PRF_FLAGGED(ch, PRF_VIEWORDER)),

          ONOFF(PRF_FLAGGED(ch, PRF_AUTOASSIST)));

          if (CONFIG_ENABLE_COMPRESSION) {
            send_to_char(ch, "    Compression: %-3s\r\n", ONOFF(!PRF_FLAGGED(ch, PRF_NOCOMPRESS)));
          }
}


int sort_commands_helper(const void *a, const void *b)
{
  return strcmp(complete_cmd_info[*(const int *)a].sort_as,
                complete_cmd_info[*(const int *)b].sort_as);
}


void sort_commands(void)
{
  int a, num_of_cmds = 0;

  while (complete_cmd_info[num_of_cmds].command[0] != '\n')
    num_of_cmds++;
  num_of_cmds++;	/* \n */

  CREATE(cmd_sort_info, int, num_of_cmds);

  for (a = 0; a < num_of_cmds; a++)
    cmd_sort_info[a] = a;

  /* Don't sort the RESERVED or \n entries. */
  qsort(cmd_sort_info + 1, num_of_cmds - 2, sizeof(int), sort_commands_helper);
}


ACMD(do_commands)
{
  int no, i, cmd_num;
  int wizhelp = 0, socials = 0;
  struct char_data *vict;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (*arg) {
    if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)) || IS_NPC(vict)) {
      send_to_char(ch, "Who is that?\r\n");
      return;
    }
    if (GET_LEVEL(ch) < GET_LEVEL(vict)) {
      send_to_char(ch, "You can't see the commands of people above your level.\r\n");
      return;
    }
  } else
    vict = ch;

  if (subcmd == SCMD_SOCIALS)
    socials = 1;
  else if (subcmd == SCMD_WIZHELP)
    wizhelp = 1;

  send_to_char(ch, "The following %s%s are available to %s:\r\n",
	  wizhelp ? "privileged " : "",
	  socials ? "socials" : "commands",
	  vict == ch ? "you" : GET_NAME(vict));

  /* cmd_num starts at 1, not 0, to remove 'RESERVED' */
  for (no = 1, cmd_num = 1; complete_cmd_info[cmd_sort_info[cmd_num]].command[0] != '\n'; cmd_num++) {
    i = cmd_sort_info[cmd_num];

    if (complete_cmd_info[i].minimum_level < 0 || GET_LEVEL(vict) < complete_cmd_info[i].minimum_level)
      continue;

    if ((complete_cmd_info[i].minimum_level >= LVL_IMMORT) != wizhelp)
      continue;

    if (!wizhelp && socials != (complete_cmd_info[i].command_pointer == do_action || complete_cmd_info[i].command_pointer == do_insult))
      continue;

     if (check_disabled(&complete_cmd_info[i]))
      sprintf(arg, "(%s)", complete_cmd_info[i].command);
    else  
      sprintf(arg, "%s", complete_cmd_info[i].command);

    send_to_char(ch, "%-11s%s", arg, no++ % 7 == 0 ? "\r\n" : "");

  }

  if (no % 7 != 1)
    send_to_char(ch, "\r\n");
}

ACMD(do_whois)
{
  const char *immlevels[LVL_IMPL - (LVL_IMMORT-1)] = {
  "[Immortal]",        /* highest mortal level +1 */
  "[God]",             /* highest mortal level +2 */
  "[Greater God]",     /* highest mortal level +3 */
  "[Implementor]",     /* highest mortal level +4 */
  };

  struct char_data *victim = 0;
  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "Who?\r\n");
  } else {
  CREATE(victim, struct char_data, 1);
  clear_char(victim);
  CREATE(victim->player_specials, struct player_special_data, 1);
  if (load_char(argument, victim) >= 0) {
    if (GET_LEVEL(victim) >= LVL_IMMORT)
      send_to_char(ch, "%s %s %s\r\n", immlevels[GET_LEVEL(victim)-LVL_IMMORT], GET_NAME(victim), GET_TITLE(victim));
    else 
      send_to_char(ch, "Level %d %s - %s %s\r\n", GET_LEVEL(victim), CLASS_ABBR(victim), GET_NAME(victim), GET_TITLE(victim));
    } else {
    send_to_char(ch, "There is no such player.\r\n"); 
    }
    free(victim); 
  }
}

void search_in_direction(struct char_data * ch, int dir)
{
  int check=0,roll;

  send_to_char(ch, "You search for secret doors.\r\n");
  act("$n searches the area intently.", TRUE, ch, 0, 0, TO_ROOM);

  roll=rand_number(1,6);

  if (GET_LEVEL(ch) < LVL_IMMORT) {
    switch (GET_RACE(ch)) {
      case RACE_ELF:
      case RACE_HALF_ELF:
       if (roll <= 2)
        check=1;
       break;
      default:
       if (roll == 1)
        check=1;
       break;
    }
  } else
     check=1;

  if (EXIT(ch, dir)) {
    if (EXIT(ch, dir)->general_description &&
        !EXIT_FLAGGED(EXIT(ch, dir), EX_SECRET))
      send_to_char(ch, EXIT(ch, dir)->general_description);
    else if (!EXIT_FLAGGED(EXIT(ch, dir), EX_SECRET))
      send_to_char(ch, "There is a normal exit there.\r\n");
    else if (EXIT_FLAGGED(EXIT(ch, dir), EX_ISDOOR) &&
             EXIT_FLAGGED(EXIT(ch, dir), EX_SECRET) &&
             EXIT(ch, dir)->keyword && (check == 1)  )
      send_to_char(ch, "There is a hidden door keyword: '%s' %sthere.\r\n",
                   fname (EXIT(ch, dir)->keyword),
                   (EXIT_FLAGGED(EXIT(ch, dir), EX_CLOSED)) ? "" : "open ");
    else
      send_to_char(ch, "There is no exit there.\r\n");
  } else
    send_to_char(ch, "There is no exit there.\r\n");
}

