/**************************************************************************
*  File: scripts.c                                                        *
*  Usage: contains general functions for using scripts.                   *
*                                                                         *
*                                                                         *
*  $Author: egreen $
*  $Date: 1996/09/24 03:48:42 $
*  $Revision: 3.25 $
**************************************************************************/

#include "conf.h"
#include "sysdep.h"

 
#include "structs.h"
#include "dg_scripts.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "dg_event.h"
#include "db.h"
#include "screen.h"
#include "constants.h"
#include "spells.h"

#define PULSES_PER_MUD_HOUR     (SECS_PER_MUD_HOUR*PASSES_PER_SEC)


/* external vars from db.c */
extern unsigned long pulse;
extern int dg_owner_purged;
extern int top_of_trigt;
extern struct index_data **trig_index;
extern struct trig_data *trigger_list;

/* other external vars */
extern struct room_data *world;
extern struct char_data *character_list;
extern struct obj_data *object_list;
extern const char *item_types[];
extern const char *genders[];
extern const char *pc_class_types[];
extern const char *exit_bits[];
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct spell_info_type spell_info[];
extern struct time_info_data time_info;
extern const char *pc_race_types[];

/* external functions */
void free_varlist(struct trig_var_data *vd);
int obj_room(obj_data *obj);
int is_empty(int zone_nr);
sh_int find_target_room(struct char_data * ch, char *rawroomstr);
trig_data *read_trigger(int nr);
struct obj_data *get_object_in_equip(struct char_data * ch, char *name);
void extract_trigger(struct trig_data *trig);
int eval_lhs_op_rhs(char *expr, char *result, void *go, struct script_data *sc,
                    trig_data *trig, int type);
char *skill_percent(struct char_data *ch, char *skill);
extern void gain_exp(struct char_data *ch, int gain);
extern int find_skill_num(char *name);
int can_edit_zone(struct char_data *ch, zone_rnum zone);
zone_rnum real_zone_by_thing(room_vnum vznum);

/* function protos from this file */
void extract_value(struct script_data *sc, trig_data *trig, char *cmd);
struct cmdlist_element *find_done(struct cmdlist_element *cl);
struct cmdlist_element * \
  find_case(struct trig_data *trig, struct cmdlist_element *cl, \
          void *go, struct script_data *sc, int type, char *cond);
void process_eval(void *go, struct script_data *sc, trig_data *trig, int type, char *cmd);

/* Return pointer to first occurrence of string ct in */
/* cs, or NULL if not present.  Case insensitive */
char *str_str(char *cs, char *ct)
{
  char *s, *t;

  if (!cs || !ct || !*ct)
    return NULL;

  while (*cs) {
    t = ct;

    while (*cs && (LOWER(*cs) != LOWER(*t)))
      cs++;
 
    s = cs;
 
    while (*t && *cs && (LOWER(*cs) == LOWER(*t))) {
      t++;
      cs++;
    }

    if (!*t)
      return s;
  }

  return NULL;
}


int trgvar_in_room(room_vnum vnum) {
    room_rnum rnum = real_room(vnum);
    int i = 0;
    char_data *ch;

    if (rnum == NOWHERE) {
        script_log("people.vnum: world[rnum] does not exist");
        return (-1);
    }

    for (ch = world[rnum].people; ch !=NULL; ch = ch->next_in_room)
        i++;

    return i;
}

obj_data *get_obj_in_list(char *name, obj_data *list)
{
    obj_data *i;
    long id;
    
    if (*name == UID_CHAR){
      id = atoi(name + 1);
     
      for (i = list; i; i = i->next_content)
        if (id == GET_ID(i))
          return i;
    } else {
      for (i = list; i; i = i->next_content)
        if (isname(name, i->name))
          return i;
    }
        
    return NULL;
}

obj_data *get_object_in_equip(char_data * ch, char *name)
{
  int j, n = 0, number;
  obj_data *obj;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp = tmpname; 
  long id;

  if (*name == UID_CHAR) {
    id = atoi(name + 1);
         
    for (j = 0; j < NUM_WEARS; j++)
      if ((obj = GET_EQ(ch, j)))
        if (id == GET_ID(obj))
          return (obj);
  } else if (is_number(name)) {
    obj_vnum ovnum = atoi(name);
    for (j = 0; j < NUM_WEARS; j++)
      if ((obj = GET_EQ(ch, j)))
        if (GET_OBJ_VNUM(obj) == ovnum)
          return (obj);
  } else {
    snprintf(tmpname, sizeof(tmpname), "%s", name);
    if (!(number = get_number(&tmp)))
      return NULL;

    for (j = 0; (j < NUM_WEARS) && (n <= number); j++)
      if ((obj = GET_EQ(ch, j)))
        if (isname(tmp, obj->name))
          if (++n == number)
            return (obj);
  }
  
  return NULL;
}

/* Handles 'held', 'light' and 'wield' positions - Welcor
   After idea from Byron Ellacott - bje@apnic.net */
int find_eq_pos_script(char *arg)
{
  int i;
  struct eq_pos_list {
    const char *pos;
    int where;
  } eq_pos[] = {
    {"hold",     WEAR_HOLD},
    {"held",     WEAR_HOLD},
    {"light",    WEAR_LIGHT},
    {"wield",    WEAR_WIELD},
    {"rfinger",  WEAR_FINGER_R},
    {"lfinger",  WEAR_FINGER_L},
    {"neck1",    WEAR_NECK_1},
    {"neck2",    WEAR_NECK_2},
    {"body",     WEAR_BODY},
    {"head",     WEAR_HEAD},
    {"legs",     WEAR_LEGS},
    {"feet",     WEAR_FEET},
    {"hands",    WEAR_HANDS},
    {"arms",     WEAR_ARMS},
    {"shield",   WEAR_SHIELD},
    {"about",    WEAR_ABOUT},
    {"waist",    WEAR_WAIST},
    {"rwrist",   WEAR_WRIST_R},
    {"lwrist",   WEAR_WRIST_L},
    {"backpack", WEAR_BACKPACK},
    {"rear",     WEAR_EAR_R},
    {"lear",     WEAR_EAR_L},
    {"wings",    WEAR_WINGS},
    {"mask",     WEAR_MASK},
    {"none", -1}
  };
  
  if (is_number(arg) && (i = atoi(arg)) >= 0 && i < NUM_WEARS)
    return i;
  
  for (i = 0;eq_pos[i].where != -1;i++) {
    if (!str_cmp(eq_pos[i].pos, arg))
      return eq_pos[i].where;
  }
  return (-1);
}

int can_wear_on_pos(struct obj_data *obj, int pos)
{
  switch (pos) {
    case WEAR_HOLD:
    case WEAR_LIGHT:    return CAN_WEAR(obj, ITEM_WEAR_HOLD);
    case WEAR_WIELD:    return CAN_WEAR(obj, ITEM_WEAR_WIELD);
    case WEAR_FINGER_R: 
    case WEAR_FINGER_L: return CAN_WEAR(obj, ITEM_WEAR_FINGER);
    case WEAR_NECK_1:   
    case WEAR_NECK_2:   return CAN_WEAR(obj, ITEM_WEAR_NECK);
    case WEAR_BODY:     return CAN_WEAR(obj, ITEM_WEAR_BODY);
    case WEAR_HEAD:     return CAN_WEAR(obj, ITEM_WEAR_HEAD);
    case WEAR_LEGS:     return CAN_WEAR(obj, ITEM_WEAR_LEGS);
    case WEAR_FEET:     return CAN_WEAR(obj, ITEM_WEAR_FEET);
    case WEAR_HANDS:    return CAN_WEAR(obj, ITEM_WEAR_HANDS);
    case WEAR_ARMS:     return CAN_WEAR(obj, ITEM_WEAR_ARMS);
    case WEAR_SHIELD:   return CAN_WEAR(obj, ITEM_WEAR_SHIELD);
    case WEAR_ABOUT:    return CAN_WEAR(obj, ITEM_WEAR_ABOUT);
    case WEAR_WAIST:    return CAN_WEAR(obj, ITEM_WEAR_WAIST);
    case WEAR_WRIST_R:
    case WEAR_WRIST_L:  return CAN_WEAR(obj, ITEM_WEAR_WRIST);
    case WEAR_BACKPACK: return CAN_WEAR(obj, ITEM_WEAR_PACK);
    case WEAR_EAR_R:
    case WEAR_EAR_L:    return CAN_WEAR(obj, ITEM_WEAR_EAR);
    case WEAR_WINGS:    return CAN_WEAR(obj, ITEM_WEAR_WINGS);
    case WEAR_MASK:     return CAN_WEAR(obj, ITEM_WEAR_MASK);
    default: return FALSE;
  }
}  

/************************************************************
 * search by number routines
 ************************************************************/
 
/* return char with UID n */
struct char_data *find_char(long n)
{
  if (n>=ROOM_ID_BASE) /* See note in dg_scripts.h */
  return NULL;

  return find_char_by_uid_in_lookup_table(n);
}


/* return object with UID n */
obj_data *find_obj(long n)
{
  if (n < OBJ_ID_BASE) /* see note in dg_scripts.h */
  return NULL;
    
  return find_obj_by_uid_in_lookup_table(n);
}

/* return room with UID n */
room_data *find_room(long n)
{
  room_rnum rnum;
  
  n -= ROOM_ID_BASE;
  if (n<0) 
    return NULL;
  rnum = real_room((room_vnum)n);  
    
  if (rnum != NOWHERE)
    return &world[rnum];

  return NULL;
}

/************************************************************
 * generic searches based only on name
 ************************************************************/

/* search the entire world for a char, and return a pointer */
char_data *get_char(char *name)
{
  char_data *i;

  if (*name == UID_CHAR) {
    i = find_char(atoi(name + 1));

    if (i && valid_dg_target(i, DG_ALLOW_GODS))
      return i;
  } else {
    for (i = character_list; i; i = i->next)
      if (isname(name, i->player.name) &&
          valid_dg_target(i, DG_ALLOW_GODS))
        return i;
  }

  return NULL;
}

/*
 * Finds a char in the same room as the object with the name 'name'
 */
char_data *get_char_near_obj(obj_data *obj, char *name)
{
  char_data *ch;
   
  if (*name == UID_CHAR) {
    ch = find_char(atoi(name + 1));
            
    if (ch && valid_dg_target(ch, DG_ALLOW_GODS))
      return ch;
  } else {
    room_rnum num;
    if ((num = obj_room(obj)) != NOWHERE)
      for (ch = world[num].people; ch; ch = ch->next_in_room) 
        if (isname(name, ch->player.name) &&
            valid_dg_target(ch, DG_ALLOW_GODS))
          return ch;
  }
        
  return NULL;
}
            
                
/*
 * returns a pointer to the first character in world by name name,
 * or NULL if none found.  Starts searching in room room first
 */
char_data *get_char_in_room(room_data *room, char *name)
{    
  char_data *ch;

  if (*name == UID_CHAR) {
    ch = find_char(atoi(name + 1));
 
    if (ch && valid_dg_target(ch, DG_ALLOW_GODS))
      return ch;
  } else {
    for (ch = room->people; ch; ch = ch->next_in_room)
      if (isname(name, ch->player.name) &&
          valid_dg_target(ch, DG_ALLOW_GODS))
        return ch;
    }
            
  return NULL;
}

/* searches the room with the object for an object with name 'name'*/

obj_data *get_obj_near_obj(obj_data *obj, char *name)
{
  obj_data *i = NULL;  
  char_data *ch;
  int rm;
  long id;

  if (!str_cmp(name, "self") || !str_cmp(name, "me"))
    return obj;
  
  /* is it inside ? */
  if (obj->contains && (i = get_obj_in_list(name, obj->contains)))
    return i;
  
  /* or outside ? */
  if (obj->in_obj) {
    if (*name == UID_CHAR) {
       id = atoi(name + 1);
      
      if (id == GET_ID(obj->in_obj))
        return obj->in_obj;
    } else if (isname(name, obj->in_obj->name))
      return obj->in_obj;
  }   
  /* or worn ?*/
  else if (obj->worn_by && (i = get_object_in_equip(obj->worn_by, name)))
    return i;
  /* or carried ? */
  else if (obj->carried_by &&
          (i = get_obj_in_list(name, obj->carried_by->carrying)))
    return i;
  else if ((rm = obj_room(obj)) != NOWHERE) {
    /* check the floor */
    if ((i = get_obj_in_list(name, world[rm].contents)))
      return i;
    
    /* check peoples' inventory */
    for (ch = world[rm].people;ch ; ch = ch->next_in_room)
      if ((i = get_object_in_equip(ch, name)))
        return i;
  }
  return NULL;
}   

/* returns the object in the world with name name, or NULL if not found */
obj_data *get_obj(char *name)  
{
  obj_data *obj;
    
  if (*name == UID_CHAR)
    return find_obj(atoi(name + 1));
  else {
    for (obj = object_list; obj; obj = obj->next)
      if (isname(name, obj->name))
        return obj;
  }

  return NULL;
}
 

/* finds room by id or vnum.  returns NULL if not found */
room_data *get_room(char *name)
{
  room_rnum nr; 

  if (*name == UID_CHAR)
    return find_room(atoi(name + 1));
  else if ((nr = real_room(atoi(name))) == NOWHERE)
    return NULL;
  else
    return &world[nr];
}


/*
 * returns a pointer to the first character in world by name name,
 * or NULL if none found.  Starts searching with the person owing the object
 */
char_data *get_char_by_obj(obj_data *obj, char *name)
{
  char_data *ch;

  if (*name == UID_CHAR) {
    ch = find_char(atoi(name + 1));
           
    if (ch && valid_dg_target(ch, DG_ALLOW_GODS))
      return ch;
  } else {
    if (obj->carried_by &&
        isname(name, obj->carried_by->player.name) &&
        valid_dg_target(obj->carried_by, DG_ALLOW_GODS))
      return obj->carried_by;
     
    if (obj->worn_by &&
        isname(name, obj->worn_by->player.name) &&
        valid_dg_target(obj->worn_by, DG_ALLOW_GODS))
      return obj->worn_by;
     
    for (ch = character_list; ch; ch = ch->next)
      if (isname(name, ch->player.name) &&
          valid_dg_target(ch, DG_ALLOW_GODS))
        return ch;
  }
        
  return NULL;
}
            
                
/*
 * returns a pointer to the first character in world by name name,
 * or NULL if none found.  Starts searching in room room first
 */
char_data *get_char_by_room(room_data *room, char *name)
{    
  char_data *ch;

  if (*name == UID_CHAR) {
    ch = find_char(atoi(name + 1));
 
    if (ch && valid_dg_target(ch, DG_ALLOW_GODS))
      return ch;
  } else {
    for (ch = room->people; ch; ch = ch->next_in_room)
      if (isname(name, ch->player.name) &&
          valid_dg_target(ch, DG_ALLOW_GODS))
        return ch;
        
    for (ch = character_list; ch; ch = ch->next)
      if (isname(name, ch->player.name) &&
          valid_dg_target(ch, DG_ALLOW_GODS))
        return ch;
  }
            
  return NULL;
}


/*
 * returns the object in the world with name name, or NULL if not found
 * search based on obj
 */  
obj_data *get_obj_by_obj(obj_data *obj, char *name)
{
  obj_data *i = NULL;  
  int rm;

  if (*name == UID_CHAR) 
    return find_obj(atoi(name + 1));

  if (!str_cmp(name, "self") || !str_cmp(name, "me"))
    return obj;
  
  if (obj->contains && (i = get_obj_in_list(name, obj->contains)))
    return i;
  
  if (obj->in_obj && isname(name, obj->in_obj->name))
      return obj->in_obj;

  if (obj->worn_by && (i = get_object_in_equip(obj->worn_by, name)))
    return i;
  
  if (obj->carried_by &&
          (i = get_obj_in_list(name, obj->carried_by->carrying)))
    return i;

  if (((rm = obj_room(obj)) != NOWHERE) &&
           (i = get_obj_in_list(name, world[rm].contents)))
    return i;
                      
  return get_obj(name);
}   

/* only searches the room */
obj_data *get_obj_in_room(room_data *room, char *name)
{
  obj_data *obj;
  long id;
   
  if (*name == UID_CHAR) {
      id = atoi(name + 1);
      for (obj = room->contents; obj; obj = obj->next_content)
          if (id == GET_ID(obj)) 
              return obj;
  } else {
      for (obj = room->contents; obj; obj = obj->next_content)
          if (isname(name, obj->name))
              return obj;
  }           
      
  return NULL;
}
        
/* returns obj with name - searches room, then world */
obj_data *get_obj_by_room(room_data *room, char *name)
{
  obj_data *obj;
   
  if (*name == UID_CHAR) 
    return find_obj(atoi(name+1));
      
    for (obj = room->contents; obj; obj = obj->next_content)
      if (isname(name, obj->name))
        return obj;
           
    for (obj = object_list; obj; obj = obj->next)
      if (isname(name, obj->name))
        return obj;
      
  return NULL;
}

/* search through all the persons items, including containers
   and 0 if it doesnt exist, and greater then 0 if it does!
   Jamie Nelson (mordecai@timespace.co.nz)
   MUD -- 4dimensions.org:6000

   Now also searches by vnum -- Welcor
   Now returns the number of matching objects -- Welcor 02/04
*/
int item_in_list(char *item, obj_data *list) 
{
  obj_data *i;
  int count = 0;    

  if (*item == UID_CHAR) {
    long id = atol(item + 1);

    for (i = list; i; i = i->next_content) {
      if (id == GET_ID(i)) 
        count ++;
      if (GET_OBJ_TYPE(i) == ITEM_CONTAINER)       
        count += item_in_list(item, i->contains);
    }
  } else if (is_number(item)) { /* check for vnum */
    obj_vnum ovnum = atoi(item);

    for (i = list; i; i = i->next_content) {
      if (GET_OBJ_VNUM(i) == ovnum) 
        count++;
      if (GET_OBJ_TYPE(i) == ITEM_CONTAINER)           
        count += item_in_list(item, i->contains);
    }
  } else {
    for (i = list; i; i = i->next_content) {
      if (isname(item, i->name)) 
        count++;
      if (GET_OBJ_TYPE(i) == ITEM_CONTAINER)           
        count += item_in_list(item, i->contains);
    }
  }
  return count;
}

/* BOOLEAN return, just check if a player or mob
   has an item of any sort, searched for by name
   or id. 
   searching equipment as well as inventory,
   and containers.
   Jamie Nelson (mordecai@timespace.co.nz)
   MUD -- 4dimensions.org:6000
*/

int char_has_item(char *item, struct char_data *ch) {

  /* If this works, no more searching needed */
  if (get_object_in_equip(ch, item) != NULL) 
    return 1; 

  if (item_in_list(item, ch->carrying) == 0)
    return 0;
  else
    return 1;
}

/* checks every PULSE_SCRIPT for random triggers */
void script_trigger_check(void)
{
  char_data *ch;
  obj_data *obj;
  struct room_data *room=NULL;
  int nr;
  struct script_data *sc;

  for (ch = character_list; ch; ch = ch->next) {
    if (SCRIPT(ch)) {
      sc = SCRIPT(ch);

      if (IS_SET(SCRIPT_TYPES(sc), WTRIG_RANDOM) &&
          (!is_empty(world[IN_ROOM(ch)].zone) ||
           IS_SET(SCRIPT_TYPES(sc), WTRIG_GLOBAL)))
        random_mtrigger(ch);
    }
  }
  
  for (obj = object_list; obj; obj = obj->next) {
    if (SCRIPT(obj)) {
      sc = SCRIPT(obj);

      if (IS_SET(SCRIPT_TYPES(sc), OTRIG_RANDOM))
        random_otrigger(obj);
    }
  }

  for (nr = 0; nr <= top_of_world; nr++) {
    if (SCRIPT(&world[nr])) {
      room = &world[nr];
      sc = SCRIPT(room);
      
      if (IS_SET(SCRIPT_TYPES(sc), WTRIG_RANDOM) &&
          (!is_empty(room->zone) ||
           IS_SET(SCRIPT_TYPES(sc), WTRIG_GLOBAL)))
        random_wtrigger(room);
    }
  }
}

void check_time_triggers(void) 
{
  char_data *ch;
  obj_data *obj;
  struct room_data *room=NULL;
  int nr;
  struct script_data *sc;

  for (ch = character_list; ch; ch = ch->next) {
    if (SCRIPT(ch)) {
      sc = SCRIPT(ch);

      if (IS_SET(SCRIPT_TYPES(sc), WTRIG_TIME) &&
          (!is_empty(world[IN_ROOM(ch)].zone) ||
           IS_SET(SCRIPT_TYPES(sc), WTRIG_GLOBAL)))
        time_mtrigger(ch);
    }
  }
  
  for (obj = object_list; obj; obj = obj->next) {
    if (SCRIPT(obj)) {
      sc = SCRIPT(obj);

      if (IS_SET(SCRIPT_TYPES(sc), OTRIG_TIME))
        time_otrigger(obj);
    }
  }

  for (nr = 0; nr <= top_of_world; nr++) {
    if (SCRIPT(&world[nr])) {
      room = &world[nr];
      sc = SCRIPT(room);
      
      if (IS_SET(SCRIPT_TYPES(sc), WTRIG_TIME) &&
          (!is_empty(room->zone) ||
           IS_SET(SCRIPT_TYPES(sc), WTRIG_GLOBAL)))
        time_wtrigger(room);
    }
  }
}


EVENTFUNC(trig_wait_event)
{
  struct wait_event_data *wait_event_obj = (struct wait_event_data *)event_obj;
  trig_data *trig;
  void *go;
  int type;

  trig = wait_event_obj->trigger;
  go = wait_event_obj->go;
  type = wait_event_obj->type;

  free(wait_event_obj);  
  GET_TRIG_WAIT(trig) = NULL;

#if 1  /* debugging */
  {
    int found = FALSE;
    if (type == MOB_TRIGGER) {
      struct char_data *tch;
      for (tch = character_list;tch && !found;tch = tch->next)
        if (tch == (struct char_data *)go)
          found = TRUE;
    } else if (type == OBJ_TRIGGER) {
      struct obj_data *obj;
      for (obj = object_list;obj && !found;obj = obj->next)
        if (obj == (struct obj_data *)go)
          found = TRUE;
    } else {
      room_rnum i;
      for (i = 0;i<top_of_world && !found;i++) 
        if (&world[i] == (struct room_data *)go)
          found = TRUE;
    }
    if (!found) {
      log("Trigger restarted on unknown entity. Vnum: %d", GET_TRIG_VNUM(trig));
      log("Type: %s trigger", type==MOB_TRIGGER ? "Mob" : type == OBJ_TRIGGER ? "Obj" : "Room");
      log("attached %d places", trig_index[trig->nr]->number);
      script_log("Trigger restart attempt on unknown entity.");
      return 0;
    }  
  }
#endif

  script_driver(&go, trig, type, TRIG_RESTART);
  
  /* Do not reenqueue*/
  return 0;
}


void do_stat_trigger(struct char_data *ch, trig_data *trig)
{
    struct cmdlist_element *cmd_list;
    char sb[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
    int len = 0;
    
    if (!trig)
    {
        log("SYSERR: NULL trigger passed to do_stat_trigger.");
        return;
    }

    len += snprintf(sb, sizeof(sb), "Name: '%s%s%s',  VNum: [%s%5d%s], RNum: [%5d]\r\n",
              CCYEL(ch, C_NRM), GET_TRIG_NAME(trig), CCNRM(ch, C_NRM),
              CCGRN(ch, C_NRM), GET_TRIG_VNUM(trig), CCNRM(ch, C_NRM),
              GET_TRIG_RNUM(trig));

    if (trig->attach_type==OBJ_TRIGGER) {
      len += snprintf(sb + len, sizeof(sb)-len, "Trigger Intended Assignment: Objects\r\n");
      sprintbit(GET_TRIG_TYPE(trig), otrig_types, buf, sizeof(buf));
    } else if (trig->attach_type==WLD_TRIGGER) {
      len += snprintf(sb + len, sizeof(sb)-len, "Trigger Intended Assignment: Rooms\r\n");
      sprintbit(GET_TRIG_TYPE(trig), wtrig_types, buf, sizeof(buf));
    } else {
      len += snprintf(sb + len, sizeof(sb)-len, "Trigger Intended Assignment: Mobiles\r\n");
      sprintbit(GET_TRIG_TYPE(trig), trig_types, buf, sizeof(buf));
    }
    
    len += snprintf(sb + len, sizeof(sb)-len, "Trigger Type: %s, Numeric Arg: %d, Arg list: %s\r\n",
                     buf, GET_TRIG_NARG(trig), 
                    ((GET_TRIG_ARG(trig) && *GET_TRIG_ARG(trig))
                     ? GET_TRIG_ARG(trig) : "None"));

    len += snprintf(sb + len, sizeof(sb)-len, "Commands:\r\n"); 

    cmd_list = trig->cmdlist;
    while (cmd_list) {
      if (cmd_list->cmd)
        len += snprintf(sb + len, sizeof(sb)-len, "%s\r\n", cmd_list->cmd);

        if (len>MAX_STRING_LENGTH-80) {
          len += snprintf(sb + len, sizeof(sb)-len, "*** Overflow - script too long! ***\r\n");
          break;
        }
      cmd_list = cmd_list->next;
    }

    page_string(ch->desc, sb, 1);
}


/* find the name of what the uid points to */
void find_uid_name(char *uid, char *name, size_t nlen)
{
  char_data *ch;
  obj_data *obj;

  if ((ch = get_char(uid)))
    snprintf(name, nlen, "%s", ch->player.name);
  else if ((obj = get_obj(uid)))
    snprintf(name, nlen, "%s", obj->name);
  else
    snprintf(name, nlen, "uid = %s, (not found)", uid + 1);
}


/* general function to display stats on script sc */
void script_stat (char_data *ch, struct script_data *sc)
{
  struct trig_var_data *tv;
  trig_data *t;
  char name[MAX_INPUT_LENGTH];
  char namebuf[512];
  char buf1[MAX_STRING_LENGTH];

  send_to_char(ch, "Global Variables: %s\r\n", sc->global_vars ? "" : "None");
  send_to_char(ch, "Global context: %ld\r\n", sc->context);
  
  for (tv = sc->global_vars; tv; tv = tv->next) {
    snprintf(namebuf, sizeof(namebuf), "%s:%ld", tv->name, tv->context);
    if (*(tv->value) == UID_CHAR) {
      find_uid_name(tv->value, name, sizeof(name));
      send_to_char(ch, "    %15s:  %s\r\n", tv->context?namebuf:tv->name, name);
    } else 
      send_to_char(ch, "    %15s:  %s\r\n", tv->context?namebuf:tv->name, tv->value);
  }

  for (t = TRIGGERS(sc); t; t = t->next) {
    send_to_char(ch, "\r\n  Trigger: %s%s%s, VNum: [%s%5d%s], RNum: [%5d]\r\n",
            CCYEL(ch, C_NRM), GET_TRIG_NAME(t), CCNRM(ch, C_NRM),
            CCGRN(ch, C_NRM), GET_TRIG_VNUM(t), CCNRM(ch, C_NRM),
            GET_TRIG_RNUM(t));

    if (t->attach_type==OBJ_TRIGGER) {
      send_to_char(ch, "  Trigger Intended Assignment: Objects\r\n");
      sprintbit(GET_TRIG_TYPE(t), otrig_types, buf1, sizeof(buf1));
    } else if (t->attach_type==WLD_TRIGGER) {
      send_to_char(ch, "  Trigger Intended Assignment: Rooms\r\n");
      sprintbit(GET_TRIG_TYPE(t), wtrig_types, buf1, sizeof(buf1));
    } else {
      send_to_char(ch, "  Trigger Intended Assignment: Mobiles\r\n");
      sprintbit(GET_TRIG_TYPE(t), trig_types, buf1, sizeof(buf1));
    }
    
    send_to_char(ch, "  Trigger Type: %s, Numeric Arg: %d, Arg list: %s\r\n", 
            buf1, GET_TRIG_NARG(t), 
            ((GET_TRIG_ARG(t) && *GET_TRIG_ARG(t)) ? GET_TRIG_ARG(t) :
             "None"));

    if (GET_TRIG_WAIT(t)) {
      send_to_char(ch, "    Wait: %ld, Current line: %s\r\n",
              event_time(GET_TRIG_WAIT(t)), 
              t->curr_state ? t->curr_state->cmd : "End of Script");
      send_to_char(ch, "  Variables: %s\r\n", GET_TRIG_VARS(t) ? "" : "None");

      for (tv = GET_TRIG_VARS(t); tv; tv = tv->next) {
        if (*(tv->value) == UID_CHAR) {
          find_uid_name(tv->value, name, sizeof(name));
          send_to_char(ch, "    %15s:  %s\r\n", tv->name, name);
        } else 
          send_to_char(ch, "    %15s:  %s\r\n", tv->name, tv->value);
      }
    }
  }  
}


void do_sstat_room(struct char_data * ch)
{
  struct room_data *rm = &world[IN_ROOM(ch)];

  send_to_char(ch, "Script information:\r\n");
  if (!SCRIPT(rm)) {
    send_to_char(ch, "  None.\r\n");
    return;
  }

  script_stat(ch, SCRIPT(rm));
}


void do_sstat_object(char_data *ch, obj_data *j)
{
  send_to_char(ch, "Script information:\r\n");
  if (!SCRIPT(j)) {
    send_to_char(ch, "  None.\r\n");
    return;
  }

  script_stat(ch, SCRIPT(j));
}


void do_sstat_character(char_data *ch, char_data *k)
{
  send_to_char(ch, "Script information:\r\n");
  if (!SCRIPT(k)) {
    send_to_char(ch, "  None.\r\n");
    return;
  }
  
  script_stat(ch, SCRIPT(k));
}


/*
 * adds the trigger t to script sc in in location loc.  loc = -1 means
 * add to the end, loc = 0 means add before all other triggers.
 */
void add_trigger(struct script_data *sc, trig_data *t, int loc)
{
  trig_data *i;
  int n;

  for (n = loc, i = TRIGGERS(sc); i && i->next && (n != 0); n--, i = i->next);

  if (!loc) {
          t->next = TRIGGERS(sc);
    TRIGGERS(sc) = t;
  } else if (!i)
    TRIGGERS(sc) = t;
  else {
    t->next = i->next;
    i->next = t;
  }

  SCRIPT_TYPES(sc) |= GET_TRIG_TYPE(t);

  t->next_in_world = trigger_list;
  trigger_list = t;
}


ACMD(do_attach) 
{
  char_data *victim;
  obj_data *object;
  room_data *room;
  trig_data *trig;
  char targ_name[MAX_INPUT_LENGTH], trig_name[MAX_INPUT_LENGTH];
  char loc_name[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];
  int loc, tn, rn, num_arg;
  room_rnum rnum;

  argument = two_arguments(argument, arg, trig_name);
  two_arguments(argument, targ_name, loc_name);

  if (!*arg || !*targ_name || !*trig_name) {
    send_to_char(ch, "Usage: attach { mob | obj | room } { trigger } { name } [ location ]\r\n");
    return;
  }
  
  num_arg = atoi(targ_name);
  tn = atoi(trig_name);
  loc = (*loc_name) ? atoi(loc_name) : -1;
  
  if (is_abbrev(arg, "mobile") || is_abbrev(arg, "mtr")) {
    victim = get_char_vis(ch, targ_name, NULL, FIND_CHAR_WORLD);
    if (!victim) { /* search room for one with this vnum */
      for (victim = world[IN_ROOM(ch)].people;victim;victim=victim->next_in_room) 
        if (GET_MOB_VNUM(victim) == num_arg)
          break;

      if (!victim) {
        send_to_char(ch, "That mob does not exist.\r\n");
        return;
      }
    }
    if (!IS_NPC(victim))  {
      send_to_char(ch, "Players can't have scripts.\r\n");
      return;
    }
    if (!can_edit_zone(ch, world[IN_ROOM(ch)].zone)) {
      send_to_char(ch, "You can only attach triggers in your own zone\r\n");
      return;
    }
    /* have a valid mob, now get trigger */
    rn = real_trigger(tn);
    if ((rn == NOTHING) || !(trig = read_trigger(rn))) {
      send_to_char(ch, "That trigger does not exist.\r\n");
      return;
    }
  
    if (!SCRIPT(victim))
      CREATE(SCRIPT(victim), struct script_data, 1);
    add_trigger(SCRIPT(victim), trig, loc);
 
    send_to_char(ch, "Trigger %d (%s) attached to %s [%d].\r\n",
                 tn, GET_TRIG_NAME(trig), GET_SHORT(victim), GET_MOB_VNUM(victim));
  }

  else if (is_abbrev(arg, "object") || is_abbrev(arg, "otr")) {
    object = get_obj_vis(ch, targ_name, NULL);
    if (!object) { /* search room for one with this vnum */
      for (object = world[IN_ROOM(ch)].contents;object;object=object->next_content) 
        if (GET_OBJ_VNUM(object) == num_arg)
          break;
    
      if (!object) { /* search inventory for one with this vnum */
        for (object = ch->carrying;object;object=object->next_content) 
          if (GET_OBJ_VNUM(object) == num_arg)
            break;
    
        if (!object) {
          send_to_char(ch, "That object does not exist.\r\n");
          return;
        }
      }
    }

    if (!can_edit_zone(ch, world[IN_ROOM(ch)].zone)) {
      send_to_char(ch, "You can only attach triggers in your own zone\r\n");
      return;
    }
    /* have a valid obj, now get trigger */
    rn = real_trigger(tn);
    if ((rn == NOTHING) || !(trig = read_trigger(rn))) {
      send_to_char(ch, "That trigger does not exist.\r\n");
      return;
    }

    if (!SCRIPT(object))
      CREATE(SCRIPT(object), struct script_data, 1);
    add_trigger(SCRIPT(object), trig, loc);
          
    send_to_char(ch, "Trigger %d (%s) attached to %s [%d].\r\n",
                 tn, GET_TRIG_NAME(trig), 
                 (object->short_description ?
                  object->short_description : object->name),
                  GET_OBJ_VNUM(object));
  }

  else if (is_abbrev(arg, "room") || is_abbrev(arg, "wtr")) {
    if (strchr(targ_name, '.'))
      rnum = IN_ROOM(ch);
    else if (isdigit(*targ_name)) 
      rnum = find_target_room(ch, targ_name);
    else
      rnum = NOWHERE;
      
    if (rnum == NOWHERE) {
      send_to_char(ch, "You need to supply a room number or . for current room.\r\n");
      return;
    }  

    if (!can_edit_zone(ch, world[rnum].zone)) {
      send_to_char(ch, "You can only attach triggers in your own zone\r\n");
      return;
    }
    /* have a valid room, now get trigger */
    rn = real_trigger(tn);
    if ((rn == NOTHING) || !(trig = read_trigger(rn))) {
      send_to_char(ch, "That trigger does not exist.\r\n");
      return;
    }

    room = &world[rnum];

    if (!SCRIPT(room))
      CREATE(SCRIPT(room), struct script_data, 1);
    add_trigger(SCRIPT(room), trig, loc);
          
    send_to_char(ch, "Trigger %d (%s) attached to room %d.\r\n",
                 tn, GET_TRIG_NAME(trig), world[rnum].number);
  }

  else
    send_to_char(ch, "Please specify 'mob', 'obj', or 'room'.\r\n");
}

/*
 * Thanks to James Long for his assistance in plugging the memory leak 
 * that used to be here.   -- Welcor
 */
/* adds a variable with given name and value to trigger */
void add_var(struct trig_var_data **var_list, char *name, char *value, long id)
{
  struct trig_var_data *vd;
  
  if (strchr(name, '.')) {
    log("add_var() : Attempt to add illegal var: %s", name);
    return;
  }
  
  for (vd = *var_list; vd && str_cmp(vd->name, name); vd = vd->next);

  if (vd && (!vd->context || vd->context==id)) {
    free(vd->value);
    CREATE(vd->value, char, strlen(value) + 1);
  }

  else {
    CREATE(vd, struct trig_var_data, 1);
    
    CREATE(vd->name, char, strlen(name) + 1);
    strcpy(vd->name, name);                            /* strcpy: ok*/
    
    CREATE(vd->value, char, strlen(value) + 1);

    vd->next = *var_list;
    vd->context = id;
    *var_list = vd;
  }

  strcpy(vd->value, value);                            /* strcpy: ok*/
}


/*
 *  removes the trigger specified by name, and the script of o if
 *  it removes the last trigger.  name can either be a number, or
 *  a 'silly' name for the trigger, including things like 2.beggar-death.
 *  returns 0 if did not find the trigger, otherwise 1.  If it matters,
 *  you might need to check to see if all the triggers were removed after
 *  this function returns, in order to remove the script.
 */
int remove_trigger(struct script_data *sc, char *name)
{
  trig_data *i, *j;
  int num = 0, string = FALSE, n;
  char *cname;
      
        
  if (!sc)
    return 0;

  if ((cname = strstr(name,".")) || (!isdigit(*name)) ) {
    string = TRUE;
    if (cname) {
      *cname = '\0';
      num = atoi(name);
      name = ++cname;
    }
  } else
    num = atoi(name);
  
  for (n = 0, j = NULL, i = TRIGGERS(sc); i; j = i, i = i->next) {
    if (string) {
      if (isname(name, GET_TRIG_NAME(i)))
        if (++n >= num)
          break;
    }

    /* this isn't clean... */
    /* a numeric value will match if it's position OR vnum */
    /* is found. originally the number was position-only */
    else if (++n >= num)
      break;
    else if (trig_index[i->nr]->vnum == num)
      break;
  }
      
  if (i) {
    if (j) {
      j->next = i->next;
      extract_trigger(i);
    }
  
    /* this was the first trigger */
    else {
      TRIGGERS(sc) = i->next;
      extract_trigger(i);
    }
 
    /* update the script type bitvector */
    SCRIPT_TYPES(sc) = 0;
    for (i = TRIGGERS(sc); i; i = i->next)
      SCRIPT_TYPES(sc) |= GET_TRIG_TYPE(i);
 
    return 1;
  } else
    return 0; 
}     

ACMD(do_detach)
{  
  char_data *victim = NULL;
  obj_data *object = NULL;
  struct room_data *room;
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH], arg3[MAX_INPUT_LENGTH];
  char *trigger = 0;   
  int num_arg;
  
  argument = two_arguments(argument, arg1, arg2);
  one_argument(argument, arg3);
  
  if (!*arg1 || !*arg2) {
    send_to_char(ch, "Usage: detach [ mob | object | room ] { target } { trigger |"
                 " 'all' }\r\n");
    return;
  }

  /* vnum of mob/obj, if given */
  num_arg = atoi(arg2);

  if (!str_cmp(arg1, "room") || !str_cmp(arg1, "wtr")) {
    room = &world[IN_ROOM(ch)];
    if (!can_edit_zone(ch, room->zone)) {
      send_to_char(ch, "You can only detach triggers in your own zone\r\n");
      return;
    }
    if (!SCRIPT(room))
      send_to_char(ch, "This room does not have any triggers.\r\n");
    else if (!str_cmp(arg2, "all")) {
      extract_script(room, WLD_TRIGGER);
      send_to_char(ch, "All triggers removed from room.\r\n");
    } else if (remove_trigger(SCRIPT(room), arg2)) {
      send_to_char(ch, "Trigger removed.\r\n");
      if (!TRIGGERS(SCRIPT(room))) {
        extract_script(room, WLD_TRIGGER);
      }
    } else
      send_to_char(ch, "That trigger was not found.\r\n");
  }
    
  else {
    if (is_abbrev(arg1, "mobile") || !str_cmp(arg1, "mtr")) {
      victim = get_char_vis(ch, arg2, NULL, FIND_CHAR_WORLD);
      if (!victim) { /* search room for one with this vnum */
        for (victim = world[IN_ROOM(ch)].people;victim;victim=victim->next_in_room) 
          if (GET_MOB_VNUM(victim) == num_arg)
            break;

        if (!victim) {
          send_to_char(ch, "No such mobile around.\r\n");
          return;
        }
      }
      
      if (!arg3 || !*arg3)
        send_to_char(ch, "You must specify a trigger to remove.\r\n");
      else
        trigger = arg3;
    }
   
    else if (is_abbrev(arg1, "object") || !str_cmp(arg1, "otr")) {
      object = get_obj_vis(ch, arg2, NULL);
      if (!object) { /* search room for one with this vnum */
        for (object = world[IN_ROOM(ch)].contents;object;object=object->next_content) 
          if (GET_OBJ_VNUM(object) == num_arg)
            break;
    
        if (!object) { /* search inventory for one with this vnum */
          for (object = ch->carrying;object;object=object->next_content) 
            if (GET_OBJ_VNUM(object) == num_arg)
              break;
    
          if (!object) { /* give up */
            send_to_char(ch, "No such object around.\r\n");
            return;
          }
        }
      }

      if (!arg3 || !*arg3)
        send_to_char(ch, "You must specify a trigger to remove.\r\n");
      else
        trigger = arg3;
    }
    else  {
      /* Thanks to Carlos Myers for fixing the line below */
      if ((object = get_obj_in_equip_vis(ch, arg1, NULL, ch->equipment)));
      else if ((object = get_obj_in_list_vis(ch, arg1, NULL, ch->carrying)));
      else if ((victim = get_char_room_vis(ch, arg1, NULL)));
      else if ((object = get_obj_in_list_vis(ch, arg1, NULL, world[IN_ROOM(ch)].contents)));
      else if ((victim = get_char_vis(ch, arg1, NULL, FIND_CHAR_WORLD)));
      else if ((object = get_obj_vis(ch, arg1, NULL)));
      else
        send_to_char(ch, "Nothing around by that name.\r\n");
    
      trigger = arg2;
    }
      
    if (victim) {
      if (!IS_NPC(victim))
        send_to_char(ch, "Players don't have triggers.\r\n");
     
      else if (!SCRIPT(victim))
        send_to_char(ch, "That mob doesn't have any triggers.\r\n");
      else if (!can_edit_zone(ch, real_zone_by_thing(GET_MOB_VNUM(victim)))) {
        send_to_char(ch, "You can only detach triggers in your own zone\r\n");
        return;
      }
      else if (trigger && !str_cmp(trigger, "all")) {
        extract_script(victim, MOB_TRIGGER);
        send_to_char(ch, "All triggers removed from %s.\r\n", GET_SHORT(victim));
      }
   
      else if (trigger && remove_trigger(SCRIPT(victim), trigger)) {
        send_to_char(ch, "Trigger removed.\r\n");
        if (!TRIGGERS(SCRIPT(victim))) {
          extract_script(victim, MOB_TRIGGER);
        }
      } else
        send_to_char(ch, "That trigger was not found.\r\n");
    }
     
    else if (object) {
      if (!SCRIPT(object))
        send_to_char(ch, "That object doesn't have any triggers.\r\n");
        
      else if (!can_edit_zone(ch, real_zone_by_thing(GET_OBJ_VNUM(object)))) {
        send_to_char(ch, "You can only detach triggers in your own zone\r\n");
        return;
      }
      else if (trigger && !str_cmp(trigger, "all")) {
        extract_script(object, OBJ_TRIGGER);
        send_to_char(ch, "All triggers removed from %s.\r\n",
                object->short_description ? object->short_description :
                object->name);
      }
       
      else if (remove_trigger(SCRIPT(object), trigger)) {
        send_to_char(ch, "Trigger removed.\r\n");
        if (!TRIGGERS(SCRIPT(object))) {
          extract_script(object, OBJ_TRIGGER);
        }
      } else
        send_to_char(ch, "That trigger was not found.\r\n");
    }
  }  
}    


/* frees memory associated with var */
void free_var_el(struct trig_var_data *var)
{
  free(var->name);
  free(var->value);
  free(var);
}


/*
 * remove var name from var_list
 * returns 1 if found, else 0
 */
int remove_var(struct trig_var_data **var_list, char *name)
{
  struct trig_var_data *i, *j;

  for (j = NULL, i = *var_list; i && str_cmp(name, i->name);
       j = i, i = i->next);

  if (i) {
    if (j) {
      j->next = i->next;
      free_var_el(i);
    } else {
      *var_list = i->next;
      free_var_el(i);
    }

    return 1;      
  }
  
  return 0;
}


/*  
 *  Logs any errors caused by scripts to the system log.
 *  Will eventually allow on-line view of script errors.
 */
void script_vlog(const char *format, va_list args) 
{
  char output[MAX_STRING_LENGTH];
  struct descriptor_data *i;
  
  snprintf(output, sizeof(output), "SCRIPT ERR: %s", format);
    
  basic_mud_vlog(output, args);
    
  /* the rest is mostly a rip from basic_mud_log() */
  strcpy(output, "[ ");            /* strcpy: OK */
  vsnprintf(output + 2, sizeof(output) - 6, format, args);
  strcat(output, " ]\r\n");        /* strcat: OK */

  for (i = descriptor_list; i; i = i->next) {
    if (STATE(i) != CON_PLAYING || IS_NPC(i->character)) /* switch */
      continue;
    if (GET_LEVEL(i->character) < LVL_BUILDER)
      continue;
    if (PLR_FLAGGED(i->character, PLR_WRITING))
      continue;
    if (NRM > (PRF_FLAGGED(i->character, PRF_LOG1) ? 1 : 0) + (PRF_FLAGGED(i->character, PRF_LOG2) ? 2 : 0))
      continue;

    send_to_char(i->character, "%s%s%s", CCGRN(i->character, C_NRM), output, CCNRM(i->character, C_NRM));
  }
}


void script_log(const char *format, ...)
{
  va_list args;
  
  va_start(args, format);
  script_vlog(format, args);
  va_end(args);
}

int text_processed(char *field, char *subfield, struct trig_var_data *vd,
                   char *str, size_t slen)
{
  char *p, *p2;
  char tmpvar[MAX_STRING_LENGTH];
  
  if (!str_cmp(field, "strlen")) {                     /* strlen    */
    snprintf(str, slen, "%d", strlen(vd->value));
    return TRUE;
  } else if (!str_cmp(field, "trim")) {                /* trim      */
    /* trim whitespace from ends */
    snprintf(tmpvar, sizeof(tmpvar)-1 , "%s", vd->value); /* -1 to use later*/
    p = tmpvar;                       
    p2 = tmpvar + strlen(tmpvar) - 1; 
    while (*p && isspace(*p)) p++;     
    while ((p<=p2) && isspace(*p2)) p2--; 
    if (p>p2) { /* nothing left */
      *str = '\0';
      return TRUE;
    }
    *(++p2) = '\0';                                         /* +1 ok (see above) */
    snprintf(str, slen, "%s", p);
    return TRUE;
  } else if (!str_cmp(field, "contains")) {            /* contains  */
    if (str_str(vd->value, subfield))
      snprintf(str, slen, "1");
    else 
      snprintf(str, slen, "0");
    return TRUE;
  } else if (!str_cmp(field, "car")) {                 /* car       */
    char *car = vd->value;
    while (*car && !isspace(*car))
      *str++ = *car++;
    *str = '\0';
    return TRUE;
 
  } else if (!str_cmp(field, "cdr")) {                 /* cdr       */
    char *cdr = vd->value;
    while (*cdr && !isspace(*cdr)) cdr++; /* skip 1st field */
    while (*cdr && isspace(*cdr)) cdr++;  /* skip to next */

    snprintf(str, slen, "%s", cdr);
    return TRUE;
  } else if (!str_cmp(field, "mudcommand")) {
    /* find the mud command returned from this text */
/* NOTE: you may need to replace "cmd_info" with "complete_cmd_info", */
/* depending on what patches you've got applied.                      */
    //extern const struct command_info cmd_info[];
/* on older source bases:    extern struct command_info *cmd_info; */
    int length, cmd;
    for (length = strlen(vd->value), cmd = 0;
         *complete_cmd_info[cmd].command != '\n'; cmd++)
      if (!strncmp(complete_cmd_info[cmd].command, vd->value, length))
        break;

    if (*complete_cmd_info[cmd].command == '\n') strcpy(str,"");
    else snprintf(str, slen, "%s", complete_cmd_info[cmd].command);
    return TRUE;
  }

  return FALSE;
}


/* sets str to be the value of var.field */
void find_replacement(void *go, struct script_data *sc, trig_data *trig,
                int type, char *var, char *field, char *subfield, char *str, size_t slen)
{
  struct trig_var_data *vd=NULL;
  char_data *ch, *c = NULL, *rndm;
  obj_data *obj, *o = NULL;
  struct room_data *room, *r = NULL;
  char *name;
  int num, count;

  char *send_cmd[]       = {"msend ",       "osend ",       "wsend "      };
  char *echo_cmd[]       = {"mecho ",       "oecho ",       "wecho "      };
  char *echoaround_cmd[] = {"mechoaround ", "oechoaround ", "wechoaround "};
  char *door[]           = {"mdoor ",       "odoor ",       "wdoor "      };
  char *force[]          = {"mforce ",      "oforce ",      "wforce "     };
  char *load[]           = {"mload ",       "oload ",       "wload "      };
  char *purge[]          = {"mpurge ",      "opurge ",      "wpurge "     };
  char *teleport[]       = {"mteleport ",   "oteleport ",   "wteleport "  };
  /* the x kills a 'shadow' warning in gcc. */
  char *xdamage[]        = {"mdamage ",     "odamage ",     "wdamage "    };
  char *zoneecho[]       = {"mzoneecho ",   "ozoneecho ",   "wzoneecho "  };
  char *asound[]         = {"masound ",     "oasound ",     "wasound "    };
  char *at[]             = {"mat ",         "oat ",         "wat "        };
  /* there is no such thing as wtransform, thus the wecho below  */
  char *transform[]      = {"mtransform ",  "otransform ",  "wecho"       };
  /* X.global() will have a NULL trig */
  if (trig)
    for (vd = GET_TRIG_VARS(trig); vd; vd = vd->next)
      if (!str_cmp(vd->name, var))
        break;
  
  /* some evil waitstates could crash the mud if sent here with sc==NULL*/
  if (!vd && sc) 
    for (vd = sc->global_vars; vd; vd = vd->next)
      if (!str_cmp(vd->name, var) &&
          (vd->context==0 || vd->context==sc->context))
        break; 

  if (!*field) {
    if (vd)
      snprintf(str, slen, "%s", vd->value);
    else {
      if (!str_cmp(var, "self"))
        switch (type) {
        case MOB_TRIGGER:
          snprintf(str, slen, "%c%ld", UID_CHAR, GET_ID((char_data *) go));
          break;
        case OBJ_TRIGGER:
          snprintf(str, slen, "%c%ld", UID_CHAR, GET_ID((obj_data *) go));
          break;
        case WLD_TRIGGER:
          snprintf(str, slen, "%c%ld", UID_CHAR, (long) ((room_data *)go)->number + ROOM_ID_BASE);
          break;
        }
//        snprintf(str, slen, "self");
      else if (!str_cmp(var, "door"))
        snprintf(str, slen, "%s", door[type]);
      else if (!str_cmp(var, "force"))
        snprintf(str, slen, "%s", force[type]);
      else if (!str_cmp(var, "load"))
        snprintf(str, slen, "%s", load[type]);
      else if (!str_cmp(var, "purge"))
        snprintf(str, slen, "%s", purge[type]);
      else if (!str_cmp(var, "teleport"))
        snprintf(str, slen, "%s", teleport[type]);
      else if (!str_cmp(var, "damage"))
        snprintf(str, slen, "%s", xdamage[type]);
      else if (!str_cmp(var, "send"))
        snprintf(str, slen, "%s", send_cmd[type]);
      else if (!str_cmp(var, "echo"))
        snprintf(str, slen, "%s", echo_cmd[type]);
      else if (!str_cmp(var, "echoaround"))
        snprintf(str, slen, "%s", echoaround_cmd[type]);
      else if (!str_cmp(var, "zoneecho"))
        snprintf(str, slen, "%s", zoneecho[type]);
      else if (!str_cmp(var, "asound"))
        snprintf(str, slen, "%s", asound[type]);
      else if (!str_cmp(var, "at"))
        snprintf(str, slen, "%s", at[type]);
      else if (!str_cmp(var, "transform"))
        snprintf(str, slen, "%s", transform[type]);
      else
        *str = '\0';
    }

    return;
  }

  else {
    if (vd) {
      name = vd->value;

      switch (type) {
      case MOB_TRIGGER:
        ch = (char_data *) go;

        if ((o = get_object_in_equip(ch, name)));
        else if ((o = get_obj_in_list(name, ch->carrying)));
        else if ((c = get_char_room(name, NULL, IN_ROOM(ch))));
        else if ((o = get_obj_in_list(name,world[IN_ROOM(ch)].contents)));
        else if ((c = get_char(name)));
        else if ((o = get_obj(name)));
        else if ((r = get_room(name))) {}

        break;
      case OBJ_TRIGGER:
        obj = (obj_data *) go;

        if ((c = get_char_by_obj(obj, name)));
        else if ((o = get_obj_by_obj(obj, name)));
        else if ((r = get_room(name))) {}

        break;
      case WLD_TRIGGER:
        room = (struct room_data *) go;

        if ((c = get_char_by_room(room, name)));
        else if ((o = get_obj_by_room(room, name)));
        else if ((r = get_room(name))) {}

        break;
      }
    }

    else {
      if (!str_cmp(var, "self")) {
        switch (type) {
        case MOB_TRIGGER:
          c = (char_data *) go;
          r = NULL;
          o = NULL;  /* NULL assignments added to avoid self to always be    */ 
          break;     /* the room.  - Welcor        */
        case OBJ_TRIGGER:
          o = (obj_data *) go;
          c = NULL;
          r = NULL;
          break;
        case WLD_TRIGGER:
          r = (struct room_data *) go;
          c = NULL;
          o = NULL;
          break;
        }
      }
      
      else if (!str_cmp(var, "people")) {
        snprintf(str, slen, "%d",((num = atoi(field)) > 0) ? trgvar_in_room(num) : 0);        
        return;
      }
      else if (!str_cmp(var, "time")) {
        if (!str_cmp(field, "hour"))
          snprintf(str, slen, "%d", time_info.hours);
        else if (!str_cmp(field, "day"))
          snprintf(str, slen, "%d", time_info.day + 1);
        else if (!str_cmp(field, "month"))
          snprintf(str, slen, "%d", time_info.month + 1);
        else if (!str_cmp(field, "year"))
          snprintf(str, slen, "%d", time_info.year);
        else *str = '\0';
        return;
      }
      else if (!str_cmp(var, "random")) {
        if (!str_cmp(field, "char")) {
          rndm = NULL;
          count = 0;

          if (type == MOB_TRIGGER) {
            ch = (char_data *) go;
            for (c = world[IN_ROOM(ch)].people; c; c = c->next_in_room)
              if ((c != ch) && valid_dg_target(c, DG_ALLOW_GODS) &&
                  CAN_SEE(ch, c)) {
                if (!rand_number(0, count))
                  rndm = c;
                count++;
              }
          }
          
          else if (type == OBJ_TRIGGER) {
            for (c = world[obj_room((obj_data *) go)].people; c;
                 c = c->next_in_room)
              if (valid_dg_target(c, DG_ALLOW_GODS)) {
                if (!rand_number(0, count))
                  rndm = c;
                count++;
              }
          }
          
          else if (type == WLD_TRIGGER) {
            for (c = ((struct room_data *) go)->people; c;
                 c = c->next_in_room)
              if (valid_dg_target(c, DG_ALLOW_GODS)) {
                  
                if (!rand_number(0, count))
                  rndm = c;
                count++;
              }
          }
          
          if (rndm)
            snprintf(str, slen, "%c%ld", UID_CHAR, GET_ID(rndm));
          else
            *str = '\0';
        }

        else
          snprintf(str, slen, "%d", ((num = atoi(field)) > 0) ? rand_number(1, num) : 0);
        
        return;
      }
    }
    
    if (c) {
      if (text_processed(field, subfield, vd, str, slen)) return;

      else if (!str_cmp(field, "global")) { /* get global of something else */
        if (IS_NPC(c) && c->script) {
          find_replacement(go, c->script, NULL, MOB_TRIGGER,
            subfield, NULL, NULL, str, slen);
        }
      }

      /* set str to some 'non-text' first */
      *str = '\x1';

      switch (LOWER(*field)) {
              case 'a':
          if (!str_cmp(field, "alias"))
            snprintf(str, slen, "%s", GET_PC_NAME(c));

          else if (!str_cmp(field, "align")) {
            if (subfield && *subfield) {
              int addition = atoi(subfield);
              GET_ALIGNMENT(c) = MAX(-1000, MIN(addition, 1000));
          }
            snprintf(str, slen, "%d", GET_ALIGNMENT(c));
        }

          else if (!str_cmp(field, "affect")) {
            if (subfield && *subfield) {
              int spell = find_skill_num(subfield);
              if (affected_by_spell(c, spell))
                snprintf(str, slen, "1");
              else
                snprintf(str, slen, "0");
            } else
              snprintf(str, slen, "0");
          }
          break;
              case 'c':
          if (!str_cmp(field, "canbeseen")) {
            if ((type == MOB_TRIGGER) && !CAN_SEE(((char_data *)go), c))
              snprintf(str, slen, "0");
            else
              snprintf(str, slen, "1");
          }

          else if (!str_cmp(field, "class"))
            sprinttype(GET_CLASS(c), pc_class_types, str, slen);

          else if (!str_cmp(field, "con")) {
            if (subfield && *subfield) {
              int addition = atoi(subfield);
              int max = (IS_NPC(c) || GET_LEVEL(c) >= LVL_GRGOD) ? 25 : 18;
              GET_CON(c) += addition;
              if (GET_CON(c) > max) GET_CON(c) = max;
              if (GET_CON(c) < 3) GET_CON(c) = 3;
            }
            snprintf(str, slen, "%d", GET_CON(c));
          }
          else if (!str_cmp(field, "cha")) {
            if (subfield && *subfield) {
              int addition = atoi(subfield);
              int max = (IS_NPC(c) || GET_LEVEL(c) >= LVL_GRGOD) ? 25 : 18;
              GET_CHA(c) += addition;
              if (GET_CHA(c) > max) GET_CHA(c) = max;
              if (GET_CHA(c) < 3) GET_CHA(c) = 3;
            }
            snprintf(str, slen, "%d", GET_CHA(c));
          }
          break;
              case 'd':
          if (!str_cmp(field, "dex")) {
            if (subfield && *subfield) {
              int addition = atoi(subfield);
              int max = (IS_NPC(c) || GET_LEVEL(c) >= LVL_GRGOD) ? 25 : 18;
              GET_DEX(c) += addition;
              if (GET_DEX(c) > max) GET_DEX(c) = max;
              if (GET_DEX(c) < 3) GET_DEX(c) = 3;
            }
            snprintf(str, slen, "%d", GET_DEX(c));
          }
          break;
              case 'e':
          if (!str_cmp(field, "exp")) {
            if (subfield && *subfield) {
              int addition = MIN(atoi(subfield), 1000);

              gain_exp(c, addition);
            }
            snprintf(str, slen, "%d", GET_EXP(c));
          }

          else if (!str_cmp(field, "eq")) {
            int pos;
            if (!subfield || !*subfield)
              strcpy(str,"");
            else if (*subfield == '*') {
              int i, j = 0;
            for (i = 0; i < NUM_WEARS; i++)
                if (GET_EQ(c, i)) {
                  j++;
                  break;
                }
              if (j > 0)
                strcpy(str,"1");
              else
                strcpy(str,"");
            } else if ((pos = find_eq_pos_script(subfield)) < 0 || !GET_EQ(c, pos))
              strcpy(str,"");
            else
              snprintf(str, slen, "%c%ld",UID_CHAR, GET_ID(GET_EQ(c, pos)));
          }
          break;
              case 'f':
          if (!str_cmp(field, "fighting")) {
            if (FIGHTING(c))
              snprintf(str, slen, "%c%ld", UID_CHAR, GET_ID(FIGHTING(c)));
            else
              *str = '\0';
          }
          else if (!str_cmp(field, "follower")) {
            if (!c->followers || !c->followers->follower)
              *str = '\0';
            else
              snprintf(str, slen, "%c%ld", UID_CHAR, GET_ID(c->followers->follower));
          }
/*

      %findobj.<room vnum X>(<object vnum/id/name>)% 
        - count number of objects in room X with this name/id/vnum
      %findmob.<room vnum X>(<mob vnum Y>)% 
        - count number of mobs in room X with vnum Y

for example you want to check how many PC's are in room with vnum 1204.
as PC's have the vnum -1...
you would type:
in any script:
%echo% players in room 1204: %findmob.1204(-1)%

Or say you had a bank, and you want a script to check the number of 
bags 
of gold (vnum: 1234)
in the vault (vnum: 453) now and then. you can just use 
%findobj.453(1234)% and it will return the number of bags of gold.

**/

          /* addition inspired by Jamie Nelson - mordecai@xtra.co.nz */
          else if (!str_cmp(var, "findmob")) {
            if (!field || !*field || !subfield || !*subfield) {
              script_log("findmob.vnum(mvnum) - illegal syntax");
              strcpy(str, "0");
            } else {
              room_rnum rrnum = real_room(atoi(field));
              mob_vnum mvnum = atoi(subfield);
          
              if (rrnum == NOWHERE) {
                script_log("findmob.vnum(ovnum): No room with vnum %d", atoi(field));
                strcpy(str, "0");
              } else {
                int i = 0;
                char_data *ch;
                for (ch = world[rrnum].people; ch; ch = ch->next_in_room)
                  if (GET_MOB_VNUM(ch) == mvnum)
                    i++;
   
                snprintf(str, slen, "%d", i);
              }
            }
          }
          /* addition inspired by Jamie Nelson - mordecai@xtra.co.nz */
          else if (!str_cmp(var, "findobj")) {
            if (!field || !*field || !subfield || !*subfield) {
              script_log("findobj.vnum(ovnum) - illegal syntax");
              strcpy(str, "0");
            } else {
              room_rnum rrnum = real_room(atoi(field));
          
              if (rrnum == NOWHERE) {
                script_log("findmob.vnum(ovnum): No room with vnum %d", atoi(field));
                strcpy(str, "0");
              } else {
                /* item_in_list looks within containers as well. */
                snprintf(str, slen, "%d", item_in_list(subfield, world[rrnum].contents));
              }
            }
          }
          break;
              case 'g':
          if (!str_cmp(field, "gold")) {
          if (subfield && *subfield) {
            int addition = atoi(subfield);
            GET_GOLD(c) += addition;
          }
          snprintf(str, slen, "%d", GET_GOLD(c));
        }
          break;
              case 'h':
          if (!str_cmp(field, "has_item")) {
            if (!(subfield && *subfield))
              *str = '\0';
            else
              snprintf(str, slen, "%d", char_has_item(subfield, c));
        }
          else if (!str_cmp(field, "hisher"))
            snprintf(str, slen, "%s", HSHR(c));

          else if (!str_cmp(field, "heshe"))
            snprintf(str, slen, "%s", HSSH(c));

          else if (!str_cmp(field, "himher"))
            snprintf(str, slen, "%s", HMHR(c));

          else if (!str_cmp(field, "hitp"))
            snprintf(str, slen, "%d", GET_HIT(c));
          break;
              case 'i':
                if (!str_cmp(field, "id"))
            snprintf(str, slen, "%ld", GET_ID(c));

          /* new check for pc/npc status */
          else if (!str_cmp(field, "is_pc")) {
            if (IS_NPC(c))
            snprintf(str, slen, "0");
          else
            snprintf(str, slen, "1");
        }

          else if (!str_cmp(field, "inventory")) {
            if(subfield && *subfield) {
              for (obj = c->carrying;obj;obj=obj->next_content) {
                if(GET_OBJ_VNUM(obj)==atoi(subfield)) {
                  snprintf(str, slen, "%c%ld", UID_CHAR, GET_ID(obj)); /* arg given, found */
                  return;
        }
              }
              if (!obj)
                strcpy(str, ""); /* arg given, not found */
            } else { /* no arg given */
              if (c->carrying) {
                snprintf(str, slen, "%c%ld", UID_CHAR, GET_ID(c->carrying));
          } else {
                strcpy(str, "");
              }
          }
        }


      else if (!str_cmp(field, "is_killer")) {
        if (subfield && *subfield) {
          if (!str_cmp("on", subfield))
            SET_BIT_AR(PLR_FLAGS(c), PLR_KILLER);
          else if (!str_cmp("off", subfield))
            REMOVE_BIT_AR(PLR_FLAGS(c), PLR_KILLER);
        }
        if (PLR_FLAGGED(c, PLR_KILLER))
          snprintf(str, slen, "1");
        else
          snprintf(str, slen, "0");
      }

      else if (!str_cmp(field, "is_thief")) {
        if (subfield && *subfield) {
          if (!str_cmp("on", subfield))
            SET_BIT_AR(PLR_FLAGS(c), PLR_THIEF);
          else if (!str_cmp("off", subfield))
            REMOVE_BIT_AR(PLR_FLAGS(c), PLR_THIEF);
        }
        if (PLR_FLAGGED(c, PLR_THIEF))
          snprintf(str, slen, "1");
        else
          snprintf(str, slen, "0");
      }

          else if (!str_cmp(field, "int")) {
            if (subfield && *subfield) {
              int addition = atoi(subfield);
              int max = (IS_NPC(c) || GET_LEVEL(c) >= LVL_GRGOD) ? 25 : 18;
              GET_INT(c) += addition;
              if (GET_INT(c) > max) GET_INT(c) = max;
              if (GET_INT(c) < 3) GET_INT(c) = 3;
            }
            snprintf(str, slen, "%d", GET_INT(c));
          }
          break;
      	case 'l':
      	  if (!str_cmp(field, "level"))
            snprintf(str, slen, "%d", GET_LEVEL(c)); 
          break;
      	case 'm':
          if (!str_cmp(field, "maxhitp"))
            snprintf(str, slen, "%d", GET_MAX_HIT(c));

          else if (!str_cmp(field, "mana"))
            snprintf(str, slen, "%d", GET_MANA(c));

          else if (!str_cmp(field, "maxmana"))
            snprintf(str, slen, "%d", GET_MAX_MANA(c));
    
          else if (!str_cmp(field, "move"))
            snprintf(str, slen, "%d", GET_MOVE(c));
    
          else if (!str_cmp(field, "maxmove"))
            snprintf(str, slen, "%d", GET_MAX_MOVE(c));
    
          else if (!str_cmp(field, "master")) {
            if (!c->master)
              strcpy(str, "");
            else
              snprintf(str, slen, "%c%ld", UID_CHAR, GET_ID(c->master));
          }
          break;
      	case 'n':
          if (!str_cmp(field, "name"))
            snprintf(str, slen, "%s", GET_NAME(c));

          else if (!str_cmp(field, "next_in_room")) {
            if (c->next_in_room)
              snprintf(str, slen,"%c%ld",UID_CHAR, GET_ID(c->next_in_room));
            else strcpy(str,"");
          }
          break;
      	case 'p':
          if (!str_cmp(field, "prac")) {
            if (subfield && *subfield) {
              int addition = atoi(subfield);
              GET_PRACTICES(c) += addition;
            }
            snprintf(str, slen, "%d", GET_PRACTICES(c));
          }
          break;
      	case 'r':
          if (!str_cmp(field, "room"))
/* see note in dg_scripts.h */
#ifdef ACTOR_ROOM_IS_UID
            snprintf(str, slen, "%c%ld",UID_CHAR, (long) world[IN_ROOM(c)].number + ROOM_ID_BASE); 
#else  
            snprintf(str, slen, "%d", world[IN_ROOM(c)].number);
#endif      

#ifdef GET_RACE
          else if (!str_cmp(field, "race")) {
            if IS_NPC(c) {
              *str='\0';
            } else {
              sprinttype(GET_RACE(c), pc_race_types, str, slen);
            }
          }
#endif

#ifdef RIDING
      else if (!str_cmp(field, "riding"))
        if (RIDING(c))
          snprintf(str, slen, "%c%ld", UID_CHAR, GET_ID(RIDING(c)));
        else *str = '\0';
#endif

#ifdef RIDDEN_BY
      else if (!str_cmp(field, "ridden_by"))
        if (RIDDEN_BY(c))
          snprintf(str, slen, "%c%ld", UID_CHAR, GET_ID(RIDDEN_BY(c)));
        else *str = '\0';
#endif
          break;
      	case 's':
          if (!str_cmp(field, "sex"))
            snprintf(str, slen, "%s", genders[(int)GET_SEX(c)]);

          else if (!str_cmp(field, "str")) {
            if (subfield && *subfield) {
              int addition = atoi(subfield);
              int max = (IS_NPC(c) || GET_LEVEL(c) >= LVL_GRGOD) ? 25 : 18;
              GET_STR(c) += addition;
              if (GET_STR(c) > max) GET_STR(c) = max;
              if (GET_STR(c) < 3) GET_STR(c) = 3;
            }
        snprintf(str, slen, "%d", GET_STR(c));
          }

          else if (!str_cmp(field, "stradd")) {
            if (GET_STR(c) >= 18) {
              if (subfield && *subfield) {
                int addition = atoi(subfield);
                GET_ADD(c) += addition;
                if (GET_ADD(c) > 100) GET_ADD(c) = 100;
                if (GET_ADD(c) < 0) GET_ADD(c) = 0;
              }
          snprintf(str, slen, "%d", GET_ADD(c));
            }
          }
      else if (!str_cmp(field, "skill"))
        snprintf(str, slen, "%s", skill_percent(c, subfield));

          else if (!str_cmp(field, "skillset")) {
            if (!IS_NPC(c) && subfield && *subfield) {
              char skillname[MAX_INPUT_LENGTH], *amount;
              amount = one_word(subfield, skillname);
              skip_spaces(&amount);
              if (amount && *amount && is_number(amount)) {
                int skillnum = find_skill_num(skillname);
                if (skillnum > 0) {
                  int new_value = MAX(0, MIN(100, atoi(amount)));
                  SET_SKILL(c, skillnum, new_value);
                } 
              }
            }
            strcpy(str, ""); /* so the parser know we recognize 'skillset' as a field */
          }
          else if (!str_cmp(field, "saving_para")) {
            if (subfield && *subfield) {
              int addition = atoi(subfield);
              GET_SAVE(c, SAVING_PARA) += addition;
        }
            snprintf(str, slen, "%d", GET_SAVE(c, SAVING_PARA));
      }
          else if (!str_cmp(field, "saving_rod")) {
            if (subfield && *subfield) {
              int addition = atoi(subfield);
              GET_SAVE(c, SAVING_ROD) += addition;
            }
            snprintf(str, slen, "%d", GET_SAVE(c, SAVING_ROD));
          }
          else if (!str_cmp(field, "saving_petri")) {
            if (subfield && *subfield) {
              int addition = atoi(subfield);
              GET_SAVE(c, SAVING_PETRI) += addition;
          }
            snprintf(str, slen, "%d", GET_SAVE(c, SAVING_PETRI));
        }
          else if (!str_cmp(field, "saving_breath")) {
            if (subfield && *subfield) {
              int addition = atoi(subfield);
              GET_SAVE(c, SAVING_SPELL) += addition;
            }
            snprintf(str, slen, "%d", GET_SAVE(c, SAVING_BREATH));
          }
          else if (!str_cmp(field, "saving_spell")) {
            if (subfield && *subfield) {
              int addition = atoi(subfield);
              GET_SAVE(c, SAVING_SPELL) += addition;
            }
            snprintf(str, slen, "%d", GET_SAVE(c, SAVING_SPELL));
      }

          break;
      	case 't':
      	  if (!str_cmp(field, "title")) {
            if (subfield && *subfield && valid_dg_target(c, DG_ALLOW_GODS)) {
              if (GET_TITLE(c)) free(GET_TITLE(c));
                GET_TITLE(c) = strdup(subfield);          
            } 
            snprintf(str, slen, "%s", GET_TITLE(c));
          }
      	case 'v':
          if (!str_cmp(field, "vnum")) {
            if (subfield && *subfield) {
              snprintf(str, slen, "%d", IS_NPC(c) ? (int)(GET_MOB_VNUM(c) == atoi(subfield)) : -1 );
            } else {
            if (IS_NPC(c))
              snprintf(str, slen, "%d", GET_MOB_VNUM(c));
            else
            /* 
             * for compatibility with unsigned indexes 
             * - this is deprecated - use %actor.is_pc% to check 
             * instead of %actor.vnum% == -1  --Welcor 09/03
             */
              strcpy(str, "-1");
          }    
          }    

      else if (!str_cmp(field, "varexists")) {
        struct trig_var_data *remote_vd;
        snprintf(str, slen, "0");
        if (SCRIPT(c)) {
          for (remote_vd = SCRIPT(c)->global_vars; remote_vd; remote_vd = remote_vd->next) {
            if (!str_cmp(remote_vd->name, subfield)) break;
          }
          if (remote_vd) snprintf(str, slen, "1");
        }
      }

          break;
      	case 'w':
          if (!str_cmp(field, "weight"))
            snprintf(str, slen, "%d", GET_WEIGHT(c));
          else if (!str_cmp(field, "wis")) {
            if (subfield && *subfield) {
              int addition = atoi(subfield);
              int max = (IS_NPC(c) || GET_LEVEL(c) >= LVL_GRGOD) ? 25 : 18;
              GET_WIS(c) += addition;
              if (GET_WIS(c) > max) GET_WIS(c) = max;
              if (GET_WIS(c) < 3) GET_WIS(c) = 3;
            }
            snprintf(str, slen, "%d", GET_WIS(c));
          }
          break;
      } /* switch *field */	

      if (*str == '\x1') { /* no match found in switch */
        if (SCRIPT(c)) {
          for (vd = (SCRIPT(c))->global_vars; vd; vd = vd->next)
            if (!str_cmp(vd->name, field))
              break;
          if (vd)
            snprintf(str, slen, "%s", vd->value);
          else {
            *str = '\0';
            script_log("Trigger: %s, VNum %d. unknown char field: '%s'",
                       GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), field);
          }
        } else {
          *str = '\0';
          script_log("Trigger: %s, VNum %d. unknown char field: '%s'",
                     GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), field);
        }
      }
    } /* if (c) ...*/

    else if (o) {
      if (text_processed(field, subfield, vd, str, slen)) return;

      *str = '\x1';
      switch (LOWER(*field)) {
        case 'c':
          if (!str_cmp(field, "cost"))
              snprintf(str, slen, "%d", GET_OBJ_COST(o));

      else if (!str_cmp(field, "cost_per_day"))
              snprintf(str, slen, "%d", GET_OBJ_RENT(o));
  
          else if (!str_cmp(field, "carried_by")) {
            if (o->carried_by)
              snprintf(str, slen,"%c%ld",UID_CHAR, GET_ID(o->carried_by));
            else strcpy(str,"");
          }

          else if (!str_cmp(field, "contents")) {
            if (o->contains)
              snprintf(str, slen, "%c%ld", UID_CHAR, GET_ID(o->contains));
            else strcpy(str, "");
          }
          /* thanks to Jamie Nelson (Mordecai of 4 Dimensions MUD) */
          else if (!str_cmp(field, "count")) {
            int count = 0; 
            if (GET_OBJ_TYPE(o) == ITEM_CONTAINER) {
              obj_vnum snum = (subfield && is_number(subfield)) ? atoi(subfield) : NOTHING;
              struct obj_data *item;
              for (item = o->contains; item; item = item->next_content) {
                if (snum != NOTHING) {
                  if (snum == GET_OBJ_VNUM(item))
                    ++count;
                } else {
                  if (is_name(subfield, item->name))
                    ++count;
                }
              }
            }
            snprintf(str, slen, "%d", count);
          }
          break;
        case 'h':
          /* thanks to Jamie Nelson (Mordecai of 4 Dimensions MUD) */
          if (!str_cmp(field, "has_in")) { 
            bool found = FALSE;
            if (GET_OBJ_TYPE(o) == ITEM_CONTAINER) {
              struct obj_data *item;
              obj_vnum snum = (subfield && is_number(subfield)) ? atoi(subfield) : NOTHING;

              for (item = o->contains; item && !found; item = item->next_content) {
                if (snum != NOTHING) {
                  if (snum == GET_OBJ_VNUM(item)) 
                    found = TRUE;
                } else {
                  if (is_name(subfield, item->name)) 
                    found = TRUE;
                }
              }
            }
            if (found)
              snprintf(str, slen, "1");
            else
              snprintf(str, slen, "0");
          }
          break;
        case 'i':
          if (!str_cmp(field, "id"))
            snprintf(str, slen, "%ld", GET_ID(o));

          else if (!str_cmp(field, "is_inroom")) {
            if (IN_ROOM(o) != NOWHERE)
              snprintf(str, slen,"%c%ld",UID_CHAR, (long) world[IN_ROOM(o)].number + ROOM_ID_BASE); 
            else strcpy(str, "");
          }
          break;
        case 'n':
          if (!str_cmp(field, "name"))
            snprintf(str, slen, "%s",  o->name);
          
          else if (!str_cmp(field, "next_in_list")) {
            if (o->next_content)
              snprintf(str, slen,"%c%ld",UID_CHAR, GET_ID(o->next_content));
            else strcpy(str,"");
          }
          break;
        case 'r':
          if (!str_cmp(field, "room")) {
        if (obj_room(o) != NOWHERE)
              snprintf(str, slen,"%c%ld",UID_CHAR, (long)world[obj_room(o)].number + ROOM_ID_BASE);
        else strcpy(str, "");
          }
          break;
        case 's':
          if (!str_cmp(field, "shortdesc"))
            snprintf(str, slen, "%s",  o->short_description);
          break;
        case 't':
          if (!str_cmp(field, "type"))
            sprinttype(GET_OBJ_TYPE(o), item_types, str, slen);

          else if (!str_cmp(field, "timer"))
            snprintf(str, slen, "%d", GET_OBJ_TIMER(o));
          break;
        case 'v':
          if (!str_cmp(field, "vnum"))
            if (subfield && *subfield) {
              snprintf(str, slen, "%d", (int)(GET_OBJ_VNUM(o) == atoi(subfield)));
            } else {
            snprintf(str, slen, "%d", GET_OBJ_VNUM(o));
            }
          else if (!str_cmp(field, "val0"))
            snprintf(str, slen, "%d", GET_OBJ_VAL(o, 0));
      
          else if (!str_cmp(field, "val1"))
            snprintf(str, slen, "%d", GET_OBJ_VAL(o, 1));
      
          else if (!str_cmp(field, "val2"))
            snprintf(str, slen, "%d", GET_OBJ_VAL(o, 2));
      
          else if (!str_cmp(field, "val3"))
            snprintf(str, slen, "%d", GET_OBJ_VAL(o, 3));

          else if (!str_cmp(field, "val4"))
            snprintf(str, slen, "%d", GET_OBJ_VAL(o, 4));

          else if (!str_cmp(field, "val5"))
            snprintf(str, slen, "%d", GET_OBJ_VAL(o, 5));

          else if (!str_cmp(field, "val6"))
            snprintf(str, slen, "%d", GET_OBJ_VAL(o, 6));

          else if (!str_cmp(field, "val7"))
            snprintf(str, slen, "%d", GET_OBJ_VAL(o, 7));
          break;
        case 'w':
          if (!str_cmp(field, "weight"))
                  snprintf(str, slen, "%d", GET_OBJ_WEIGHT(o));
      
          else if (!str_cmp(field, "worn_by")) {
        if (o->worn_by)
          snprintf(str, slen,"%c%ld",UID_CHAR, GET_ID(o->worn_by));
        else strcpy(str,"");
          }
          break;
      } /* switch *field */
      

      if (*str == '\x1') { /* no match in switch */
        if (SCRIPT(o)) { /* check for global var */
          for (vd = (SCRIPT(o))->global_vars; vd; vd = vd->next)
            if (!str_cmp(vd->name, field))
              break;
          if (vd)
            snprintf(str, slen, "%s", vd->value);
          else {
            *str = '\0';
            script_log("Trigger: %s, VNum %d, type: %d. unknown object field: '%s'",
                       GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), type, field);
          }
        } else {
          *str = '\0';
          script_log("Trigger: %s, VNum %d, type: %d. unknown object field: '%s'",
                     GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), type, field);
        }
      }
    } /* if (o) ... */

    else if (r) {
      if (text_processed(field, subfield, vd, str, slen)) return;

      else if (!str_cmp(field, "name"))
        snprintf(str, slen, "%s",  r->name);

      else if (!str_cmp(field, "sector"))
        sprinttype(r->sector_type, sector_types, str, slen);
        
      else if (!str_cmp(field, "vnum")) {
        if (subfield && *subfield) {
          snprintf(str, slen, "%d", (int)(r->number == atoi(subfield)));
        } else {
        snprintf(str, slen,"%d",r->number); 
        }    
      } else if (!str_cmp(field, "contents")) {
        if (subfield && *subfield) {
          for (obj = r->contents; obj; obj = obj->next_content) {
            if (GET_OBJ_VNUM(obj) == atoi(subfield)) {
              /* arg given, found */
              snprintf(str, slen, "%c%ld", UID_CHAR, GET_ID(obj)); 
              return;
            }
          }
          if (!obj)
            strcpy(str, ""); /* arg given, not found */
        } else { /* no arg given */
          if (r->contents) {
            snprintf(str, slen, "%c%ld", UID_CHAR, GET_ID(r->contents));
          } else {
            strcpy(str, "");
          }
        }
      }
      
      else if (!str_cmp(field, "people")) {
        if (r->people)
          snprintf(str, slen, "%c%ld", UID_CHAR, GET_ID(r->people));
        else
          *str = '\0';
      }
      else if (!str_cmp(field, "id")) {
        room_rnum rnum = real_room(r->number);
        if (rnum != NOWHERE)
          snprintf(str, slen, "%ld", (long) world[rnum].number + ROOM_ID_BASE); 
        else
          *str = '\0';
      } 
      else if (!str_cmp(field, "weather")) {
        const char *sky_look[] = {
          "sunny",
          "cloudy",
          "rainy",
          "lightning"
        };

        if (!IS_SET_AR(r->room_flags, ROOM_INDOORS))
          snprintf(str, slen, "%s", sky_look[weather_info.sky]);
        else
          *str = '\0';
      }
      else if (!str_cmp(field, "contents")) {
        if (r->contents)
          snprintf(str, slen, "%c%ld", UID_CHAR, GET_ID(r->contents));
        else 
          *str = '\0';
      }
      else if (!str_cmp(field, "north")) {
        if (R_EXIT(r, NORTH)) {
          if (subfield && *subfield) {
            if (!str_cmp(subfield, "vnum"))
              snprintf(str, slen, "%d", GET_ROOM_VNUM(R_EXIT(r, NORTH)->to_room));
            else if (!str_cmp(subfield, "key"))
              snprintf(str, slen, "%d", R_EXIT(r, NORTH)->key);
            else if (!str_cmp(subfield, "bits"))
              sprintbit(R_EXIT(r, NORTH)->exit_info ,exit_bits, str, slen);
            else if (!str_cmp(subfield, "room")) {
              if (R_EXIT(r, NORTH)->to_room != NOWHERE)
                snprintf(str, slen, "%c%ld", UID_CHAR, (long) world[R_EXIT(r, NORTH)->to_room].number + ROOM_ID_BASE); 
              else
                *str = '\0';
            }
          } else /* no subfield - default to bits */
            sprintbit(R_EXIT(r, NORTH)->exit_info ,exit_bits, str, slen);
        } else
          *str = '\0';
      }
      else if (!str_cmp(field, "east")) {
        if (R_EXIT(r, EAST)) {
          if (subfield && *subfield) {
            if (!str_cmp(subfield, "vnum"))
              snprintf(str, slen, "%d", GET_ROOM_VNUM(R_EXIT(r, EAST)->to_room));
            else if (!str_cmp(subfield, "key"))
              snprintf(str, slen, "%d", R_EXIT(r, EAST)->key);
            else if (!str_cmp(subfield, "bits"))
              sprintbit(R_EXIT(r, EAST)->exit_info ,exit_bits, str, slen);
            else if (!str_cmp(subfield, "room")) {
              if (R_EXIT(r, EAST)->to_room != NOWHERE)
                snprintf(str, slen, "%c%ld", UID_CHAR, (long) world[R_EXIT(r, EAST)->to_room].number + ROOM_ID_BASE); 
              else
                *str = '\0';
            }
          } else /* no subfield - default to bits */
            sprintbit(R_EXIT(r, EAST)->exit_info ,exit_bits, str, slen);
        } else
          *str = '\0';
      }
      else if (!str_cmp(field, "south")) {
        if (R_EXIT(r, SOUTH)) {
          if (subfield && *subfield) {
            if (!str_cmp(subfield, "vnum"))
              snprintf(str, slen, "%d", GET_ROOM_VNUM(R_EXIT(r, SOUTH)->to_room));
            else if (!str_cmp(subfield, "key"))
              snprintf(str, slen, "%d", R_EXIT(r, SOUTH)->key);
            else if (!str_cmp(subfield, "bits"))
              sprintbit(R_EXIT(r, SOUTH)->exit_info ,exit_bits, str, slen);
            else if (!str_cmp(subfield, "room")) {
              if (R_EXIT(r, SOUTH)->to_room != NOWHERE)
                snprintf(str, slen, "%c%ld", UID_CHAR, (long) world[R_EXIT(r, SOUTH)->to_room].number + ROOM_ID_BASE); 
              else
                *str = '\0';
            }
          } else /* no subfield - default to bits */
            sprintbit(R_EXIT(r, SOUTH)->exit_info ,exit_bits, str, slen);
        } else
          *str = '\0';
      }
      else if (!str_cmp(field, "west")) {
        if (R_EXIT(r, WEST)) {
          if (subfield && *subfield) {
            if (!str_cmp(subfield, "vnum"))
              snprintf(str, slen, "%d", GET_ROOM_VNUM(R_EXIT(r, WEST)->to_room));
            else if (!str_cmp(subfield, "key"))
              snprintf(str, slen, "%d", R_EXIT(r, WEST)->key);
            else if (!str_cmp(subfield, "bits"))
              sprintbit(R_EXIT(r, WEST)->exit_info ,exit_bits, str, slen);
            else if (!str_cmp(subfield, "room")) {
              if (R_EXIT(r, WEST)->to_room != NOWHERE)
                snprintf(str, slen, "%c%ld", UID_CHAR, (long) world[R_EXIT(r, WEST)->to_room].number + ROOM_ID_BASE); 
              else
                *str = '\0';
            }
          } else /* no subfield - default to bits */
            sprintbit(R_EXIT(r, WEST)->exit_info ,exit_bits, str, slen);
        } else
          *str = '\0';
      }
      else if ((!str_cmp(field, "northwest")) || (!str_cmp(field, "nw"))) {
        if (r->dir_option[NORTHWEST])
          sprintbit(r->dir_option[NORTHWEST]->exit_info ,exit_bits, str, sizeof(str));
        else
          *str = '\0';
      }
      else if ((!str_cmp(field, "northeast")) || (!str_cmp(field, "ne"))) {
        if (r->dir_option[NORTHEAST])
          sprintbit(r->dir_option[NORTHEAST]->exit_info ,exit_bits, str, sizeof(str));
        else
          *str = '\0';
      } 
      else if ((!str_cmp(field, "southeast")) || (!str_cmp(field, "se"))) {
        if (r->dir_option[SOUTHEAST])
          sprintbit(r->dir_option[SOUTHEAST]->exit_info ,exit_bits, str, sizeof(str));
        else
          *str = '\0';
      } 
      else if ((!str_cmp(field, "southwest")) || (!str_cmp(field, "sw"))) {
        if (r->dir_option[SOUTHWEST])
          sprintbit(r->dir_option[SOUTHWEST]->exit_info ,exit_bits, str, sizeof(str));
        else
          *str = '\0';
      } 
      else if (!str_cmp(field, "in")) {
        if (r->dir_option[INDIR])
          sprintbit(r->dir_option[INDIR]->exit_info ,exit_bits, str, sizeof(str));
        else
          *str = '\0';
      } 
      else if (!str_cmp(field, "out")) {
        if (r->dir_option[OUTDIR])
          sprintbit(r->dir_option[OUTDIR]->exit_info ,exit_bits, str, sizeof(str));
        else
          *str = '\0';
      }
      else if (!str_cmp(field, "up")) {
        if (R_EXIT(r, UP)) {
          if (subfield && *subfield) {
            if (!str_cmp(subfield, "vnum"))
              snprintf(str, slen, "%d", GET_ROOM_VNUM(R_EXIT(r, UP)->to_room));
            else if (!str_cmp(subfield, "key"))
              snprintf(str, slen, "%d", R_EXIT(r, UP)->key);
            else if (!str_cmp(subfield, "bits"))
              sprintbit(R_EXIT(r, UP)->exit_info ,exit_bits, str, slen);
            else if (!str_cmp(subfield, "room")) {
              if (R_EXIT(r, UP)->to_room != NOWHERE)
                snprintf(str, slen, "%c%ld", UID_CHAR, (long) world[R_EXIT(r, UP)->to_room].number + ROOM_ID_BASE); 
              else
                *str = '\0';
            }
          } else /* no subfield - default to bits */
            sprintbit(R_EXIT(r, UP)->exit_info ,exit_bits, str, slen);
        } else
          *str = '\0';
      }
      else if (!str_cmp(field, "down")) {
        if (R_EXIT(r, DOWN)) {
          if (subfield && *subfield) {
            if (!str_cmp(subfield, "vnum"))
              snprintf(str, slen, "%d", GET_ROOM_VNUM(R_EXIT(r, DOWN)->to_room));
            else if (!str_cmp(subfield, "key"))
              snprintf(str, slen, "%d", R_EXIT(r, DOWN)->key);
            else if (!str_cmp(subfield, "bits"))
              sprintbit(R_EXIT(r, DOWN)->exit_info ,exit_bits, str, slen);
            else if (!str_cmp(subfield, "room")) {
              if (R_EXIT(r, DOWN)->to_room != NOWHERE)
                snprintf(str, slen, "%c%ld", UID_CHAR, (long) world[R_EXIT(r, DOWN)->to_room].number + ROOM_ID_BASE);
        else
          *str = '\0';
      }
          } else /* no subfield - default to bits */
            sprintbit(R_EXIT(r, DOWN)->exit_info ,exit_bits, str, slen);
        } else
          *str = '\0';
      } 
      else {
        if (SCRIPT(r)) { /* check for global var */
          for (vd = (SCRIPT(r))->global_vars; vd; vd = vd->next)
            if (!str_cmp(vd->name, field))
              break;
          if (vd)
            snprintf(str, slen, "%s", vd->value);
          else {
            *str = '\0';
            script_log("Trigger: %s, VNum %d, type: %d. unknown room field: '%s'",
                       GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), type, field);
          }
        } else {
          *str = '\0';
          script_log("Trigger: %s, VNum %d, type: %d. unknown room field: '%s'",
                     GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), type, field);
        }
      }
    }
  }
}

/* 
 * Now automatically checks if the variable has more then one field
 * in it. And if the field returns a name or a script UID or the like
 * it can recurse.
 * If you supply a value like, %actor.int.str% it wont blow up on you
 * either.
 * - Jamie Nelson 31st Oct 2003 01:03
 *
 * Now also lets subfields have variables parsed inside of them
 * so that:
 * %echo% %actor.gold(%actor.gold%)%
 * will double the actors gold every time its called.  etc...
 * - Jamie Nelson 31st Oct 2003 01:24
 */
   
/* substitutes any variables into line and returns it as buf */
void var_subst(void *go, struct script_data *sc, trig_data *trig,
               int type, char *line, char *buf)
{
  char tmp[MAX_INPUT_LENGTH], repl_str[MAX_INPUT_LENGTH];
  char *var = NULL, *field = NULL, *p = NULL;
  char tmp2[MAX_INPUT_LENGTH];
  char *subfield_p, subfield[MAX_INPUT_LENGTH];
  int left, len;
  int paren_count = 0;
  int dots = 0;

  /* skip out if no %'s */
  if (!strchr(line, '%')) {
    strcpy(buf, line);
    return;
  }
  /*lets just empty these to start with*/
  *repl_str = *tmp = *tmp2 = '\0';
  
  p = strcpy(tmp, line);
  subfield_p = subfield;
  
  left = MAX_INPUT_LENGTH - 1;

  while (*p && (left > 0)) {


    /* copy until we find the first % */
    while (*p && (*p != '%') && (left > 0)) {
      *(buf++) = *(p++);
      left--;
    }
    
    *buf = '\0';
    
    /* double % */
    if (*p && (*(++p) == '%') && (left > 0)) {
      *(buf++) = *(p++);
      *buf = '\0';
      left--;
      continue;
    }

    /* so it wasn't double %'s */
    else if (*p && (left > 0)) {
      
      /* search until end of var or beginning of field */      
      for (var = p; *p && (*p != '%') && (*p != '.'); p++);
      
      field = p;
      if (*p == '.') {
        *(p++) = '\0';
        dots = 0;
        for (field = p; *p && ((*p != '%')||(paren_count > 0) || (dots)); p++) {
          if (dots > 0) {
            *subfield_p = '\0';
            find_replacement(go, sc, trig, type, var, field, subfield, repl_str, sizeof(repl_str));
            if (*repl_str) {   
              snprintf(tmp2, sizeof(tmp2), "eval tmpvr %s", repl_str); //temp var
              process_eval(go, sc, trig, type, tmp2);
              strcpy(var, "tmpvr");
              field = p;
              dots = 0;
              continue;
            }
            dots = 0;
          } else if (*p=='(') {
            *p = '\0';
            paren_count++;
          } else if (*p==')') {
            *p = '\0';
            paren_count--;
          } else if (paren_count > 0) {
            *subfield_p++ = *p;
          } else if (*p=='.') {
            *p = '\0';
            dots++;
          } 
        } /* for (field.. */
      } /* if *p == '.' */
      
      *(p++) = '\0';
      *subfield_p = '\0';

      if (*subfield) {
        var_subst(go, sc, trig, type, subfield, tmp2);
        strcpy(subfield, tmp2);
      }

      find_replacement(go, sc, trig, type, var, field, subfield, repl_str, sizeof(repl_str));
      
      strncat(buf, repl_str, left);
      len = strlen(repl_str);
      buf += len;
      left -= len;
    } /* else if *p .. */
  } /* while *p .. */ 
}

/* returns 1 if string is all digits, else 0 */
int is_num(char *num)
{
  while (*num && (isdigit(*num) || *num=='-'))
    num++;

  if (!*num || isspace(*num))
    return 1;
  else
    return 0;
}


/* evaluates 'lhs op rhs', and copies to result */
void eval_op(char *op, char *lhs, char *rhs, char *result, void *go,
             struct script_data *sc, trig_data *trig)
{
  unsigned char *p;
  int n;

  /* strip off extra spaces at begin and end */
  while (*lhs && isspace(*lhs)) 
    lhs++;
  while (*rhs && isspace(*rhs))
    rhs++;
  
  for (p = lhs; *p; p++);
  for (--p; isspace(*p) && ((char *)p > lhs); *p-- = '\0');
  for (p = rhs; *p; p++);
  for (--p; isspace(*p) && ((char *)p > rhs); *p-- = '\0');  


  /* find the op, and figure out the value */
  if (!strcmp("||", op)) {
    if ((!*lhs || (*lhs == '0')) && (!*rhs || (*rhs == '0')))
      strcpy(result, "0");
    else
      strcpy(result, "1");
  }

  else if (!strcmp("&&", op)) {
    if (!*lhs || (*lhs == '0') || !*rhs || (*rhs == '0'))
      strcpy (result, "0");
    else
      strcpy (result, "1");
  }

  else if (!strcmp("==", op)) {
    if (is_num(lhs) && is_num(rhs))
      sprintf(result, "%d", atoi(lhs) == atoi(rhs));
    else
      sprintf(result, "%d", !str_cmp(lhs, rhs));
  }   

  else if (!strcmp("!=", op)) {
    if (is_num(lhs) && is_num(rhs))
      sprintf(result, "%d", atoi(lhs) != atoi(rhs));
    else
      sprintf(result, "%d", str_cmp(lhs, rhs));
  }   

  else if (!strcmp("<=", op)) {
    if (is_num(lhs) && is_num(rhs))
      sprintf(result, "%d", atoi(lhs) <= atoi(rhs));
    else
      sprintf(result, "%d", str_cmp(lhs, rhs) <= 0);
  }

  else if (!strcmp(">=", op)) {
    if (is_num(lhs) && is_num(rhs))
      sprintf(result, "%d", atoi(lhs) >= atoi(rhs));
    else
      sprintf(result, "%d", str_cmp(lhs, rhs) <= 0);
  }

  else if (!strcmp("<", op)) {
    if (is_num(lhs) && is_num(rhs))
      sprintf(result, "%d", atoi(lhs) < atoi(rhs));
    else
      sprintf(result, "%d", str_cmp(lhs, rhs) < 0);
  }

  else if (!strcmp(">", op)) {
    if (is_num(lhs) && is_num(rhs))
      sprintf(result, "%d", atoi(lhs) > atoi(rhs));
    else
      sprintf(result, "%d", str_cmp(lhs, rhs) > 0);
  }

  else if (!strcmp("/=", op))
    sprintf(result, "%c", str_str(lhs, rhs) ? '1' : '0');

  else if (!strcmp("*", op))
    sprintf(result, "%d", atoi(lhs) * atoi(rhs));
  
  else if (!strcmp("/", op))
    sprintf(result, "%d", (n = atoi(rhs)) ? (atoi(lhs) / n) : 0);

  else if (!strcmp("+", op)) 
    sprintf(result, "%d", atoi(lhs) + atoi(rhs));

  else if (!strcmp("-", op))
    sprintf(result, "%d", atoi(lhs) - atoi(rhs));

  else if (!strcmp("!", op)) {
    if (is_num(rhs))
      sprintf(result, "%d", !atoi(rhs));
    else
      sprintf(result, "%d", !*rhs);
  }
}


/*
 * p points to the first quote, returns the matching
 * end quote, or the last non-null char in p.
*/
char *matching_quote(char *p)
{
  for (p++; *p && (*p != '"'); p++) {
    if (*p == '\\')
      p++;
  }

  if (!*p)
    p--;

  return p;
}

/*
 * p points to the first paren.  returns a pointer to the
 * matching closing paren, or the last non-null char in p.
 */
char *matching_paren(char *p)
{
  int i;

  for (p++, i = 1; *p && i; p++) {
    if (*p == '(')
      i++;
    else if (*p == ')')
      i--;
    else if (*p == '"')
      p = matching_quote(p);
  }

  return --p;
}


/* evaluates line, and returns answer in result */
void eval_expr(char *line, char *result, void *go, struct script_data *sc,
               trig_data *trig, int type)
{
  char expr[MAX_INPUT_LENGTH], *p;

  while (*line && isspace(*line))
    line++;
  
  if (eval_lhs_op_rhs(line, result, go, sc, trig, type));

  else if (*line == '(') {
    p = strcpy(expr, line);
    p = matching_paren(expr);
    *p = '\0';
    eval_expr(expr + 1, result, go, sc, trig, type);
  }

  else
    var_subst(go, sc, trig, type, line, result);
}


/*
 * evaluates expr if it is in the form lhs op rhs, and copies
 * answer in result.  returns 1 if expr is evaluated, else 0
 */
int eval_lhs_op_rhs(char *expr, char *result, void *go, struct script_data *sc,
                    trig_data *trig, int type)
{
  char *p, *tokens[MAX_INPUT_LENGTH];
  char line[MAX_INPUT_LENGTH], lhr[MAX_INPUT_LENGTH], rhr[MAX_INPUT_LENGTH];
  int i, j;
  
  /*
   * valid operands, in order of priority
   * each must also be defined in eval_op()
   */
  static char *ops[] = {
    "||",
    "&&",
    "==",
    "!=",
    "<=",
    ">=",
    "<",
    ">",
    "/=",
    "-",
    "+",
    "/",
    "*",
    "!",
    "\n"
  };

  p = strcpy(line, expr);

  /*
   * initialize tokens, an array of pointers to locations
   * in line where the ops could possibly occur.
   */
  for (j = 0; *p; j++) {
    tokens[j] = p;
    if (*p == '(')
      p = matching_paren(p) + 1;
    else if (*p == '"')
      p = matching_quote(p) + 1;
    else if (isalnum(*p))
      for (p++; *p && (isalnum(*p) || isspace(*p)); p++);
    else
      p++;
  }
  tokens[j] = NULL;

  for (i = 0; *ops[i] != '\n'; i++)
    for (j = 0; tokens[j]; j++)
      if (!strn_cmp(ops[i], tokens[j], strlen(ops[i]))) {
        *tokens[j] = '\0';
        p = tokens[j] + strlen(ops[i]);

        eval_expr(line, lhr, go, sc, trig, type);
        eval_expr(p, rhr, go, sc, trig, type);
        eval_op(ops[i], lhr, rhr, result, go, sc, trig);

        return 1;
      }

  return 0;
}



/* returns 1 if cond is true, else 0 */
int process_if(char *cond, void *go, struct script_data *sc,
               trig_data *trig, int type)
{
  char result[MAX_INPUT_LENGTH], *p;

  eval_expr(cond, result, go, sc, trig, type);
  
  p = result;
  skip_spaces(&p);

  if (!*p || *p == '0')
    return 0;
  else
    return 1;
}


/*
 * scans for end of if-block.
 * returns the line containg 'end', or the last
 * line of the trigger if not found.
 */
struct cmdlist_element *find_end(struct cmdlist_element *cl)
{
  struct cmdlist_element *c;
  char *p;

  if (!(cl->next))
    return cl;

  for (c = cl->next; c->next; c = c->next) {
    for (p = c->cmd; *p && isspace(*p); p++);

    if (!strn_cmp("if ", p, 3))
      c = find_end(c); /* may return NULL ? */
    else if (!strn_cmp("end", p, 3))
      return c;
  }
  
  return c;
}


/*
 * searches for valid elseif, else, or end to continue execution at.
 * returns line of elseif, else, or end if found, or last line of trigger.
 */
struct cmdlist_element *find_else_end(trig_data *trig,
                                      struct cmdlist_element *cl, void *go,
                                      struct script_data *sc, int type)
{
  struct cmdlist_element *c;
  char *p;

  if (!(cl->next))
    return cl;

  for (c = cl->next; c && c->next; c = c ? c->next : NULL) {
    for (p = c->cmd; *p && isspace(*p); p++); /* skip spaces */

    if (!strn_cmp("if ", p, 3))
      c = find_end(c);

    else if (!strn_cmp("elseif ", p, 7)) {
      if (process_if(p + 7, go, sc, trig, type)) {
        GET_TRIG_DEPTH(trig)++;
        return c;
      }
    }
    
    else if (!strn_cmp("else", p, 4)) {
      GET_TRIG_DEPTH(trig)++;
      return c;
    }

    else if (!strn_cmp("end", p, 3))
      return c;
  }

  return c;
}


/* processes any 'wait' commands in a trigger */
void process_wait(void *go, trig_data *trig, int type, char *cmd,
                  struct cmdlist_element *cl)
{
  char buf[MAX_INPUT_LENGTH], *arg;
  struct wait_event_data *wait_event_obj;
  long when, hr, min, ntime;
  char c;

  arg = any_one_arg(cmd, buf);
  skip_spaces(&arg);
  
  if (!*arg) {
    script_log("Trigger: %s, VNum %d. wait w/o an arg: '%s'",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cl->cmd);
    return;
  }

  if (!strn_cmp(arg, "until ", 6)) {

    /* valid forms of time are 14:30 and 1430 */
    if (sscanf(arg, "until %ld:%ld", &hr, &min) == 2)
      min += (hr * 60);
    else
      min = (hr % 100) + ((hr / 100) * 60);

    /* calculate the pulse of the day of "until" time */
    ntime = (min * SECS_PER_MUD_HOUR * PASSES_PER_SEC) / 60;

    /* calculate pulse of day of current time */
    when = (pulse % (SECS_PER_MUD_HOUR * PASSES_PER_SEC)) +
      (time_info.hours * SECS_PER_MUD_HOUR * PASSES_PER_SEC);
    
    if (when >= ntime) /* adjust for next day */
      when = (SECS_PER_MUD_DAY * PASSES_PER_SEC) - when + ntime;
    else
      when = ntime - when;
  }

  else {
    if (sscanf(arg, "%ld %c", &when, &c) == 2) {
      if (c == 't')
        when *= PULSES_PER_MUD_HOUR;
      else if (c == 's')
        when *= PASSES_PER_SEC;
    }
  }

  CREATE(wait_event_obj, struct wait_event_data, 1);
  wait_event_obj->trigger = trig;
  wait_event_obj->go = go;
  wait_event_obj->type = type;

  GET_TRIG_WAIT(trig) = event_create(trig_wait_event, wait_event_obj, when);
  trig->curr_state = cl->next;
}


/* processes a script set command */
void process_set(struct script_data *sc, trig_data *trig, char *cmd)
{
  char arg[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH], *value;
  
  value = two_arguments(cmd, arg, name);

  skip_spaces(&value);

  if (!*name) {
    script_log("Trigger: %s, VNum %d. set w/o an arg: '%s'",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
    return;
  }

  add_var(&GET_TRIG_VARS(trig), name, value, sc ? sc->context : 0);

}

/* processes a script eval command */
void process_eval(void *go, struct script_data *sc, trig_data *trig,
                 int type, char *cmd)
{
  char arg[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH];
  char result[MAX_INPUT_LENGTH], *expr;
  
  expr = one_argument(cmd, arg); /* cut off 'eval' */
  expr = one_argument(expr, name); /* cut off name */

  skip_spaces(&expr);

  if (!*name) {
    script_log("Trigger: %s, VNum %d. eval w/o an arg: '%s'",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
    return;
  }

  eval_expr(expr, result, go, sc, trig, type);
  add_var(&GET_TRIG_VARS(trig), name, result, sc ? sc->context : 0);
}


/* script attaching a trigger to something */
void process_attach(void *go, struct script_data *sc, trig_data *trig,
                    int type, char *cmd)
{
  char arg[MAX_INPUT_LENGTH], trignum_s[MAX_INPUT_LENGTH];
  char result[MAX_INPUT_LENGTH], *id_p;
  trig_data *newtrig;
  char_data *c=NULL;
  obj_data *o=NULL;
  room_data *r=NULL;
  long trignum, id;

  id_p = two_arguments(cmd, arg, trignum_s);
  skip_spaces(&id_p);

  if (!*trignum_s) {
    script_log("Trigger: %s, VNum %d. attach w/o an arg: '%s'",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
    return;
  }

  if (!id_p || !*id_p || atoi(id_p)==0) {
    script_log("Trigger: %s, VNum %d. attach invalid id arg: '%s'",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
    return;
  }

  /* parse and locate the id specified */
  eval_expr(id_p, result, go, sc, trig, type);
  if (!(id = atoi(result))) {
    script_log("Trigger: %s, VNum %d. attach invalid id arg: '%s'",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
    return;
  }
  c = find_char(id);
  if (!c) {
    o = find_obj(id);
    if (!o) {
      r = find_room(id);
      if (!r) {
        script_log("Trigger: %s, VNum %d. attach invalid id arg: '%s'",
                GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
        return;
      }
    }
  }

  /* locate and load the trigger specified */
  trignum = real_trigger(atoi(trignum_s));
  if (trignum == NOTHING || !(newtrig=read_trigger(trignum))) {
    script_log("Trigger: %s, VNum %d. attach invalid trigger: '%s'",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), trignum_s);
    return;
  }

  if (c) {
    if (!IS_NPC(c)) {
      script_log("Trigger: %s, VNum %d. attach invalid target: '%s'",
              GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), GET_NAME(c));
      return;
    }
    if (!SCRIPT(c))
      CREATE(SCRIPT(c), struct script_data, 1);
    add_trigger(SCRIPT(c), newtrig, -1);
    return;
  }
  
  if (o) {
    if (!SCRIPT(o))
      CREATE(SCRIPT(o), struct script_data, 1);
    add_trigger(SCRIPT(o), newtrig, -1);
    return;
  }
  
  if (r) {
    if (!SCRIPT(r))
      CREATE(SCRIPT(r), struct script_data, 1);
    add_trigger(SCRIPT(r), newtrig, -1);
    return;
  }
  
}


/* script detaching a trigger from something */
void process_detach(void *go, struct script_data *sc, trig_data *trig,
                    int type, char *cmd)
{
  char arg[MAX_INPUT_LENGTH], trignum_s[MAX_INPUT_LENGTH];
  char result[MAX_INPUT_LENGTH], *id_p;
  char_data *c=NULL;
  obj_data *o=NULL;
  room_data *r=NULL;
  long id;

  id_p = two_arguments(cmd, arg, trignum_s);
  skip_spaces(&id_p);

  if (!*trignum_s) {
    script_log("Trigger: %s, VNum %d. detach w/o an arg: '%s'",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
    return;
  }

  if (!id_p || !*id_p || atoi(id_p)==0) {
    script_log("Trigger: %s, VNum %d. detach invalid id arg: '%s'",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
    return;
  }

  /* parse and locate the id specified */
  eval_expr(id_p, result, go, sc, trig, type);
  if (!(id = atoi(result))) {
    script_log("Trigger: %s, VNum %d. detach invalid id arg: '%s'",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
    return;
  }
  c = find_char(id);
  if (!c) {
    o = find_obj(id);
    if (!o) {
      r = find_room(id);
      if (!r) {
        script_log("Trigger: %s, VNum %d. detach invalid id arg: '%s'",
                GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
        return;
      }
    }
  }


  if (c && SCRIPT(c)) {
    if (!strcmp(trignum_s, "all")) {
      extract_script(c, MOB_TRIGGER);
      return;
    }
    if (remove_trigger(SCRIPT(c), trignum_s)) {
      if (!TRIGGERS(SCRIPT(c))) {
        extract_script(c, MOB_TRIGGER);
      }
    }
    return;
  }
  
  if (o && SCRIPT(o)) {
    if (!strcmp(trignum_s, "all")) {
      extract_script(o, OBJ_TRIGGER);
      return;
    }
    if (remove_trigger(SCRIPT(o), trignum_s)) {
      if (!TRIGGERS(SCRIPT(o))) {
        extract_script(o, OBJ_TRIGGER);
      }
    }
    return;
  }
  
  if (r && SCRIPT(r)) {
    if (!strcmp(trignum_s, "all")) {
      extract_script(r, WLD_TRIGGER);
      return;
    }
    if (remove_trigger(SCRIPT(r), trignum_s)) {
      if (!TRIGGERS(SCRIPT(r))) {
        extract_script(r, WLD_TRIGGER);
      }
    }
    return;
  }
  
}

struct room_data *dg_room_of_obj(struct obj_data *obj)
{
  if (IN_ROOM(obj) != NOWHERE) return &world[IN_ROOM(obj)];
  if (obj->carried_by)        return &world[IN_ROOM(obj->carried_by)];
  if (obj->worn_by)           return &world[IN_ROOM(obj->worn_by)];
  if (obj->in_obj)            return (dg_room_of_obj(obj->in_obj));
  return NULL;
}


/* create a UID variable from the id number */
void makeuid_var(void *go, struct script_data *sc, trig_data *trig,
                 int type, char *cmd)
{
  char junk[MAX_INPUT_LENGTH], varname[MAX_INPUT_LENGTH];
  char arg[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH];
  char uid[MAX_INPUT_LENGTH];
  
  *uid = '\0';
  half_chop(cmd, junk, cmd);    /* makeuid */
  half_chop(cmd, varname, cmd); /* variable name */
  half_chop(cmd, arg, cmd);     /* numerical id or 'obj' 'mob' or 'room' */
  half_chop(cmd, name, cmd);    /* if the above was obj, mob or room, this is the name */

  if (!*varname) {
    script_log("Trigger: %s, VNum %d. makeuid w/o an arg: '%s'",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);

    return;
  }

  if (!arg || !*arg) {
    script_log("Trigger: %s, VNum %d. makeuid invalid id arg: '%s'",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
    return;
  }

  if (atoi(arg)!=0) { /* easy, if you pass an id number */
    char result[MAX_INPUT_LENGTH];

    eval_expr(arg, result, go, sc, trig, type);
    snprintf(uid, sizeof(uid), "%c%s", UID_CHAR, result);
  } else { /* a lot more work without it */
    if (!name || !*name) {
      script_log("Trigger: %s, VNum %d. makeuid needs name: '%s'",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
      return;
    }
    
    if (is_abbrev(arg, "mob")) {
      struct char_data *c = NULL;
      switch (type) {
        case WLD_TRIGGER:
          c = get_char_in_room((struct room_data *)go, name);
          break;
        case OBJ_TRIGGER:
          c = get_char_near_obj((struct obj_data *)go, name);
          break;
        case MOB_TRIGGER:
          c = get_char_room_vis((struct char_data *)go, name, NULL);
          break;
      }
      if (c) 
        snprintf(uid, sizeof(uid), "%c%ld", UID_CHAR, GET_ID(c));
    } else if (is_abbrev(arg, "obj")) {
      struct obj_data *o = NULL;
      switch (type) {
        case WLD_TRIGGER:
          o = get_obj_in_room((struct room_data *)go, name);
          break;
        case OBJ_TRIGGER:
          o = get_obj_near_obj((struct obj_data *)go, name);
          break;
        case MOB_TRIGGER:
          if ((o = get_obj_in_list_vis((struct char_data *)go, name, NULL, 
                    ((struct char_data *)go)->carrying)) == NULL)
            o = get_obj_in_list_vis((struct char_data *)go, name, NULL, 
                    world[IN_ROOM((struct char_data *)go)].contents);
          break;
      }
      if (o)
        snprintf(uid, sizeof(uid), "%c%ld", UID_CHAR, GET_ID(o));
    } else if (is_abbrev(arg, "room")) {
      room_rnum r = NOWHERE;
      switch (type) {
        case WLD_TRIGGER:
          r = real_room(((struct room_data *) go)->number);
          break;
        case OBJ_TRIGGER:
          r = obj_room((struct obj_data *)go);
          break;
        case MOB_TRIGGER:
          r = IN_ROOM((struct char_data *)go);
          break;
      }
      if (r != NOWHERE) 
        snprintf(uid, sizeof(uid), "%c%ld", UID_CHAR, (long)world[r].number+ROOM_ID_BASE);
    } else {
      script_log("Trigger: %s, VNum %d. makeuid syntax error: '%s'",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
  
      return;
    }
  }
  if (*uid)
    add_var(&GET_TRIG_VARS(trig), varname, uid, sc ? sc->context : 0);
}

/*
 * processes a script return command.
 * returns the new value for the script to return.
 */
int process_return(trig_data *trig, char *cmd)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  
  two_arguments(cmd, arg1, arg2);
  
  if (!*arg2) {
    script_log("Trigger: %s, VNum %d. return w/o an arg: '%s'",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);

    return 1;
  }

  return atoi(arg2);
}


/*
 * removes a variable from the global vars of sc,
 * or the local vars of trig if not found in global list.
 */
void process_unset(struct script_data *sc, trig_data *trig, char *cmd)
{
  char arg[MAX_INPUT_LENGTH], *var;

  var = any_one_arg(cmd, arg);

  skip_spaces(&var);

  if (!*var) {
    script_log("Trigger: %s, VNum %d. unset w/o an arg: '%s'",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
    return;
  }

  if (!remove_var(&(sc->global_vars), var))
    remove_var(&GET_TRIG_VARS(trig), var);
}


/*
 * copy a locally owned variable to the globals of another script
 *     'remote <variable_name> <uid>'
 */
void process_remote(struct script_data *sc, trig_data *trig, char *cmd)
{
  struct trig_var_data *vd;
  struct script_data *sc_remote=NULL;
  char *line, *var, *uid_p;
  char arg[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
  long uid, context;
  room_data *room;
  char_data *mob;
  obj_data *obj;

  line = any_one_arg(cmd, arg);
  two_arguments(line, buf, buf2);
  var = buf;
  uid_p = buf2;
  skip_spaces(&var);
  skip_spaces(&uid_p);
  

  if (!*buf || !*buf2) {
    script_log("Trigger: %s, VNum %d. remote: invalid arguments '%s'",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
    return;
  }

  /* find the locally owned variable */
  for (vd = GET_TRIG_VARS(trig); vd; vd = vd->next)
    if (!str_cmp(vd->name, buf))
      break;

  if (!vd)
    for (vd = sc->global_vars; vd; vd = vd->next)
      if (!str_cmp(vd->name, var) &&
          (vd->context==0 || vd->context==sc->context))
        break; 

  if (!vd) {
    script_log("Trigger: %s, VNum %d. local var '%s' not found in remote call",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), buf);
    return;
  }    

  /* find the target script from the uid number */
  uid = atoi(buf2);
  if (uid<=0) {
    script_log("Trigger: %s, VNum %d. remote: illegal uid '%s'",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), buf2);
    return;
  }

  /* for all but PC's, context comes from the existing context. */
  /* for PC's, context is 0 (global) */
  context = vd->context;

  if ((room = find_room(uid))) {
    sc_remote = SCRIPT(room);
  } else if ((mob = find_char(uid))) {
    sc_remote = SCRIPT(mob);
    if (!IS_NPC(mob)) context = 0;
  } else if ((obj = find_obj(uid))) {
    sc_remote = SCRIPT(obj);
  } else {
    script_log("Trigger: %s, VNum %d. remote: uid '%ld' invalid",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), uid);
    return;
  }

  if (sc_remote==NULL) return; /* no script to assign */

  add_var(&(sc_remote->global_vars), vd->name, vd->value, context);
}


/*
 * command-line interface to rdelete
 * named vdelete so people didn't think it was to delete rooms
 */
ACMD(do_vdelete)
{
  struct trig_var_data *vd, *vd_prev=NULL;
  struct script_data *sc_remote=NULL;
  char *var, *uid_p;
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
  long uid, context;
  room_data *room;
  char_data *mob;
  obj_data *obj;

  argument = two_arguments(argument, buf, buf2);
  var = buf;
  uid_p = buf2;
  skip_spaces(&var);
  skip_spaces(&uid_p);


  if (!*buf || !*buf2) {
    send_to_char(ch, "Usage: vdelete <variablename> <id>\r\n");
    return;
  }


  /* find the target script from the uid number */
  uid = atoi(buf2);
  if (uid<=0) {
    send_to_char(ch, "vdelete: illegal id specified.\r\n");
    return;
  }


  if ((room = find_room(uid))) {
    sc_remote = SCRIPT(room);
  } else if ((mob = find_char(uid))) {
    sc_remote = SCRIPT(mob);
    if (!IS_NPC(mob)) context = 0;
  } else if ((obj = find_obj(uid))) {
    sc_remote = SCRIPT(obj);
  } else {
    send_to_char(ch, "vdelete: cannot resolve specified id.\r\n");
    return;
  }

  if (sc_remote==NULL) {
    send_to_char(ch, "That id represents no global variables.(1)\r\n");
    return;
  }

  if (sc_remote->global_vars==NULL) {
    send_to_char(ch, "That id represents no global variables.(2)\r\n");
    return;
  }

  /* find the global */
  for (vd = sc_remote->global_vars; vd; vd_prev = vd, vd = vd->next)
    if (!str_cmp(vd->name, var))
      break;

  if (!vd) {
    send_to_char(ch, "That variable cannot be located.\r\n");
    return;
  }

  /* ok, delete the variable */
  if (vd_prev) vd_prev->next = vd->next;
  else sc_remote->global_vars = vd->next;

  /* and free up the space */
  free(vd->value);
  free(vd->name);
  free(vd);

  send_to_char(ch, "Deleted.\r\n");
}

/*
 * delete a variable from the globals of another script
 *     'rdelete <variable_name> <uid>'
 */
void process_rdelete(struct script_data *sc, trig_data *trig, char *cmd)
{
  struct trig_var_data *vd, *vd_prev=NULL;
  struct script_data *sc_remote=NULL;
  char *line, *var, *uid_p;
  char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
  long uid, context;
  room_data *room;
  char_data *mob;
  obj_data *obj;

  line = any_one_arg(cmd, arg);
  two_arguments(line, buf, buf2);
  var = buf;
  uid_p = buf2;
  skip_spaces(&var);
  skip_spaces(&uid_p);


  if (!*buf || !*buf2) {
    script_log("Trigger: %s, VNum %d. rdelete: invalid arguments '%s'",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
    return;
  }


  /* find the target script from the uid number */
  uid = atoi(buf2);
  if (uid<=0) {
    script_log("Trigger: %s, VNum %d. rdelete: illegal uid '%s'",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), buf2);
    return;
  }


  if ((room = find_room(uid))) {
    sc_remote = SCRIPT(room);
  } else if ((mob = find_char(uid))) {
    sc_remote = SCRIPT(mob);
    if (!IS_NPC(mob)) context = 0;
  } else if ((obj = find_obj(uid))) {
    sc_remote = SCRIPT(obj);
  } else {
    script_log("Trigger: %s, VNum %d. remote: uid '%ld' invalid",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), uid);
    return;
  }

  if (sc_remote==NULL) return; /* no script to delete a trigger from */
  if (sc_remote->global_vars==NULL) return; /* no script globals */

  /* find the global */
  for (vd = sc_remote->global_vars; vd; vd_prev = vd, vd = vd->next)
    if (!str_cmp(vd->name, var) &&
        (vd->context==0 || vd->context==sc->context))
      break;

  if (!vd) return; /* the variable doesn't exist, or is the wrong context */

  /* ok, delete the variable */
  if (vd_prev) vd_prev->next = vd->next;
  else sc_remote->global_vars = vd->next;

  /* and free up the space */
  free(vd->value);
  free(vd->name);
  free(vd);
}


/*
 * makes a local variable into a global variable
 */
void process_global(struct script_data *sc, trig_data *trig, char *cmd, long id)
{
  struct trig_var_data *vd;
  char arg[MAX_INPUT_LENGTH], *var;

  var = any_one_arg(cmd, arg);

  skip_spaces(&var);

  if (!*var) {
    script_log("Trigger: %s, VNum %d. global w/o an arg: '%s'",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
    return;
  }

  for (vd = GET_TRIG_VARS(trig); vd; vd = vd->next)
    if (!str_cmp(vd->name, var))
      break;

  if (!vd) {
    script_log("Trigger: %s, VNum %d. local var '%s' not found in global call",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), var);
    return;
  }    

  add_var(&(sc->global_vars), vd->name, vd->value, id);
  remove_var(&GET_TRIG_VARS(trig), vd->name);
}


/* set the current context for a script */
void process_context(struct script_data *sc, trig_data *trig, char *cmd)
{
  char arg[MAX_INPUT_LENGTH], *var;
  
  var = any_one_arg(cmd, arg);

  skip_spaces(&var);

  if (!*var) {
    script_log("Trigger: %s, VNum %d. context w/o an arg: '%s'",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
    return;
  }

  sc->context = atol(var);
}

void extract_value(struct script_data *sc, trig_data *trig, char *cmd)
{
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
  char *buf3;
  char to[128];
  int num;

  buf3 = any_one_arg(cmd, buf);
  half_chop(buf3, buf2, buf);
  strcpy(to, buf2);

  num = atoi(buf);
  if (num < 1) {
    script_log("extract number < 1!");
    return;
  }

  half_chop(buf, buf3, buf2);

  while (num>0) {
    half_chop(buf2, buf, buf2);
    num--;
  }

  add_var(&GET_TRIG_VARS(trig), to, buf, sc ? sc->context : 0);
}

/*  This is the core driver for scripts. */
int script_driver(void *go_adress, trig_data *trig, int type, int mode)
// int script_driver(union script_driver_data_u *sdd, trig_data *trig, int type, int mode)
{
  static int depth = 0;
  int ret_val = 1;
  struct cmdlist_element *cl;
  char cmd[MAX_INPUT_LENGTH], *p;
  struct script_data *sc = 0;
  struct cmdlist_element *temp;
  unsigned long loops = 0;
  void *go = NULL;

  void obj_command_interpreter(obj_data *obj, char *argument);
  void wld_command_interpreter(struct room_data *room, char *argument);

  if (depth > MAX_SCRIPT_DEPTH) {
    script_log("Triggers recursed beyond maximum allowed depth.");
    return ret_val;
  }

  depth++;

  switch (type) {
  case MOB_TRIGGER:
    go = *(char_data **)go_adress;
    sc = SCRIPT((char_data *) go);
    break;
  case OBJ_TRIGGER:
    go = *(obj_data **)go_adress;
    sc = SCRIPT((obj_data *) go);
    break;
  case WLD_TRIGGER:
    go = *(room_data **)go_adress;
    sc = SCRIPT((room_data *) go);
    break;
  }

  if (mode == TRIG_NEW) {
    GET_TRIG_DEPTH(trig) = 1;
    GET_TRIG_LOOPS(trig) = 0;
    sc->context = 0;
  }

  dg_owner_purged = 0;
  
  for (cl = (mode == TRIG_NEW) ? trig->cmdlist : trig->curr_state;
       cl && GET_TRIG_DEPTH(trig); cl = cl ? cl->next : NULL) {
    for (p = cl->cmd; *p && isspace(*p); p++);

    if (*p == '*') /* comment */
      continue;

    else if (!strn_cmp(p, "if ", 3)) {
      if (process_if(p + 3, go, sc, trig, type))
        GET_TRIG_DEPTH(trig)++;
      else
        cl = find_else_end(trig, cl, go, sc, type);
    }
    
    else if (!strn_cmp("elseif ", p, 7) ||
             !strn_cmp("else", p, 4)) {
      /*
       * if not in an if-block, ignore the extra 'else[if]' and warn about it
       */
      if (GET_TRIG_DEPTH(trig) == 1) { 
        script_log("Trigger VNum %d has 'else' without 'if'.", 
                   GET_TRIG_VNUM(trig));
        continue; 
      }
      cl = find_end(cl);
      GET_TRIG_DEPTH(trig)--;
    } else if (!strn_cmp("while ", p, 6)) {
      temp = find_done(cl);  
      if (!temp) {
        script_log("Trigger VNum %d has 'while' without 'done'.", 
                   GET_TRIG_VNUM(trig));
        return ret_val;
      }
      if (process_if(p + 6, go, sc, trig, type)) {
         temp->original = cl;
      } else {
         cl = temp;
         loops = 0;
      }
    } else if (!strn_cmp("switch ", p, 7)) {
      cl = find_case(trig, cl, go, sc, type, p + 7);
    } else if (!strn_cmp("end", p, 3)) {   
      /*
       * if not in an if-block, ignore the extra 'end' and warn about it.
       */
      if (GET_TRIG_DEPTH(trig) == 1) { 
        script_log("Trigger VNum %d has 'end' without 'if'.", 
                   GET_TRIG_VNUM(trig));
        continue; 
      }
      GET_TRIG_DEPTH(trig)--;
    } else if (!strn_cmp("done", p, 4)) {
      /* if in a while loop, cl->original is non-NULL */
      if (cl->original) {
        char *orig_cmd = cl->original->cmd;
        while (*orig_cmd && isspace(*orig_cmd)) orig_cmd++;
        if (cl->original && process_if(orig_cmd + 6, go, sc, trig,
            type)) {
          cl = cl->original;
          loops++;   
          GET_TRIG_LOOPS(trig)++;
          if (loops == 30) {
            process_wait(go, trig, type, "wait 1", cl);
             depth--;
            return ret_val;
          }
          if (GET_TRIG_LOOPS(trig) >= 100) {
          script_log("Trigger VNum %d has looped 100 times!!!",
              GET_TRIG_VNUM(trig));
            break;
          }
        } else {
         /* if we're falling through a switch statement, this ends it. */
        }
      }
    } else if (!strn_cmp("break", p, 5)) {
      cl = find_done(cl);
    } else if (!strn_cmp("case", p, 4)) { 
       /* Do nothing, this allows multiple cases to a single instance */
    }
    
      
    else {
      
      var_subst(go, sc, trig, type, p, cmd);

      if (!strn_cmp(cmd, "eval ", 5))
        process_eval(go, sc, trig, type, cmd);

      else if (!strn_cmp(cmd, "nop ", 4)); /* nop: do nothing */

      else if (!strn_cmp(cmd, "extract ", 8))
        extract_value(sc, trig, cmd);

      else if (!strn_cmp(cmd, "makeuid ", 8))
        makeuid_var(go, sc, trig, type, cmd);

      else if (!strn_cmp(cmd, "halt", 4))
        break;

      else if (!strn_cmp(cmd, "dg_cast ", 8))
        do_dg_cast(go, sc, trig, type, cmd);

      else if (!strn_cmp(cmd, "dg_affect ", 10))
        do_dg_affect(go, sc, trig, type, cmd);

      else if (!strn_cmp(cmd, "global ", 7))
        process_global(sc, trig, cmd, sc->context);

      else if (!strn_cmp(cmd, "context ", 8))
        process_context(sc, trig, cmd);

      else if (!strn_cmp(cmd, "remote ", 7))
        process_remote(sc, trig, cmd);

      else if (!strn_cmp(cmd, "rdelete ", 8))
        process_rdelete(sc, trig, cmd);

      else if (!strn_cmp(cmd, "return ", 7))
        ret_val = process_return(trig, cmd);
      
      else if (!strn_cmp(cmd, "set ", 4))
        process_set(sc, trig, cmd);
      
      else if (!strn_cmp(cmd, "unset ", 6))
        process_unset(sc, trig, cmd);
      
      else if (!strn_cmp(cmd, "wait ", 5)) {
        process_wait(go, trig, type, cmd, cl);
        depth--;
        return ret_val;
      }

      else if (!strn_cmp(cmd, "attach ", 7))
        process_attach(go, sc, trig, type, cmd);
      
      else if (!strn_cmp(cmd, "detach ", 7))
        process_detach(go, sc, trig, type, cmd);
      
      else if (!strn_cmp(cmd, "version", 7))
        mudlog(NRM, LVL_GOD, TRUE, "%s", DG_SCRIPT_VERSION);
      
      else {
        switch (type) {
        case MOB_TRIGGER:
          command_interpreter((char_data *) go, cmd);
          break;
        case OBJ_TRIGGER:
          obj_command_interpreter((obj_data *) go, cmd);
          break;
        case WLD_TRIGGER:
          wld_command_interpreter((struct room_data *) go, cmd);
          break;
        }
        if (dg_owner_purged) {
          depth--;
          if (type == OBJ_TRIGGER) 
            *(obj_data **)go_adress = NULL;
          return ret_val;
        }
      }

    }
  }

  switch (type) { /* the script may have been detached */
    case MOB_TRIGGER:    sc = SCRIPT((char_data *) go);           break;
    case OBJ_TRIGGER:    sc = SCRIPT((obj_data *) go);            break;
    case WLD_TRIGGER:    sc = SCRIPT((room_data *) go);    break;
  }
  if (sc)
    free_varlist(GET_TRIG_VARS(trig));
  GET_TRIG_VARS(trig) = NULL;
  GET_TRIG_DEPTH(trig) = 0;

  depth--;
  return ret_val;
}

/* returns the real number of the trigger with given virtual number */
int real_trigger(int vnum)
{
  int bot = 0, mid, top = top_of_trigt-1, last_top;

  /* perform binary search on trigger-table */
  for (;;) {
    last_top = top;
    mid = (bot + top) / 2;

  /* Thanks to Derek Fisk for fixing this loop */
    if (bot > top)
      return (NOTHING);
    if (trig_index[mid]->vnum == vnum)
      return (mid);
    if (trig_index[mid]->vnum > vnum)
      top = mid - 1;
    else
      bot = mid + 1;

    if (top > last_top)
      return NOWHERE;
  }
}

ACMD(do_tstat)
{
  int rnum;
  char str[MAX_INPUT_LENGTH];

  half_chop(argument, str, argument);
  if (*str) {
    rnum = real_trigger(atoi(str));
    if (rnum == NOTHING) {
      send_to_char(ch, "That vnum does not exist.\r\n");
      return;
    }

    do_stat_trigger(ch, trig_index[rnum]->proto);
  } else 
    send_to_char(ch, "Usage: tstat <vnum>\r\n");
}

/*
* scans for a case/default instance
* returns the line containg the correct case instance, or the last
* line of the trigger if not found.
*/
struct cmdlist_element *
find_case(struct trig_data *trig, struct cmdlist_element *cl,
          void *go, struct script_data *sc, int type, char *cond)
{
  char result[MAX_INPUT_LENGTH];
  struct cmdlist_element *c;
  char *p, *buf;

  eval_expr(cond, result, go, sc, trig, type);
  
  if (!(cl->next))
    return cl;  
        
  for (c = cl->next; c->next; c = c->next) {
    for (p = c->cmd; *p && isspace(*p); p++);
      
    if (!strn_cmp("while ", p, 6) || !strn_cmp("switch", p, 6))
      c = find_done(c);
    else if (!strn_cmp("case ", p, 5)) {
      buf = (char*)malloc(MAX_STRING_LENGTH);
#if 0 /* the original implementation */
      sprintf(buf, "(%s) == (%s)", cond, p + 5);
      if (process_if(buf, go, sc, trig, type)) {
#else /* new! improved! bug fixed! */
      eval_op("==", result, p + 5, buf, go, sc, trig);
      if (*buf && *buf!='0') {
#endif
        free(buf);
        return c;
      }
      free(buf);
    } else if (!strn_cmp("default", p, 7))
      return c;
    else if (!strn_cmp("done", p, 3))   
     return c;
  }
  return c;
}        
       
/*
* scans for end of while/switch-blocks.   
* returns the line containg 'end', or the last
* line of the trigger if not found.     
* Malformed scripts may cause NULL to be returned.
*/
struct cmdlist_element *
find_done(struct cmdlist_element *cl)
{
  struct cmdlist_element *c;
  char *p;
  
  if (!cl || !(cl->next))
    return cl;

  for (c = cl->next; c && c->next; c = c->next) {
    for (p = c->cmd; *p && isspace(*p); p++);

    if (!strn_cmp("while ", p, 6) || !strn_cmp("switch ", p, 7))
      c = find_done(c);
    else if (!strn_cmp("done", p, 3))
      return c;
  }
    
  return c;
}


/* read a line in from a file, return the number of chars read */
int fgetline(FILE *file, char *p)
{
  int count = 0;

  do {
    *p = fgetc(file);
    if (*p != '\n' && !feof(file)) {
      p++;
      count++;
    }
  } while (*p != '\n' && !feof(file));

  if (*p == '\n') *p = '\0';

  return count;
}


/* load in a character's saved variables */
void read_saved_vars(struct char_data *ch)
{
  FILE *file;
  long context;
  char fn[127];
  char input_line[1024], *temp, *p;
  char varname[32];
  char context_str[16];

  /* create the space for the script structure which holds the vars */
  /* We need to do this first, because later calls to 'remote' will need */
  /* a script already assigned. */
  CREATE(SCRIPT(ch), struct script_data, 1);

  /* find the file that holds the saved variables and open it*/
  get_filename(fn, sizeof(fn), SCRIPT_VARS_FILE, GET_NAME(ch));
  file = fopen(fn,"r");

  /* if we failed to open the file, return */
  if( !file ) {
    log("%s had no variable file", GET_NAME(ch)); 
    return;
  }
  /* walk through each line in the file parsing variables */
  do {
    if (get_line(file, input_line)>0) {
      p = temp = strdup(input_line);
      temp = any_one_arg(temp, varname);
      temp = any_one_arg(temp, context_str);
      skip_spaces(&temp); /* temp now points to the rest of the line */

      context = atol(context_str);
      add_var(&(SCRIPT(ch)->global_vars), varname, temp, context);
      free(p); /* plug memory hole */
    }
  } while( !feof(file) );

  /* close the file and return */
  fclose(file);
}

/* save a characters variables out to disk */
void save_char_vars(struct char_data *ch)
{
  FILE *file;
  char fn[127];
  struct trig_var_data *vars;

  /* immediate return if no script (and therefore no variables) structure */
  /* has been created. this will happen when the player is logging in */
  if (SCRIPT(ch) == NULL) return;

  /* we should never be called for an NPC, but just in case... */
  if (IS_NPC(ch)) return;

  get_filename(fn, sizeof(fn), SCRIPT_VARS_FILE, GET_NAME(ch));
  unlink(fn);

  /* make sure this char has global variables to save */
  if (ch->script->global_vars == NULL) return;
  vars = ch->script->global_vars;

  file = fopen(fn,"wt");
  if (!file) {
    mudlog( NRM, LVL_GOD, TRUE, 
            "SYSERR: Could not open player variable file %s for writing.:%s",
            fn, strerror(errno));
    return;
  }
  /* note that currently, context will always be zero. this may change */
  /* in the future */
  while (vars) {
    if (*vars->name != '-') /* don't save if it begins with - */
      fprintf(file, "%s %ld %s\n", vars->name, vars->context, vars->value);
    vars = vars->next;
  }

  fclose(file);
}

/* find_char() helpers */

// Must be power of 2
#define BUCKET_COUNT 64
// to recognize an empty bucket
#define UID_OUT_OF_RANGE 1000000000

struct lookup_table_t {
  long uid;
  void * c; 
  struct lookup_table_t *next;
};
struct lookup_table_t lookup_table[BUCKET_COUNT];

void init_lookup_table(void)
{
  int i;
  for (i = 0; i < BUCKET_COUNT; i++) {
    lookup_table[i].uid  = UID_OUT_OF_RANGE;
    lookup_table[i].c    = NULL;
    lookup_table[i].next = NULL;
  }
}

struct char_data *find_char_by_uid_in_lookup_table(long uid)
{
  int bucket = (int) (uid & (BUCKET_COUNT - 1));
  struct lookup_table_t *lt = &lookup_table[bucket];

  for (;lt && lt->uid != uid ; lt = lt->next) ;

  if (lt)
    return (struct char_data *)(lt->c);

  log("find_char_by_uid_in_lookup_table : No entity with number %ld in lookup table", uid);
  return NULL;
}

struct obj_data *find_obj_by_uid_in_lookup_table(long uid)
{
  int bucket = (int) (uid & (BUCKET_COUNT - 1));
  struct lookup_table_t *lt = &lookup_table[bucket];

  for (;lt && lt->uid != uid ; lt = lt->next) ;

  if (lt)
    return (struct obj_data *)(lt->c);

  log("find_obj_by_uid_in_lookup_table : No entity with number %ld in lookup table", uid);
  return NULL;
}

long max_id = 1;

void add_to_lookup_table(long uid, void *c)
{
  int bucket = (int) (uid & (BUCKET_COUNT - 1));
  struct lookup_table_t *lt = &lookup_table[bucket];

  for (;lt->next; lt = lt->next)
    if (lt->c == c && lt->uid == uid) {
      log ("Add_to_lookup failed. Already there. (uid = %ld)", uid);
      return;
    }

  CREATE(lt->next, struct lookup_table_t, 1);
  lt->next->uid = uid;
  lt->next->c = c;
}

void remove_from_lookup_table(long uid)
{
  int bucket = (int) (uid & (BUCKET_COUNT - 1));
  struct lookup_table_t *lt = &lookup_table[bucket], *flt = NULL;

  /*
   * This is not supposed to happen. UID 0 is not used.
   * However, while I'm debugging the issue, let's just return right away.
   * 
   * Welcor 02/04
   */
  if (uid == 0)
    return;

  for (;lt;lt = lt->next)
    if (lt->uid == uid)
      flt = lt;

  if (flt) {
    for (lt = &lookup_table[bucket];lt->next != flt;lt = lt->next)
      ;
    lt->next = flt->next;
    free(flt);
    return;
  }

  log("remove_from_lookup. UID %ld not found.", uid);
}

