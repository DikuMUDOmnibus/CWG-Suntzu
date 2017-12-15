/* ************************************************************************
*   File: constants.c                                   Part of CircleMUD *
*  Usage: Numeric and string contants used by the MUD                     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "interpreter.h"	/* alias_data */

cpp_extern const char *circlemud_version =
	"CircleMUD, version 3.1";

cpp_extern const char *oasisolc_version =
	"OasisOLC 2.0.6";

cpp_extern const char *ascii_pfiles_version =
        "ASCII Player Files 3.0.1";

const char *patch_list[] = 
{
  "Oasis OLC 				version 2.0.6	(2003/07/28)",
  "DG Scripts 				version 1.0.13	(2004/02/27)",
  "ASCII Player Files 			version 3.0.1   (2004/06/10)",
  "Races Support 			version 3.1",
  "Xapobjs 				version 1.2",
  "Spoken Language Code 		version 2.1",
  "Copyover 				version 1.2",
  "128bit Support 			version 1.4",
  "Assembly Edit Code 			version 1.1",
  "Whois Command 			version 1.0",
  "Weapon Skill/Skill Progression 	version 1.3",
  "Race/Class Restriction 		version 1.0",
  "Vehicle Code 			version 2.0	(2003/10/01)",
  "Compare Object Code 		version 1.1",
  "Object Damage/Material Types 	version 1.3",
  "Container Patch 			version 1.1",
  "Percentage Zone Loads 		version 1.2",
  "Patch list 				version 1.3	(2003/08/27)",
  "Portal Object/Spell 		version 1.3	(2003/09/13)",
  "Mobile stacking 			version 3.1	(2003/08/30)",
  "Object stacking 			version 3.1	(2003/08/30)",
  "Stacking in cedit 			version 1.1	(2003/08/30)",
  "Fixtake in extra descs 		version 1.1	(2003/06/21)",
  "Exits 				version 3.2	(2003/10/22)",
  "Seelight 				version 1.1	(2003/06/22)",
  "Manual Color 			version 3.1	(2003/04/15)",
  "Command Disable 			version 1.1 	(2003/12/09)",
  "Reroll Player Stats w/ cedit 	version 1.1 	(2004/05/23)",
  "Dynamic Boards 			version 2.4 (2004/04/23)",
  "MCCP2				version cwg1.0	(2004/09/05)",
  "Dupecheck				version 1.0	(2003/12/07)",
  "Auto-Assist				version 1.0	(2004/9/06)",
  "Remove color question		version 1.0	(2004/9/27)",
  "Room currents 			version 1.0	(2004/9/27)",
  "Timed Deathtraps 			version 1.0	(2004/9/27)",
  "\n"
};

/* strings corresponding to ordinals/bitvectors in structs.h ***********/


/* (Note: strings for class definitions in class.c instead of here) */


/* cardinal directions */
const char *dirs[] =
{
  "north",
  "east",
  "south",
  "west",
  "up",
  "down",
  "northwest",
  "northeast",
  "southeast",
  "southwest",
  "inside",
  "outside",
  "\n"
};
const char *abbr_dirs[] = 
{
  "n",
  "e",
  "s",
  "w",
  "u",
  "d",
  "nw",
  "ne",
  "se",
  "sw",
  "in",
  "out",
  "\n"
};

/* ROOM_x */
const char *room_bits[] = {
  "DARK",
  "DEATH",
  "NO_MOB",
  "INDOORS",
  "PEACEFUL",
  "SOUNDPROOF",
  "NO_TRACK",
  "NO_MAGIC",
  "TUNNEL",
  "PRIVATE",
  "GODROOM",
  "HOUSE",
  "HCRSH",
  "ATRIUM",
  "OLC",
  "*",				/* BFS MARK */
  "VEHICLE",
  "UNDERGROUND",
  "CURRENT",
  "TIMED_DT",
  "\n"
};


/* EX_x */
const char *exit_bits[] = {
  "DOOR",
  "CLOSED",
  "LOCKED",
  "PICKPROOF",
  "SECRET",
  "\n"
};


/* SECT_ */
const char *sector_types[] = {
  "Inside",
  "City",
  "Field",
  "Forest",
  "Hills",
  "Mountains",
  "Water (Swim)",
  "Water (No Swim)",
  "In Flight",
  "Underwater",
  "\n"
};


/*
 * SEX_x
 * Not used in sprinttype() so no \n.
 */
const char *genders[] =
{
  "neutral",
  "male",
  "female",
  "\n"
};


/* POS_x */
const char *position_types[] = {
  "Dead",
  "Mortally wounded",
  "Incapacitated",
  "Stunned",
  "Sleeping",
  "Resting",
  "Sitting",
  "Fighting",
  "Standing",
  "\n"
};


/* PLR_x */
const char *player_bits[] = {
  "KILLER",
  "THIEF",
  "FROZEN",
  "DONTSET",
  "WRITING",
  "MAILING",
  "CSH",
  "SITEOK",
  "NOSHOUT",
  "NOTITLE",
  "DELETED",
  "LOADRM",
  "NO_WIZL",
  "NO_DEL",
  "INVST",
  "CRYO",
  "DEAD",    /* You should never see this. */
  "UNUSED1",
  "UNUSED2",
  "UNUSED3",
  "UNUSED4",
  "UNUSED5",
  "\n"
};


/* MOB_x */
const char *action_bits[] = {
  "SPEC",
  "SENTINEL",
  "SCAVENGER",
  "ISNPC",
  "AWARE",
  "AGGR",
  "STAY-ZONE",
  "WIMPY",
  "AGGR_EVIL",
  "AGGR_GOOD",
  "AGGR_NEUTRAL",
  "MEMORY",
  "HELPER",
  "NO_CHARM",
  "NO_SUMMN",
  "NO_SLEEP",
  "NO_BASH",
  "NO_BLIND",
  "NO_KILL",
  "DEAD",    /* You should never see this. */
  "SPARE1",
  "SPARE2",
  "SPARE3",
  "SPARE4",
  "SPARE5",
  "SPARE6",
  "SPARE7",
  "SPARE8",
  "SPARE9",
  "SPARE10",
  "SPARE11",
  "SPARE12",
  "SPARE13",
  "SPARE14",
  "SPARE15",
  "SPARE16",
  "\n"
};


/* PRF_x */
const char *preference_bits[] = {
  "BRIEF",
  "COMPACT",
  "DEAF",
  "NO_TELL",
  "D_HP",
  "D_MANA",
  "D_MOVE",
  "AUTOEX",
  "NO_HASS",
  "QUEST",
  "SUMN",
  "NO_REP",
  "LIGHT",
  "C1",
  "C2",
  "NO_WIZ",
  "L1",
  "L2",
  "NO_AUC",
  "NO_GOS",
  "NO_GTZ",
  "RMFLG",
  "D_AUTO",
  "CLS",
  "BLDWLK",
  "AFK",
  "AUTOLOOT",
  "AUTOGOLD",
  "AUTOSPLIT",
  "FULL_AUTOEX",
  "AUTOSAC",
  "VIEWORDER",
  "NO_COMPRESS",
  "AUTOASSIST",
  "\n"
};


/* AFF_x */
const char *affected_bits[] =
{
  "\0", /* DO NOT REMOVE!! */
  "BLIND",
  "INVIS",
  "DET-ALIGN",
  "DET-INVIS",
  "DET-MAGIC",
  "SENSE-LIFE",
  "WATWALK",
  "SANCT",
  "GROUP",
  "CURSE",
  "INFRA",
  "POISON",
  "PROT-EVIL",
  "PROT-GOOD",
  "SLEEP",
  "NO_TRACK",
  "UNDEAD",
  "PARALYZED",
  "SNEAK",
  "HIDE",
  "UNUSED",
  "CHARM",
  "FLYING",
  "WATERB",
  "ANGELIC",
  "ETHEREAL",
  "MAGICONLY",
  "\n"
};


/* CON_x */
const char *connected_types[] = {
  "Playing",
  "Disconnecting",
  "Get name",
  "Confirm name",
  "Get password",
  "Get new PW",
  "Confirm new PW",
  "Select sex",
  "Select class",
  "Reading MOTD",
  "Main Menu",
  "Get descript.",
  "Changing PW 1",
  "Changing PW 2",
  "Changing PW 3",
  "Self-Delete 1",
  "Self-Delete 2",
  "Disconnecting",
  "Object edit",
  "Room edit",
  "Zone edit",
  "Mobile edit",
  "Shop edit",
  "Text edit",
  "Config edit",
  "Select race",
  "Assembly Edit",
  "Social edit",
  "Trigger Edit",
  "Race help",
  "Class help",
  "Query ANSI",
  "Guild edit",
  "Reroll stats",
  "iObject Edit",
  "\n"
};


/*
 * WEAR_x - for eq list
 * Not use in sprinttype() so no \n.
 */
const char *wear_where[] = {
  "<used as light>      ",
  "<worn on finger>     ",
  "<worn on finger>     ",
  "<worn around neck>   ",
  "<worn around neck>   ",
  "<worn on body>       ",
  "<worn on head>       ",
  "<worn on legs>       ",
  "<worn on feet>       ",
  "<worn on hands>      ",
  "<worn on arms>       ",
  "<worn as shield>     ",
  "<worn about body>    ",
  "<worn about waist>   ",
  "<worn around wrist>  ",
  "<worn around wrist>  ",
  "<wielded>            ",
  "<held>               ",
  "<worn on back>       ",
  "<worn in ear>        ",
  "<worn in ear>        ",
  "<worn as wings>      ",
  "<worn as mask>       ",
  "\n"
};


/* WEAR_x - for stat */
const char *equipment_types[] = {
  "Used as light",
  "Worn on right finger",
  "Worn on left finger",
  "First worn around Neck",
  "Second worn around Neck",
  "Worn on body",
  "Worn on head",
  "Worn on legs",
  "Worn on feet",
  "Worn on hands",
  "Worn on arms",
  "Worn as shield",
  "Worn about body",
  "Worn around waist",
  "Worn around right wrist",
  "Worn around left wrist",
  "Wielded",
  "Held",
  "Worn on back",
  "Worn in ear",
  "Worn in ear",
  "Worn as wings",
  "Worn as mask",
  "\n"
};


/* ITEM_x (ordinal object types) */
const char *item_types[] = {
  "UNDEFINED",
  "LIGHT",
  "SCROLL",
  "WAND",
  "STAFF",
  "WEAPON",
  "FIRE WEAPON",
  "MISSILE",
  "TREASURE",
  "ARMOR",
  "POTION",
  "WORN",
  "OTHER",
  "TRASH",
  "TRAP",
  "CONTAINER",
  "NOTE",
  "LIQ CONTAINER",
  "KEY",
  "FOOD",
  "MONEY",
  "PEN",
  "BOAT",
  "FOUNTAIN",
  "VEHICLE",
  "HATCH",
  "WINDOW",
  "CONTROL",
  "PORTAL",
  "BOARD",
  "\n"
};


/* ITEM_WEAR_ (wear bitvector) */
const char *wear_bits[] = {
  "TAKE",
  "FINGER",
  "NECK",
  "BODY",
  "HEAD",
  "LEGS",
  "FEET",
  "HANDS",
  "ARMS",
  "SHIELD",
  "ABOUT",
  "WAIST",
  "WRIST",
  "WIELD",
  "HOLD",
  "PACK",
  "EAR",
  "WINGS",
  "MASK",
  "\n"
};


/* ITEM_x (extra bits) */
const char *extra_bits[] = {
  "GLOW",
  "HUM",
  "NO_RENT",
  "NO_DONATE",
  "NO_INVIS",
  "INVISIBLE",
  "MAGIC",
  "NO_DROP",
  "BLESS",
  "ANTI_GOOD",
  "ANTI_EVIL",
  "ANTI_NEUTRAL",
  "ANTI_MAGE",
  "ANTI_CLERIC",
  "ANTI_THIEF",
  "ANTI_WARRIOR",
  "NO_SELL",
  "ANTI_DRUID",
  "2H",
  "ANTI_BARD",
  "ANTI_RANGER",
  "ANTI_PALADIN",
  "ANTI_HUMAN",
  "ANTI_DWARF",
  "ANTI_ELF",
  "ANTI_GNOME",
  "UNIQUE",
  "BROKEN",
  "UNBREAKABLE",
  "ANTI_MONK",
  "ANTI_BARBARIAN",
  "ANTI_SORCERER",
  "\n"
};


/* APPLY_x */
const char *apply_types[] = {
  "NONE",
  "STR",
  "DEX",
  "INT",
  "WIS",
  "CON",
  "CHA",
  "CLASS",
  "LEVEL",
  "AGE",
  "CHAR_WEIGHT",
  "CHAR_HEIGHT",
  "MAXMANA",
  "MAXHIT",
  "MAXMOVE",
  "GOLD",
  "EXP",
  "ARMOR",
  "HITROLL",
  "DAMROLL",
  "SAVING_PARA",
  "SAVING_ROD",
  "SAVING_PETRI",
  "SAVING_BREATH",
  "SAVING_SPELL",
  "RACE",
  "TURN_LEVEL",
  "\n"
};


/* CONT_x */
const char *container_bits[] = {
  "CLOSEABLE",
  "PICKPROOF",
  "CLOSED",
  "LOCKED",
  "\n",
};


/* LIQ_x */
const char *drinks[] =
{
  "water",
  "beer",
  "wine",
  "ale",
  "dark ale",
  "whisky",
  "lemonade",
  "firebreather",
  "local speciality",
  "slime mold juice",
  "milk",
  "tea",
  "coffee",
  "blood",
  "salt water",
  "clear water",
  "\n"
};

/* MATERIAL_ */
const char *material_names[] = {
   "bone",
   "ceramic",
   "copper",
   "diamond",
   "gold",
   "iron",
   "leather",
   "mithril",
   "obsidian",
   "steel",
   "stone",
   "silver",
   "wood",
   "glass",
   "organic",
   "currency",
   "paper",
   "cotton",
   "satin",
   "silk",
   "burlap",
   "velvet",
   "platinum",
   "adamantite",
   "wool",
   "onyx",
   "ivory",
   "brass",
   "marble",
   "bronze",
   "pewter",
   "ruby",
   "sapphire",
   "emerald",
   "gemstone",
   "granite",
   "energy",
   "hemp",
   "crystal",
   "earth",
   "\n"
};

/* Constants for Assemblies    *****************************************/
const char *AssemblyTypes[] = {
  "assemble",
  "bake",
  "brew",
  "craft",
  "fletch",
  "knit",
  "make",
  "mix",
  "thatch",
  "weave",
  "forge",
  "\n"
};

/* other constants for liquids ******************************************/


/* one-word alias for each drink */
const char *drinknames[] =
{
  "water",
  "beer",
  "wine",
  "ale",
  "ale",
  "whisky",
  "lemonade",
  "firebreather",
  "local",
  "juice",
  "milk",
  "tea",
  "coffee",
  "blood",
  "salt",
  "water",
  "\n"
};


/* effect of drinks on hunger, thirst, and drunkenness -- see values.doc */
int drink_aff[][3] = {
  {0, 1, 10},
  {3, 2, 5},
  {5, 2, 5},
  {2, 2, 5},
  {1, 2, 5},
  {6, 1, 4},
  {0, 1, 8},
  {10, 0, 0},
  {3, 3, 3},
  {0, 4, -8},
  {0, 3, 6},
  {0, 1, 6},
  {0, 1, 6},
  {0, 2, -1},
  {0, 1, -2},
  {0, 0, 13}
};


/* color of the various drinks */
const char *color_liquid[] =
{
  "clear",
  "brown",
  "clear",
  "brown",
  "dark",
  "golden",
  "red",
  "green",
  "clear",
  "light green",
  "white",
  "brown",
  "black",
  "red",
  "clear",
  "crystal clear",
  "\n"
};


/*
 * level of fullness for drink containers
 * Not used in sprinttype() so no \n.
 */
const char *fullness[] =
{
  "less than half ",
  "about half ",
  "more than half ",
  ""
};


/* str, int, wis, dex, con applies **************************************/


/* [ch] strength apply (all) */
cpp_extern const struct str_app_type str_app[] = {
  {-5, -4, 0, 0},	/* str = 0 */
  {-5, -4, 3, 1},	/* str = 1 */
  {-3, -2, 3, 2},
  {-3, -1, 10, 3},
  {-2, -1, 25, 4},
  {-2, -1, 55, 5},	/* str = 5 */
  {-1, 0, 80, 6},
  {-1, 0, 90, 7},
  {0, 0, 100, 8},
  {0, 0, 100, 9},
  {0, 0, 115, 10},	/* str = 10 */
  {0, 0, 115, 11},
  {0, 0, 140, 14},
  {0, 0, 140, 14},
  {0, 0, 170, 17},
  {0, 0, 170, 17},	/* str = 15 */
  {0, 1, 195, 20},
  {1, 1, 220, 22},
  {1, 2, 255, 25},	/* str = 18 */
  {3, 7, 640, 64},
  {3, 8, 700, 70},	/* str = 20 */
  {4, 9, 810, 81},
  {4, 10, 970, 97},
  {5, 11, 1130, 113},
  {6, 12, 1440, 144},
  {7, 14, 1750, 175},	/* str = 25 */
  {1, 3, 280, 28},	/* str = 18/0 - 18-50 */
  {2, 3, 305, 30},	/* str = 18/51 - 18-75 */
  {2, 4, 330, 33},	/* str = 18/76 - 18-90 */
  {2, 5, 380, 38},	/* str = 18/91 - 18-99 */
  {3, 6, 480, 48}	/* str = 18/100 */
};



/* [dex] skill apply (thieves only) */
cpp_extern const struct dex_skill_type dex_app_skill[] = {
  {-99, -99, -90, -99, -60},	/* dex = 0 */
  {-90, -90, -60, -90, -50},	/* dex = 1 */
  {-80, -80, -40, -80, -45},
  {-70, -70, -30, -70, -40},
  {-60, -60, -30, -60, -35},
  {-50, -50, -20, -50, -30},	/* dex = 5 */
  {-40, -40, -20, -40, -25},
  {-30, -30, -15, -30, -20},
  {-20, -20, -15, -20, -15},
  {-15, -10, -10, -20, -10},
  {-10, -5, -10, -15, -5},	/* dex = 10 */
  {-5, 0, -5, -10, 0},
  {0, 0, 0, -5, 0},
  {0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0},		/* dex = 15 */
  {0, 5, 0, 0, 0},
  {5, 10, 0, 5, 5},
  {10, 15, 5, 10, 10},		/* dex = 18 */
  {15, 20, 10, 15, 15},
  {15, 20, 10, 15, 15},		/* dex = 20 */
  {20, 25, 10, 15, 20},
  {20, 25, 15, 20, 20},
  {25, 25, 15, 20, 20},
  {25, 30, 15, 25, 25},
  {25, 30, 15, 25, 25}		/* dex = 25 */
};

cpp_extern const struct dex_skill_type race_app_skill[NUM_RACES] =
{
        { 0,  0,  0,  0,  0, }, /* HUMAN     */
        { 5, -5,  0,  5, 10, }, /* ELF       */
        { 0,  5, 10,  5,  5, }, /* GNOME     */
        { 0, 10, 15,  0,  0, }, /* DWARF     */
        { 0,  0,  0,  0,  0, }, /* HALF ELF  */
        { 0,  0,  0,  0,  0, }, /* HALFLING  */
        { 0,  0,  0,  0,  0, }, /* DROW ELF  */
        { 0,  0,  0,  0,  0, }, /* HALF ORC  */
        { 0,  0,  0,  0,  0, }, /* ANIMAL    */
        { 0,  0,  0,  0,  0, }, /* CONSTRUCT */
        { 0,  0,  0,  0,  0, }, /* DEMON     */
        { 0,  0,  0,  0,  0, }, /* DRAGON    */
        { 0,  0,  0,  0,  0, }, /* FISH      */
        { 0,  0,  0,  0,  0, }, /* GIANT     */
        { 0,  0,  0,  0,  0, }, /* GOBLIN    */
        { 0,  0,  0,  0,  0, }, /* INSECT    */
        { 0,  0,  0,  0,  0, }, /* ORC       */
        { 0,  0,  0,  0,  0, }, /* SNAKE     */
        { 0,  0,  0,  0,  0, }, /* TROLL     */
        { 0,  0,  0,  0,  0, }, /* MINOTAUR  */
        { 0,  0,  0,  0,  0, }, /* WARHOST   */
        { 0,  0,  0,  0,  0, }  /* FAERIE    */
};

/* [dex] apply (all) */
cpp_extern const struct dex_app_type dex_app[] = {
  {-7, -7, 6},		/* dex = 0 */
  {-6, -6, 5},		/* dex = 1 */
  {-4, -4, 5},
  {-3, -3, 4},
  {-2, -2, 3},
  {-1, -1, 2},		/* dex = 5 */
  {0, 0, 1},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},		/* dex = 10 */
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, -1},		/* dex = 15 */
  {1, 1, -2},
  {2, 2, -3},
  {2, 2, -4},		/* dex = 18 */
  {3, 3, -4},
  {3, 3, -4},		/* dex = 20 */
  {4, 4, -5},
  {4, 4, -5},
  {4, 4, -5},
  {5, 5, -6},
  {5, 5, -6}		/* dex = 25 */
};



/* [con] apply (all) */
cpp_extern const struct con_app_type con_app[] = {
  {-4, 20},		/* con = 0 */
  {-3, 25},		/* con = 1 */
  {-2, 30},
  {-2, 35},
  {-1, 40},
  {-1, 45},		/* con = 5 */
  {-1, 50},
  {0, 55},
  {0, 60},
  {0, 65},
  {0, 70},		/* con = 10 */
  {0, 75},
  {0, 80},
  {0, 85},
  {0, 88},
  {1, 90},		/* con = 15 */
  {2, 95},
  {2, 97},
  {3, 99},		/* con = 18 */
  {3, 99},
  {4, 99},		/* con = 20 */
  {5, 99},
  {5, 99},
  {5, 99},
  {6, 99},
  {6, 99}		/* con = 25 */
};

/* mob trigger types */
const char *trig_types[] = {
  "Global", 
  "Random",
  "Command",
  "Speech",
  "Act",
  "Death",
  "Greet",
  "Greet-All",
  "Entry",
  "Receive",
  "Fight",
  "HitPrcnt",
  "Bribe",
  "Load",
  "Memory",
  "Cast",
  "Leave",
  "Door",
  "UNUSED",
  "Time",
  "\n"
};


/* obj trigger types */
const char *otrig_types[] = {
  "Global",
  "Random",
  "Command",
  "UNUSED",
  "UNUSED",
  "Timer",
  "Get",
  "Drop",
  "Give",
  "Wear",
  "UNUSED",
  "Remove",
  "UNUSED",
  "Load",
  "UNUSED",
  "Cast",
  "Leave",
  "UNUSED",
  "Consume",
  "Time",
  "\n"
};


/* wld trigger types */
const char *wtrig_types[] = {
  "Global",
  "Random",
  "Command",
  "Speech",
  "UNUSED",
  "Zone Reset",
  "Enter",
  "Drop",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "Cast",
  "Leave",
  "Door",
  "UNUSED",
  "Time",
  "\n"
};

#if defined(CONFIG_OASIS_MPROG)
/*
 * Definitions necessary for MobProg support in OasisOLC
 */
const char *mobprog_types[] = {
  "INFILE",
  "ACT",
  "SPEECH",
  "RAND",
  "FIGHT",
  "DEATH",
  "HITPRCNT",
  "ENTRY",
  "GREET",
  "ALL_GREET",
  "GIVE",
  "BRIBE",
  "\n"
};
#endif


/* [int] apply (all) */
cpp_extern const struct int_app_type int_app[] = {
  {3},		/* int = 0 */
  {5},		/* int = 1 */
  {7},
  {8},
  {9},
  {10},		/* int = 5 */
  {11},
  {12},
  {13},
  {15},
  {17},		/* int = 10 */
  {19},
  {22},
  {25},
  {30},
  {35},		/* int = 15 */
  {40},
  {45},
  {50},		/* int = 18 */
  {53},
  {55},		/* int = 20 */
  {56},
  {57},
  {58},
  {59},
  {60}		/* int = 25 */
};


/* [wis] apply (all) */
cpp_extern const struct wis_app_type wis_app[] = {
  {0},	/* wis = 0 */
  {0},  /* wis = 1 */
  {0},
  {0},
  {0},
  {0},  /* wis = 5 */
  {0},
  {0},
  {0},
  {0},
  {0},  /* wis = 10 */
  {0},
  {2},
  {2},
  {3},
  {3},  /* wis = 15 */
  {3},
  {4},
  {5},	/* wis = 18 */
  {6},
  {6},  /* wis = 20 */
  {6},
  {6},
  {7},
  {7},
  {7}  /* wis = 25 */
};


const char *npc_class_types[] = {
  "Normal",
  "Undead",
  "\n"
};


int rev_dir[] =
{
  /* North */ SOUTH,
  /* East  */ WEST,
  /* South */ NORTH,
  /* West  */ EAST,
  /* Up    */ DOWN,
  /* Down  */ UP,
  /* NW    */ SOUTHEAST,
  /* NE    */ SOUTHWEST,
  /* SE    */ NORTHWEST,
  /* SW    */ NORTHEAST,
  /* In    */ OUTDIR,
  /* Out   */ INDIR
};

int movement_loss[] =
{
  1,	/* Inside     */
  1,	/* City       */
  2,	/* Field      */
  3,	/* Forest     */
  4,	/* Hills      */
  6,	/* Mountains  */
  4,	/* Swimming   */
  1,	/* Unswimable */
  1,	/* Flying     */
  5     /* Underwater */
};

/* Not used in sprinttype(). */
const char *weekdays[] = {
  "the Day of the Moon",
  "the Day of the Bull",
  "the Day of the Deception",
  "the Day of Thunder",
  "the Day of Freedom",
  "the Day of the Great Gods",
  "the Day of the Sun"
};


/* Not used in sprinttype(). */
const char *month_name[] = {
  "Month of Winter",		/* 0 */
  "Month of the Winter Wolf",
  "Month of the Frost Giant",
  "Month of the Old Forces",
  "Month of the Grand Struggle",
  "Month of the Spring",
  "Month of Nature",
  "Month of Futility",
  "Month of the Dragon",
  "Month of the Sun",
  "Month of the Heat",
  "Month of the Battle",
  "Month of the Dark Shades",
  "Month of the Shadows",
  "Month of the Long Shadows",
  "Month of the Ancient Darkness",
  "Month of the Great Evil"
};

#if defined(CONFIG_OASIS_MPROG)
/*
 * Definitions necessary for MobProg support in OasisOLC
 */
const char *mobprog_types[] = {
  "INFILE",
  "ACT",
  "SPEECH",
  "RAND",
  "FIGHT",
  "DEATH",
  "HITPRCNT",
  "ENTRY",
  "GREET",
  "ALL_GREET",
  "GIVE",
  "BRIBE",
  "\n"
};
#endif

/* --- End of constants arrays. --- */

/*
 * Various arrays we count so we can check the world files.  These
 * must be at the bottom of the file so they're pre-declared.
 */
size_t	room_bits_count = sizeof(room_bits) / sizeof(room_bits[0]) - 1,
	action_bits_count = sizeof(action_bits) / sizeof(action_bits[0]) - 1,
	affected_bits_count = sizeof(affected_bits) / sizeof(affected_bits[0]) - 1,
	extra_bits_count = sizeof(extra_bits) / sizeof(extra_bits[0]) - 1,
	wear_bits_count = sizeof(wear_bits) / sizeof(wear_bits[0]) - 1;

