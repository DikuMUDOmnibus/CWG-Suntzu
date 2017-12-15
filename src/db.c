/* ************************************************************************
*   File: db.c                                          Part of CircleMUD *
*  Usage: Loading/saving chars, booting/resetting world, internal funcs   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define __DB_C__

#include "conf.h"
#include "sysdep.h"


#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "handler.h"
#include "spells.h"
#include "mail.h"
#include "interpreter.h"
#include "house.h"
#include "constants.h"
#include "oasis.h"
#include "dg_scripts.h"
#include "dg_event.h"
#include "assemblies.h"
#include "boards.h"
#include "htree.h"


/**************************************************************************
*  declarations of most of the 'global' variables                         *
**************************************************************************/

struct config_data config_info; /* Game configuration list.    */

struct room_data *world = NULL;	/* array of rooms		 */
room_rnum top_of_world = 0;	/* ref to top element of world	 */
struct htree_node *room_htree = NULL;	/* hash tree for fast room lookup */

struct char_data *character_list = NULL; /* global linked list of chars	 */
struct index_data *mob_index;	/* index table for mobile file	 */
struct char_data *mob_proto;	/* prototypes for mobs		 */
mob_rnum top_of_mobt = 0;	/* top of mobile index table	 */
struct htree_node *mob_htree = NULL;	/* hash tree for fast mob lookup */

struct obj_data *object_list = NULL;	/* global linked list of objs	 */
struct index_data *obj_index;	/* index table for object file	 */
struct obj_data *obj_proto;	/* prototypes for objs		 */
obj_rnum top_of_objt = 0;	/* top of object index table	 */
struct htree_node *obj_htree = NULL;	/* hash tree for fast obj lookup */

struct zone_data *zone_table;	/* zone table			 */
zone_rnum top_of_zone_table = 0;/* top element of zone tab	 */
struct message_list fight_messages[MAX_MESSAGES];	/* fighting messages	 */

struct index_data **trig_index; /* index table for triggers      */
struct trig_data *trigger_list = NULL;  /* all attached triggers */
int top_of_trigt = 0;           /* top of trigger index table    */
long max_mob_id = MOB_ID_BASE;  /* for unique mob id's       */
long max_obj_id = OBJ_ID_BASE;  /* for unique obj id's       */
int dg_owner_purged;            /* For control of scripts */

int no_mail = 0;		/* mail disabled?		 */
int mini_mud = 0;		/* mini-mud mode?		 */
int no_rent_check = 0;		/* skip rent check on boot?	 */
time_t boot_time = 0;		/* time of mud boot		 */
int circle_restrict = 0;	/* level of game restriction	 */
room_rnum r_mortal_start_room;	/* rnum of mortal start room	 */
room_rnum r_immort_start_room;	/* rnum of immort start room	 */
room_rnum r_frozen_start_room;	/* rnum of frozen start room	 */
int xap_objs = 0;               /* Xap objs                      */
int converting = FALSE;

char *credits = NULL;		/* game credits			 */
char *news = NULL;		/* mud news			 */
char *motd = NULL;		/* message of the day - mortals */
char *imotd = NULL;		/* message of the day - immorts */
char *GREETINGS = NULL;		/* opening credits screen	*/
char *GREETANSI = NULL;		/* ansi opening credits screen	*/
char *help = NULL;		/* help screen			 */
char *info = NULL;		/* info page			 */
char *wizlist = NULL;		/* list of higher gods		 */
char *immlist = NULL;		/* list of peon gods		 */
char *background = NULL;	/* background story		 */
char *handbook = NULL;		/* handbook for new immortals	 */
char *policies = NULL;		/* policies page		 */

struct help_index_element *help_table = 0;	/* the help table	 */
int top_of_helpt = 0;		/* top of help index table	 */

struct social_messg *soc_mess_list = NULL;      /* list of socials */
int top_of_socialt = -1;                        /* number of socials */

struct time_info_data time_info;/* the infomation about the time    */
struct weather_data weather_info;	/* the infomation about the weather */
struct player_special_data dummy_mob;	/* dummy spec area for mobs	*/
struct reset_q_type reset_q;	/* queue of zones to be reset	 */

extern struct board_info *boards; /* our boards */

/* local functions */
int check_bitvector_names(bitvector_t bits, size_t namecount, const char *whatami, const char *whatbits);
int check_object_spell_number(struct obj_data *obj, int val);
int check_object_level(struct obj_data *obj, int val);
void setup_dir(FILE *fl, int room, int dir);
void index_boot(int mode);
void discrete_load(FILE *fl, int mode, char *filename);
int check_object(struct obj_data *);
void parse_room(FILE *fl, int virtual_nr);
void parse_mobile(FILE *mob_f, int nr);
char *parse_object(FILE *obj_f, int nr);
void parse_trigger(FILE *fl, int virtual_nr);
void load_zones(FILE *fl, char *zonename);
void load_help(FILE *fl);
void assign_mobiles(void);
void assign_objects(void);
void assign_rooms(void);
void assign_the_shopkeepers(void);
int is_empty(zone_rnum zone_nr);
void reset_zone(zone_rnum zone);
int file_to_string(const char *name, char *buf);
int file_to_string_alloc(const char *name, char **buf);
void reboot_wizlists(void);
ACMD(do_reboot);
void boot_world(void);
int count_alias_records(FILE *fl);
int count_hash_records(FILE *fl);
bitvector_t asciiflag_conv(char *flag);
bitvector_t asciiflag_conv_aff(char *flag);
int parse_simple_mob(FILE *mob_f, struct char_data *ch, int nr);
void interpret_espec(const char *keyword, const char *value, struct char_data *ch, int nr);
void parse_espec(char *buf, struct char_data *ch, int nr);
int parse_enhanced_mob(FILE *mob_f, struct char_data *ch, int nr);
void get_one_line(FILE *fl, char *buf);
void save_char_vars(struct char_data *ch);
void check_start_rooms(void);
void renum_world(void);
void renum_zone_table(void);
void log_zone_error(zone_rnum zone, int cmd_no, const char *message);
void reset_time(void);

/* external functions */
void paginate_string(char *str, struct descriptor_data *d);
struct time_info_data *mud_time_passed(time_t t2, time_t t1);
void free_alias(struct alias_data *a);
void load_messages(void);
void weather_and_time(int mode);
void mag_assign_spells(void);
void boot_social_messages(void);
void update_obj_file(void);	/* In objsave.c */
void create_command_list(void);
void sort_commands(void);
void sort_spells(void);
void load_banned(void);
void Read_Invalid_List(void);
void boot_the_shops(FILE *shop_f, char *filename, int rec_count);
int hsort(const void *a, const void *b);
void prune_crlf(char *txt);
void destroy_shops(void);
void build_player_index(void);
void clean_pfiles(void);
void free_object_strings(struct obj_data *obj);
void free_object_strings_proto(struct obj_data *obj);
void boot_context_help(void);
void free_context_help(void);
struct time_info_data *real_time_passed(time_t t2, time_t t1);
int add_to_save_list(zone_vnum, int type);
int save_all(void);
extern zone_rnum real_zone_by_thing(room_vnum vznum);
void boot_the_guilds(FILE *gm_f, char *filename, int rec_count);
void destroy_guilds(void);
void assign_the_guilds(void);

/* external vars */
extern struct descriptor_data *descriptor_list;
extern const char *unused_spellname;	/* spell_parser.c */

extern struct player_index_element *player_table;
int top_of_p_table;
long top_idnum;

/* external ASCII Player Files vars */
extern int auto_pwipe;

extern int no_specials;
extern int scheck;
extern int bitwarning;
extern int bitsavetodisk;

/*************************************************************************
*  routines for booting the system                                       *
*************************************************************************/

/* this is necessary for the autowiz system */
void reboot_wizlists(void)
{
  file_to_string_alloc(WIZLIST_FILE, &wizlist);
  file_to_string_alloc(IMMLIST_FILE, &immlist);
}

/* Wipe out all the loaded text files, for shutting down. */
void free_text_files(void)
{
  char **textfiles[] = {
	&wizlist, &immlist, &news, &credits, &motd, &imotd, &help, &info,
	&policies, &handbook, &background, &GREETINGS, &GREETANSI, NULL
  };
  int rf;

  for (rf = 0; textfiles[rf]; rf++)
    if (*textfiles[rf]) {
      free(*textfiles[rf]);
      *textfiles[rf] = NULL;
    }
}


/*
 * Too bad it doesn't check the return values to let the user
 * know about -1 values.  This will result in an 'Okay.' to a
 * 'reload' command even when the string was not replaced.
 * To fix later, if desired. -gg 6/24/99
 */
ACMD(do_reboot)
{
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!str_cmp(arg, "all") || *arg == '*') {
    if (file_to_string_alloc(GREETINGS_FILE, &GREETINGS) == 0)
      prune_crlf(GREETINGS);
	if (file_to_string_alloc(GREETANSI_FILE, &GREETANSI) == 0)
      prune_crlf(GREETANSI);
    if (file_to_string_alloc(WIZLIST_FILE, &wizlist) < 0) 
      send_to_char(ch, "Can not read wizlist\r\n");
    if (file_to_string_alloc(IMMLIST_FILE, &immlist) < 0) 
      send_to_char(ch, "Can not read immlist\r\n");
    if (file_to_string_alloc(NEWS_FILE, &news) < 0) 
      send_to_char(ch, "Can not read news\r\n");
    if (file_to_string_alloc(CREDITS_FILE, &credits) < 0) 
      send_to_char(ch, "Can not read credits\r\n");
    if (file_to_string_alloc(MOTD_FILE, &motd) < 0) 
      send_to_char(ch, "Can not read motd\r\n");
    if (file_to_string_alloc(IMOTD_FILE, &imotd) < 0) 
      send_to_char(ch, "Can not read imotd\r\n");
    if (file_to_string_alloc(HELP_PAGE_FILE, &help) < 0) 
      send_to_char(ch, "Can not read help front page\r\n");
    if (file_to_string_alloc(INFO_FILE, &info) < 0) 
      send_to_char(ch, "Can not read info file\r\n");
    if (file_to_string_alloc(POLICIES_FILE, &policies) < 0) 
      send_to_char(ch, "Can not read policies\r\n");
    if (file_to_string_alloc(HANDBOOK_FILE, &handbook) < 0) 
      send_to_char(ch, "Can not read handbook\r\n");
    if (file_to_string_alloc(BACKGROUND_FILE, &background) < 0) 
      send_to_char(ch, "Can not read background\r\n");
    if (help_table)
      free_help();
    index_boot(DB_BOOT_HLP);
  } else if (!str_cmp(arg, "wizlist")) {
    if (file_to_string_alloc(WIZLIST_FILE, &wizlist) < 0) 
      send_to_char(ch, "Can not read wizlist\r\n");
  } else if (!str_cmp(arg, "immlist")) {
    if (file_to_string_alloc(IMMLIST_FILE, &immlist) < 0) 
      send_to_char(ch, "Can not read immlist\r\n");
  } else if (!str_cmp(arg, "news")) {
    if (file_to_string_alloc(NEWS_FILE, &news) < 0) 
      send_to_char(ch, "Can not read news\r\n");
  } else if (!str_cmp(arg, "credits")) {
    if (file_to_string_alloc(CREDITS_FILE, &credits) < 0) 
      send_to_char(ch, "Can not read credits\r\n");
  } else if (!str_cmp(arg, "motd")) {
    if (file_to_string_alloc(MOTD_FILE, &motd) < 0) 
      send_to_char(ch, "Can not read motd\r\n");
  } else if (!str_cmp(arg, "imotd")) {
    if (file_to_string_alloc(IMOTD_FILE, &imotd) < 0) 
      send_to_char(ch, "Can not read imotd\r\n");
  } else if (!str_cmp(arg, "help")) {
    if (file_to_string_alloc(HELP_PAGE_FILE, &help) < 0) 
      send_to_char(ch, "Can not read help front page\r\n");
  } else if (!str_cmp(arg, "info")) {
    if (file_to_string_alloc(INFO_FILE, &info) < 0) 
      send_to_char(ch, "Can not read info\r\n");
  } else if (!str_cmp(arg, "policy")) {
    if (file_to_string_alloc(POLICIES_FILE, &policies) < 0) 
      send_to_char(ch, "Can not read policy\r\n");
  } else if (!str_cmp(arg, "handbook")) {
    if (file_to_string_alloc(HANDBOOK_FILE, &handbook) < 0) 
      send_to_char(ch, "Can not read handbook\r\n");
  } else if (!str_cmp(arg, "background")) {
    if (file_to_string_alloc(BACKGROUND_FILE, &background) < 0) 
      send_to_char(ch, "Can not read background\r\n");
  } else if (!str_cmp(arg, "greetings")) {
    if (file_to_string_alloc(GREETINGS_FILE, &GREETINGS) == 0)
      prune_crlf(GREETINGS);
    else
      send_to_char(ch, "Can not read greetings.\r\n");
  } else if (!str_cmp(arg, "greetansi")) {
    if (file_to_string_alloc(GREETANSI_FILE, &GREETANSI) == 0)
      prune_crlf(GREETANSI);
    else
      send_to_char(ch, "Can not read greetings.\r\n");
  } else if (!str_cmp(arg, "xhelp")) {
    if (help_table)
      free_help();
    index_boot(DB_BOOT_HLP);
  } else {
    send_to_char(ch, "Unknown reload option.\r\n");
    return;
  }

  send_to_char(ch, "%s", CONFIG_OK);
}


void boot_world(void)
{
  log("Loading zone table.");
  index_boot(DB_BOOT_ZON);

  log("Loading triggers and generating index.");
  index_boot(DB_BOOT_TRG);

  log("Loading rooms.");
  index_boot(DB_BOOT_WLD);

  log("Renumbering rooms.");
  renum_world();

  log("Checking start rooms.");
  check_start_rooms();

  log("Loading mobs and generating index.");
  index_boot(DB_BOOT_MOB);

  log("Loading objs and generating index.");
  index_boot(DB_BOOT_OBJ);

  log("Renumbering zone table.");
  renum_zone_table();

  log("Loading disabled commands list...");
  load_disabled();

  if(converting) {
    log("Saving 128bit worldfiles to disk.");
	save_all();
  }

  if (!no_specials) {
    log("Loading shops.");
    index_boot(DB_BOOT_SHP);
  }

  log("Loading guild masters.");
  index_boot(DB_BOOT_GLD);

}


void free_extra_descriptions(struct extra_descr_data *edesc)
{
  struct extra_descr_data *enext;

  for (; edesc; edesc = enext) {
    enext = edesc->next;

    free(edesc->keyword);
    free(edesc->description);
    free(edesc);
  }
}


/* Free the world, in a memory allocation sense. */
void destroy_db(void)
{
  ssize_t cnt, itr;
  struct char_data *chtmp;
  struct obj_data *objtmp;

  /* Active Mobiles & Players */
  while (character_list) {
    chtmp = character_list;
    character_list = character_list->next;
    free_char(chtmp);
  }

  /* Active Objects */
  while (object_list) {
    objtmp = object_list;
    object_list = object_list->next;
    free_obj(objtmp);
  }

  /* Rooms */
  for (cnt = 0; cnt <= top_of_world; cnt++) {
    if (world[cnt].name)
      free(world[cnt].name);
    if (world[cnt].description)
      free(world[cnt].description);
    free_extra_descriptions(world[cnt].ex_description);

    /* free any assigned scripts */
    if (SCRIPT(&world[cnt]))
      extract_script(&world[cnt], WLD_TRIGGER);
    /* free script proto list */
    free_proto_script(&world[cnt], WLD_TRIGGER);
    
    for (itr = 0; itr < NUM_OF_DIRS; itr++) {
      if (!world[cnt].dir_option[itr])
        continue;

      if (world[cnt].dir_option[itr]->general_description)
        free(world[cnt].dir_option[itr]->general_description);
      if (world[cnt].dir_option[itr]->keyword)
        free(world[cnt].dir_option[itr]->keyword);
      free(world[cnt].dir_option[itr]);
    }
  }
  free(world);
  top_of_world = 0;
  htree_free(room_htree);

  /* Objects */
  for (cnt = 0; cnt <= top_of_objt; cnt++) {
    if (obj_proto[cnt].name)
      free(obj_proto[cnt].name);
    if (obj_proto[cnt].description)
      free(obj_proto[cnt].description);
    if (obj_proto[cnt].short_description)
      free(obj_proto[cnt].short_description);
    if (obj_proto[cnt].action_description)
      free(obj_proto[cnt].action_description);
    free_extra_descriptions(obj_proto[cnt].ex_description);

    /* free script proto list */
    free_proto_script(&obj_proto[cnt], OBJ_TRIGGER);
  }
  free(obj_proto);
  free(obj_index);
  htree_free(obj_htree);

  /* Mobiles */
  for (cnt = 0; cnt <= top_of_mobt; cnt++) {
    if (mob_proto[cnt].player.name)
      free(mob_proto[cnt].player.name);
    if (mob_proto[cnt].player.title)
      free(mob_proto[cnt].player.title);
    if (mob_proto[cnt].player.short_descr)
      free(mob_proto[cnt].player.short_descr);
    if (mob_proto[cnt].player.long_descr)
      free(mob_proto[cnt].player.long_descr);
    if (mob_proto[cnt].player.description)
      free(mob_proto[cnt].player.description);

    /* free script proto list */
    free_proto_script(&mob_proto[cnt], MOB_TRIGGER);

    while (mob_proto[cnt].affected)
      affect_remove(&mob_proto[cnt], mob_proto[cnt].affected);
  }
  free(mob_proto);
  free(mob_index);
  htree_free(mob_htree);

  /* Shops */
  destroy_shops();

  /* Guilds */
  destroy_guilds();

  /* Zones */
  /* zone table reset queue */
  if (reset_q.head) {
    struct reset_q_element *ftemp=reset_q.head, *temp;
    while (ftemp) {
      temp = ftemp->next;
      free(ftemp);
      ftemp = temp;
    }
  }

#define THIS_CMD zone_table[cnt].cmd[itr]

  for (cnt = 0; cnt <= top_of_zone_table; cnt++) {
    if (zone_table[cnt].name)
      free(zone_table[cnt].name);
    if (zone_table[cnt].builders)
      free(zone_table[cnt].builders);
    if (zone_table[cnt].cmd) {
       /* first see if any vars were defined in this zone */
       for (itr = 0;THIS_CMD.command != 'S';itr++)
         if (THIS_CMD.command == 'V') {
           if (THIS_CMD.sarg1)
             free(THIS_CMD.sarg1);
           if (THIS_CMD.sarg2)
             free(THIS_CMD.sarg2);
         }
       /* then free the command list */
      free(zone_table[cnt].cmd);
    }
  }
  free(zone_table);

#undef THIS_CMD

  /* zone table reset queue */
  if (reset_q.head) {
    struct reset_q_element *ftemp=reset_q.head, *temp;
    while (ftemp) {
      temp = ftemp->next;
      free(ftemp);
      ftemp = temp;
    }
  }

  /* Triggers */
  for (cnt=0; cnt < top_of_trigt; cnt++) {
    if (trig_index[cnt]->proto) {
      /* make sure to nuke the command list (memory leak) */
      /* free_trigger() doesn't free the command list */
      if (trig_index[cnt]->proto->cmdlist) {
        struct cmdlist_element *i, *j;
        i = trig_index[cnt]->proto->cmdlist;
        while (i) {
          j = i->next;
          if (i->cmd)
            free(i->cmd);
          free(i);
          i = j;
        }
      }
      free_trigger(trig_index[cnt]->proto);
    }
    free(trig_index[cnt]);
  }
  free(trig_index);

  /* Events */
  event_free_all();

  /* context sensitive help system */
  free_context_help();

}


/* You can define this to anything you want; 1 would work but it would
   be very inefficient. I would recommend that it actually be close to
   your total number of in-game objects if not double or triple it just
   to minimize collisions. The only O(n) [n=NUM_OBJ_UNIQUE_POOLS]
   operation is initialization of the hash table, all other operations
   that have to traverse are O(n) [n=num elements in pool], so more
   pools are better.
     - Elie Rosenblum Dec. 12 2003 */
#define NUM_OBJ_UNIQUE_POOLS 5000

struct obj_unique_hash_elem **obj_unique_hash_pools = NULL;

void init_obj_unique_hash()
{
  int i;
  CREATE(obj_unique_hash_pools, struct obj_unique_hash_elem *, NUM_OBJ_UNIQUE_POOLS);
  for (i = 0; i < NUM_OBJ_UNIQUE_POOLS; i++) {
    obj_unique_hash_pools[i] = NULL;
  }
}

/* body of the booting system */
void boot_db(void)
{
  zone_rnum i;

  log("Boot db -- BEGIN.");

  log("Resetting the game time:");
  reset_time();

  log("Reading news, credits, help, bground, info & motds.");
  file_to_string_alloc(NEWS_FILE, &news);
  file_to_string_alloc(CREDITS_FILE, &credits);
  file_to_string_alloc(MOTD_FILE, &motd);
  file_to_string_alloc(IMOTD_FILE, &imotd);
  file_to_string_alloc(HELP_PAGE_FILE, &help);
  file_to_string_alloc(INFO_FILE, &info);
  file_to_string_alloc(WIZLIST_FILE, &wizlist);
  file_to_string_alloc(IMMLIST_FILE, &immlist);
  file_to_string_alloc(POLICIES_FILE, &policies);
  file_to_string_alloc(HANDBOOK_FILE, &handbook);
  file_to_string_alloc(BACKGROUND_FILE, &background);
  if (file_to_string_alloc(GREETINGS_FILE, &GREETINGS) == 0)
    prune_crlf(GREETINGS);
  if (file_to_string_alloc(GREETANSI_FILE, &GREETANSI) == 0)
    prune_crlf(GREETANSI);

  log("Loading spell definitions.");
  mag_assign_spells();

  boot_world();

  htree_test();

  log("Loading help entries.");
  index_boot(DB_BOOT_HLP);

  log("Setting up context sensitive help system for OLC");
  boot_context_help();

  log("Generating player index.");
  build_player_index();

  insure_directory(LIB_PLROBJS "CRASH", 0);

  if (auto_pwipe) {
    log("Cleaning out inactive players.");
    clean_pfiles();
  }

  log("Loading fight messages.");
  load_messages();

  log("Loading social messages.");
  boot_social_messages();

  log("Building command list.");
  create_command_list(); /* aedit patch -- M. Scott */

  log("Assigning function pointers:");

  if (!no_specials) {
    log("   Mobiles.");
    assign_mobiles();
    log("   Shopkeepers.");
    assign_the_shopkeepers();
    log("   Objects.");
    assign_objects();
    log("   Rooms.");
    assign_rooms();
    log("   Guildmasters.");
    assign_the_guilds();
  }

  log("Init Object Unique Hash");
  init_obj_unique_hash();

  log("Booting assembled objects.");
  assemblyBootAssemblies();

  log("Assigning spell and skill levels.");
  init_spell_levels();

  log("Assigning race spell and skill levels.");
  init_spell_race_levels();

  log("Sorting command list and spells.");
  sort_commands();
  sort_spells();

  log("Booting mail system.");
  if (!scan_file()) {
    log("    Mail boot failed -- Mail system disabled");
    no_mail = 1;
  }
  
  log("Booting boards system.");
  init_boards();
  
  log("Reading banned site and invalid-name list.");
  load_banned();
  Read_Invalid_List();

  if (!no_rent_check) {
    log("Deleting timed-out crash and rent files:");
    update_obj_file();
    log("   Done.");
  }

  /* Moved here so the object limit code works. -gg 6/24/98 */
  if (!mini_mud) {
    log("Booting houses.");
    House_boot();
  }

  for (i = 0; i <= top_of_zone_table; i++) {
    log("Resetting #%d: %s (rooms %d-%d).", zone_table[i].number,
	zone_table[i].name, zone_table[i].bot, zone_table[i].top);
    reset_zone(i);
  }

  reset_q.head = reset_q.tail = NULL;

  boot_time = time(0);

  log("Boot db -- DONE.");
}


/* reset the time in the game from file */
void reset_time(void)
{
  time_t beginning_of_time = 0;
  FILE *bgtime;

  if ((bgtime = fopen(TIME_FILE, "r")) == NULL)
    log("SYSERR: Can't read from '%s' time file.", TIME_FILE);
  else {
    fscanf(bgtime, "%ld\n", &beginning_of_time);
    fclose(bgtime);
  }
  if (beginning_of_time == 0)
    beginning_of_time = 650336715;

  time_info = *mud_time_passed(time(0), beginning_of_time);

  if (time_info.hours <= 4)
    weather_info.sunlight = SUN_DARK;
  else if (time_info.hours == 5)
    weather_info.sunlight = SUN_RISE;
  else if (time_info.hours <= 20)
    weather_info.sunlight = SUN_LIGHT;
  else if (time_info.hours == 21)
    weather_info.sunlight = SUN_SET;
  else
    weather_info.sunlight = SUN_DARK;

  log("   Current Gametime: %dH %dD %dM %dY.", time_info.hours,
	  time_info.day, time_info.month, time_info.year);

  weather_info.pressure = 960;
  if ((time_info.month >= 7) && (time_info.month <= 12))
    weather_info.pressure += dice(1, 50);
  else
    weather_info.pressure += dice(1, 80);

  weather_info.change = 0;

  if (weather_info.pressure <= 980)
    weather_info.sky = SKY_LIGHTNING;
  else if (weather_info.pressure <= 1000)
    weather_info.sky = SKY_RAINING;
  else if (weather_info.pressure <= 1020)
    weather_info.sky = SKY_CLOUDY;
  else
    weather_info.sky = SKY_CLOUDLESS;
}


/* Write the time in 'when' to the MUD-time file. */
void save_mud_time(struct time_info_data *when)
{
  FILE *bgtime;

  if ((bgtime = fopen(TIME_FILE, "w")) == NULL)
    log("SYSERR: Can't write to '%s' time file.", TIME_FILE);
  else {
    fprintf(bgtime, "%ld\n", mud_time_to_secs(when));
    fclose(bgtime);
  }
}


/*
 * Thanks to Andrey (andrey@alex-ua.com) for this bit of code, although I
 * did add the 'goto' and changed some "while()" into "do { } while()".
 *	-gg 6/24/98 (technically 6/25/98, but I care not.)
 */
int count_alias_records(FILE *fl)
{
  char key[READ_SIZE], next_key[READ_SIZE];
  char line[READ_SIZE], *scan;
  int total_keywords = 0;

  /* get the first keyword line */
  get_one_line(fl, key);

  while (*key != '$') {
    /* skip the text */
    do {
      get_one_line(fl, line);
      if (feof(fl))
	goto ackeof;
    } while (*line != '#');

    /* now count keywords */
    scan = key;
    do {
      scan = one_word(scan, next_key);
      if (*next_key)
        ++total_keywords;
    } while (*next_key);

    /* get next keyword line (or $) */
    get_one_line(fl, key);

    if (feof(fl))
      goto ackeof;
  }

  return (total_keywords);

  /* No, they are not evil. -gg 6/24/98 */
ackeof:	
  log("SYSERR: Unexpected end of help file.");
  exit(1);	/* Some day we hope to handle these things better... */
}

/* function to count how many hash-mark delimited records exist in a file */
int count_hash_records(FILE *fl)
{
  char buf[128];
  int count = 0;

  while (fgets(buf, 128, fl))
    if (*buf == '#')
      count++;

  return (count);
}



void index_boot(int mode)
{
  const char *index_filename, *prefix = NULL;	/* NULL or egcs 1.1 complains */
  FILE *db_index, *db_file;
  int rec_count = 0, size[2];
  char buf2[PATH_MAX], buf1[MAX_STRING_LENGTH];

  switch (mode) {
  case DB_BOOT_WLD:
    prefix = WLD_PREFIX;
    break;
  case DB_BOOT_MOB:
    prefix = MOB_PREFIX;
    break;
  case DB_BOOT_OBJ:
    prefix = OBJ_PREFIX;
    break;
  case DB_BOOT_ZON:
    prefix = ZON_PREFIX;
    break;
  case DB_BOOT_SHP:
    prefix = SHP_PREFIX;
    break;
  case DB_BOOT_HLP:
    prefix = HLP_PREFIX;
    break;
  case DB_BOOT_TRG:
    prefix = TRG_PREFIX;
    break;
  case DB_BOOT_GLD:
    prefix = GLD_PREFIX;
    break;
  default:
    log("SYSERR: Unknown subcommand %d to index_boot!", mode);
    exit(1);
  }

  if (mini_mud)
    index_filename = MINDEX_FILE;
  else
    index_filename = INDEX_FILE;

  snprintf(buf2, sizeof(buf2), "%s%s", prefix, index_filename);
  if (!(db_index = fopen(buf2, "r"))) {
    log("SYSERR: opening index file '%s': %s", buf2, strerror(errno));
    exit(1);
  }

  /* first, count the number of records in the file so we can malloc */
  fscanf(db_index, "%s\n", buf1);
  while (*buf1 != '$') {
    snprintf(buf2, sizeof(buf2), "%s%s", prefix, buf1);
    if (!(db_file = fopen(buf2, "r"))) {
      log("SYSERR: File '%s' listed in '%s/%s': %s", buf2, prefix,
	  index_filename, strerror(errno));
      fscanf(db_index, "%s\n", buf1);
      continue;
    } else {
      if (mode == DB_BOOT_ZON)
	rec_count++;
      else if (mode == DB_BOOT_HLP)
	rec_count += count_alias_records(db_file);
      else
	rec_count += count_hash_records(db_file);
    }

    fclose(db_file);
    fscanf(db_index, "%s\n", buf1);
  }

  /* Exit if 0 records, unless this is shops */
  if (!rec_count) {
    if (mode == DB_BOOT_SHP || mode == DB_BOOT_GLD)
      return;
    log("SYSERR: boot error - 0 records counted in %s/%s.", prefix,
	index_filename);
    exit(1);
  }

  /*
   * NOTE: "bytes" does _not_ include strings or other later malloc'd things.
   */
  switch (mode) {
  case DB_BOOT_TRG:
    CREATE(trig_index, struct index_data *, rec_count);
    break;
  case DB_BOOT_WLD:
    CREATE(world, struct room_data, rec_count);
    size[0] = sizeof(struct room_data) * rec_count;
    log("   %d rooms, %d bytes.", rec_count, size[0]);
    break;
  case DB_BOOT_MOB:
    CREATE(mob_proto, struct char_data, rec_count);
    CREATE(mob_index, struct index_data, rec_count);
    size[0] = sizeof(struct index_data) * rec_count;
    size[1] = sizeof(struct char_data) * rec_count;
    log("   %d mobs, %d bytes in index, %d bytes in prototypes.", rec_count, size[0], size[1]);
    break;
  case DB_BOOT_OBJ:
    CREATE(obj_proto, struct obj_data, rec_count);
    CREATE(obj_index, struct index_data, rec_count);
    size[0] = sizeof(struct index_data) * rec_count;
    size[1] = sizeof(struct obj_data) * rec_count;
    log("   %d objs, %d bytes in index, %d bytes in prototypes.", rec_count, size[0], size[1]);
    break;
  case DB_BOOT_ZON:
    CREATE(zone_table, struct zone_data, rec_count);
    size[0] = sizeof(struct zone_data) * rec_count;
    log("   %d zones, %d bytes.", rec_count, size[0]);
    break;
  case DB_BOOT_HLP:
    CREATE(help_table, struct help_index_element, rec_count);
    size[0] = sizeof(struct help_index_element) * rec_count;
    log("   %d entries, %d bytes.", rec_count, size[0]);
    break;
  }

  rewind(db_index);
  fscanf(db_index, "%s\n", buf1);
  while (*buf1 != '$') {
    snprintf(buf2, sizeof(buf2), "%s%s", prefix, buf1);
    if (!(db_file = fopen(buf2, "r"))) {
      log("SYSERR: %s: %s", buf2, strerror(errno));
      exit(1);
    }
    switch (mode) {
    case DB_BOOT_WLD:
    case DB_BOOT_OBJ:
    case DB_BOOT_MOB:
    case DB_BOOT_TRG:
      discrete_load(db_file, mode, buf2);
      break;
    case DB_BOOT_ZON:
      load_zones(db_file, buf2);
      break;
    case DB_BOOT_HLP:
      /*
       * If you think about it, we have a race here.  Although, this is the
       * "point-the-gun-at-your-own-foot" type of race.
       */
      load_help(db_file);
      break;
    case DB_BOOT_SHP:
      boot_the_shops(db_file, buf2, rec_count);
      break;
    case DB_BOOT_GLD:
      boot_the_guilds(db_file, buf2, rec_count);
      break;
    }

    fclose(db_file);
    fscanf(db_index, "%s\n", buf1);
  }
  fclose(db_index);

  /* sort the help index */
  if (mode == DB_BOOT_HLP) {
    qsort(help_table, top_of_helpt, sizeof(struct help_index_element), hsort);
    top_of_helpt--;
  }
}


void discrete_load(FILE *fl, int mode, char *filename)
{
  int nr = -1, last;
  char line[READ_SIZE];

  const char *modes[] = {"world", "mob", "obj", "ZON", "SHP", "HLP", "trg"};
  /* modes positions correspond to DB_BOOT_xxx in db.h */

  for (;;) {
    /*
     * we have to do special processing with the obj files because they have
     * no end-of-record marker :(
     */
    if (mode != DB_BOOT_OBJ || nr < 0)
      if (!get_line(fl, line)) {
	if (nr == -1) {
	  log("SYSERR: %s file %s is empty!", modes[mode], filename);
	} else {
	  log("SYSERR: Format error in %s after %s #%d\n"
	      "...expecting a new %s, but file ended!\n"
	      "(maybe the file is not terminated with '$'?)", filename,
	      modes[mode], nr, modes[mode]);
	}
	exit(1);
      }
    if (*line == '$')
      return;

    if (*line == '#') {
      last = nr;
      if (sscanf(line, "#%d", &nr) != 1) {
	log("SYSERR: Format error after %s #%d", modes[mode], last);
	exit(1);
      }
      if (nr >= 99999)
	return;
      else
	switch (mode) {
	case DB_BOOT_WLD:
	  parse_room(fl, nr);
	  break;
	case DB_BOOT_MOB:
	  parse_mobile(fl, nr);
	  break;
        case DB_BOOT_TRG:
          parse_trigger(fl, nr);
          break;
	case DB_BOOT_OBJ:
	  strlcpy(line, parse_object(fl, nr), sizeof(line));
	  break;
	}
    } else {
      log("SYSERR: Format error in %s file %s near %s #%d", modes[mode],
	  filename, modes[mode], nr);
      log("SYSERR: ... offending line: '%s'", line);
      exit(1);
    }
  }
}

char fread_letter(FILE *fp)
{
  char c;
  do {
    c = getc(fp);  
  } while (isspace(c));
  return c;
}

bitvector_t asciiflag_conv(char *flag)
{
  bitvector_t flags = 0;
  int is_num = TRUE;
  char *p;

  for (p = flag; *p; p++) {
    if (islower(*p))
      flags |= 1 << (*p - 'a');
    else if (isupper(*p))
      flags |= 1 << (26 + (*p - 'A'));

    if (!(isdigit(*p) || (*p == '-')))
      is_num = FALSE;
  }

  if (is_num)
    flags = atol(flag);

  return (flags);
}

bitvector_t asciiflag_conv_aff(char *flag)
{
  bitvector_t flags = 0;
  int is_num = TRUE;
  char *p;

  for (p = flag; *p; p++) {
    if (islower(*p))
      flags |= 1 << (1 + (*p - 'a'));
    else if (isupper(*p))
      flags |= 1 << (26 + (*p - 'A'));

    if (!(isdigit(*p) || (*p == '-')))
      is_num = FALSE;
  }

  if (is_num)
    flags = atol(flag);

  return (flags);
}
/* load the rooms */
void parse_room(FILE *fl, int virtual_nr)
{
  static int room_nr = 0, zone = 0;
  int t[10], i, retval;
  char line[READ_SIZE], flags[128], flags2[128], flags3[128];
  char flags4[128], buf2[MAX_STRING_LENGTH], buf[128];
  struct extra_descr_data *new_descr;
  char letter;

  /* This really had better fit or there are other problems. */
  snprintf(buf2, sizeof(buf2), "room #%d", virtual_nr);

  if (virtual_nr < zone_table[zone].bot) {
    log("SYSERR: Room #%d is below zone %d.", virtual_nr, zone);
    exit(1);
  }
  while (virtual_nr > zone_table[zone].top)
    if (++zone > top_of_zone_table) {
      log("SYSERR: Room %d is outside of any zone.", virtual_nr);
      exit(1);
    }
  world[room_nr].zone = zone;
  world[room_nr].number = virtual_nr;
  world[room_nr].name = fread_string(fl, buf2);
  world[room_nr].description = fread_string(fl, buf2);

  if (! room_htree)
    room_htree = htree_init();
  htree_add(room_htree, virtual_nr, room_nr);

  if (!get_line(fl, line)) {
    log("SYSERR: Expecting roomflags/sector type of room #%d but file ended!",
	virtual_nr);
    exit(1);
  }
 
  if (((retval = sscanf(line, " %d %s %s %s %s %d ", t, flags, flags2, flags3, flags4, t + 2)) == 3) && (bitwarning == TRUE)) {
    log("WARNING: Conventional worldfiles detected. Please read 128bit.readme.");
    exit(1);
  } else if ((retval == 3) && (bitwarning == FALSE)) {
  /* 
   * Looks like the implementor is ready, so let's load the worldfiles. We 
   * load the extra three flags as 0, since they won't be anything anyway. We 
   * will save the entire world later on, when every room, mobile, and object 
   * is converted.
   */

    log("Converting room #%d to 128bits..", virtual_nr);
    world[room_nr].room_flags[0] = asciiflag_conv(flags);
    world[room_nr].room_flags[1] = 0;
    world[room_nr].room_flags[2] = 0;
    world[room_nr].room_flags[3] = 0;

    world[room_nr].sector_type = atoi(flags2);
    
    sprintf(flags, "room #%d", virtual_nr);	/* sprintf: OK (until 399-bit integers) */
    
    /* No need to scan the other three sections; they're 0 anyway */
    check_bitvector_names(world[room_nr].room_flags[0], room_bits_count, flags, "room"); 
	
    if(bitsavetodisk) { /* Maybe the implementor just wants to look at the 128bit files */
      add_to_save_list(zone_table[real_zone_by_thing(virtual_nr)].number, 3);
      converting = TRUE;
    }

    log("   done.");
  } else if (retval == 6) {
  int taeller;

  world[room_nr].room_flags[0] = asciiflag_conv(flags);
  world[room_nr].room_flags[1] = asciiflag_conv(flags2);
  world[room_nr].room_flags[2] = asciiflag_conv(flags3);
  world[room_nr].room_flags[3] = asciiflag_conv(flags4);

  world[room_nr].sector_type = t[2];
  sprintf(flags, "object #%d", virtual_nr);	/* sprintf: OK (until 399-bit integers) */
  for(taeller=0; taeller < AF_ARRAY_MAX; taeller++) 
    check_bitvector_names(world[room_nr].room_flags[taeller], room_bits_count, flags, "room");
  } else {
  log("SYSERR: Format error in roomflags/sector type of room #%d", virtual_nr);
  exit(1);
  }

  world[room_nr].func = NULL;
  world[room_nr].contents = NULL;
  world[room_nr].people = NULL;
  world[room_nr].light = 0;	/* Zero light sources */
  world[room_nr].timed = -1;

  for (i = 0; i < NUM_OF_DIRS; i++)
    world[room_nr].dir_option[i] = NULL;

  world[room_nr].ex_description = NULL;

  snprintf(buf, sizeof(buf), "SYSERR: Format error in room #%d (expecting D/E/S)", virtual_nr);

  for (;;) {
    if (!get_line(fl, line)) {
      log("%s", buf);
      exit(1);
    }
    switch (*line) {
    case 'D':
      setup_dir(fl, room_nr, atoi(line + 1));
      break;
    case 'E':
      CREATE(new_descr, struct extra_descr_data, 1);
      new_descr->keyword = fread_string(fl, buf2);
      new_descr->description = fread_string(fl, buf2);
      /* fix for crashes in the editor when formatting 
       * - e-descs are assumed to end with a \r\n
       * -- Welcor 09/03 
       */
      { 
      	char *t = strchr(new_descr->description, '\0');
      	if (t > new_descr->description && *(t-1) != '\n') {
      	  CREATE(t, char, strlen(new_descr->description)+3);
      	  sprintf(t, "%s\r\n", new_descr->description); /* sprintf ok : size checked above*/
      	  free(new_descr->description);
      	  new_descr->description = t;
      	}
      }
      new_descr->next = world[room_nr].ex_description;
      world[room_nr].ex_description = new_descr;
      break;
    case 'S':			/* end of room */
      /* DG triggers -- script is defined after the end of the room */
      letter = fread_letter(fl);
      ungetc(letter, fl);
      while (letter=='T') {
        dg_read_trigger(fl, &world[room_nr], WLD_TRIGGER);
        letter = fread_letter(fl);
        ungetc(letter, fl);
      }
      top_of_world = room_nr++;
      return;
    default:
      log("%s", buf);
      exit(1);
    }
  }
}



/* read direction data */
void setup_dir(FILE *fl, int room, int dir)
{
  int t[5];
  char line[READ_SIZE], buf2[128];

  snprintf(buf2, sizeof(buf2), "room #%d, direction D%d", GET_ROOM_VNUM(room-1)+1, dir);

  CREATE(world[room].dir_option[dir], struct room_direction_data, 1);
  world[room].dir_option[dir]->general_description = fread_string(fl, buf2);
  world[room].dir_option[dir]->keyword = fread_string(fl, buf2);

  if (!get_line(fl, line)) {
    log("SYSERR: Format error, %s", buf2);
    exit(1);
  }
  if (sscanf(line, " %d %d %d ", t, t + 1, t + 2) != 3) {
    log("SYSERR: Format error, %s", buf2);
    exit(1);
  }
  if (t[0] == 1)
    world[room].dir_option[dir]->exit_info = EX_ISDOOR;
  else if (t[0] == 2)
    world[room].dir_option[dir]->exit_info = EX_ISDOOR | EX_PICKPROOF;
  else if (t[0] == 3)
    world[room].dir_option[dir]->exit_info = EX_ISDOOR | EX_SECRET;
  else if (t[0] == 4)
    world[room].dir_option[dir]->exit_info = EX_ISDOOR|EX_PICKPROOF|EX_SECRET;
  else
    world[room].dir_option[dir]->exit_info = 0;

  world[room].dir_option[dir]->key = ((t[1] == -1 || t[1] == 65535) ? NOTHING : t[1]);
  world[room].dir_option[dir]->to_room = ((t[2] == -1  || t[2] == 65535) ? NOWHERE : t[2]);
}


/* make sure the start rooms exist & resolve their vnums to rnums */
void check_start_rooms(void)
{
  if ((r_mortal_start_room = real_room(CONFIG_MORTAL_START)) == NOWHERE) {
    log("SYSERR:  Mortal start room does not exist.  Change in config.c.");
    exit(1);
  }
  if ((r_immort_start_room = real_room(CONFIG_IMMORTAL_START)) == NOWHERE) {
    if (!mini_mud)
      log("SYSERR:  Warning: Immort start room does not exist.  Change in config.c.");
    r_immort_start_room = r_mortal_start_room;
  }
  if ((r_frozen_start_room = real_room(CONFIG_FROZEN_START)) == NOWHERE) {
    if (!mini_mud)
      log("SYSERR:  Warning: Frozen start room does not exist.  Change in config.c.");
    r_frozen_start_room = r_mortal_start_room;
  }
}


/* resolve all vnums into rnums in the world */
void renum_world(void)
{
  int room, door;

  for (room = 0; room <= top_of_world; room++)
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (world[room].dir_option[door])
	if (world[room].dir_option[door]->to_room != NOWHERE)
	  world[room].dir_option[door]->to_room =
	    real_room(world[room].dir_option[door]->to_room);
}


#define ZCMD zone_table[zone].cmd[cmd_no]

/*
 * "resulve vnums into rnums in the zone reset tables"
 *
 * Or in English: Once all of the zone reset tables have been loaded, we
 * resolve the virtual numbers into real numbers all at once so we don't have
 * to do it repeatedly while the game is running.  This does make adding any
 * room, mobile, or object a little more difficult while the game is running.
 *
 * NOTE 1: Assumes NOWHERE == NOBODY == NOTHING.
 * NOTE 2: Assumes sizeof(room_rnum) >= (sizeof(mob_rnum) and sizeof(obj_rnum))
 */
void renum_zone_table(void)
{
  int cmd_no;
  room_rnum a, b, c, olda, oldb, oldc;
  zone_rnum zone;
  char buf[128];

  for (zone = 0; zone <= top_of_zone_table; zone++)
    for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++) {
      a = b = c = 0;
      olda = ZCMD.arg1;
      oldb = ZCMD.arg2;
      oldc = ZCMD.arg3;
      switch (ZCMD.command) {
      case 'M':
	a = ZCMD.arg1 = real_mobile(ZCMD.arg1);
	c = ZCMD.arg3 = real_room(ZCMD.arg3);
	break;
      case 'O':
	a = ZCMD.arg1 = real_object(ZCMD.arg1);
	if (ZCMD.arg3 != NOWHERE)
	  c = ZCMD.arg3 = real_room(ZCMD.arg3);
	break;
      case 'G':
	a = ZCMD.arg1 = real_object(ZCMD.arg1);
	break;
      case 'E':
	a = ZCMD.arg1 = real_object(ZCMD.arg1);
	break;
      case 'P':
	a = ZCMD.arg1 = real_object(ZCMD.arg1);
	c = ZCMD.arg3 = real_object(ZCMD.arg3);
	break;
      case 'D':
	a = ZCMD.arg1 = real_room(ZCMD.arg1);
	break;
      case 'R': /* rem obj from room */
        a = ZCMD.arg1 = real_room(ZCMD.arg1);
	b = ZCMD.arg2 = real_object(ZCMD.arg2);
        break;
      case 'T': /* a trigger */
        b = ZCMD.arg2 = real_trigger(ZCMD.arg2);
        c = ZCMD.arg3 = real_room(ZCMD.arg3);
        break;
      case 'V': /* trigger variable assignment */
        b = ZCMD.arg3 = real_room(ZCMD.arg3);
        break;
      }
      if (a == NOWHERE || b == NOWHERE || c == NOWHERE) {
	if (!mini_mud) {
	  snprintf(buf, sizeof(buf), "Invalid vnum %d, cmd disabled",
			 a == NOWHERE ? olda : b == NOWHERE ? oldb : oldc);
	  log_zone_error(zone, cmd_no, buf);
	}
	ZCMD.command = '*';
      }
    }
}



int parse_simple_mob(FILE *mob_f, struct char_data *ch, int nr)
{
  int j, t[10];
  char line[READ_SIZE];

  ch->real_abils.str = 11;
  ch->real_abils.intel = 11;
  ch->real_abils.wis = 11;
  ch->real_abils.dex = 11;
  ch->real_abils.con = 11;
  ch->real_abils.cha = 11;

  if (!get_line(mob_f, line)) {
    log("SYSERR: Format error in mob #%d, file ended after S flag!", nr);
    return 0;
  }

  if (sscanf(line, " %d %d %d %dd%d+%d %dd%d+%d ",
	  t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7, t + 8) != 9) {
    log("SYSERR: Format error in mob #%d, first line after S flag\n"
	"...expecting line of form '# # # #d#+# #d#+#'", nr);
    return 0;
  }

  GET_LEVEL(ch) = t[0];
  GET_HITROLL(ch) = 20 - t[1];
  GET_AC(ch) = 10 * t[2];

  /* max hit = 0 is a flag that H, M, V is xdy+z */
  GET_MAX_HIT(ch) = 0;
  GET_HIT(ch) = t[3];
  GET_MANA(ch) = t[4];
  GET_MOVE(ch) = t[5];

  GET_MAX_MANA(ch) = 10 * GET_LEVEL(ch);
  GET_MAX_MOVE(ch) = 50;

  ch->mob_specials.damnodice = t[6];
  ch->mob_specials.damsizedice = t[7];
  GET_DAMROLL(ch) = t[8];

  if (!get_line(mob_f, line)) {
      log("SYSERR: Format error in mob #%d, second line after S flag\n"
	  "...expecting line of form '# #', but file ended!", nr);
      return 0;
    }

  if (sscanf(line, " %d %d %d %d", t, t + 1, t + 2, t + 3) != 4) {
    log("SYSERR: Format error in mob #%d, second line after S flag\n"
        "...expecting line of form '# # # #'", nr);
    return 0;
  }

  GET_GOLD(ch) = t[0];
  GET_EXP(ch) = t[1];
  GET_RACE(ch) = t[2];
  GET_CLASS(ch) = t[3];

  if (!IS_HUMAN(ch))
    if (!AFF_FLAGGED(ch, AFF_INFRAVISION))
      SET_BIT_AR(AFF_FLAGS(ch), AFF_INFRAVISION);

  SPEAKING(ch) = SKILL_LANG_COMMON;

  for (j = 0; j < MAX_SKILLS + 1; j++)
    ch->mob_specials.mskills[j] = MIN((GET_LEVEL(ch) *5 + ch->real_abils.intel), 98);

  if (GET_LEVEL(ch) < 1) {
    log("SYSERR: Level 0 Mob (#%d) detected!",  nr);
    GET_LEVEL(ch) = 1;
  }

  if (!get_line(mob_f, line)) {
    log("SYSERR: Format error in last line of mob #%d\n"
        "...expecting line of form '# # #', but file ended!", nr);
    return 0;
  }

  if (sscanf(line, " %d %d %d ", t, t + 1, t + 2) != 3) {
    log("SYSERR: Format error in last line of mob #%d\n"
        "...expecting line of form '# # #'", nr);
    return 0;
  }

  GET_POS(ch) = t[0];
  GET_DEFAULT_POS(ch) = t[1];
  GET_SEX(ch) = t[2];

  SPEAKING(ch) = MIN_LANGUAGES;
  GET_WEIGHT(ch) = 200;
  GET_HEIGHT(ch) = 198;

  /*
   * these are now save applies; base save numbers for MOBs are now from
   * the warrior save table.
   */
  for (j = 0; j < 5; j++)
    GET_SAVE(ch, j) = 0;

  return 1;
}


/*
 * interpret_espec is the function that takes espec keywords and values
 * and assigns the correct value to the mob as appropriate.  Adding new
 * e-specs is absurdly easy -- just add a new CASE statement to this
 * function!  No other changes need to be made anywhere in the code.
 *
 * CASE		: Requires a parameter through 'value'.
 * BOOL_CASE	: Being specified at all is its value.
 */

#define CASE(test)	\
	if (value && !matched && !str_cmp(keyword, test) && (matched = TRUE))

#define BOOL_CASE(test)	\
	if (!value && !matched && !str_cmp(keyword, test) && (matched = TRUE))

#define RANGE(low, high)	\
	(num_arg = MAX((low), MIN((high), (num_arg))))

void interpret_espec(const char *keyword, const char *value, struct char_data *ch, int nr)
{
  int num_arg = 0, matched = FALSE;
  int num, num2, num3, num4, num5;
  struct affected_type af;

  /*
   * If there isn't a colon, there is no value.  While Boolean options are
   * possible, we don't actually have any.  Feel free to make some.
   */

  if (value)
    num_arg = atoi(value);

  CASE("BareHandAttack") {
    RANGE(0, 99);
    ch->mob_specials.attack_type = num_arg;
  }

  CASE("Str") {
    RANGE(0, 200);
    ch->real_abils.str = num_arg;
  }

  CASE("StrAdd") {
    RANGE(0, 100);
    ch->real_abils.str_add = num_arg;    
  }

  CASE("Int") {
    RANGE(0, 200);
    ch->real_abils.intel = num_arg;
  }

  CASE("Wis") {
    RANGE(0, 200);
    ch->real_abils.wis = num_arg;
  }

  CASE("Dex") {
    RANGE(0, 200);
    ch->real_abils.dex = num_arg;
  }

  CASE("Con") {
    RANGE(0, 200);
    ch->real_abils.con = num_arg;
  }

  CASE("Cha") {
    RANGE(0, 200);
    ch->real_abils.cha = num_arg;
  }

  CASE("Hit") {
    RANGE(0, 99999);
    GET_HIT(ch) = num_arg;
  }

  CASE("MaxHit") {
    RANGE(0, 99999);
    GET_MAX_HIT(ch) = num_arg;
  }

  CASE("Mana") {
    RANGE(0, 99999);
    GET_MANA(ch) = num_arg;
  }

  CASE("MaxMana") {
    RANGE(0, 99999);
    GET_MAX_MANA(ch) = num_arg;
  }

  CASE("Moves") {
    RANGE(0, 99999);
    GET_MOVE(ch) = num_arg;
  }

  CASE("MaxMoves") {
    RANGE(0, 99999);
    GET_MAX_MOVE(ch) = num_arg;
  }

  CASE("Affect") {
    sscanf(value, "%d %d %d %d %d", &num, &num2, &num3, &num4, &num5);
    if (num > 0) {
      af.type = num;
      af.duration = num2;
      af.modifier = num3;
      af.location = num4;
      af.bitvector = num5;
      affect_to_char(ch, &af);
    }
  }

  if (!matched) {
    log("SYSERR: Warning: unrecognized espec keyword %s in mob #%d",
	    keyword, nr);
  }    
}

#undef CASE
#undef BOOL_CASE
#undef RANGE

void parse_espec(char *buf, struct char_data *ch, int nr)
{
  char *ptr;

  if ((ptr = strchr(buf, ':')) != NULL) {
    *(ptr++) = '\0';
    while (isspace(*ptr))
      ptr++;
  }
  interpret_espec(buf, ptr, ch, nr);
}


int parse_enhanced_mob(FILE *mob_f, struct char_data *ch, int nr)
{
  char line[READ_SIZE];

  parse_simple_mob(mob_f, ch, nr);

  while (get_line(mob_f, line)) {
    if (!strcmp(line, "E"))	/* end of the enhanced section */
      return 1;
    else if (*line == '#') {	/* we've hit the next mob, maybe? */
      log("SYSERR: Unterminated E section in mob #%d", nr);
      return 0;
    } else
      parse_espec(line, ch, nr);
  }

  log("SYSERR: Unexpected end of file reached after mob #%d", nr);
  return 0;
}


int parse_mobile_from_file(FILE *mob_f, struct char_data *ch)
{
  int j, t[10], retval;
  char line[READ_SIZE], *tmpptr, letter;
  char f1[128], f2[128], f3[128], f4[128], f5[128], f6[128];
  char f7[128], f8[128], buf2[128];

  /*
   * Mobiles should NEVER use anything in the 'player_specials' structure.
   * The only reason we have every mob in the game share this copy of the
   * structure is to save newbie coders from themselves. -gg 2/25/98
   */
  ch->player_specials = &dummy_mob;
  sprintf(buf2, "mob vnum %d", GET_MOB_VNUM(ch));	/* sprintf: OK (for 'buf2 >= 19') */

  /***** String data *****/
  ch->player.name = fread_string(mob_f, buf2);
  tmpptr = ch->player.short_descr = fread_string(mob_f, buf2);
  if (tmpptr && *tmpptr)
    if (!str_cmp(fname(tmpptr), "a") || !str_cmp(fname(tmpptr), "an") ||
	!str_cmp(fname(tmpptr), "the"))
      *tmpptr = LOWER(*tmpptr);
  ch->player.long_descr = fread_string(mob_f, buf2);
  ch->player.description = fread_string(mob_f, buf2);
  GET_TITLE(ch) = NULL;

  /* *** Numeric data *** */
  if (!get_line(mob_f, line)) {
    log("SYSERR: Format error after string section of mob #%d\n"
	"...expecting line of form '# # # {S | E}', but file ended!", GET_MOB_VNUM(ch));
    return 0;
  }
 
  if (((retval = sscanf(line, "%s %s %s %s %s %s %s %s %d %c", f1, f2, f3, f4, f5, f6, f7, f8, t + 2, &letter)) == 10) && (bitwarning == TRUE)) {
/* Let's make the implementor read some, before converting his worldfiles */
    log("WARNING: Conventional mobilefiles detected. Please read 128bit.readme.");
    return 0;
  } else if ((retval == 4) && (bitwarning == FALSE)) {
	    
    log("Converting mobile #%d to 128bits..", GET_MOB_VNUM(ch));
    MOB_FLAGS(ch)[0] = asciiflag_conv(f1);
    MOB_FLAGS(ch)[1] = 0;
    MOB_FLAGS(ch)[2] = 0;
    MOB_FLAGS(ch)[3] = 0;
    check_bitvector_names(MOB_FLAGS(ch)[0], action_bits_count, buf2, "mobile");
    
    AFF_FLAGS(ch)[0] = asciiflag_conv_aff(f2);
    AFF_FLAGS(ch)[1] = 0;
    AFF_FLAGS(ch)[2] = 0;
    AFF_FLAGS(ch)[3] = 0;

    GET_ALIGNMENT(ch) = atoi(f3);
    
	/* Make some basic checks. */
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_CHARM); 
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_POISON);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_GROUP);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_SLEEP);
    if (MOB_FLAGGED(ch, MOB_AGGRESSIVE) && MOB_FLAGGED(ch, MOB_AGGR_GOOD)) 
      REMOVE_BIT_AR(MOB_FLAGS(ch), MOB_AGGR_GOOD);
    if (MOB_FLAGGED(ch, MOB_AGGRESSIVE) && MOB_FLAGGED(ch, MOB_AGGR_NEUTRAL))
      REMOVE_BIT_AR(MOB_FLAGS(ch), MOB_AGGR_NEUTRAL);
    if (MOB_FLAGGED(ch, MOB_AGGRESSIVE) && MOB_FLAGGED(ch, MOB_AGGR_EVIL))
      REMOVE_BIT_AR(MOB_FLAGS(ch), MOB_AGGR_EVIL);

    check_bitvector_names(AFF_FLAGS(ch)[0], affected_bits_count, buf2, "mobile affect");

    /* 
     * This is necessary, since if we have conventional worldfiles, &letter
     * is loaded into f4 instead of the letter characters. So what we do, is 
     * copy f4 into letter. Disadvantage is that &letter cannot be longer
     * then 128 characters, but this shouldn't occur anyway.
     */
    letter = *f4;

    if(bitsavetodisk) {
      add_to_save_list(zone_table[real_zone_by_thing(GET_MOB_VNUM(ch))].number, 0);
      converting =TRUE;
    }

    log("   done.");
  } else if (retval == 10) {
    int taeller;

    MOB_FLAGS(ch)[0] = asciiflag_conv(f1);
    MOB_FLAGS(ch)[1] = asciiflag_conv(f2);
    MOB_FLAGS(ch)[2] = asciiflag_conv(f3);
    MOB_FLAGS(ch)[3] = asciiflag_conv(f4);
    for(taeller=0; taeller < AF_ARRAY_MAX; taeller++) 
      check_bitvector_names(MOB_FLAGS(ch)[taeller], action_bits_count, buf2, "mobile");
  
    AFF_FLAGS(ch)[0] = asciiflag_conv(f5);
    AFF_FLAGS(ch)[1] = asciiflag_conv(f6);
    AFF_FLAGS(ch)[2] = asciiflag_conv(f7);
    AFF_FLAGS(ch)[3] = asciiflag_conv(f8);

    GET_ALIGNMENT(ch) = t[2];

    for(taeller=0; taeller < AF_ARRAY_MAX; taeller++) 
      check_bitvector_names(AFF_FLAGS(ch)[taeller], affected_bits_count, buf2, "mobile affect");
  } else {
    log("SYSERR: Format error after string section of mob #%d\n"
	"...expecting line of form '# # # {S | E}'", GET_MOB_VNUM(ch));
    return 0;
  }

  SET_BIT_AR(MOB_FLAGS(ch), MOB_ISNPC);
  if (MOB_FLAGGED(ch, MOB_NOTDEADYET)) {
    /* Rather bad to load mobiles with this bit already set. */
    log("SYSERR: Mob #%d has reserved bit MOB_NOTDEADYET set.", GET_MOB_VNUM(ch));
    REMOVE_BIT_AR(MOB_FLAGS(ch), MOB_NOTDEADYET);
  }

  /* AGGR_TO_ALIGN is ignored if the mob is AGGRESSIVE.
  if (MOB_FLAGGED(ch, MOB_AGGRESSIVE) && MOB_FLAGGED(ch, MOB_AGGR_GOOD | MOB_AGGR_EVIL | MOB_AGGR_NEUTRAL))
    log("SYSERR: Mob #%d both Aggressive and Aggressive_to_Alignment.", GET_MOB_VNUM(ch)); */

  switch (UPPER(letter)) {
  case 'S':	/* Simple monsters */
    parse_simple_mob(mob_f, ch, GET_MOB_VNUM(ch));
    break;
  case 'E':	/* Circle3 Enhanced monsters */
    if (parse_enhanced_mob(mob_f, ch, GET_MOB_VNUM(ch)))
      break;
    else
      return 0;
  /* add new mob types here.. */
  default:
    log("SYSERR: Unsupported mob type '%c' in mob #%d", letter, GET_MOB_VNUM(ch));
    return 0;
  }

  /* DG triggers -- script info follows mob S/E section */
  letter = fread_letter(mob_f);
  ungetc(letter, mob_f);
  while (letter=='T') {
    dg_read_trigger(mob_f, ch, MOB_TRIGGER);
    letter = fread_letter(mob_f);
    ungetc(letter, mob_f);
  }

  ch->aff_abils = ch->real_abils;

  for (j = 0; j < NUM_WEARS; j++)
    ch->equipment[j] = NULL;

  return 1;
}


void parse_mobile(FILE *mob_f, int nr)
{
  static int i = 0;

  mob_index[i].vnum = nr;
  mob_index[i].number = 0;
  mob_index[i].func = NULL;

  clear_char(mob_proto + i);

  mob_proto[i].nr = i;
  mob_proto[i].desc = NULL;

  if (parse_mobile_from_file(mob_f, mob_proto + i)) {
    if (! mob_htree)
      mob_htree = htree_init();
    htree_add(mob_htree, nr, i);

    top_of_mobt = i++;
  } else { /* We used to exit in the file reading code, but now we do it here */
    exit(1);
  }
}



/* read all objects from obj file; generate index and prototypes */
char *parse_object(FILE *obj_f, int nr)
{
  static int i = 0;
  static char line[READ_SIZE];
  int t[10], j, retval;
  char *tmpptr, buf2[128];
  char f1[READ_SIZE], f2[READ_SIZE], f3[READ_SIZE], f4[READ_SIZE];
  char f5[READ_SIZE], f6[READ_SIZE], f7[READ_SIZE], f8[READ_SIZE];
  char f9[READ_SIZE], f10[READ_SIZE], f11[READ_SIZE], f12[READ_SIZE];
  struct extra_descr_data *new_descr;

  obj_index[i].vnum = nr;
  obj_index[i].number = 0;
  obj_index[i].func = NULL;

  if (! obj_htree)
    obj_htree = htree_init();
  htree_add(obj_htree, nr, i);

  clear_object(obj_proto + i);
  obj_proto[i].item_number = i;

  sprintf(buf2, "object #%d", nr);	/* sprintf: OK (for 'buf2 >= 19') */

  /* *** string data *** */
  if ((obj_proto[i].name = fread_string(obj_f, buf2)) == NULL) {
    log("SYSERR: Null obj name or format error at or near %s", buf2);
    exit(1);
  }
  tmpptr = obj_proto[i].short_description = fread_string(obj_f, buf2);
  if (tmpptr && *tmpptr)
    if (!str_cmp(fname(tmpptr), "a") || !str_cmp(fname(tmpptr), "an") ||
	!str_cmp(fname(tmpptr), "the"))
      *tmpptr = LOWER(*tmpptr);

  tmpptr = obj_proto[i].description = fread_string(obj_f, buf2);
  if (tmpptr && *tmpptr)
    CAP(tmpptr);
  obj_proto[i].action_description = fread_string(obj_f, buf2);

  /* *** numeric data *** */
  if (!get_line(obj_f, line)) {
    log("SYSERR: Expecting first numeric line of %s, but file ended!", buf2);
    exit(1);
  }

  if (((retval = sscanf(line, " %d %s %s %s %s %s %s %s %s %s %s %s %s", t, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12)) == 4) && (bitwarning == TRUE)) {
  /* Let's make the implementor read some, before converting his worldfiles */
  log("WARNING: Conventional objectfiles detected. Please read 128bit.readme.");
    exit(1);
  } else if (((retval == 4) || (retval == 3)) && (bitwarning == FALSE)) {
    
	if (retval == 3)
          t[3] = 0;
        else if (retval == 4)
          t[3] = asciiflag_conv_aff(f3);
	    
	log("Converting object #%d to 128bits..", nr);
    GET_OBJ_EXTRA(obj_proto + i)[0] = asciiflag_conv(f1);
    GET_OBJ_EXTRA(obj_proto + i)[1] = 0;
    GET_OBJ_EXTRA(obj_proto + i)[2] = 0;
    GET_OBJ_EXTRA(obj_proto + i)[3] = 0;
    GET_OBJ_WEAR(obj_proto + i)[0] = asciiflag_conv(f2);
    GET_OBJ_WEAR(obj_proto + i)[1] = 0;
    GET_OBJ_WEAR(obj_proto + i)[2] = 0;
    GET_OBJ_WEAR(obj_proto + i)[3] = 0;
    GET_OBJ_PERM(obj_proto + i)[0] = asciiflag_conv_aff(f3);
    GET_OBJ_PERM(obj_proto + i)[1] = 0;
    GET_OBJ_PERM(obj_proto + i)[2] = 0;
    GET_OBJ_PERM(obj_proto + i)[3] = 0;
    
	if(bitsavetodisk) { 
	  add_to_save_list(zone_table[real_zone_by_thing(nr)].number, 1);
	  converting = TRUE;
	}
	
	log("   done.");
  } else if (retval == 13) {
 
    GET_OBJ_EXTRA(obj_proto + i)[0] = asciiflag_conv(f1);
    GET_OBJ_EXTRA(obj_proto + i)[1] = asciiflag_conv(f2);
    GET_OBJ_EXTRA(obj_proto + i)[2] = asciiflag_conv(f3);
    GET_OBJ_EXTRA(obj_proto + i)[3] = asciiflag_conv(f4);
    GET_OBJ_WEAR(obj_proto + i)[0] = asciiflag_conv(f5);
    GET_OBJ_WEAR(obj_proto + i)[1] = asciiflag_conv(f6);
    GET_OBJ_WEAR(obj_proto + i)[2] = asciiflag_conv(f7);
    GET_OBJ_WEAR(obj_proto + i)[3] = asciiflag_conv(f8);
    GET_OBJ_PERM(obj_proto + i)[0] = asciiflag_conv(f9);
    GET_OBJ_PERM(obj_proto + i)[1] = asciiflag_conv(f10);
    GET_OBJ_PERM(obj_proto + i)[2] = asciiflag_conv(f11);
    GET_OBJ_PERM(obj_proto + i)[3] = asciiflag_conv(f12);

  } else {
    log("SYSERR: Format error in first numeric line (expecting 13 args, got %d), %s", retval, buf2);
    exit(1);
  }
  
  /* Object flags checked in check_object(). */
  GET_OBJ_TYPE(obj_proto + i) = t[0];
  
  if (!get_line(obj_f, line)) {
    log("SYSERR: Expecting second numeric line of %s, but file ended!", buf2);
    exit(1);
  }
  if ((retval = sscanf(line, "%d %d %d %d %d %d %d %d", t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7)) != 8) {
    log("SYSERR: Format error in second numeric line (expecting 8 args, got %d), %s", retval, buf2);
    exit(1);
  }
  GET_OBJ_VAL(obj_proto + i, 0) = t[0];
  GET_OBJ_VAL(obj_proto + i, 1) = t[1];
  GET_OBJ_VAL(obj_proto + i, 2) = t[2];
  GET_OBJ_VAL(obj_proto + i, 3) = t[3];
  GET_OBJ_VAL(obj_proto + i, 4) = t[4];
  GET_OBJ_VAL(obj_proto + i, 5) = t[5];
  GET_OBJ_VAL(obj_proto + i, 6) = t[6];
  GET_OBJ_VAL(obj_proto + i, 7) = t[7];

  if (!get_line(obj_f, line)) {
    log("SYSERR: Expecting third numeric line of %s, but file ended!", buf2);
    exit(1);
  }
  if ((retval = sscanf(line, "%d %d %d %d", t, t + 1, t + 2, t + 3)) != 4) {
    if (retval == 3)
      t[3] = 0;
    else {
      log("SYSERR: Format error in third numeric line (expecting 4 args, got %d), %s", retval, buf2);
    exit(1);
  }
  }
  GET_OBJ_WEIGHT(obj_proto + i) = t[0];
  GET_OBJ_COST(obj_proto + i) = t[1];
  GET_OBJ_RENT(obj_proto + i) = t[2];
  GET_OBJ_LEVEL(obj_proto + i) = t[3];

  /* check to make sure that weight of containers exceeds curr. quantity */
  if (GET_OBJ_TYPE(obj_proto + i) == ITEM_DRINKCON ||
      GET_OBJ_TYPE(obj_proto + i) == ITEM_FOUNTAIN) {
    if (GET_OBJ_WEIGHT(obj_proto + i) < GET_OBJ_VAL(obj_proto + i, 1))
      GET_OBJ_WEIGHT(obj_proto + i) = GET_OBJ_VAL(obj_proto + i, 1) + 5;
  }

  /* *** make sure portal objects have their timer set correctly *** */
  if (GET_OBJ_TYPE(obj_proto + i) == ITEM_PORTAL) {
    GET_OBJ_TIMER(obj_proto + i) =  -1;
  }

  /* *** extra descriptions and affect fields *** */

  for (j = 0; j < MAX_OBJ_AFFECT; j++) {
    obj_proto[i].affected[j].location = APPLY_NONE;
    obj_proto[i].affected[j].modifier = 0;
  }

  strcat(buf2, ", after numeric constants\n"	/* strcat: OK (for 'buf2 >= 87') */
	 "...expecting 'E', 'A', '$', or next object number");
  j = 0;

  for (;;) {
    if (!get_line(obj_f, line)) {
      log("SYSERR: Format error in %s", buf2);
      exit(1);
    }
    switch (*line) {
    case 'E':
      CREATE(new_descr, struct extra_descr_data, 1);
      new_descr->keyword = fread_string(obj_f, buf2);
      new_descr->description = fread_string(obj_f, buf2);
      new_descr->next = obj_proto[i].ex_description;
      obj_proto[i].ex_description = new_descr;
      break;
    case 'A':
      if (j >= MAX_OBJ_AFFECT) {
	log("SYSERR: Too many A fields (%d max), %s", MAX_OBJ_AFFECT, buf2);
	exit(1);
      }
      if (!get_line(obj_f, line)) {
	log("SYSERR: Format error in 'A' field, %s\n"
	    "...expecting 2 numeric constants but file ended!", buf2);
	exit(1);
      }

      if ((retval = sscanf(line, " %d %d ", t, t + 1)) != 2) {
	log("SYSERR: Format error in 'A' field, %s\n"
	    "...expecting 2 numeric arguments, got %d\n"
	    "...offending line: '%s'", buf2, retval, line);
	exit(1);
      }
      obj_proto[i].affected[j].location = t[0];
      obj_proto[i].affected[j].modifier = t[1];
      j++;
      break;
    case 'T':  /* DG triggers */
      dg_obj_trigger(line, &obj_proto[i]);
      break;
    case '$':
    case '#':
      top_of_objt = i;
      check_object(obj_proto + i);
      i++;
      return (line);
    default:
      log("SYSERR: Format error in (%c): %s", *line, buf2);
      exit(1);
    }
  }
}


#define Z	zone_table[zone]

/* load the zone table and command tables */
void load_zones(FILE *fl, char *zonename)
{
  static zone_rnum zone = 0;
  int cmd_no, num_of_cmds = 0, line_num = 0, tmp, error, arg_num;
  char *ptr, buf[READ_SIZE], zname[READ_SIZE], buf2[MAX_STRING_LENGTH];
  int zone_fix = FALSE;
  char t1[80], t2[80];

  strlcpy(zname, zonename, sizeof(zname));

  /* Skip first 3 lines lest we mistake the zone name for a command. */
  for (tmp = 0; tmp < 3; tmp++)
    get_line(fl, buf);

  /*  More accurate count. Previous was always 4 or 5 too high. -gg 2001/1/17
   *  Note that if a new zone command is added to reset_zone(), this string
   *  will need to be updated to suit. - ae.
   */
  while (get_line(fl, buf))
    if ((strchr("MOPGERDTV", buf[0]) && buf[1] == ' ') || (buf[0] == 'S' && buf[1] == '\0'))
      num_of_cmds++;

  rewind(fl);

  if (num_of_cmds == 0) {
    log("SYSERR: %s is empty!", zname);
    exit(1);
  } else
    CREATE(Z.cmd, struct reset_com, num_of_cmds);

  line_num += get_line(fl, buf);

  if (sscanf(buf, "#%hd", &Z.number) != 1) {
    log("SYSERR: Format error in %s, line %d", zname, line_num);
    exit(1);
  }
  snprintf(buf2, sizeof(buf2), "beginning of zone #%d", Z.number);

  line_num += get_line(fl, buf);
  if ((ptr = strchr(buf, '~')) != NULL)	/* take off the '~' if it's there */
    *ptr = '\0';
  Z.builders = strdup(buf);
  
  line_num += get_line(fl, buf);
  if ((ptr = strchr(buf, '~')) != NULL)	/* take off the '~' if it's there */
    *ptr = '\0';
  Z.name = strdup(buf);

  line_num += get_line(fl, buf);
  if (sscanf(buf, " %hd %hd %d %d ", &Z.bot, &Z.top, &Z.lifespan, &Z.reset_mode) != 4) {
    /*
     * This may be due to the fact that the zone has no builder.  So, we just attempt
     * to fix this by copying the previous 2 last reads into this variable and the
     * last one.
     */
    log("SYSERR: Format error in numeric constant line of %s, attempting to fix.", zname);
    if (sscanf(Z.name, " %hd %hd %d %d ", &Z.bot, &Z.top, &Z.lifespan, &Z.reset_mode) != 4) {
      log("SYSERR: Could not fix previous error, aborting game.");
    exit(1);
    } else {
      free(Z.name);
      Z.name = strdup(Z.builders);
      free(Z.builders);
      Z.builders = strdup("None.");
      zone_fix = TRUE;
    }
  }
  if (Z.bot > Z.top) {
    log("SYSERR: Zone %d bottom (%d) > top (%d).", Z.number, Z.bot, Z.top);
    exit(1);
  }

  cmd_no = 0;

  for (;;) {
    /* skip reading one line if we fixed above (line is correct already) */
    if (zone_fix != TRUE) {
    if ((tmp = get_line(fl, buf)) == 0) {
      log("SYSERR: Format error in %s - premature end of file", zname);
      exit(1);
    }
    } else
      zone_fix = FALSE;
    
    line_num += tmp;
    ptr = buf;
    skip_spaces(&ptr);

    if ((ZCMD.command = *ptr) == '*')
      continue;

    ptr++;

    if (ZCMD.command == 'S' || ZCMD.command == '$') {
      ZCMD.command = 'S';
      break;
    }
    error = 0;
    if (strchr("MOEPDTVG", ZCMD.command) == NULL) {	/* a 3-arg command */
      if (sscanf(ptr, " %d %d %d ", &tmp, &ZCMD.arg1, &ZCMD.arg2) != 3)
	error = 1;
    } else if (ZCMD.command=='V') { /* a string-arg command */
      if (sscanf(ptr, " %d %d %d %d %79s %79[^\f\n\r\t\v]", &tmp, &ZCMD.arg1, &ZCMD.arg2,
               &ZCMD.arg3, t1, t2) != 6)
      error = 1;
      else {
        ZCMD.sarg1 = strdup(t1);
        ZCMD.sarg2 = strdup(t2);
      }
    } else {
      if ((arg_num = sscanf(ptr, " %d %d %d %d %d ", &tmp, &ZCMD.arg1, &ZCMD.arg2, &ZCMD.arg3, &ZCMD.arg4)) != 5){
        if (arg_num != 4)
	error = 1;
        else
          ZCMD.arg4 = 0;
        }
    }

    ZCMD.if_flag = tmp;

    if (error) {
      log("SYSERR: Format error in %s, line %d: '%s'", zname, line_num, buf);
      exit(1);
    }
    ZCMD.line = line_num;
    cmd_no++;
  }

  if (num_of_cmds != cmd_no + 1) {
    log("SYSERR: Zone command count mismatch for %s. Estimated: %d, Actual: %d", zname, num_of_cmds, cmd_no + 1);
    exit(1);
  }

  top_of_zone_table = zone++;
}

#undef Z


void get_one_line(FILE *fl, char *buf)
{
  if (fgets(buf, READ_SIZE, fl) == NULL) {
    log("SYSERR: error reading help file: not terminated with $?");
    exit(1);
  }

  buf[strlen(buf) - 1] = '\0'; /* take off the trailing \n */
}


void free_help(void)
{
  int hp;

  if (!help_table)
     return;

  for (hp = 0; hp <= top_of_helpt; hp++) {
    if (help_table[hp].keyword)
      free(help_table[hp].keyword);
    if (help_table[hp].entry && !help_table[hp].duplicate)
      free(help_table[hp].entry);
  }

  free(help_table);
  help_table = NULL;
  top_of_helpt = 0;
}


void load_help(FILE *fl)
{
#if defined(CIRCLE_MACINTOSH)
  static char key[READ_SIZE + 1], next_key[READ_SIZE + 1], entry[32384]; /* too big for stack? */
#else
  char key[READ_SIZE + 1], next_key[READ_SIZE + 1], entry[32384];
#endif
  size_t entrylen;
  char line[READ_SIZE + 1], *scan;
  struct help_index_element el;

  /* get the first keyword line */
  get_one_line(fl, key);
  while (*key != '$') {
    strcat(key, "\r\n");	/* strcat: OK (READ_SIZE - "\n" + "\r\n" == READ_SIZE + 1) */
    entrylen = strlcpy(entry, key, sizeof(entry));

    /* read in the corresponding help entry */
    get_one_line(fl, line);
    while (*line != '#' && entrylen < sizeof(entry) - 1) {
      entrylen += strlcpy(entry + entrylen, line, sizeof(entry) - entrylen);

      if (entrylen + 2 < sizeof(entry) - 1) {
        strcpy(entry + entrylen, "\r\n");	/* strcpy: OK (size checked above) */
        entrylen += 2;
      }
      get_one_line(fl, line);
    }

    if (entrylen >= sizeof(entry) - 1) {
      int keysize;
      const char *truncmsg = "\r\n*TRUNCATED*\r\n";

      strcpy(entry + sizeof(entry) - strlen(truncmsg) - 1, truncmsg);	/* strcpy: OK (assuming sane 'entry' size) */

      keysize = strlen(key) - 2;
      log("SYSERR: Help entry exceeded buffer space: %.*s", keysize, key);

      /* If we ran out of buffer space, eat the rest of the entry. */
      while (*line != '#')
        get_one_line(fl, line);
    }

    /* now, add the entry to the index with each keyword on the keyword line */
    el.duplicate = 0;
    el.entry = strdup(entry);
    scan = one_word(key, next_key);
    while (*next_key) {
      el.keyword = strdup(next_key);
      help_table[top_of_helpt++] = el;
      el.duplicate++;
      scan = one_word(scan, next_key);
    }

    /* get next keyword line (or $) */
    get_one_line(fl, key);
  }
}


int hsort(const void *a, const void *b)
{
  const struct help_index_element *a1, *b1;

  a1 = (const struct help_index_element *) a;
  b1 = (const struct help_index_element *) b;

  return (str_cmp(a1->keyword, b1->keyword));
}


/*************************************************************************
*  procedures for resetting, both play-time and boot-time	 	 *
*************************************************************************/


int vnum_mobile(char *searchname, struct char_data *ch)
{
  int nr, found = 0;

  for (nr = 0; nr <= top_of_mobt; nr++)
    if (isname(searchname, mob_proto[nr].player.name))
      send_to_char(ch, "%3d. [%5d] %-40s %s\r\n",
                   ++found, mob_index[nr].vnum, mob_proto[nr].player.short_descr,
                   mob_proto[nr].proto_script ? "[TRIG]" : "" );

  return (found);
}



int vnum_object(char *searchname, struct char_data *ch)
{
  int nr, found = 0;

  for (nr = 0; nr <= top_of_objt; nr++)
    if (isname(searchname, obj_proto[nr].name))
      send_to_char(ch, "%3d. [%5d] %-40s %s\r\n",
                    ++found, obj_index[nr].vnum, obj_proto[nr].short_description,
                    obj_proto[nr].proto_script ? "[TRIG]" : "" );

  return (found);
}


/* create a character, and add it to the char list */
struct char_data *create_char(void)
{
  struct char_data *ch;

  CREATE(ch, struct char_data, 1);
  clear_char(ch);
  ch->next = character_list;
  character_list = ch;
  GET_ID(ch) = max_mob_id++;
  /* find_char helper */
  add_to_lookup_table(GET_ID(ch), (void *)ch);
  
  return (ch);
}


/* create a new mobile from a prototype */
struct char_data *read_mobile(mob_vnum nr, int type) /* and mob_rnum */
{
  mob_rnum i;
  struct char_data *mob;

  if (type == VIRTUAL) {
    if ((i = real_mobile(nr)) == NOBODY) {
      log("WARNING: Mobile vnum %d does not exist in database.", nr);
      return (NULL);
    }
  } else
    i = nr;

  CREATE(mob, struct char_data, 1);
  clear_char(mob);
  *mob = mob_proto[i];
  mob->next = character_list;
  character_list = mob;

  if (!mob->points.max_hit) {
    mob->points.max_hit = dice(mob->points.hit, mob->points.mana) +
      mob->points.move;
  } else
    mob->points.max_hit = rand_number(mob->points.hit, mob->points.mana);

  mob->points.hit = mob->points.max_hit;
  mob->points.mana = mob->points.max_mana;
  mob->points.move = mob->points.max_move;

  mob->player.time.birth = time(0);
  mob->player.time.played = 0;
  mob->player.time.logon = time(0);

  mob_index[i].number++;

  GET_ID(mob) = max_mob_id++;
  /* find_char helper */
  add_to_lookup_table(GET_ID(mob), (void *)mob);

  copy_proto_script(&mob_proto[i], mob, MOB_TRIGGER);
  assign_triggers(mob, MOB_TRIGGER);

  return (mob);
}


struct obj_unique_hash_elem {
  long generation;
  long long unique_id;
  struct obj_data *obj;
  struct obj_unique_hash_elem *next_e;
};

void add_unique_id(struct obj_data *obj)
{
  struct obj_unique_hash_elem *elem;
  int i;
  if (!obj_unique_hash_pools)
    init_obj_unique_hash();
  if (obj->unique_id == -1) {
    if (sizeof(long long) > sizeof(long))
      obj->unique_id = (((long long)circle_random()) << (sizeof(long long) * 4)) +
                       circle_random();
    else
      obj->unique_id = circle_random();
  }
  if (CONFIG_ALL_ITEMS_UNIQUE) {
    if (!IS_SET_AR(GET_OBJ_EXTRA(obj), ITEM_UNIQUE_SAVE))
      SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_UNIQUE_SAVE);
  }
  CREATE(elem, struct obj_unique_hash_elem, 1);
  elem->generation = obj->generation;
  elem->unique_id = obj->unique_id;
  elem->obj = obj;
  i = obj->unique_id % NUM_OBJ_UNIQUE_POOLS;
  elem->next_e = obj_unique_hash_pools[i];
  obj_unique_hash_pools[i] = elem;
}

void remove_unique_id(struct obj_data *obj)
{
  struct obj_unique_hash_elem *elem, **ptr, *tmp;

  ptr = obj_unique_hash_pools + (obj->unique_id % NUM_OBJ_UNIQUE_POOLS);

  if (!(ptr && *ptr))
    return;

  elem = *ptr;

  while (elem) {
    tmp = elem->next_e;
    if (elem->obj == obj) {
      free(elem);
      *ptr = tmp;
    } else {
      ptr = &(elem->next_e);
    } 
    elem = tmp;
  }
}

void log_dupe_objects(struct obj_data *obj1, struct obj_data *obj2)
{
  mudlog(BRF, LVL_GOD, TRUE, "DUPE: Dupe object found: %s [%d] [%ld:%lld]", 
        obj1->short_description ? obj1->short_description : "<No name>",
        GET_OBJ_VNUM(obj1), obj1->generation, obj1->unique_id);
  mudlog(BRF, LVL_GOD, TRUE, "DUPE: First: In room: %d (%s), "
                             "In object: %s, Carried by: %s, Worn by: %s",
        GET_ROOM_VNUM(IN_ROOM(obj1)),
        IN_ROOM(obj1) == NOWHERE ? "Nowhere" : world[IN_ROOM(obj1)].name,
        obj1->in_obj ? obj1->in_obj->short_description : "None",
        obj1->carried_by ? GET_NAME(obj1->carried_by) : "Nobody",
        obj1->worn_by ? GET_NAME(obj1->worn_by) : "Nobody");
  mudlog(BRF, LVL_GOD, TRUE, "DUPE: Newer: In room: %d (%s), "
                             "In object: %s, Carried by: %s, Worn by: %s",
        GET_ROOM_VNUM(IN_ROOM(obj2)),
        IN_ROOM(obj2) == NOWHERE ? "Nowhere" : world[IN_ROOM(obj2)].name,
        obj2->in_obj ? obj2->in_obj->short_description : "None",
        obj2->carried_by ? GET_NAME(obj2->carried_by) : "Nobody",
        obj2->worn_by ? GET_NAME(obj2->worn_by) : "Nobody");
}

void check_unique_id(struct obj_data *obj)
{
  struct obj_unique_hash_elem *elem;
  if (obj->unique_id == -1)
    return;
  elem = obj_unique_hash_pools[obj->unique_id % NUM_OBJ_UNIQUE_POOLS];
  while (elem) {
    if (elem->obj == obj) {
      log("SYSERR: check_unique_id checking for existing object?!");
    }
    if (elem->generation == obj->generation && elem->unique_id == obj->unique_id)
      log_dupe_objects(elem->obj, obj);
    elem = elem->next_e;
  }
}

char *sprintuniques(int low, int high)
{
  int i, count = 0, remain, header;
  struct obj_unique_hash_elem *q;
  char *str, *ptr;
  remain = 40;
  for (i = 0; i < NUM_OBJ_UNIQUE_POOLS; i++) {
    q = obj_unique_hash_pools[i];
    remain += 40;
    while (q) {
      count++;
      remain += 80 + (q->obj->short_description ? strlen(q->obj->short_description) : 20);
      q = q->next_e;
    }
  }
  if (count < 1) {
    return strdup("No objects in unique hash.\r\n");
  }
  CREATE(str, char, remain + 1);
  ptr = str;
  count = snprintf(ptr, remain, "Unique object hashes (vnums %d - %d)\r\n",
                low, high);
  ptr += count;
  remain -= count;
  for (i = 0; i < NUM_OBJ_UNIQUE_POOLS; i++) {
    header = 0;
    q = obj_unique_hash_pools[i];
    while (q) {
      if (GET_OBJ_VNUM(q->obj) >= low && GET_OBJ_VNUM(q->obj) <= high) {
        if (!header) {
          header = 1;
          count = snprintf(ptr, remain, "|-Hash %d\r\n", i);
          ptr += count;
          remain -= count;
        }
        count = snprintf(ptr, remain, "| |- [@g%6d@n] - [@y%10ld:%-19lld@n] - %s\r\n",
                GET_OBJ_VNUM(q->obj), q->generation, q->unique_id,
                q->obj->short_description ? q->obj->short_description : "<Unknown>");
        ptr += count;
        remain -= count;
      }
      q = q->next_e;
    }
  }
  return str;
}


/* create an object, and add it to the object list */
struct obj_data *create_obj(void)
{
  struct obj_data *obj;

  CREATE(obj, struct obj_data, 1);
  clear_object(obj);
  obj->next = object_list;
  object_list = obj;

  GET_ID(obj) = max_obj_id++;
  /* find_obj helper */
  add_to_lookup_table(GET_ID(obj), (void *)obj);

  obj->generation = time(0);
  obj->unique_id = -1;

  assign_triggers(obj, OBJ_TRIGGER);
  /* find_obj helper */
  add_to_lookup_table(GET_ID(obj), (void *)obj);

  return (obj);
}


/* create a new object from a prototype */
struct obj_data *read_object(obj_vnum nr, int type) /* and obj_rnum */
{
  struct obj_data *obj;
  obj_rnum i = type == VIRTUAL ? real_object(nr) : nr;

  if (i == NOTHING || i > top_of_objt) {
    log("Object (%c) %d does not exist in database.", type == VIRTUAL ? 'V' : 'R', nr);
    return (NULL);
  }

  CREATE(obj, struct obj_data, 1);
  clear_object(obj);
  *obj = obj_proto[i];
  obj->next = object_list;
  object_list = obj;

  obj_index[i].number++;

  GET_ID(obj) = max_obj_id++;
  /* find_obj helper */
  add_to_lookup_table(GET_ID(obj), (void *)obj);

  obj->generation = time(0);
  obj->unique_id = -1;

  copy_proto_script(&obj_proto[i], obj, OBJ_TRIGGER);
  assign_triggers(obj, OBJ_TRIGGER);
  return (obj);
}



#define ZO_DEAD  999

/* update zone ages, queue for reset if necessary, and dequeue when possible */
void zone_update(void)
{
  int i;
  struct reset_q_element *update_u, *temp;
  static int timer = 0;

  /* jelson 10/22/92 */
  if (((++timer * PULSE_ZONE) / PASSES_PER_SEC) >= 60) {
    /* one minute has passed */
    /*
     * NOT accurate unless PULSE_ZONE is a multiple of PASSES_PER_SEC or a
     * factor of 60
     */

    timer = 0;

    /* since one minute has passed, increment zone ages */
    for (i = 0; i <= top_of_zone_table; i++) {
      if (zone_table[i].age < zone_table[i].lifespan &&
	  zone_table[i].reset_mode)
	(zone_table[i].age)++;

      if (zone_table[i].age >= zone_table[i].lifespan &&
	  zone_table[i].age < ZO_DEAD && zone_table[i].reset_mode) {
	/* enqueue zone */

	CREATE(update_u, struct reset_q_element, 1);

	update_u->zone_to_reset = i;
	update_u->next = 0;

	if (!reset_q.head)
	  reset_q.head = reset_q.tail = update_u;
	else {
	  reset_q.tail->next = update_u;
	  reset_q.tail = update_u;
	}

	zone_table[i].age = ZO_DEAD;
      }
    }
  }	/* end - one minute has passed */


  /* dequeue zones (if possible) and reset */
  /* this code is executed every 10 seconds (i.e. PULSE_ZONE) */
  for (update_u = reset_q.head; update_u; update_u = update_u->next)
    if (zone_table[update_u->zone_to_reset].reset_mode == 2 ||
	is_empty(update_u->zone_to_reset)) {
      reset_zone(update_u->zone_to_reset);
      mudlog(CMP, LVL_GOD, FALSE, "Auto zone reset: %s (Zone %d)", 
          zone_table[update_u->zone_to_reset].name, zone_table[update_u->zone_to_reset].number);
      /* dequeue */
      if (update_u == reset_q.head)
	reset_q.head = reset_q.head->next;
      else {
	for (temp = reset_q.head; temp->next != update_u;
	     temp = temp->next);

	if (!update_u->next)
	  reset_q.tail = temp;

	temp->next = update_u->next;
      }

      free(update_u);
      break;
    }
}

void log_zone_error(zone_rnum zone, int cmd_no, const char *message)
{
  mudlog(NRM, LVL_GOD, TRUE, "SYSERR: zone file: %s", message);
  mudlog(NRM, LVL_GOD, TRUE, "SYSERR: ...offending cmd: '%c' cmd in zone #%d, line %d",
	ZCMD.command, zone_table[zone].number, ZCMD.line);
}

#define ZONE_ERROR(message) \
	{ log_zone_error(zone, cmd_no, message); last_cmd = 0; }

/* execute the reset command table of a given zone */
void reset_zone(zone_rnum zone)
{
  int cmd_no, last_cmd = 0;
  struct char_data *mob = NULL;
  struct obj_data *obj, *obj_to;
  room_vnum rvnum;
  room_rnum rrnum;
  struct char_data *tmob=NULL; /* for trigger assignment */
  struct obj_data *tobj=NULL;  /* for trigger assignment */
  int mob_load = FALSE; /* ### */
  int obj_load = FALSE; /* ### */


  for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++) {

    if (ZCMD.if_flag && !last_cmd && !mob_load && !obj_load)
      continue;

     if (!ZCMD.if_flag) { /* ### */
       mob_load = FALSE;
       obj_load = FALSE;
     }

    /*  This is the list of actual zone commands.  If any new
     *  zone commands are added to the game, be certain to update
     *  the list of commands in load_zone() so that the counting
     *  will still be correct. - ae.
     */
    switch (ZCMD.command) {
    case '*':			/* ignore command */
      last_cmd = 0;
      break;

    case 'M':			/* read a mobile */
      if ((mob_index[ZCMD.arg1].number < ZCMD.arg2) &&
           (rand_number(1, 100) >= ZCMD.arg4)) {
	mob = read_mobile(ZCMD.arg1, REAL);
	char_to_room(mob, ZCMD.arg3);
        load_mtrigger(mob);
        tmob = mob;
	last_cmd = 1;
        mob_load = TRUE;
      } else
	last_cmd = 0;
        tobj = NULL;
        break;

    case 'O':			/* read an object */
       if ((obj_index[ZCMD.arg1].number < ZCMD.arg2) &&
           (rand_number(1, 100) >= ZCMD.arg4)) {
	if (ZCMD.arg3 != NOWHERE) {
	  obj = read_object(ZCMD.arg1, REAL);
          add_unique_id(obj);
	  obj_to_room(obj, ZCMD.arg3);
	  last_cmd = 1;
          load_otrigger(obj);
          tobj = obj;
	  obj_load = TRUE;
	} else {
	  obj = read_object(ZCMD.arg1, REAL);
          add_unique_id(obj);
	  IN_ROOM(obj) = NOWHERE;
	  last_cmd = 1;
          tobj = obj;
	  obj_load = TRUE;
	}
      } else
	last_cmd = 0;
	tmob = NULL;
      break;

    case 'P':			/* object to object */
       if ((obj_index[ZCMD.arg1].number < ZCMD.arg2) &&
           obj_load && (rand_number(1, 100) >= ZCMD.arg4)) {
	obj = read_object(ZCMD.arg1, REAL);
	if (!(obj_to = get_obj_num(ZCMD.arg3))) {
	  ZONE_ERROR("target obj not found, command disabled");
	  ZCMD.command = '*';
	  break;
	}
        add_unique_id(obj);
	obj_to_obj(obj, obj_to);
	last_cmd = 1;
        load_otrigger(obj);
        tobj = obj;
      } else
	last_cmd = 0;
        tmob = NULL;
      break;

    case 'G':			/* obj_to_char */
      if (!mob) {
	ZONE_ERROR("attempt to give obj to non-existant mob, command disabled");
	ZCMD.command = '*';
	break;
      }
      if ((obj_index[ZCMD.arg1].number < ZCMD.arg2) &&
          mob_load && (rand_number(1, 100) >= ZCMD.arg4)) {
	obj = read_object(ZCMD.arg1, REAL);
        add_unique_id(obj);
	obj_to_char(obj, mob);
	last_cmd = 1;
        load_otrigger(obj);
        tobj = obj;
      } else
	last_cmd = 0;
        tmob = NULL;
      break;

    case 'E':			/* object to equipment list */
      if (!mob) {
	ZONE_ERROR("trying to equip non-existant mob, command disabled");
	ZCMD.command = '*';
	break;
      }
      if ((obj_index[ZCMD.arg1].number < ZCMD.arg2) &&
          mob_load && (rand_number(1, 100) >= ZCMD.arg4)) {
	if (ZCMD.arg3 < 0 || ZCMD.arg3 >= NUM_WEARS) {
	  ZONE_ERROR("invalid equipment pos number");
	} else {
	  obj = read_object(ZCMD.arg1, REAL);
          add_unique_id(obj);
          IN_ROOM(obj) = IN_ROOM(mob);
          load_otrigger(obj);
          if (wear_otrigger(obj, mob, ZCMD.arg3)) {
            IN_ROOM(obj) = NOWHERE;
	    equip_char(mob, obj, ZCMD.arg3);
          } else
            obj_to_char(obj, mob);
            tobj = obj;
	    last_cmd = 1;
	  }
      } else
	last_cmd = 0;
        tmob = NULL;
      break;

    case 'R': /* rem obj from room */
      if ((obj = get_obj_in_list_num(ZCMD.arg2, world[ZCMD.arg1].contents)) != NULL)
        extract_obj(obj);
        last_cmd = 1;
        tmob = NULL;
        tobj = NULL;
      break;


    case 'D':			/* set state of door */
      if (ZCMD.arg2 < 0 || ZCMD.arg2 >= NUM_OF_DIRS ||
	  (world[ZCMD.arg1].dir_option[ZCMD.arg2] == NULL)) {
	ZONE_ERROR("door does not exist, command disabled");
	ZCMD.command = '*';
      } else
	switch (ZCMD.arg3) {
	case 0:
	  REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
		     EX_LOCKED);
	  REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
		     EX_CLOSED);
	  break;
	case 1:
	  SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
		  EX_CLOSED);
	  REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
		     EX_LOCKED);
	  break;
	case 2:
	  SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
		  EX_LOCKED);
	  SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
		  EX_CLOSED);
	  break;
	}
      last_cmd = 1;
      tmob = NULL;
      tobj = NULL;
      break;

    case 'T': /* trigger command */
      if (ZCMD.arg1==MOB_TRIGGER && tmob) {
        if (!SCRIPT(tmob))
          CREATE(SCRIPT(tmob), struct script_data, 1);
        add_trigger(SCRIPT(tmob), read_trigger(ZCMD.arg2), -1);
        last_cmd = 1;
      } else if (ZCMD.arg1==OBJ_TRIGGER && tobj) {
        if (!SCRIPT(tobj))
          CREATE(SCRIPT(tobj), struct script_data, 1);
        add_trigger(SCRIPT(tobj), read_trigger(ZCMD.arg2), -1);
        last_cmd = 1;
      } else if (ZCMD.arg1==WLD_TRIGGER) {
        if (ZCMD.arg3 == NOWHERE || ZCMD.arg3>top_of_world) {
          ZONE_ERROR("Invalid room number in trigger assignment");
        }
        if (!world[ZCMD.arg3].script)
          CREATE(world[ZCMD.arg3].script, struct script_data, 1);
        add_trigger(world[ZCMD.arg3].script, read_trigger(ZCMD.arg2), -1);
        last_cmd = 1;
      }

      break;

    case 'V':
      if (ZCMD.arg1==MOB_TRIGGER && tmob) {
        if (!SCRIPT(tmob)) {
          ZONE_ERROR("Attempt to give variable to scriptless mobile");
        } else
          add_var(&(SCRIPT(tmob)->global_vars), ZCMD.sarg1, ZCMD.sarg2,
                  ZCMD.arg3);
        last_cmd = 1;
      } else if (ZCMD.arg1==OBJ_TRIGGER && tobj) {
        if (!SCRIPT(tobj)) {
          ZONE_ERROR("Attempt to give variable to scriptless object");
        } else
          add_var(&(SCRIPT(tobj)->global_vars), ZCMD.sarg1, ZCMD.sarg2,
                  ZCMD.arg3);
        last_cmd = 1;
      } else if (ZCMD.arg1==WLD_TRIGGER) {
        if (ZCMD.arg3 == NOWHERE || ZCMD.arg3>top_of_world) {
          ZONE_ERROR("Invalid room number in variable assignment");
        } else {
          if (!(world[ZCMD.arg3].script)) {
            ZONE_ERROR("Attempt to give variable to scriptless object");
          } else
            add_var(&(world[ZCMD.arg3].script->global_vars),
                    ZCMD.sarg1, ZCMD.sarg2, ZCMD.arg2);
          last_cmd = 1;
        }
      }
      break;

    default:
      ZONE_ERROR("unknown cmd in reset table; cmd disabled");
      ZCMD.command = '*';
      break;
    }
  }

  zone_table[zone].age = 0;

  /* handle reset_wtrigger's */
  rvnum = zone_table[zone].bot;
  while (rvnum <= zone_table[zone].top) {
    rrnum = real_room(rvnum);
    if (rrnum != NOWHERE) reset_wtrigger(&world[rrnum]);
    rvnum++;
  }
}



/* for use in reset_zone; return TRUE if zone 'nr' is free of PC's  */
int is_empty(zone_rnum zone_nr)
{
  struct descriptor_data *i;

  for (i = descriptor_list; i; i = i->next) {
    if (STATE(i) != CON_PLAYING)
      continue;
    if (IN_ROOM(i->character) == NOWHERE)
      continue;
    if (world[IN_ROOM(i->character)].zone != zone_nr)
      continue;
    /*
     * if an immortal has nohassle off, he counts as present 
     * added for testing zone reset triggers - Welcor 
     */
    if ((GET_LEVEL(i->character) >= LVL_IMMORT) && (PRF_FLAGGED(i->character, PRF_NOHASSLE)))
      continue;

    return (0);
  }

  return (1);
}


/************************************************************************
*  funcs of a (more or less) general utility nature			*
************************************************************************/


/* read and allocate space for a '~'-terminated string from a given file */
char *fread_string(FILE *fl, const char *error)
{
  char buf[MAX_STRING_LENGTH], tmp[513];
  char *point;
  int done = 0, length = 0, templength;

  *buf = '\0';

  do {
    if (!fgets(tmp, 512, fl)) {
      log("SYSERR: fread_string: format error at or near %s", error);
      exit(1);
    }
    /* If there is a '~', end the string; else put an "\r\n" over the '\n'. */
    /* now only removes trailing ~'s -- Welcor */
    point = strchr(tmp, '\0');
    for (point-- ; (*point=='\r' || *point=='\n'); point--);
    if (*point=='~') {
      *point='\0';
      done = 1;
    } else {
      *(++point) = '\r';
      *(++point) = '\n';
      *(++point) = '\0';
    }

    templength = point - tmp;

    if (length + templength >= MAX_STRING_LENGTH) {
      log("SYSERR: fread_string: string too large (db.c)");
      log("%s", error);
      exit(1);
    } else {
      strcat(buf + length, tmp);	/* strcat: OK (size checked above) */
      length += templength;
    }
  } while (!done);

  /* allocate space for the new string and copy it */
  return (strlen(buf) ? strdup(buf) : NULL);
}

/* Called to free all allocated follow_type structs - Update by Jamie Nelson */
void free_followers(struct follow_type *k)
{
  if (!k)
    return;

  if (k->next)
    free_followers(k->next);

  k->follower = NULL;
    free(k);
}

/* release memory allocated for a char struct */
void free_char(struct char_data *ch)
{
  int i;
  struct alias_data *a;

  if (ch->player_specials != NULL && ch->player_specials != &dummy_mob) {
    while ((a = GET_ALIASES(ch)) != NULL) {
      GET_ALIASES(ch) = (GET_ALIASES(ch))->next;
      free_alias(a);
    }
    if (ch->player_specials->poofin)
      free(ch->player_specials->poofin);
    if (ch->player_specials->poofout)
      free(ch->player_specials->poofout);
    if (ch->player_specials->host)
      free(ch->player_specials->host);
    free(ch->player_specials);
    if (IS_NPC(ch))
      log("SYSERR: Mob %s (#%d) had player_specials allocated!", GET_NAME(ch), GET_MOB_VNUM(ch));
  }
  if (!IS_NPC(ch) || (IS_NPC(ch) && GET_MOB_RNUM(ch) == NOBODY)) {
    /* if this is a player, or a non-prototyped non-player, free all */
    if (GET_NAME(ch))
      free(GET_NAME(ch));
    if (ch->player.title)
      free(ch->player.title);
    if (ch->player.short_descr)
      free(ch->player.short_descr);
    if (ch->player.long_descr)
      free(ch->player.long_descr);
    if (ch->player.description)
      free(ch->player.description);
    
    /* free script proto list */
    free_proto_script(ch, MOB_TRIGGER);
    
  } else if ((i = GET_MOB_RNUM(ch)) != NOBODY) {
    /* otherwise, free strings only if the string is not pointing at proto */
    if (ch->player.name && ch->player.name != mob_proto[i].player.name)
      free(ch->player.name);
    if (ch->player.title && ch->player.title != mob_proto[i].player.title)
      free(ch->player.title);
    if (ch->player.short_descr && ch->player.short_descr != mob_proto[i].player.short_descr)
      free(ch->player.short_descr);
    if (ch->player.long_descr && ch->player.long_descr != mob_proto[i].player.long_descr)
      free(ch->player.long_descr);
    if (ch->player.description && ch->player.description != mob_proto[i].player.description)
      free(ch->player.description);
    /* free script proto list if it's not the prototype */
    if (ch->proto_script && ch->proto_script != mob_proto[i].proto_script)
      free_proto_script(ch, MOB_TRIGGER);
  }
  while (ch->affected)
    affect_remove(ch, ch->affected);

  /* free any assigned scripts */
  if (SCRIPT(ch))
    extract_script(ch, MOB_TRIGGER);

  /* new version of free_followers take the followers pointer as arg */
  free_followers(ch->followers);

  if (ch->desc)
    ch->desc->character = NULL;

  /* find_char helper */
  /*
  * when free_char is called with a blank character struct, ID is set
  * to 0, and has not yet been added to the lookup table.
  */
  if (GET_ID(ch) != 0)
    remove_from_lookup_table(GET_ID(ch));

  free(ch);
}




/* release memory allocated for an obj struct */
void free_obj(struct obj_data *obj)
{
  remove_unique_id(obj);
  if (GET_OBJ_RNUM(obj) == NOWHERE) {
    free_object_strings(obj);
    /* free script proto list */
    free_proto_script(obj, OBJ_TRIGGER);
  } else {
    free_object_strings_proto(obj);
    if (obj->proto_script != obj_proto[GET_OBJ_RNUM(obj)].proto_script)
      free_proto_script(obj, OBJ_TRIGGER);

  }

  /* free any assigned scripts */
  if (SCRIPT(obj))
    extract_script(obj, OBJ_TRIGGER);

  /* find_obj helper */
  remove_from_lookup_table(GET_ID(obj));

  free(obj);
}


/*
 * Steps:
 *   1: Read contents of a text file.
 *   2: Make sure no one is using the pointer in paging.
 *   3: Allocate space.
 *   4: Point 'buf' to it.
 *
 * We don't want to free() the string that someone may be
 * viewing in the pager.  page_string() keeps the internal
 * strdup()'d copy on ->showstr_head and it won't care
 * if we delete the original.  Otherwise, strings are kept
 * on ->showstr_vector but we'll only match if the pointer
 * is to the string we're interested in and not a copy.
 *
 * If someone is reading a global copy we're trying to
 * replace, give everybody using it a different copy so
 * as to avoid special cases.
 */
int file_to_string_alloc(const char *name, char **buf)
{
  int temppage;
  char temp[MAX_STRING_LENGTH];
  struct descriptor_data *in_use;

  for (in_use = descriptor_list; in_use; in_use = in_use->next)
    if (in_use->showstr_vector && *in_use->showstr_vector == *buf)
      return (-1);

  /* Lets not free() what used to be there unless we succeeded. */
  if (file_to_string(name, temp) < 0)
    return (-1);

  for (in_use = descriptor_list; in_use; in_use = in_use->next) {
    if (!in_use->showstr_count || *in_use->showstr_vector != *buf)
      continue;

    /* Let's be nice and leave them at the page they were on. */
    temppage = in_use->showstr_page;
    paginate_string((in_use->showstr_head = strdup(*in_use->showstr_vector)), in_use);
    in_use->showstr_page = temppage;
  }

  if (*buf)
    free(*buf);

  *buf = strdup(temp);
  return (0);
}


/* read contents of a text file, and place in buf */
int file_to_string(const char *name, char *buf)
{
  FILE *fl;
  char tmp[READ_SIZE + 3];
  int len;

  *buf = '\0';

  if (!(fl = fopen(name, "r"))) {
    log("SYSERR: reading %s: %s", name, strerror(errno));
    return (-1);
  }

  for (;;) {
    if (!fgets(tmp, READ_SIZE, fl))	/* EOF check */
      break;
    if ((len = strlen(tmp)) > 0)
      tmp[len - 1] = '\0'; /* take off the trailing \n */
    strcat(tmp, "\r\n");	/* strcat: OK (tmp:READ_SIZE+3) */

    if (strlen(buf) + strlen(tmp) + 1 > MAX_STRING_LENGTH) {
      log("SYSERR: %s: string too big (%d max)", name, MAX_STRING_LENGTH);
      *buf = '\0';
      fclose(fl);
      return (-1);
    }
    strcat(buf, tmp);	/* strcat: OK (size checked above) */
  }

  fclose(fl);

  return (0);
}



/* clear some of the the working variables of a char */
void reset_char(struct char_data *ch)
{
  int i;

  for (i = 0; i < NUM_WEARS; i++)
    GET_EQ(ch, i) = NULL;

  IN_ROOM(ch) = NOWHERE;
  ch->carrying = NULL;
  ch->next = NULL;
  ch->next_fighting = NULL;
  ch->next_in_room = NULL;
  FIGHTING(ch) = NULL;
  ch->char_specials.position = POS_STANDING;
  ch->mob_specials.default_pos = POS_STANDING;
  ch->char_specials.carry_weight = 0;
  ch->char_specials.carry_items = 0;
  ch->player.time.logon = time(0);

  if (GET_HIT(ch) <= 0)
    GET_HIT(ch) = 1;
  if (GET_MOVE(ch) <= 0)
    GET_MOVE(ch) = 1;
  if (GET_MANA(ch) <= 0)
    GET_MANA(ch) = 1;

  GET_LAST_TELL(ch) = NOBODY;
}



/* clear ALL the working variables of a char; do NOT free any space alloc'ed */
void clear_char(struct char_data *ch)
{
  memset((char *) ch, 0, sizeof(struct char_data));

  IN_ROOM(ch) = NOWHERE;
  GET_PFILEPOS(ch) = -1;
  GET_MOB_RNUM(ch) = NOBODY;
  GET_WAS_IN(ch) = NOWHERE;
  GET_POS(ch) = POS_STANDING;
  ch->followers = NULL;
  ch->master = NULL;
  ch->mob_specials.default_pos = POS_STANDING;
  ch->master_id = -1;

  GET_AC(ch) = 100;		/* Basic Armor */
  if (ch->points.max_mana < 100)
    ch->points.max_mana = 100;
}


void clear_object(struct obj_data *obj)
{
  memset((char *) obj, 0, sizeof(struct obj_data));

  obj->item_number = NOTHING;
  IN_ROOM(obj) = NOWHERE;
  obj->worn_on = NOWHERE;
}




/*
 * Called during character creation after picking character class
 * (and then never again for that character).
 */
void init_char(struct char_data *ch)
{
  int i;

  /* create a player_special structure */
  if (ch->player_specials == NULL)
    CREATE(ch->player_specials, struct player_special_data, 1);

  /* *** if this is our first player --- he be God *** */
  if (top_of_p_table == 0) {
    GET_LEVEL(ch) = LVL_IMPL;
    GET_EXP(ch) = 7000000;

    /* The implementor never goes through do_start(). */
    GET_MAX_HIT(ch) = 500;
    GET_MAX_MANA(ch) = 100;
    GET_MAX_MOVE(ch) = 82;
    GET_HIT(ch) = GET_MAX_HIT(ch);
    GET_MANA(ch) = GET_MAX_MANA(ch);
    GET_MOVE(ch) = GET_MAX_MOVE(ch);
  }

  set_title(ch, NULL);
  ch->player.short_descr = NULL;
  ch->player.long_descr = NULL;
  ch->player.description = NULL;

  ch->player.time.birth = time(0);
  ch->player.time.logon = time(0);
  ch->player.time.played = 0;

  GET_HOME(ch) = 1;
  GET_AC(ch) = 100;

  for (i = 0; i < MAX_TONGUE; i++)
    GET_TALK(ch, i) = 0;

  /*
   * make favors for sex -- or in English, we bias the height and weight of the
   * character depending on what gender they've chosen for themselves. While it
   * is possible to have a tall, heavy female it's not as likely as a male.
   *
   * Height is in centimeters. Weight is in pounds.  The only place they're
   * ever printed (in stock code) is SPELL_IDENTIFY.
   */
  if (GET_SEX(ch) == SEX_MALE) {
    GET_WEIGHT(ch) = rand_number(120, 180);
    GET_HEIGHT(ch) = rand_number(160, 200); /* 5'4" - 6'8" */
  } else {
    GET_WEIGHT(ch) = rand_number(100, 160);
    GET_HEIGHT(ch) = rand_number(150, 180); /* 5'0" - 6'0" */
  }

  if ((i = get_ptable_by_name(GET_NAME(ch))) != -1)
    player_table[i].id = GET_IDNUM(ch) = ++top_idnum;
  else
    log("SYSERR: init_char: Character '%s' not found in player table.", GET_NAME(ch));

  for (i = 1; i <= MAX_SKILLS; i++) {
    if (GET_LEVEL(ch) < LVL_IMPL)
      SET_SKILL(ch, i, 0);
    else
      SET_SKILL(ch, i, 100);
  }

  for (i = 0; i < AF_ARRAY_MAX; i++)
    AFF_FLAGS(ch)[i] = 0;

  for (i = 0; i < 5; i++)
    GET_SAVE(ch, i) = 0;

/*  ch->real_abils.intel = 25;
  ch->real_abils.wis = 25;
  ch->real_abils.dex = 25;
  ch->real_abils.str = 25;
  ch->real_abils.str_add = 100;
  ch->real_abils.con = 25;
  ch->real_abils.cha = 25; */

  for (i = 0; i < 3; i++)
    GET_COND(ch, i) = (GET_LEVEL(ch) == LVL_IMPL ? -1 : 24);

  GET_LOADROOM(ch) = NOWHERE;
  SPEAKING(ch) = SKILL_LANG_COMMON;
}

/* returns the real number of the room with given virtual number */
room_rnum real_room(room_vnum vnum)
{
  room_rnum bot, top, mid, i, last_top;

  i = htree_find(room_htree, vnum);

  if (i != NOWHERE && world[i].number == vnum)
    return i;
  else {
    bot = 0;
    top = top_of_world;

    /* perform binary search on world-table */
    for (;;) {
      last_top = top;
      mid = (bot + top) / 2;

      if ((world + mid)->number == vnum) {
        log("room_htree sync fix: %d: %d -> %d", vnum, i, mid);
        htree_add(room_htree, vnum, mid);
        return (mid);
      }
      if (bot >= top)
        return (NOWHERE);
      if ((world + mid)->number > vnum)
        top = mid - 1;
      else
        bot = mid + 1;
 
      if (top > last_top)
        return NOWHERE;
    }
  }
}



/* returns the real number of the monster with given virtual number */
mob_rnum real_mobile(mob_vnum vnum)
{
  mob_rnum bot, top, mid, i, last_top;

  i = htree_find(mob_htree, vnum);

  if (i != NOBODY && mob_index[i].vnum == vnum)
    return i;
  else {
    bot = 0;
    top = top_of_mobt;

    /* perform binary search on mob-table */
    for (;;) {
      last_top = top;
      mid = (bot + top) / 2;

      if ((mob_index + mid)->vnum == vnum) {
        log("mob_htree sync fix: %d: %d -> %d", vnum, i, mid);
        htree_add(mob_htree, vnum, mid);
        return (mid);
      }
      if (bot >= top)
        return (NOBODY);
      if ((mob_index + mid)->vnum > vnum)
        top = mid - 1;
      else
        bot = mid + 1;

      if (top > last_top)
        return NOWHERE;
    }
  }
}


/* returns the real number of the object with given virtual number */
obj_rnum real_object(obj_vnum vnum)
{
  obj_rnum bot, top, mid, i, last_top;

  i = htree_find(obj_htree, vnum);

  if (i != NOWHERE && obj_index[i].vnum == vnum)
    return i;
  else {
    bot = 0;
    top = top_of_objt;

    /* perform binary search on obj-table */
    for (;;) {
      last_top = top;
      mid = (bot + top) / 2;

      if ((obj_index + mid)->vnum == vnum) {
        log("obj_htree sync fix: %d: %d -> %d", vnum, i, mid);
        htree_add(obj_htree, vnum, mid);
        return (mid);
      }
      if (bot >= top)
        return (NOTHING);
      if ((obj_index + mid)->vnum > vnum)
        top = mid - 1;
      else
        bot = mid + 1;

      if (top > last_top)
        return NOWHERE;
    }
  }
}

/* returns the real number of the room with given virtual number */
zone_rnum real_zone(zone_vnum vnum)
{
  zone_rnum bot, top, mid, last_top;

  bot = 0;
  top = top_of_zone_table;

  /* perform binary search on world-table */
  for (;;) {
    last_top = top;
    mid = (bot + top) / 2;

    if ((zone_table + mid)->number == vnum)
      return (mid);
    if (bot >= top)
      return (NOWHERE);
    if ((zone_table + mid)->number > vnum)
      top = mid - 1;
    else
      bot = mid + 1;
 
    if (top > last_top)
      return NOWHERE;
  }
}



/*
 * Extend later to include more checks.
 *
 * TODO: Add checks for unknown bitvectors.
 */
int check_object(struct obj_data *obj)
{
  char objname[MAX_INPUT_LENGTH + 32];
  int error = FALSE, y;

  if (GET_OBJ_WEIGHT(obj) < 0 && (error = TRUE))
    log("SYSERR: Object #%d (%s) has negative weight (%d).",
	GET_OBJ_VNUM(obj), obj->short_description, GET_OBJ_WEIGHT(obj));

  if (GET_OBJ_RENT(obj) < 0 && (error = TRUE))
    log("SYSERR: Object #%d (%s) has negative cost/day (%d).",
	GET_OBJ_VNUM(obj), obj->short_description, GET_OBJ_RENT(obj));

  snprintf(objname, sizeof(objname), "Object #%d (%s)", GET_OBJ_VNUM(obj), obj->short_description);
  for(y = 0; y < TW_ARRAY_MAX; y++) {
    error |= check_bitvector_names(GET_OBJ_WEAR(obj)[y], wear_bits_count, objname, "object wear");
    error |= check_bitvector_names(GET_OBJ_EXTRA(obj)[y], extra_bits_count, objname, "object extra");
    error |= check_bitvector_names(GET_OBJ_AFFECT(obj)[y], affected_bits_count, objname, "object affect");
  }

  switch (GET_OBJ_TYPE(obj)) {
  case ITEM_DRINKCON:
  {
    char onealias[MAX_INPUT_LENGTH], *space = strrchr(obj->name, ' ');

    strlcpy(onealias, space ? space + 1 : obj->name, sizeof(onealias));
    if (search_block(onealias, drinknames, TRUE) < 0 && (error = TRUE))
      log("SYSERR: Object #%d (%s) doesn't have drink type as last alias. (%s)",
		GET_OBJ_VNUM(obj), obj->short_description, obj->name);
  }
  /* Fall through. */
  case ITEM_FOUNTAIN:
    if ((GET_OBJ_VAL(obj,0) > 0) && (GET_OBJ_VAL(obj, 1) > GET_OBJ_VAL(obj, 0) && (error = TRUE)))
      log("SYSERR: Object #%d (%s) contains (%d) more than maximum (%d).",
		GET_OBJ_VNUM(obj), obj->short_description,
		GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 0));
    break;
  case ITEM_SCROLL:
  case ITEM_POTION:
    error |= check_object_level(obj, 0);
    error |= check_object_spell_number(obj, 1);
    error |= check_object_spell_number(obj, 2);
    error |= check_object_spell_number(obj, 3);
    break;
  case ITEM_WAND:
  case ITEM_STAFF:
    error |= check_object_level(obj, 0);
    error |= check_object_spell_number(obj, 3);
    if (GET_OBJ_VAL(obj, 2) > GET_OBJ_VAL(obj, 1) && (error = TRUE))
      log("SYSERR: Object #%d (%s) has more charges (%d) than maximum (%d).",
		GET_OBJ_VNUM(obj), obj->short_description,
		GET_OBJ_VAL(obj, 2), GET_OBJ_VAL(obj, 1));
    break;
 }

  return (error);
}


int check_object_spell_number(struct obj_data *obj, int val)
{
  int error = FALSE;
  const char *spellname;

  if (GET_OBJ_VAL(obj, val) == -1)	/* i.e.: no spell */
    return (error);

  /*
   * Check for negative spells, spells beyond the top define, and any
   * spell which is actually a skill.
   */
  if (GET_OBJ_VAL(obj, val) < 0)
    error = TRUE;
  if (GET_OBJ_VAL(obj, val) > TOP_SPELL_DEFINE)
    error = TRUE;
  if (GET_OBJ_VAL(obj, val) > MAX_SPELLS && GET_OBJ_VAL(obj, val) <= MAX_SKILLS)
    error = TRUE;
  if (error)
    log("SYSERR: Object #%d (%s) has out of range spell #%d.",
	GET_OBJ_VNUM(obj), obj->short_description, GET_OBJ_VAL(obj, val));

  /*
   * This bug has been fixed, but if you don't like the special behavior...
   */
#if 0
  if (GET_OBJ_TYPE(obj) == ITEM_STAFF &&
	HAS_SPELL_ROUTINE(GET_OBJ_VAL(obj, val), MAG_AREAS | MAG_MASSES))
    log("... '%s' (#%d) uses %s spell '%s'.",
	obj->short_description,	GET_OBJ_VNUM(obj),
	HAS_SPELL_ROUTINE(GET_OBJ_VAL(obj, val), MAG_AREAS) ? "area" : "mass",
	skill_name(GET_OBJ_VAL(obj, val)));
#endif

  if (scheck)		/* Spell names don't exist in syntax check mode. */
    return (error);

  /* Now check for unnamed spells. */
  spellname = skill_name(GET_OBJ_VAL(obj, val));

  if ((spellname == unused_spellname || !str_cmp("UNDEFINED", spellname)) && (error = TRUE))
    log("SYSERR: Object #%d (%s) uses '%s' spell #%d.",
		GET_OBJ_VNUM(obj), obj->short_description, spellname,
		GET_OBJ_VAL(obj, val));

  return (error);
}

int check_object_level(struct obj_data *obj, int val)
{
  int error = FALSE;

  if ((GET_OBJ_VAL(obj, val) < 0 || GET_OBJ_VAL(obj, val) > LVL_IMPL) && (error = TRUE))
    log("SYSERR: Object #%d (%s) has out of range level #%d.",
	GET_OBJ_VNUM(obj), obj->short_description, GET_OBJ_VAL(obj, val));

  return (error);
}

int check_bitvector_names(bitvector_t bits, size_t namecount, const char *whatami, const char *whatbits)
{
  unsigned int flagnum;
  bool error = FALSE;

  /* See if any bits are set above the ones we know about. */
  if (bits <= (~(bitvector_t)0 >> (sizeof(bitvector_t) * 8 - namecount)))
    return (FALSE);

  for (flagnum = namecount; flagnum < sizeof(bitvector_t) * 8; flagnum++)
    if ((1 << flagnum) & bits) {
      log("SYSERR: %s has unknown %s flag, bit %d (0 through %d known).", whatami, whatbits, flagnum, namecount - 1);
      error = TRUE;
    }

  return (error);
}

int my_obj_save_to_disk(FILE *fp, struct obj_data *obj, int locate)
{
  int counter2;
  struct extra_descr_data *ex_desc;
  char buf1[MAX_STRING_LENGTH +1];

  if (obj->action_description) {
    strcpy(buf1, obj->action_description);
    strip_string(buf1);
    } else
      *buf1 = 0;

    fprintf(fp,
      "#%d\n"
      "%d %d %d %d %d %d %d %d %d %d %d %d %d\n",
      GET_OBJ_VNUM(obj), locate, GET_OBJ_VAL(obj, 0), GET_OBJ_VAL(obj, 1),
      GET_OBJ_VAL(obj, 2), GET_OBJ_VAL(obj, 3), GET_OBJ_VAL(obj, 4), 
      GET_OBJ_VAL(obj, 5), GET_OBJ_VAL(obj, 6), GET_OBJ_VAL(obj, 7),
      GET_OBJ_EXTRA(obj)[0], GET_OBJ_EXTRA(obj)[1], GET_OBJ_EXTRA(obj)[2], 
      GET_OBJ_EXTRA(obj)[3]);

  if(!(OBJ_FLAGGED(obj,ITEM_UNIQUE_SAVE))) {
    return 1;
  }

  fprintf(fp,
    "XAP\n"
    "%s~\n"
    "%s~\n"
    "%s~\n"
    "%s~\n"
    "%d %d %d %d %d %d %d %d\n", obj->name ? obj->name : "undefined",
    obj->short_description ? obj->short_description : "undefined",
    obj->description ? obj->description : "undefined",
    buf1, GET_OBJ_TYPE(obj), GET_OBJ_WEAR(obj)[0], 
    GET_OBJ_WEAR(obj)[1], GET_OBJ_WEAR(obj)[2], GET_OBJ_WEAR(obj)[3], 
    GET_OBJ_WEIGHT(obj), GET_OBJ_COST(obj), GET_OBJ_RENT(obj));

  fprintf(fp, "G\n%ld\n", obj->generation);
  fprintf(fp, "U\n%lld\n", obj->unique_id);

  /* Do we have affects? */
  for (counter2 = 0; counter2 < MAX_OBJ_AFFECT; counter2++)
    if (obj->affected[counter2].modifier)
      fprintf(fp, "A\n"
        "%d %d\n",
        obj->affected[counter2].location, obj->affected[counter2].modifier);

  /* Do we have extra descriptions? */
    if (obj->ex_description) {        /*. Yep, save them too . */
      for (ex_desc = obj->ex_description; ex_desc; ex_desc = ex_desc->next) {
        /*. Sanity check to prevent nasty protection faults . */
        if (!*ex_desc->keyword || !*ex_desc->description) {
          continue;
        }
        strcpy(buf1, ex_desc->description);
        strip_string(buf1);
        fprintf(fp, "E\n"
          "%s~\n"
          "%s~\n",
          ex_desc->keyword,
          buf1);
      }
    }
    return 1;
 }

 /* This procedure removes the '\r\n' from a string so that it may be
   saved to a file.  Use it only on buffers, not on the orginal
   strings. */

 void strip_string(char *buffer)
 {
   register char *ptr, *str;

   ptr = buffer;
   str = ptr;

   while ((*str = *ptr)) {
     str++;
     ptr++;
     if (*ptr == '\r')
       ptr++;
   }
 }

 int read_xap_objects(FILE *fl,struct char_data *ch) {
   char line[MAX_STRING_LENGTH];
   char buf2[MAX_STRING_LENGTH];
   int t[13],nr,danger,j,k,zwei;
   struct obj_data *temp=NULL;
   struct extra_descr_data *new_descr;
   int num_of_objs=0;

   if(!feof(fl))
     get_line(fl, line);
   while (!feof(fl) && line != NULL) {
        /* first, we get the number. Not too hard. */
     if(*line == '#') {
       num_of_objs++;
       if (sscanf(line, "#%d", &nr) != 1) {
         continue;
       }
       /* we have the number, check it, load obj. */
       if (nr == -1) {  /* then its unique */
         temp = create_obj();
         temp->item_number=NOTHING;
         SET_BIT_AR(GET_OBJ_EXTRA(temp), ITEM_UNIQUE_SAVE);
       } else if ( nr < 0) {
          continue;
       } else  {
         if(nr >= 99999) {
           continue;
         }
         temp=read_object(nr,VIRTUAL);
         if (!temp) {
           continue;
         }
       }
       /* now we read locate. - this is for autoeq, will be 0 elsewise */
       /* only the rest of the vals, and extra flags will be read */
       get_line(fl,line);
       sscanf(line,"%d %d %d %d %d %d %d %d %d %d %d %d %d",t, t + 1, t + 2, t + 3, t + 4,t + 5, t + 6, t + 7, t + 8, t + 9, t + 10, t + 11, t + 12);
       GET_OBJ_VAL(temp,0) = t[1];
       GET_OBJ_VAL(temp,1) = t[2];
       GET_OBJ_VAL(temp,2) = t[3];
       GET_OBJ_VAL(temp,3) = t[4];
       GET_OBJ_VAL(temp,4) = t[5];
       GET_OBJ_VAL(temp,5) = t[6];
       GET_OBJ_VAL(temp,6) = t[7];
       GET_OBJ_VAL(temp,7) = t[8];
       GET_OBJ_EXTRA(temp)[0] = t[9];
       GET_OBJ_EXTRA(temp)[1] = t[10];
       GET_OBJ_EXTRA(temp)[2] = t[11];
       GET_OBJ_EXTRA(temp)[3] = t[12];

       get_line(fl,line);
        /* read line check for xap. */
       if(!strcmp("XAP",line)) {  /* then this is a Xap Obj, requires
                                        special care */
         if ((temp->name = fread_string(fl, buf2)) == NULL) {
           temp->name = strdup("undefined");
         }
         if ((temp->short_description = fread_string(fl, buf2)) == NULL) {
           temp->short_description = strdup("undefined");
         }
         if ((temp->description = fread_string(fl, buf2)) == NULL) {
           temp->description = strdup("undefined");
         }
         if ((temp->action_description = fread_string(fl, buf2)) == NULL) {
           temp->action_description=0;
         }
         if (!get_line(fl, line) ||
            (sscanf(line, "%d %d %d %d %d %d %d %d", t,t+1,t+2,t+3,t+4,t+5,t+6,t+7) != 8)) {
           fprintf(stderr, "Format error in first numeric line (expecting _x_ args)");
           return 0;
         }
         temp->obj_flags.type_flag = t[0];
         temp->obj_flags.wear_flags[0] = t[1];
         temp->obj_flags.wear_flags[1] = t[2];
         temp->obj_flags.wear_flags[2] = t[3];
         temp->obj_flags.wear_flags[3] = t[4];
         temp->obj_flags.weight = t[5];
         temp->obj_flags.cost = t[6];
         temp->obj_flags.cost_per_day = t[7];

        /* buf2 is error codes pretty much */
         strcat(buf2, ", after numeric constants (expecting E/#xxx)");

        /* we're clearing these for good luck */
         for (j = 0; j < MAX_OBJ_AFFECT; j++) {
           temp->affected[j].location = APPLY_NONE;
           temp->affected[j].modifier = 0;
         }
         free_extra_descriptions(temp->ex_description);
         temp->ex_description = NULL;
         for (k=j=zwei=0;!zwei && !feof(fl);) {
           switch (*line) {
             case 'E':
               CREATE(new_descr, struct extra_descr_data, 1);
               new_descr->keyword = fread_string(fl, buf2);
               new_descr->description = fread_string(fl, buf2);
               new_descr->next = temp->ex_description;
               temp->ex_description = new_descr;
               get_line(fl,line);
               break;
            case 'A':
               if (j >= MAX_OBJ_AFFECT) {
                 log("Too many object affectations in loading rent file");
                 danger=1;
               }
               get_line(fl, line);
               sscanf(line, "%d %d", t, t + 1);

               temp->affected[j].location = t[0];
               temp->affected[j].modifier = t[1];
               j++;
               get_line(fl,line);
               break;
             case 'G':
               get_line(fl, line);
               sscanf(line, "%ld", &temp->generation);
               get_line(fl, line);
               break;
             case 'U':
               get_line(fl, line);
               sscanf(line, "%lld", &temp->unique_id);
               get_line(fl, line);
               break;
             case '$':
             case '#':
               zwei=1;
               break;
             default:
               zwei=1;
               break;
           }
         }      /* exit our for loop */
         get_line(fl,line);
       }   /* exit our xap loop */
       IN_ROOM(ch)=1;
       if(temp != NULL) {
         obj_to_char(temp,ch);
         check_unique_id(temp);
         add_unique_id(temp);
       }
     }
   }  /* exit our while loop */
   return num_of_objs;
 }

/* External variables from config.c */
extern int pk_allowed;
extern int pt_allowed;
extern int level_can_shout;
extern int holler_move_cost;
extern int tunnel_size;
extern int max_exp_gain;
extern int max_exp_loss;
extern int max_npc_corpse_time;
extern int max_pc_corpse_time;
extern int idle_void;
extern int idle_rent_time;
extern int idle_max_level;
extern int dts_are_dumps;
extern int load_into_inventory;
extern int track_through_doors;
extern int immort_level_ok;
extern int show_mob_stacking;
extern int show_obj_stacking;
extern int disp_closed_doors;
extern int reroll_status;
extern int enable_compression;
extern int enable_languages;
extern int all_items_unique;
extern float exp_multiplier;
extern int mob_fighting;
extern int free_rent;
extern int max_obj_save;
extern int min_rent_cost;
extern int auto_save;
extern int autosave_time;
extern int crash_file_timeout;
extern int rent_file_timeout;
extern room_vnum mortal_start_room;
extern room_vnum immort_start_room;
extern room_vnum frozen_start_room;
extern room_vnum donation_room_1;
extern room_vnum donation_room_2;
extern room_vnum donation_room_3;
extern ush_int DFLT_PORT;
extern const char *DFLT_IP;
extern const char *DFLT_DIR;
extern const char *LOGNAME;
extern int max_playing;
extern int max_filesize;
extern int max_bad_pws;
extern int siteok_everyone;
extern int nameserver_is_slow;
extern int use_new_socials;
extern int auto_save_olc;
extern const char *MENU;
extern const char *WELC_MESSG;
extern const char *START_MESSG;
extern int use_autowiz;
extern int min_wizlist_lev;
extern const char *OK;
extern const char *NOPERSON;
extern const char *NOEFFECT;

void load_default_config( void )
{
  /****************************************************************************/
  /** This function is called only once, at boot-time.                       **/
  /** - We assume config_info is empty                          -- Welcor    **/
  /****************************************************************************/
  /****************************************************************************/
  /** Game play options.                                                     **/
  /****************************************************************************/
  CONFIG_PK_ALLOWED 	        = pk_allowed;
  CONFIG_PT_ALLOWED             = pt_allowed;
  CONFIG_LEVEL_CAN_SHOUT 	= level_can_shout;
  CONFIG_HOLLER_MOVE_COST 	= holler_move_cost;
  CONFIG_TUNNEL_SIZE 	        = tunnel_size;
  CONFIG_MAX_EXP_GAIN	        = max_exp_gain;
  CONFIG_MAX_EXP_LOSS 	        = max_exp_loss;
  CONFIG_MAX_NPC_CORPSE_TIME    = max_npc_corpse_time;
  CONFIG_MAX_PC_CORPSE_TIME	= max_pc_corpse_time;
  CONFIG_IDLE_VOID		= idle_void;
  CONFIG_IDLE_RENT_TIME	        = idle_rent_time;
  CONFIG_IDLE_MAX_LEVEL	        = idle_max_level;
  CONFIG_DTS_ARE_DUMPS	        = dts_are_dumps;
  CONFIG_LOAD_INVENTORY         = load_into_inventory;
  CONFIG_OK			= strdup(OK);
  CONFIG_NOPERSON		= strdup(NOPERSON);
  CONFIG_NOEFFECT		= strdup(NOEFFECT);
  CONFIG_TRACK_T_DOORS          = track_through_doors;
  CONFIG_IMMORT_LEVEL_OK	= immort_level_ok;
  CONFIG_STACK_MOBS		= show_mob_stacking;
  CONFIG_STACK_OBJS		= show_obj_stacking;
  CONFIG_MOB_FIGHTING           = mob_fighting;  
  CONFIG_DISP_CLOSED_DOORS      = disp_closed_doors;
  CONFIG_REROLL_PLAYER_CREATION       = reroll_status;
  CONFIG_ENABLE_COMPRESSION	= enable_compression;
  CONFIG_ENABLE_LANGUAGES	= enable_languages;
  CONFIG_ALL_ITEMS_UNIQUE	= all_items_unique;
  CONFIG_EXP_MULTIPLIER		= exp_multiplier;
  /****************************************************************************/
  /** Rent / crashsave options.                                              **/
  /****************************************************************************/
  CONFIG_FREE_RENT              = free_rent;
  CONFIG_MAX_OBJ_SAVE           = max_obj_save;
  CONFIG_MIN_RENT_COST	        = min_rent_cost;
  CONFIG_AUTO_SAVE		= auto_save;
  CONFIG_AUTOSAVE_TIME	        = autosave_time;
  CONFIG_CRASH_TIMEOUT          = crash_file_timeout;
  CONFIG_RENT_TIMEOUT	        = rent_file_timeout;
  
  /****************************************************************************/
  /** Room numbers.                                                          **/
  /****************************************************************************/
  CONFIG_MORTAL_START           = mortal_start_room;
  CONFIG_IMMORTAL_START         = immort_start_room;
  CONFIG_FROZEN_START           = frozen_start_room;
  CONFIG_DON_ROOM_1             = donation_room_1;
  CONFIG_DON_ROOM_2             = donation_room_2;
  CONFIG_DON_ROOM_3             = donation_room_3;
  
  /****************************************************************************/
  /** Game operation options.                                                **/
  /****************************************************************************/
  CONFIG_DFLT_PORT              = DFLT_PORT;
  
  if (DFLT_IP)
    CONFIG_DFLT_IP              = strdup(DFLT_IP);
  else
    CONFIG_DFLT_IP              = NULL;
  
  CONFIG_DFLT_DIR               = strdup(DFLT_DIR);
  
  if (LOGNAME)
    CONFIG_LOGNAME              = strdup(LOGNAME);
  else
    CONFIG_LOGNAME              = NULL;
  
  CONFIG_MAX_PLAYING            = max_playing;
  CONFIG_MAX_FILESIZE           = max_filesize;
  CONFIG_MAX_BAD_PWS            = max_bad_pws;
  CONFIG_SITEOK_ALL             = siteok_everyone;
  CONFIG_NS_IS_SLOW             = nameserver_is_slow;
  CONFIG_NEW_SOCIALS            = use_new_socials;
  CONFIG_OLC_SAVE               = auto_save_olc;
  CONFIG_MENU                   = strdup(MENU);
  CONFIG_WELC_MESSG             = strdup(WELC_MESSG);
  CONFIG_START_MESSG            = strdup(START_MESSG);
  
  /****************************************************************************/
  /** Autowiz options.                                                       **/
  /****************************************************************************/
  CONFIG_USE_AUTOWIZ            = use_autowiz;
  CONFIG_MIN_WIZLIST_LEV        = min_wizlist_lev;
}

void load_config( void )
{
  FILE *fl;
  char line[MAX_STRING_LENGTH];
  char tag[MAX_INPUT_LENGTH];
  int  num;
  float  fum;
  char buf[MAX_INPUT_LENGTH];

  load_default_config();
  
  snprintf(buf, sizeof(buf), "%s/%s", DFLT_DIR, CONFIG_CONFFILE);
  if ( !(fl = fopen(CONFIG_CONFFILE, "r")) && !(fl = fopen(buf, "r")) ) {
    snprintf(buf, sizeof(buf), "Game Config File: %s", CONFIG_CONFFILE);
    perror(buf);
    return;
  }
  
  /****************************************************************************/
  /** Load the game configuration file.                                      **/
  /****************************************************************************/
  while (get_line(fl, line)) {
    split_argument(line, tag);
    num = atoi(line);
    fum = atof(line);
    
    switch (LOWER(*tag)) {
      case 'a':
        if (!str_cmp(tag, "auto_save"))
          CONFIG_AUTO_SAVE = num;
        else if (!str_cmp(tag, "autosave_time"))
          CONFIG_AUTOSAVE_TIME = num;
        else if (!str_cmp(tag, "auto_save_olc"))
          CONFIG_OLC_SAVE = num;
        else if (!str_cmp(tag, "all_items_unique"))
          CONFIG_ALL_ITEMS_UNIQUE = num;
        break;
        
      case 'c':
        if (!str_cmp(tag, "crash_file_timeout"))
          CONFIG_CRASH_TIMEOUT = num;
	else if (!str_cmp(tag, "compression")) {
	  CONFIG_ENABLE_COMPRESSION = num;
#ifndef HAVE_ZLIB_H
	  if (CONFIG_ENABLE_COMPRESSION) {
            CONFIG_ENABLE_COMPRESSION = 0;
            log("config tried to enable compression but it is not supported on this system");
          }
#endif /* !HAVE_ZLIB_H */
        }
        break;
        
      case 'd':
        if (!str_cmp(tag, "disp_closed_doors"))
          CONFIG_DISP_CLOSED_DOORS = num;
        else if (!str_cmp(tag, "dts_are_dumps"))
          CONFIG_DTS_ARE_DUMPS = num;
        else if (!str_cmp(tag, "donation_room_1"))
          if (num == -1)
            CONFIG_DON_ROOM_1 = NOWHERE;
          else
            CONFIG_DON_ROOM_1 = num;
        else if (!str_cmp(tag, "donation_room_2"))
          if (num == -1)
            CONFIG_DON_ROOM_2 = NOWHERE;
          else
            CONFIG_DON_ROOM_2 = num;
        else if (!str_cmp(tag, "donation_room_3"))
          if (num == -1)
            CONFIG_DON_ROOM_3 = NOWHERE;
          else
            CONFIG_DON_ROOM_3 = num;
        else if (!str_cmp(tag, "dflt_dir")) {
          if (CONFIG_DFLT_DIR)
            free(CONFIG_DFLT_DIR);
          if (line && *line)
            CONFIG_DFLT_DIR = strdup(line);
          else
            CONFIG_DFLT_DIR = strdup(DFLT_DIR);
        } else if (!str_cmp(tag, "dflt_ip")) {
          if (CONFIG_DFLT_IP)
            free(CONFIG_DFLT_IP);
          if (line && *line)
            CONFIG_DFLT_IP = strdup(line);
          else
            CONFIG_DFLT_IP = NULL;
        } else if (!str_cmp(tag, "dflt_port"))
          CONFIG_DFLT_PORT = num;
        break;
        
      case 'e':
        if (!str_cmp(tag, "enable_languages"))
          CONFIG_ENABLE_LANGUAGES = num;
        else if (!str_cmp(tag, "exp_multiplier"))
          CONFIG_EXP_MULTIPLIER = fum;
        break;

      case 'f':
        if (!str_cmp(tag, "free_rent"))
          CONFIG_FREE_RENT = num;
        else if (!str_cmp(tag, "frozen_start_room"))
          CONFIG_FROZEN_START = num;
        break;
        
      case 'h':
        if (!str_cmp(tag, "holler_move_cost"))
          CONFIG_HOLLER_MOVE_COST = num;
        break;
        
      case 'i':
        if (!str_cmp(tag, "idle_void"))
          CONFIG_IDLE_VOID = num;
        else if (!str_cmp(tag, "idle_rent_time"))
          CONFIG_IDLE_RENT_TIME = num;
        else if (!str_cmp(tag, "idle_max_level"))
          CONFIG_IDLE_MAX_LEVEL = num;
        else if (!str_cmp(tag, "immort_level_ok"))
          CONFIG_IMMORT_LEVEL_OK = num;
        else if (!str_cmp(tag, "immort_start_room"))
          CONFIG_IMMORTAL_START = num;
        break;
        
      case 'l':
        if (!str_cmp(tag, "level_can_shout"))
          CONFIG_LEVEL_CAN_SHOUT = num;
        else if (!str_cmp(tag, "load_into_inventory"))
          CONFIG_LOAD_INVENTORY = num;
        else if (!str_cmp(tag, "logname")) {
          if (CONFIG_LOGNAME)
            free(CONFIG_LOGNAME);
          if (line && *line)
            CONFIG_LOGNAME = strdup(line);
          else
            CONFIG_LOGNAME = NULL;
        }
        break;
        
      case 'm':
        if (!str_cmp(tag, "max_bad_pws"))
          CONFIG_MAX_BAD_PWS = num;
        else if (!str_cmp(tag, "max_exp_gain"))
          CONFIG_MAX_EXP_GAIN = num;
        else if (!str_cmp(tag, "max_exp_loss"))
          CONFIG_MAX_EXP_LOSS = num;
        else if (!str_cmp(tag, "max_filesize"))
          CONFIG_MAX_FILESIZE = num;
        else if (!str_cmp(tag, "max_npc_corpse_time"))
          CONFIG_MAX_NPC_CORPSE_TIME = num;
        else if (!str_cmp(tag, "max_obj_save"))
          CONFIG_MAX_OBJ_SAVE = num;
        else if (!str_cmp(tag, "max_pc_corpse_time"))
          CONFIG_MAX_PC_CORPSE_TIME = num;
        else if (!str_cmp(tag, "max_playing"))
          CONFIG_MAX_PLAYING = num;
        else if (!str_cmp(tag, "menu")) {
          if (CONFIG_MENU)
            free(CONFIG_MENU);
          strncpy(buf, "Reading menu in load_config()", sizeof(buf));
          CONFIG_MENU = fread_string(fl, buf);
        } else if (!str_cmp(tag, "min_rent_cost"))
          CONFIG_MIN_RENT_COST = num;
        else if (!str_cmp(tag, "min_wizlist_lev"))
          CONFIG_MIN_WIZLIST_LEV = num;
        else if (!str_cmp(tag, "mob_fighting"))
          CONFIG_MOB_FIGHTING = num;
        else if (!str_cmp(tag, "mortal_start_room"))
          CONFIG_MORTAL_START = num;
        break;
        
      case 'n':
        if (!str_cmp(tag, "nameserver_is_slow"))
          CONFIG_NS_IS_SLOW = num;
        else if (!str_cmp(tag, "noperson")) {
          char tmp[READ_SIZE];
          if (CONFIG_NOPERSON)
            free(CONFIG_NOPERSON);
          snprintf(tmp, sizeof(tmp), "%s\r\n", line);
          CONFIG_NOPERSON = strdup(tmp);
        } else if (!str_cmp(tag, "noeffect")) {
          char tmp[READ_SIZE];
          if (CONFIG_NOEFFECT)
            free(CONFIG_NOEFFECT);
          snprintf(tmp, sizeof(tmp), "%s\r\n", line);
          CONFIG_NOEFFECT = strdup(tmp);
        }
        break;
      
      case 'o':
        if (!str_cmp(tag, "ok")) {
          char tmp[READ_SIZE];
          if (CONFIG_OK)
            free(CONFIG_OK);
          snprintf(tmp, sizeof(tmp), "%s\r\n", line);
          CONFIG_OK = strdup(tmp);
        }
        break;
                  
      case 'p':
        if (!str_cmp(tag, "pk_allowed"))
          CONFIG_PK_ALLOWED = num;
        else if (!str_cmp(tag, "pt_allowed"))
          CONFIG_PT_ALLOWED = num;
        break;
        
      case 'r':
        if (!str_cmp(tag, "rent_file_timeout"))
          CONFIG_RENT_TIMEOUT = num;
        else if (!str_cmp(tag, "reroll_stats"))
          CONFIG_REROLL_PLAYER_CREATION = num;
        break;
        
      case 's':
        if (!str_cmp(tag, "siteok_everyone"))
          CONFIG_SITEOK_ALL = num;
        else if (!str_cmp(tag, "start_messg")) {
          strncpy(buf, "Reading start message in load_config()", sizeof(buf));
          if (CONFIG_START_MESSG)
            free(CONFIG_START_MESSG);
          CONFIG_START_MESSG = fread_string(fl, buf);
        }
	else if (!str_cmp(tag, "stack_mobs"))
	  CONFIG_STACK_MOBS = num;
	else if (!str_cmp(tag, "stack_objs"))
	  CONFIG_STACK_OBJS = num;
        break;
        
      case 't':
        if (!str_cmp(tag, "tunnel_size"))
          CONFIG_TUNNEL_SIZE = num;
        else if (!str_cmp(tag, "track_through_doors"))
          CONFIG_TRACK_T_DOORS = num;
        break;
        
      case 'u':
        if (!str_cmp(tag, "use_autowiz"))
          CONFIG_USE_AUTOWIZ = num;
        else if (!str_cmp(tag, "use_new_socials")) 
          CONFIG_NEW_SOCIALS = num;
        break;
        
      case 'w':
        if (!str_cmp(tag, "welc_messg")) {
          strncpy(buf, "Reading welcome message in load_config()", sizeof(buf));
          if (CONFIG_WELC_MESSG)
            free(CONFIG_WELC_MESSG);
          CONFIG_WELC_MESSG = fread_string(fl, buf);
        }
        break;
        
      default:
        break;
    }
  }
  
  fclose(fl);
}
