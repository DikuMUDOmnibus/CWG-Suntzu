#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"

const char *race_names[] = {
  "human",
  "elf",
  "gnome",
  "dwarf",
  "half elf",
  "halfling",
  "drow",
  "half orc",
  "animal",
  "construct",
  "demon",
  "dragon",
  "fish",
  "giant",
  "goblin",
  "insect",
  "orc",
  "snake",
  "troll",
  "minotaur",
  "kobold",
  "mindflayer",
  "warhost",
  "faerie",
  "\n"
};

const char *race_abbrevs[] = {
  "Hum",
  "Elf",
  "Gno",
  "Dwa",
  "HEl",
  "Hlf",
  "Drw",
  "HOr",
  "Ani",
  "Con",
  "Dem",
  "Drg",
  "Fsh",
  "Gnt",
  "Gob",
  "Ict",
  "Orc",
  "Snk",
  "Trl",
  "Min",
  "Kob",
  "Mnd",
  "War",
  "Fae",
  "\n"
};

const char *pc_race_types[] = {
  "Human",
  "Elf",
  "Gnome",
  "Dwarf",
  "Half Elf",
  "Halfling",
  "Drow Elf",
  "Half Orc",
  "Animal",
  "Construct",
  "Demon",
  "Dragon",
  "Fish",
  "Giant",
  "Goblin",
  "Insect",
  "Orc",
  "Snake",
  "Troll",
  "Minotaur",
  "Kobold",
  "Mindflayer",
  "Warhost",
  "Faerie",
  "\n"
};

#define Y   TRUE
#define N   FALSE

/* Original Race/Gender Breakout */
int race_ok_gender[NUM_SEX][NUM_RACES] = {
/*        H, E, G, D, H, H, D, A, C, D, D, F, G, G, I, O, S, T, H, M, K, M, W, F */
/* N */ { N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N },
/* M */ { Y, Y, Y, Y, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N },
/* F */ { Y, Y, Y, Y, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N }
};

const char *race_display[NUM_RACES] = {
  "@B1@W) Human\r\n",
  "@B2@W) @GElf\r\n",
  "@B3@W) @MGnome\r\n",
  "@B4@W) @BDwarf\r\n",
  "@B5@W) @RHalf Elf\r\n",
  "@B6@W) @CHalfling\r\n",
  "@B7@W) @YDrow Elf\r\n",
  "@B8@W) Half Orc\r\n",
  "@B9@W) @GAnimal\r\n",
  "@B10@W) @MConstruct\r\n",
  "@B11@W) @BDemon\r\n",
  "@B12@W) @RDragon\r\n",
  "@B13@W) @CFish\r\n",
  "@B14@W) @YGiant\r\n",
  "@B15@W) Goblin\r\n",
  "@B16@W) @GInsect\r\n",
  "@B17@W) @MOrc\r\n",
  "@B18@W) @BSnake\r\n",
  "@B19@W) @RTroll\r\n",
  "@B20@W) @CMinotaur\r\n",
  "@B21@W) @YKobold\r\n",
  "@B22@W) Mindflayer\r\n",
  "@B23@W) @GWarhost\r\n",
  "@B24@W) @MFaerie\r\n",
};

/*
 * The code to interpret a race letter (used in interpreter.c when a
 * new character is selecting a race).
 */
int parse_race(struct char_data *ch, int arg)
{
  int race = RACE_UNDEFINED;

  switch (arg) {
  case 1:  race = RACE_HUMAN      ; break;
  case 2:  race = RACE_ELF        ; break;
  case 3:  race = RACE_GNOME      ; break;
  case 4:  race = RACE_DWARF      ; break;
  case 5:  race = RACE_HALF_ELF   ; break;
  case 6:  race = RACE_HALFLING   ; break;
  case 7:  race = RACE_DROW_ELF   ; break;
  case 8:  race = RACE_HALF_ORC   ; break;
  case 9:  race = RACE_ANIMAL     ; break;
  case 10: race = RACE_CONSTRUCT  ; break;
  case 11: race = RACE_DEMON      ; break;
  case 12: race = RACE_DRAGON     ; break;
  case 13: race = RACE_FISH       ; break;
  case 14: race = RACE_GIANT      ; break;
  case 15: race = RACE_GOBLIN     ; break;
  case 16: race = RACE_INSECT     ; break;
  case 17: race = RACE_ORC        ; break;
  case 18: race = RACE_SNAKE      ; break;
  case 19: race = RACE_TROLL      ; break;
  case 20: race = RACE_MINOTAUR   ; break;
  case 21: race = RACE_KOBOLD     ; break;
  case 22: race = RACE_MINDFLAYER ; break;
  case 23: race = RACE_WARHOST    ; break;
  case 24: race = RACE_FAERIE     ; break;
  default: race = RACE_UNDEFINED  ; break;
  }
  if (race >= 0 && race < NUM_RACES)
    if (!race_ok_gender[(int)GET_SEX(ch)][race])
      race = RACE_UNDEFINED;

  return (race);
}

void racial_ability_modifiers(struct char_data *ch)
{
  switch (GET_RACE(ch)) {
    case RACE_HUMAN:
      break;
    case RACE_ELF:
      ch->real_abils.dex += 1;
      ch->real_abils.con -= 1;
      break;
    case RACE_GNOME:
      ch->real_abils.intel += 1;
      ch->real_abils.wis -= 1;
      break;
    case RACE_DWARF:
      ch->real_abils.con += 1;
      ch->real_abils.cha -= 1;
      break;
    default:
      break;
  }
}


void set_height_by_race(struct char_data *ch)
{
  if (GET_SEX(ch) == SEX_MALE) {
    if (IS_DWARF(ch))
      GET_HEIGHT(ch) = 43 + dice(1, 10);
    else if (IS_ELF(ch))
      GET_HEIGHT(ch) = 55 + dice(1, 10);
    else if (IS_GNOME(ch))
      GET_HEIGHT(ch) = 38 + dice(1, 6);
    else /* if (IS_HUMAN(ch)) */
      GET_HEIGHT(ch) = 60 + dice(2, 10);
  } else /* if (IS_FEMALE(ch)) */ {
    if (IS_DWARF(ch))
      GET_HEIGHT(ch) = 41 + dice(1, 10);
    else if (IS_ELF(ch))
      GET_HEIGHT(ch) = 50 + dice(1, 10);
    else if (IS_GNOME(ch))
      GET_HEIGHT(ch) = 36 + dice(1, 6);
    else /* if (IS_HUMAN(ch)) */
      GET_HEIGHT(ch) = 59 + dice(2, 10);
  }
  return;
}

void set_weight_by_race(struct char_data *ch)
{
  if (GET_SEX(ch) == SEX_MALE) {
    if (IS_DWARF(ch))
      GET_WEIGHT(ch) = 130 + dice(4, 10);
    else if (IS_ELF(ch))
      GET_WEIGHT(ch) = 90 + dice(3, 10);
    else if (IS_GNOME(ch))
      GET_WEIGHT(ch) = 72 + dice(5, 4);
    else /* if (IS_HUMAN(ch)) */
      GET_WEIGHT(ch) = 140 + dice(6, 10);
  } else /* if (IS_FEMALE(ch)) */ {
    if (IS_DWARF(ch))
      GET_WEIGHT(ch) = 105 + dice(4, 10);
    else if (IS_ELF(ch))
      GET_WEIGHT(ch) = 70 + dice(3, 10);
    else if (IS_GNOME(ch))
      GET_WEIGHT(ch) = 68 + dice(5, 4);
    else /* if (IS_HUMAN(ch)) */
      GET_WEIGHT(ch) = 100 + dice(6, 10);
  }
  return;
}


int invalid_race(struct char_data *ch, struct obj_data *obj)
{
  if (GET_LEVEL(ch) >= LVL_IMMORT)
    return FALSE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_HUMAN) && IS_HUMAN(ch))
    return (TRUE);

  if (OBJ_FLAGGED(obj, ITEM_ANTI_ELF) && IS_ELF(ch))
    return (TRUE);

  if (OBJ_FLAGGED(obj, ITEM_ANTI_DWARF) && IS_DWARF(ch))
    return (TRUE);

  if (OBJ_FLAGGED(obj, ITEM_ANTI_GNOME) && IS_GNOME(ch))
    return (TRUE);

  return (FALSE);
}

