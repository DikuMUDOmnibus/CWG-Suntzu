/**************************************************************************
*  File: scripts.h                                                        *
*  Usage: header file for script structures and contstants, and           *
*         function prototypes for scripts.c                               *
*                                                                         *
*                                                                         *
*  $Author: egreen $
*  $Date: 1996/09/24 03:48:42 $
*  $Revision: 3.6 $
**************************************************************************/

#define DG_SCRIPT_VERSION "DG Scripts 1.0.13"

#define    MOB_TRIGGER   0
#define    OBJ_TRIGGER   1
#define    WLD_TRIGGER   2

/* unless you change this, Puff casts all your dg spells */
#define DG_CASTER_PROXY 1
/* spells cast by objects and rooms use this level */
#define DG_SPELL_LEVEL  25 

/*
 * define this if you don't want wear/remove triggers to fire when
 * a player is saved.
 */
#define NO_EXTRANEOUS_TRIGGERS
/* 
 * %actor.room% behaviour :
 * Until pl 7 %actor.room% returned a room vnum. 
 * Working with this number in scripts was unnecessarily hard,
 * especially in those situations one needed the id of the room,
 * the items in it, etc. As a result of this, the output
 * has been changed (as of pl 8) to a room variable.
 * This means old scripts will need a minor adjustment;
 * 
 * Before:
 * if %actor.room%==3001
 *   %echo% You are at the main temple.
 *
 * After:
 * eval room %actor.room% 
 * if %room.vnum%==3001
 *   %echo% You are at the main temple.
 *
 * If you wish to continue using the old style, comment out the line below.
 *
 * Welcor
 */
#define ACTOR_ROOM_IS_UID 1

/* mob trigger types */
#define MTRIG_GLOBAL           (1 << 0)      /* check even if zone empty   */
#define MTRIG_RANDOM           (1 << 1)      /* checked randomly           */
#define MTRIG_COMMAND          (1 << 2)	     /* character types a command  */
#define MTRIG_SPEECH           (1 << 3)	     /* a char says a word/phrase  */
#define MTRIG_ACT              (1 << 4)      /* word or phrase sent to act */
#define MTRIG_DEATH            (1 << 5)      /* character dies             */
#define MTRIG_GREET            (1 << 6)      /* something enters room seen */
#define MTRIG_GREET_ALL        (1 << 7)      /* anything enters room       */
#define MTRIG_ENTRY            (1 << 8)      /* the mob enters a room      */
#define MTRIG_RECEIVE          (1 << 9)      /* character is given obj     */
#define MTRIG_FIGHT            (1 << 10)     /* each pulse while fighting  */
#define MTRIG_HITPRCNT         (1 << 11)     /* fighting and below some hp */
#define MTRIG_BRIBE	       (1 << 12)     /* coins are given to mob     */
#define MTRIG_LOAD             (1 << 13)     /* the mob is loaded          */
#define MTRIG_MEMORY           (1 << 14)     /* mob see's someone remembered */
#define MTRIG_CAST             (1 << 15)     /* mob targetted by spell     */
#define MTRIG_LEAVE            (1 << 16)     /* someone leaves room seen   */
#define MTRIG_DOOR             (1 << 17)     /* door manipulated in room   */

#define MTRIG_TIME             (1 << 19)     /* trigger based on game hour */

/* obj trigger types */
#define OTRIG_GLOBAL           (1 << 0)	     /* unused                     */
#define OTRIG_RANDOM           (1 << 1)	     /* checked randomly           */
#define OTRIG_COMMAND          (1 << 2)      /* character types a command  */

#define OTRIG_TIMER            (1 << 5)     /* item's timer expires       */
#define OTRIG_GET              (1 << 6)     /* item is picked up          */
#define OTRIG_DROP             (1 << 7)     /* character trys to drop obj */
#define OTRIG_GIVE             (1 << 8)     /* character trys to give obj */
#define OTRIG_WEAR             (1 << 9)     /* character trys to wear obj */
#define OTRIG_REMOVE           (1 << 11)    /* character trys to remove obj */

#define OTRIG_LOAD             (1 << 13)    /* the object is loaded       */

#define OTRIG_CAST             (1 << 15)    /* object targetted by spell */
#define OTRIG_LEAVE            (1 << 16)    /* someone leaves room seen    */

#define OTRIG_CONSUME          (1 << 18)    /* char tries to eat/drink obj */
#define OTRIG_TIME             (1 << 19)     /* trigger based on game hour */

/* wld trigger types */
#define WTRIG_GLOBAL           (1 << 0)      /* check even if zone empty   */
#define WTRIG_RANDOM           (1 << 1)	     /* checked randomly           */
#define WTRIG_COMMAND          (1 << 2)	     /* character types a command  */
#define WTRIG_SPEECH           (1 << 3)      /* a char says word/phrase    */

#define WTRIG_RESET            (1 << 5)      /* zone has been reset        */
#define WTRIG_ENTER            (1 << 6)	     /* character enters room      */
#define WTRIG_DROP             (1 << 7)      /* something dropped in room  */

#define WTRIG_CAST             (1 << 15)     /* spell cast in room */
#define WTRIG_LEAVE            (1 << 16)     /* character leaves the room */
#define WTRIG_DOOR             (1 << 17)     /* door manipulated in room  */

#define WTRIG_TIME             (1 << 19)     /* trigger based on game hour */
/* obj command trigger types */
#define OCMD_EQUIP             (1 << 0)	     /* obj must be in char's equip */
#define OCMD_INVEN             (1 << 1)	     /* obj must be in char's inven */
#define OCMD_ROOM              (1 << 2)	     /* obj must be in char's room  */

/* obj consume trigger commands */
#define OCMD_EAT    1
#define OCMD_DRINK  2
#define OCMD_QUAFF  3

#define TRIG_NEW                0	     /* trigger starts from top  */
#define TRIG_RESTART            1	     /* trigger restarting       */


/*
 * These are slightly off of PULSE_MOBILE so
 * everything isnt happening at the same time 
 */
#define PULSE_DG_SCRIPT         (13 RL_SEC)


#define MAX_SCRIPT_DEPTH      10          /* maximum depth triggers can
					     recurse into each other */


/* one line of the trigger */
struct cmdlist_element {
  char *cmd;				/* one line of a trigger */
  struct cmdlist_element *original;
  struct cmdlist_element *next;
};

struct trig_var_data {
  char *name;				/* name of variable  */
  char *value;				/* value of variable */
  long context;				/* 0: global context */
  
  struct trig_var_data *next;
};

/* structure for triggers */
struct trig_data {
    IDXTYPE nr; 	                /* trigger's rnum                  */
    byte attach_type;			/* mob/obj/wld intentions          */
    byte data_type;		        /* type of game_data for trig      */
    char *name;			        /* name of trigger                 */
    long trigger_type;			/* type of trigger (for bitvector) */
    struct cmdlist_element *cmdlist;	/* top of command list             */
    struct cmdlist_element *curr_state;	/* ptr to current line of trigger  */
    int narg;				/* numerical argument              */
    char *arglist;			/* argument list                   */
    int depth;				/* depth into nest ifs/whiles/etc  */
    int loops;				/* loop iteration counter          */
    struct event *wait_event;   	/* event to pause the trigger      */
    ubyte purged;			/* trigger is set to be purged     */
    struct trig_var_data *var_list;	/* list of local vars for trigger  */
    
    struct trig_data *next;  
    struct trig_data *next_in_world;    /* next in the global trigger list */
};


/* a complete script (composed of several triggers) */
struct script_data {
  long types;				/* bitvector of trigger types */
  struct trig_data *trig_list;	        /* list of triggers           */
  struct trig_var_data *global_vars;	/* list of global variables   */
  ubyte purged;				/* script is set to be purged */
  long context;				/* current context for statics */

  struct script_data *next;		/* used for purged_scripts    */
};

/* The event data for the wait command */
struct wait_event_data {
  struct trig_data *trigger;
  void *go;
  int type;
};

/* typedefs that the dg functions rely on */
typedef struct index_data index_data;
typedef struct room_data room_data;
typedef struct obj_data obj_data;
typedef struct trig_data trig_data;
typedef struct char_data char_data;

/* used for actor memory triggers */
struct script_memory {
  long id;				/* id of who to remember */
  char *cmd;				/* command, or NULL for generic */
  struct script_memory *next;
};


/* function prototypes from triggers.c (and others) */
void act_mtrigger(const char_data *ch, char *str,
	char_data *actor, char_data *victim, obj_data *object, obj_data *target, char *arg);  
void speech_mtrigger(char_data *actor, char *str);
void speech_wtrigger(char_data *actor, char *str);
void greet_memory_mtrigger(char_data *ch);
int greet_mtrigger(char_data *actor, int dir);
int entry_mtrigger(char_data *ch);
void entry_memory_mtrigger(char_data *ch);
int enter_wtrigger(room_data *room, char_data *actor, int dir);
int drop_otrigger(obj_data *obj, char_data *actor);
void timer_otrigger(obj_data *obj);
int get_otrigger(obj_data *obj, char_data *actor);
int drop_wtrigger(obj_data *obj, char_data *actor);
int give_otrigger(obj_data *obj, char_data *actor,
         char_data *victim);
int receive_mtrigger(char_data *ch, char_data *actor,
         obj_data *obj);
void bribe_mtrigger(char_data *ch, char_data *actor,
         int amount);
int wear_otrigger(obj_data *obj, char_data *actor, int where);
int remove_otrigger(obj_data *obj, char_data *actor);
int command_mtrigger(char_data *actor, char *cmd, char *argument);
int command_otrigger(char_data *actor, char *cmd, char *argument);
int command_wtrigger(char_data *actor, char *cmd, char *argument);
int death_mtrigger(char_data *ch, char_data *actor);
void fight_mtrigger(char_data *ch);
void hitprcnt_mtrigger(char_data *ch);

void random_mtrigger(char_data *ch);
void random_otrigger(obj_data *obj);
void random_wtrigger(room_data *ch);
void reset_wtrigger(room_data *ch);

void load_mtrigger(char_data *ch);
void load_otrigger(obj_data *obj);

int cast_mtrigger(char_data *actor, char_data *ch, int spellnum);
int cast_otrigger(char_data *actor, obj_data *obj, int spellnum);
int cast_wtrigger(char_data *actor, char_data *vict, obj_data *obj, int spellnum);

int leave_mtrigger(char_data *actor, int dir);
int leave_wtrigger(room_data *room, char_data *actor, int dir);
int leave_otrigger(room_data *room, char_data *actor, int dir);

int door_mtrigger(char_data *actor, int subcmd, int dir);
int door_wtrigger(char_data *actor, int subcmd, int dir);

int consume_otrigger(obj_data *obj, char_data *actor, int cmd);

void time_mtrigger(char_data *ch);
void time_otrigger(obj_data *obj);
void time_wtrigger(room_data *room);

/* function prototypes from scripts.c */
void script_trigger_check(void);
void check_time_triggers(void);
void add_trigger(struct script_data *sc, trig_data *t, int loc);
char_data *get_char(char *name);
char_data *get_char_by_obj(obj_data *obj, char *name);
obj_data *get_obj(char *name);

void do_stat_trigger(char_data *ch, trig_data *trig);
void do_sstat_room(char_data * ch);
void do_sstat_object(char_data * ch, obj_data * j);
void do_sstat_character(char_data * ch, char_data * k);

void script_vlog(const char *format, va_list args);
void script_log(const char *format, ...) __attribute__ ((format (printf, 1, 2)));
void mob_log(char_data *mob, const char *format, ...) __attribute__ ((format (printf, 2, 3)));
void obj_log(obj_data *obj, const char *format, ...) __attribute__ ((format (printf, 2, 3)));
void wld_log(room_data *room, const char *format, ...) __attribute__ ((format (printf, 2, 3)));
void dg_read_trigger(FILE *fp, void *i, int type);
void dg_obj_trigger(char *line, obj_data *obj);
void assign_triggers(void *i, int type);
void parse_trigger(FILE *trig_f, int nr);
int real_trigger(int vnum);
void extract_script(void *thing, int type);
void extract_script_mem(struct script_memory *sc);
void free_proto_script(void *thing, int type);
void free_script(void *thing, int type);
void free_trigger(trig_data *trig);
void copy_proto_script(void *source, void *dest, int type);
void copy_script(void *source, void *dest, int type);
void trig_data_copy(trig_data *this_data, const trig_data *trg);
void delete_variables(const char *charname);

trig_data *read_trigger(int nr);
void add_var(struct trig_var_data **var_list, char *name, char *value, long id);
room_data *dg_room_of_obj(obj_data *obj);
void do_dg_cast(void *go, struct script_data *sc, trig_data *trig, 
    int type, char *cmd);
void do_dg_affect(void *go, struct script_data *sc, trig_data *trig, 
    int type, char *cmd);
void script_damage(char_data *vict, int dam);

void extract_value(struct script_data *sc, trig_data *trig, char *cmd);

/* To maintain strict-aliasing we'll have to do this trick with a union */
/* Thanks to Chris Gilbert for reminding me that there are other options. */
int script_driver(void *go_adress, trig_data *trig, int type, int mode);

int trgvar_in_room(room_vnum vnum);
struct cmdlist_element *find_done(struct cmdlist_element *cl);
struct cmdlist_element * \
  find_case(trig_data *trig, struct cmdlist_element *cl, \
          void *go, struct script_data *sc, int type, char *cond);
int find_eq_pos_script(char *arg);
char_data *get_char_near_obj(obj_data *obj, char *name);
obj_data *get_obj_near_obj(obj_data *obj, char *name);


/* defines for valid_dg_target */
int valid_dg_target(char_data *ch, int bitvector);
#define DG_ALLOW_GODS (1<<0)

/* Macros for scripts */

// #define UID_CHAR   '\x1b'
#define UID_CHAR   '}'
#define GET_TRIG_NAME(t)          ((t)->name)
#define GET_TRIG_RNUM(t)          ((t)->nr)
#define GET_TRIG_VNUM(t)	  (trig_index[(t)->nr]->vnum)
#define GET_TRIG_TYPE(t)          ((t)->trigger_type)
#define GET_TRIG_DATA_TYPE(t)	  ((t)->data_type)
#define GET_TRIG_NARG(t)          ((t)->narg)
#define GET_TRIG_ARG(t)           ((t)->arglist)
#define GET_TRIG_VARS(t)	  ((t)->var_list)
#define GET_TRIG_WAIT(t)	  ((t)->wait_event)
#define GET_TRIG_DEPTH(t)         ((t)->depth)
#define GET_TRIG_LOOPS(t)         ((t)->loops)

/* player id's: 0 to MOB_ID_BASE - 1            */
/* mob id's: MOB_ID_BASE to ROOM_ID_BASE - 1      */
/* room id's: ROOM_ID_BASE to OBJ_ID_BASE - 1    */
/* object id's: OBJ_ID_BASE and higher           */
#define MOB_ID_BASE	  50000  /* 50000 player IDNUMS should suffice */
#define ROOM_ID_BASE    1050000 /* 1000000 Mobs */
#define OBJ_ID_BASE     1300000 /* 250000 Rooms */

#define SCRIPT(o)		  ((o)->script)
#define SCRIPT_MEM(c)             ((c)->memory)

#define SCRIPT_TYPES(s)		  ((s)->types)				  

#define TRIGGERS(s)		  ((s)->trig_list)

#define GET_SHORT(ch)    ((ch)->player.short_descr)


#define SCRIPT_CHECK(go, type)   (SCRIPT(go) && \
				  IS_SET(SCRIPT_TYPES(SCRIPT(go)), type))
#define TRIGGER_CHECK(t, type)   (IS_SET(GET_TRIG_TYPE(t), type) && \
				  !GET_TRIG_DEPTH(t))

#define ADD_UID_VAR(buf, trig, go, name, context) do { \
		         sprintf(buf, "%c%ld", UID_CHAR, GET_ID(go)); \
                         add_var(&GET_TRIG_VARS(trig), name, buf, context); } while (0)


/* needed for new %load% handling */
int can_wear_on_pos(obj_data *obj, int pos);

/* find_char helpers */
extern long max_id;
void init_lookup_table(void);
struct char_data *find_char_by_uid_in_lookup_table(long uid);
struct obj_data *find_obj_by_uid_in_lookup_table(long uid);
void add_to_lookup_table(long uid, void *c);
void remove_from_lookup_table(long uid);

