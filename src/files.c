/***************************************************************************
*					 MEDIEVIA CyberSpace Code and Data files		       *
*       Copyright (C) 1992, 1996 INTENSE Software(tm) and Mike Krause	   *
*							   All rights reserved				           *
***************************************************************************/
/***************************************************************************
* This program belongs to INTENSE Software, and contains trade secrets of  *
* INTENSE Software.  The program and its contents are not to be disclosed  *
* to or used by any person who has not received prior authorization from   *
* INTENSE Software.  Any such disclosure or use may subject the violator   *
* to civil and criminal penalties by law.                                  *
***************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>
#include <time.h>

#include "structs.h"
#include "mob.h"
#include "obj.h"
#include "utils.h"
#include "db.h"
#include "handler.h"
#include "limits.h"
#include "spells.h"

#define NEW_ZONE_SYSTEM

/*
 * Sizes.
 */
#define	MAX_RESET   9792

/**************************************************************************
*  declarations of most of the 'global' variables                         *
************************************************************************ */
extern int weigh(struct obj_data *obj);
extern void MakeFLyStoreRoom(void);
extern int VERSIONNUMBER;
extern void add_object(struct obj_data *obj);
extern void sanity_check_object_list();
extern struct room_data *world[MAX_ROOM];	/* array of rooms                  */
struct char_data *mobs[MAX_MOB];
struct obj_data *objs[MAX_OBJ];
extern char use_hash_table;
char fixit = 0;
int top_of_world = 0;		/* ref to the top element of world */
struct obj_data *object_list = NULL;	/* the global linked list of obj's */
struct char_data *character_list = NULL;	/* global l-list of chars          */
struct ban_t *ban_list = NULL;	/* list of banned site--sigh       */
extern char global_color;
struct zone_data zone_table_array[MAX_ZONE];
struct zone_data *zone_table = zone_table_array;
#define zone_table  zone_table_array
		      /* table of reset data             */
int top_of_zone_table = 0;
int giNewsVersion;
int number_of_rooms = 0, number_of_zones = 0;
struct message_list fight_messages[MAX_MESSAGES];	/* fighting messages   */
char greetings_ansi1[MAX_STRING_LENGTH];	/* ANSI COLOR greetings file     */
char greetings_ansi2[MAX_STRING_LENGTH];	/* ANSI COLOR greetings file     */
char greetings_ansi3[MAX_STRING_LENGTH];	/* ANSI COLOR greetings file     */
char greetings_ansi4[MAX_STRING_LENGTH];	/* ANSI COLOR greetings file     */
char greetings_ansi5[MAX_STRING_LENGTH];	/* ANSI COLOR greetings file     */
char greetings1[MAX_STRING_LENGTH];	/* the greeting screen             */
char greetings2[MAX_STRING_LENGTH];	/* the greeting screen             */
char greetings3[MAX_STRING_LENGTH];	/* the greeting screen             */
char greetings4[MAX_STRING_LENGTH];	/* the greeting screen             */
char greetings5[MAX_STRING_LENGTH];	/* the greeting screen             */
char greetings6[MAX_STRING_LENGTH];	/* the greeting screen             */
char credits[MAX_STRING_LENGTH];	/* the Credits List                */
char motd[MAX_STRING_LENGTH];	/* the messages of today           */
char mods[MAX_STRING_LENGTH];	/* Our mods                        */
char story[MAX_STRING_LENGTH];	/* the game story                  */
char help[MAX_STRING_LENGTH];	/* the main help page              */
char info[MAX_STRING_LENGTH];	/* the info text                   */
char wizlist[MAX_STRING_LENGTH];	/* the wizlist                     */
char wmfha[1300];		/* Work Made For Hire Agreement    */
char *trivia[MAX_TRIVIA];	/* trivia array */
char trivia_file[MAX_INPUT_LENGTH];	/* file name for trivia file */

FILE *mob_f,			/* file containing mob prototypes  */
*obj_f,				/* obj prototypes                  */
*help_fl,			/* file for help texts (HELP <kwd>) */
*trivia_fl;			/* file for trivia items           */

struct index_data mob_index_array[MAX_MOB];
struct index_data *mob_index = mob_index_array;
#define mob_index   mob_index_array
		      /* index table for mobile file     */
struct index_data obj_index_array[MAX_OBJ];
struct index_data *obj_index = obj_index_array;
		      /* index table for object file     */
#define obj_index   obj_index_array
struct help_index_element *help_index = NULL;

int top_of_mobt = 0;		/* top of mobile index table       */
int top_of_objt = 0;		/* top of object index table       */
int top_of_helpt = 0;		/* top of help index table         */
int top_trivia = 0;		/* top of trivia table */

struct time_info_data time_info;	/* the infomation about the time   */
struct weather_data weather_info;	/* the infomation about the weather */
struct damage_rooms *daroom_list = NULL;

/* local procedures */
void boot_damage_rooms(void);
void boot_zones(void);
void setup_dir(FILE * fl, int room, int dir);
void build_trivia(FILE *);

void boot_world(void);
struct index_data *generate_indices_objs(FILE * fl, int *top,
					 struct index_data *index);
int is_empty(int zone_nr);
void reset_zone(int zone);
int file_to_string(char *name, char *buf);
void renum_zone_table(void);
void reset_time(void);
void clear_char(struct char_data *ch);
int slot_machine(struct char_data *ch, int cmd, char *arg);

/* external refs */
extern struct obj_data *tweak(struct obj_data *obj);
extern struct index_data *generate_indices_mobs(FILE * fl, int *top,
						struct index_data *index);
extern void load_ban_list(void);
extern struct descriptor_data *descriptor_list;
extern void load_messages(void);
extern void weather_and_time(int mode);
extern void assign_spell_pointers(void);
extern void build_catacombs(void);
extern int dice(int number, int size);
extern int number(int from, int to);
extern void write_filtered_text(FILE * fh, char *text);
extern void boot_social_messages(void);
extern void boot_pose_messages(void);
extern struct help_index_element *build_help_index(FILE * fl, int *num);
extern int _filbuf(FILE *);
extern void space_to_underline(char *text);
extern int port;
extern bool in_a_shop(struct char_data *ch);
extern void load_room_actions(void);
extern struct char_data *finger_ch;
extern void BootHoloCode(void);
extern int num_mob_classless;
extern int num_mob_class;
extern int num_mob_thief;
extern int num_mob_magic_user;
extern int num_mob_warrior;
extern int num_mob_cleric;
extern int num_mob_other;
extern void LoadSurveyList();
extern void BootTradingSystem(void);
extern void SetupDragons(void);

void boot_db(void)
{
	int i, k;
	struct descriptor_data *tmp_d;
	num_mob_classless = 0;
	num_mob_class = 0;
	num_mob_magic_user = 0;
	num_mob_cleric = 0;
	num_mob_warrior = 0;
	num_mob_thief = 0;
	num_mob_other = 0;
	reset_time();
	log_hd("Reading aux files.");
	file_to_string("../lib/knight_col.pic", greetings_ansi1);
	file_to_string("../lib/original_col.pic", greetings_ansi2);
	file_to_string("../lib/castle1_col.pic", greetings_ansi3);
	file_to_string("../lib/mortal_col.pic", greetings_ansi4);
	file_to_string("../lib/castle2_col.pic", greetings_ansi5);
	file_to_string("../lib/castle2_mon.pic", greetings1);
	file_to_string("../lib/knight_mon.pic", greetings2);
	file_to_string("../lib/mortal_mon.pic", greetings3);
	file_to_string("../lib/original_mon.pic", greetings4);
	file_to_string("../lib/viking_mon.pic", greetings5);
	file_to_string("../lib/wiz_desk_mon.pic", greetings6);
	file_to_string(CREDITS_FILE, credits);
	file_to_string(MOTD_FILE, motd);
	file_to_string(MODS_FILE, mods);
	file_to_string("wmfhagreement.txt", wmfha);
	strcat(wmfha, "\n\rPress RETURN to continue: ");
	strcat(motd, "\n\rPress RETURN to continue: ");
	strcat(mods, "\n\rPress RETURN to continue: ");
	file_to_string(STORY_FILE, story);
	strcat(story, "\n\rPress RETURN to continue: ");
	file_to_string(HELP_PAGE_FILE, help);
	file_to_string(INFO_FILE, info);
	file_to_string(WIZLIST_FILE, wizlist);

	log_hd("Opening mobile, object and help files.");
	if (!(mob_f = fopen("../news/news.dat", "r"))) {
		perror("../news/news.dat");
		SUICIDE;
	}
	fscanf(mob_f, " %d ", &giNewsVersion);
	fclose(mob_f);
	if (!(mob_f = fopen(MOB_FILE, "r"))) {
		perror(MOB_FILE);
		SUICIDE;
	}
	open_files++;

	if (!(obj_f = fopen(OBJ_FILE, "r"))) {
		perror(OBJ_FILE);
		SUICIDE;
	}

	open_files++;
	if (!(help_fl = fopen(HELP_KWRD_FILE, "r"))) {
		perror(HELP_KWRD_FILE);
		SUICIDE;
	}
	open_files++;

	/* Build trivia array */
	if (!(trivia_fl = fopen(TRIVIA_FILE, "r"))) {
		perror(TRIVIA_FILE);
		SUICIDE;
	}
	open_files++;
	build_trivia(trivia_fl);
	fclose(trivia_fl);
	/* End of Trivia Array building */

	help_index = build_help_index(help_fl, &top_of_helpt);

	log_hd("Loading MEDIEVIA files.....");
	log_hd("Loading list of banned sites....");
	load_ban_list();
	boot_zones();
	boot_world();
	log_hd("Verifying all room EXITS:");
	for (i = 0; i < MAX_ROOM; i++) {
		for (k = 0; k < 6; k++)
			if (world[i] && world[i]->dir_option[k] && !world[world[i]->dir_option[k]->to_room]) {
				sprintf(log_buf, "##room %d has exit to room %d which isn't here!", i, world[i]->dir_option[k]->to_room);
				if (port != 1220)
					log_hd(log_buf);
				world[i]->dir_option[k]->general_description = my_free(world[i]->dir_option[k]->general_description);
				world[i]->dir_option[k]->keyword = my_free(world[i]->dir_option[k]->keyword);
				world[i]->dir_option[k]->exit = my_free(world[i]->dir_option[k]->exit);
				world[i]->dir_option[k]->entrance = my_free(world[i]->dir_option[k]->entrance);
				world[i]->dir_option[k] = my_free(world[i]->dir_option[k]);
				world[i]->dir_option[k] = NULL;
			}
	}

	log_hd("Loading Mobile files");
	generate_indices_mobs(mob_f, &top_of_mobt, mob_index);
	log_hd("Loading Object files");
	generate_indices_objs(obj_f, &top_of_objt, obj_index);
	log_hd("Loading Room Actions");
	load_room_actions();
	log_hd("Loading messages.");
	load_messages();
	log_hd("Loading socials.");
	boot_social_messages();
	log_hd("Loading poses.");
	boot_pose_messages();
	world[0]->funct = slot_machine;
	log_hd("Assigning function pointers.");
	if (port != 1220) {
		assign_mobiles();
		assign_objects();
		assign_rooms();
	}
	assign_spell_pointers();
	log_hd("Building the CATACOMBS...");
	build_catacombs();
	if (port != 7777) {
		BootHoloCode();
		BootTradingSystem();
	}
	boot_damage_rooms();

	/*all this is for the finger data, finger @ourmachine */
	finger_ch = read_mobile(1219, 1);
	GET_LEVEL(finger_ch) = 32;
	CREATE(tmp_d, struct descriptor_data, 1);
	finger_ch->specials.holyLite = 1;
	finger_ch->in_room = 0;
	finger_ch->desc = tmp_d;
	character_list = finger_ch->next;
	finger_ch->next = NULL;

	MakeFLyStoreRoom();
	SetupDragons();

	fprintf(stderr, "Done\n\r******* Medievia is Online and ready *******\n\r");
	fclose(mob_f);
	open_files--;
	fclose(obj_f);
	open_files--;
}

/* reset the time in the game from file */
void reset_time(void)
{
	long beginning_of_time = 650336715;
	FILE *fh;

	struct time_info_data mud_time_passed(time_t t2, time_t t1);

	if ((fh = fopen("../lib/time.dat", "r")) != NULL) {
		fread(&time_info, sizeof(time_info), 1, fh);
		fclose(fh);
	} else {
		time_info = mud_time_passed(time(0), beginning_of_time);
	}
	switch (time_info.hours) {
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
		{
			weather_info.sunlight = SUN_DARK;
			break;
		}
	case 5:
		{
			weather_info.sunlight = SUN_RISE;
			break;
		}
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
	case 16:
	case 17:
	case 18:
	case 19:
	case 20:
		{
			weather_info.sunlight = SUN_LIGHT;
			break;
		}
	case 21:
		{
			weather_info.sunlight = SUN_SET;
			break;
		}
	case 22:
	case 23:
	default:
		{
			weather_info.sunlight = SUN_DARK;
			break;
		}
	}

	sprintf(log_buf, "Current Gametime: %dH %dD %dM %dY.",
		time_info.hours, time_info.day,
		time_info.month, time_info.year);
	log_hd(log_buf);

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

/* load the rooms */
void boot_world(void)
{
	FILE *fl;
	int room_nr = 0, zone = 0, flag, tmp, x;
	char *temp, chk[MAX_STRING_LENGTH], filename[255], full_filename[255];
	struct extra_descr_data *new_descr = NULL;
	character_list = NULL;
	object_list = NULL;

	log_hd("Loading Medievia Room Files (New INTENSE type)");
	for (x = 0; x < MAX_ZONE; x++)
		if (zone_table[x].reset_mode != -1) {
			fprintf(stderr, ".");
			strcpy(filename, zone_table[x].name);
			space_to_underline(filename);
			sprintf(full_filename, "../lib/wld/%s", filename);
			if (!(fl = fopen(full_filename, "r"))) {
				perror("fopen");
				sprintf(log_buf, "Could not open file %s",
					full_filename);
				log_hd(log_buf);
				SUICIDE;
			}
			open_files++;
			do {
				fscanf(fl, " #%d\n", &room_nr);
				temp = fread_string(fl);
				if ((flag = (*temp != '$')) != 0) {	/* a new record to be read */
					if (room_nr >= MAX_ROOM) {
						sprintf(log_buf, "Room number too high %d", room_nr);
						perror(log_buf);
						SUICIDE;
					}
					if (world[room_nr]) {
						sprintf(log_buf, "ROOM #%d already exists!", room_nr);
						perror(log_buf);
						SUICIDE;
					}
					CREATE(world[room_nr], struct room_data, 1);
					number_of_rooms++;
					world[room_nr]->number = room_nr;
					world[room_nr]->name = temp;
					world[room_nr]->description = fread_string(fl);
					world[room_nr]->stpFreight = NULL;
					world[room_nr]->room_afs = NULL;
					world[room_nr]->holox = 0;
					world[room_nr]->holoy = 0;
					fscanf(fl, " %d ", &zone);
					world[room_nr]->zone = zone;

					fscanf(fl, " %d ", &tmp);

					world[room_nr]->room_flags = tmp;
					fscanf(fl, " %d ", &tmp);
					world[room_nr]->sector_type = tmp;
					fscanf(fl, " %d\n", &tmp);
					world[room_nr]->extra_flags = tmp;

					fscanf(fl, " %d ", &tmp);
					world[room_nr]->class_restriction = tmp;
					fscanf(fl, " %d ", &tmp);
					world[room_nr]->level_restriction = tmp;
					fscanf(fl, " %d ", &tmp);
					world[room_nr]->align_restriction = tmp;
					fscanf(fl, " %d\n", &tmp);
					world[room_nr]->mount_restriction = tmp;

					fscanf(fl, " %d ", &tmp);
					world[room_nr]->move_mod = tmp;
					fscanf(fl, " %d ", &tmp);
					world[room_nr]->pressure_mod = tmp;
					fscanf(fl, " %d\n", &tmp);
					world[room_nr]->temperature_mod = tmp;

					world[room_nr]->funct = NULL;
					world[room_nr]->contents = NULL;
					world[room_nr]->people = NULL;
					world[room_nr]->light = 0;	/* Zero light sources */

					for (tmp = 0; tmp <= 5; tmp++)
						world[room_nr]->dir_option[tmp] = NULL;

					world[room_nr]->ex_description = NULL;

					for (;;) {
						fscanf(fl, " %s \n", chk);

						if (*chk == 'D')	/* direction field */
							setup_dir(fl, room_nr, atoi(chk + 1));
						else if (*chk == 'E') {	/* extra description field */
							CREATE(new_descr, struct extra_descr_data, 1);
							new_descr->keyword = fread_string(fl);
							new_descr->description = fread_string(fl);
							new_descr->next = world[room_nr]->ex_description;
							world[room_nr]->ex_description = new_descr;
						} else if (*chk == 'S')	/* end of current room */
							break;
					}

				}
			}
			while (flag);
			fclose(fl);
			open_files--;
			temp = my_free(temp);	/* cleanup the area containing the terminal $  */
		}
	top_of_world = MAX_ROOM - 1;
	fprintf(stderr, "Done\n\r");
}

/* read direction data */
void setup_dir(FILE * fl, int room, int dir)
{
	if (room == 3704) {
		fprintf(stderr, "setup_dir room=%d dir=%d\n", room, dir);
	}
	int tmp;

	CREATE(world[room]->dir_option[dir], struct room_direction_data, 1);

	world[room]->dir_option[dir]->general_description = fread_string(fl);
	world[room]->dir_option[dir]->keyword = fread_string(fl);
	world[room]->dir_option[dir]->exit = fread_string(fl);
	world[room]->dir_option[dir]->entrance = fread_string(fl);
	fscanf(fl, " %d ", &tmp);
	if (tmp == 1)
		world[room]->dir_option[dir]->exit_info = EX_ISDOOR;
	else if (tmp == 2)
		world[room]->dir_option[dir]->exit_info = EX_ISDOOR | EX_PICKPROOF;
	else if (tmp == 3)
		world[room]->dir_option[dir]->exit_info = EX_ISDOOR | EX_SECRET;
	else if (tmp == 4)
		world[room]->dir_option[dir]->exit_info = EX_ISDOOR | EX_HIDDEN;
	else if (tmp == 5)
		world[room]->dir_option[dir]->exit_info = EX_ISDOOR | EX_ILLUSION;
	else
		world[room]->dir_option[dir]->exit_info = 0;
	if (world[room]->dir_option[dir]->keyword && tmp == 0)
		world[room]->dir_option[dir]->exit_info = EX_ISDOOR;
	fscanf(fl, " %d ", &tmp);
	world[room]->dir_option[dir]->key = tmp;

	fscanf(fl, " %d ", &tmp);
	world[room]->dir_option[dir]->to_room = tmp;
}

#ifdef NEW_ZONE_SYSTEM

/* load the zone table and command tables */
void boot_zones(void)
{
	FILE *fl;
	int zon = 0, tmp;
	char *check, buf[MAX_STRING_LENGTH];
	static struct reset_com reset_tab[MAX_RESET];
	static int reset_top = 0;

	for (tmp = 0; tmp < MAX_ZONE; tmp++) {
		zone_table[tmp].reset_mode = -1;	/* make all non-used first */
		zone_table[tmp].population = 0;
		zone_table[tmp].populated = FALSE;
		zone_table[tmp].time_to_empty = 0;
		zone_table[tmp].num_resets = 0;
		zone_table[tmp].continent = 1;
		zone_table[tmp].siX = 0;
		zone_table[tmp].siY = 0;
		zone_table[tmp].iRecallRoom = 0;
		zone_table[tmp].iSocialRestricted = 0;
	}
	if (!(fl = fopen(ZONE_FILE, "r"))) {
		perror(ZONE_FILE);
		SUICIDE;
	}
	open_files++;
	log_hd("Loading Zone File");
	for (;;) {
		fscanf(fl, " #%d\n", &zon);
		check = fread_string(fl);
		if (port == 1220 && zon > 1)
			break;

		if (*check == '$')
			break;	/* end of file */

		/* alloc a new zone */

		if (zon >= MAX_ZONE) {
			perror("Too many zones");
			SUICIDE;
		}
		number_of_zones++;
		zone_table[zon].name = check;
		fscanf(fl, " %d ", &zone_table[zon].top);
		fscanf(fl, " %d ", &zone_table[zon].lifespan);
		fscanf(fl, " %d ", &zone_table[zon].reset_mode);
		fscanf(fl, " %d\n ", &zone_table[zon].iSocialRestricted);
		fscanf(fl, " %hd ", &zone_table[zon].siX);
		fscanf(fl, " %hd ", &zone_table[zon].siY);
		fscanf(fl, " %d ", &zone_table[zon].iRecallRoom);
		/* read the command table */
		zone_table[zon].cmd = &reset_tab[reset_top];
		fprintf(stderr, ".");
		for (;;) {
			if (reset_top >= MAX_RESET) {
				sprintf(log_buf, "Too many zone resets at[%d]..", reset_top);
				perror(log_buf);
				SUICIDE;
			}

			reset_tab[reset_top].zone = zon;
			fscanf(fl, " ");	/* skip blanks */
			fscanf(fl, "%c", &reset_tab[reset_top].command);

			if (reset_tab[reset_top].command == 'S') {
				reset_top++;
				break;
			}

			if (reset_tab[reset_top].command == '*') {

				fgets(buf, 80, fl);	/* skip command */
				continue;
			}

			fscanf(fl, " %d %d %d", &tmp, &reset_tab[reset_top].arg1, &reset_tab[reset_top].arg2);

			reset_tab[reset_top].if_flag = tmp;

			if (reset_tab[reset_top].command == 'M' ||
			    reset_tab[reset_top].command == 'O' ||
			    reset_tab[reset_top].command == 'E' ||
			    reset_tab[reset_top].command == 'P' ||
			    reset_tab[reset_top].command == 'D' ||
			    reset_tab[reset_top].command == 'W')
				fscanf(fl, " %d", &reset_tab[reset_top].arg3);

			if (reset_tab[reset_top].command == 'W')
				fscanf(fl, " %d", &reset_tab[reset_top].arg4);

			fgets(buf, 80, fl);	/* read comment */
			reset_top++;
		}
	}
	top_of_zone_table = MAX_ZONE - 1;
	check = my_free(check);
	fclose(fl);
	open_files--;
	fprintf(stderr, "Done\n\r");
}

#endif

void boot_damage_rooms(void)
{
	int num = 0, room;
	char buf[300];

	for (room = 0; room < MAX_ROOM; room++) {
		if (world[room]) {
			if (IS_SET(world[room]->room_flags, FIRE)) {
				struct damage_rooms *d_room;
				CREATE(d_room, struct damage_rooms, 1);
				d_room->room_num = room;
				d_room->damage_type = FIRE;
				d_room->damage_amt = number(50, 200);
				d_room->next = daroom_list;
				daroom_list = d_room;
				num++;
				continue;
			}
			if (IS_SET(world[room]->room_flags, COLD)) {
				struct damage_rooms *d_room;
				CREATE(d_room, struct damage_rooms, 1);
				d_room->room_num = room;
				d_room->damage_type = COLD;
				d_room->damage_amt = number(75, 100);
				d_room->next = daroom_list;
				daroom_list = d_room;
				num++;
				continue;
			}
			if (IS_SET(world[room]->room_flags, GAS)) {
				struct damage_rooms *d_room;
				CREATE(d_room, struct damage_rooms, 1);
				d_room->room_num = room;
				d_room->damage_type = GAS;
				d_room->damage_amt = number(50, 100);
				d_room->next = daroom_list;
				daroom_list = d_room;
				num++;
				continue;
			}
		}
	}
	sprintf(buf, "Booting Damage Rooms... %d rooms loaded.", num);
	log_hd(buf);
}

void do_zonestats(struct char_data *ch, char *argument, int cmd)
{
	int i;

	send_to_char
	    ("Zone NUM  Populatn Countdn Loaded?    Reset span Minutes  # of Resets\n\r",
	     ch);

	send_to_char
	    ("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\r",
	     ch);
	for (i = 0; i < MAX_ZONE; i++) {
		if (zone_table[i].reset_mode == -1)	/* this zone node not used */
			continue;
		sprintf(log_buf,
			"[Zone %3d] Pop=%3d TTE=%2d Status=%s Lifespan=%2d Age=%2d Num_Resets=%d\n\r",
			i, zone_table[i].population,
			zone_table[i].time_to_empty,
			((zone_table[i].populated) ? "LOAD" : "FREE"),
			zone_table[i].lifespan, zone_table[i].age,
			zone_table[i].num_resets);
		send_to_char(log_buf, ch);
	}

}

void free_zone(int zone)
{
	struct char_data *i = NULL;
	struct descriptor_data *k = NULL;
	struct char_data *next_char = NULL;

	int iWear;
	global_color = 34;
	for (k = descriptor_list; k; k = k->next)
		if (k->character && !k->connected && GET_LEVEL(k->character) > 30)
			if (IS_SET(k->character->specials.god_display, GOD_ZONERESET)) {
				sprintf(log_buf, "(ZONE) %s,   Being un-populated.\n\r", zone_table[zone].name);
				send_to_char(log_buf, k->character);
			}
	global_color = 0;

	for (i = character_list; i; i = next_char) {
		next_char = i->next;
		if (!IS_NPC(i))
			continue;
		if (i->desc) {	/* in case undead */
			continue;
		}
		if (i->in_room == -1)
			continue;
		if (in_a_shop(i))
			continue;
		if (GET_ZONE(i) == zone) {
			for (iWear = 0; iWear < MAX_WEAR; iWear++)
				if (i->equipment[iWear])
					obj_to_char(unequip_char(i, iWear), i);
			while (i->carrying)
				extract_obj(i->carrying);
			i->specials.home_number = 20000;
			extract_char(i, TRUE);
		}
	}
	zone_table[zone].populated = 0;
}

void zone_update(void)
{
	int i;

	for (i = 0; i < MAX_ZONE; i++) {

		if (zone_table[i].reset_mode == -1)	/* this zone node not used */
			continue;

		if (!zone_table[i].populated)
			continue;

		if (zone_table[i].reset_mode == 0)
			continue;

		if (zone_table[i].age < zone_table[i].lifespan) {
			zone_table[i].age++;
			continue;
		}

		if (zone_table[i].reset_mode == 1 && !is_empty(i))
			continue;

		reset_zone(i);
	}
}

#define ZCMD zone_table[zone].cmd[cmd_no]

/* execute the reset command table of a given zone */
void reset_zone(int zone)
{
	int cmd_no, last_cmd = 1;
	struct char_data *mob = NULL;
	struct obj_data *obj = NULL, *obj_to = NULL;
	struct descriptor_data *i = NULL;

	if (zone == 198)
		return;		/*catacombs */
	if (zone == 197)
		return;		/*landmasses */
	global_color = 34;
	for (i = descriptor_list; i; i = i->next)
		if (i->character && !i->connected && GET_LEVEL(i->character) > 30)
			if (IS_SET(i->character->specials.god_display, GOD_ZONERESET)) {
				sprintf(log_buf, "(ZONE) %s, resetting.\n\r", zone_table[zone].name);
				send_to_char(log_buf, i->character);
			}
	global_color = 0;
	zone_table[zone].populated = TRUE;
	for (cmd_no = 0;; cmd_no++) {
		if (ZCMD.command == 'S')
			break;

		if (last_cmd || ZCMD.if_flag == 0)
			switch (ZCMD.command) {
			case 'M':	/* read a mobile */
				if (mob_index[ZCMD.arg1].number < ZCMD.arg2 || !zone_table[zone].num_resets) {
					if (!world[ZCMD.arg3]) {
						sprintf(log_buf, "## zone reset trying to load mob in room %d.", ZCMD.arg3);
						log_hd(log_buf);
						break;
					}
					mob = read_mobile(ZCMD.arg1, REAL);
					if (!mob) {
						sprintf(log_buf, "##ZONE O mob %d doesn't exist", ZCMD.arg1);
						log_hd(log_buf);
						last_cmd = 0;
						continue;
					}

					char_to_room(mob, ZCMD.arg3);
					last_cmd = 1;
				} else
					last_cmd = 0;
				break;

			case 'O':	/* read an object */
				if (obj_index[ZCMD.arg1].number < ZCMD.arg2 || !zone_table[zone].num_resets)
					if (ZCMD.arg3 >= 0) {
						if (world[ZCMD.arg3])
							if (!get_obj_in_list_num(ZCMD.arg1, world[ZCMD.arg3]->contents)) {
								obj = read_object(ZCMD.arg1, 0);
								if (!obj) {
									sprintf(log_buf, "##ZONE O  object %d doesn't exist", ZCMD.arg1);
									log_hd(log_buf);
									last_cmd = 0;
									continue;
								}
								obj = tweak(obj);
								obj_to_room(obj, ZCMD.arg3);
								last_cmd = 1;
							} else
								last_cmd = 0;
					} else {
						obj = read_object(ZCMD.arg1, 0);
						if (!obj) {
							sprintf(log_buf, "##ZONE O  object %d doesn't exist", ZCMD.arg1);
							log_hd(log_buf);
							last_cmd = 0;
							continue;
						}
						obj = tweak(obj);
						obj->in_room = NOWHERE;
						last_cmd = 1;
				} else
					last_cmd = 0;
				break;

			case 'P':	/* object to object */
				if (obj_index[ZCMD.arg1].number < ZCMD.arg2 || !zone_table[zone].num_resets) {
					obj_to = get_obj_num(ZCMD.arg3);
					if (!obj_to) {
						if (!obj_to) {
							sprintf(log_buf, "##ZONE P1  object %d doesn't exist", ZCMD.arg3);
							log_hd(log_buf);
							last_cmd = 0;
							continue;
						}

					}
					obj = read_object(ZCMD.arg1, obj_to->obj_flags.eq_level);
					if (!obj) {
						sprintf(log_buf, "##ZONE P2  object %d doesn't exist", ZCMD.arg1);
						log_hd(log_buf);
						last_cmd = 0;
						continue;
					}
					obj = tweak(obj);
					obj_to_obj(obj, obj_to);
					last_cmd = 1;
				} else
					last_cmd = 0;
				break;

			case 'G':	/* obj_to_char */
				if (mob == NULL) {
					log_hd("Null mob in G");
					last_cmd = 0;
					break;
				}
				if (obj_index[ZCMD.arg1].number < ZCMD.arg2 || !zone_table[zone].num_resets) {
					obj = read_object(ZCMD.arg1, map_eq_level(mob));
					if (!obj) {
						sprintf(log_buf, "##ZONE G  object %d doesn't exist", ZCMD.arg1);
						log_hd(log_buf);
						last_cmd = 0;
						continue;
					}
					obj = tweak(obj);
					obj_to_char(obj, mob);
					last_cmd = 1;
				} else
					last_cmd = 0;
				break;

			case 'E':	/* object to equipment list */
				if (mob == NULL) {
					log_hd("Null mob in E");
					last_cmd = 0;
					break;
				}
				if (obj_index[ZCMD.arg1].number < ZCMD.arg2 || !zone_table[zone].num_resets) {
					obj = read_object(ZCMD.arg1, map_eq_level(mob));
					if (!obj) {
						sprintf(log_buf, "##ZONE E  object %d doesn't exist", ZCMD.arg1);
						log_hd(log_buf);
						last_cmd = 0;
						continue;
					}
					obj = tweak(obj);
					equip_char(mob, obj, ZCMD.arg3);
					last_cmd = 1;
				} else
					last_cmd = 0;
				break;

			case 'D':	/* set state of door */
				if (!world[ZCMD.arg1])
					continue;
				if (!world[ZCMD.arg1]->dir_option[ZCMD.arg2]) {
					sprintf(log_buf, "room %d door %d not found!!!!! state %d", ZCMD.arg1, ZCMD.arg2, ZCMD.arg3);
					log_hd(log_buf);
					continue;
				}
				switch (ZCMD.arg3) {
				case 0:
					REMOVE_BIT(world[ZCMD.arg1]->dir_option[ZCMD.arg2]->exit_info, EX_LOCKED);
					REMOVE_BIT(world[ZCMD.arg1]->dir_option[ZCMD.arg2]->exit_info, EX_CLOSED);
					break;
				case 1:
					SET_BIT(world[ZCMD.arg1]->dir_option[ZCMD.arg2]->exit_info, EX_CLOSED);
					REMOVE_BIT(world[ZCMD.arg1]->dir_option[ZCMD.arg2]->exit_info, EX_LOCKED);
					break;
				case 2:
					SET_BIT(world[ZCMD.arg1]->dir_option[ZCMD.arg2]->exit_info, EX_LOCKED);
					SET_BIT(world[ZCMD.arg1]->dir_option[ZCMD.arg2]->exit_info, EX_CLOSED);
					break;
				}
				last_cmd = 1;
				break;
			case 'W':	/* Just like 'E' but with a percent chance to load */
				//if (number(1, 100) > ZCMD.arg4)
				//	break;
				if (mob == NULL) {
					log_hd("Null mob in E");
					last_cmd = 0;
					break;
				}
				if (obj_index[ZCMD.arg1].number < ZCMD.arg2 || !zone_table[zone].num_resets) {
					obj = read_object(ZCMD.arg1, map_eq_level(mob));
					if (!obj) {
						sprintf(log_buf, "##ZONE E  object %d doesn't exist", ZCMD.arg1);
						log_hd(log_buf);
						last_cmd = 0;
						continue;
					}
					obj = tweak(obj);
					equip_char(mob, obj, ZCMD.arg3);
					last_cmd = 1;
				} else
					last_cmd = 1;
				break;

			default:
				sprintf(log_buf, "Undef command: zone %d cmd %d.", zone, cmd_no);
				log_hd(log_buf);
				SUICIDE;
				break;
		} else
			last_cmd = 0;

	}

	zone_table[zone].num_resets++;
	zone_table[zone].age = 0;
}

#undef ZCMD

/* for use in reset_zone; return TRUE if zone 'nr' is free of PC's  */
int is_empty(int zone_nr)
{
	struct descriptor_data *i = NULL;

	for (i = descriptor_list; i; i = i->next)
		if (!i->connected)
			if (world[i->character->in_room]->zone == zone_nr)
				return (0);

	return (1);
}

/************************************************************************
*  procs of a (more or less) general utility nature         *
********************************************************************** */

/* read and allocate space for a '~'-terminated string from a given file */
char *fread_string(FILE * fl)
{
	char buf[MAX_STRING_LENGTH];
	char *pAlloc;
	char *pBufLast;

	if (!fl)
		SUICIDE;

	for (pBufLast = buf; pBufLast < &buf[sizeof(buf) - 2];) {
		switch (*pBufLast = getc(fl)) {
		default:
			pBufLast++;
			break;

		case EOF:
			log_hd(log_buf);
			perror("FREAD_STRING: EOF");
			SUICIDE;
			break;

		case '\n':
			while (pBufLast > buf && isspace(pBufLast[-1]))
				pBufLast--;
			*pBufLast++ = '\n';
			*pBufLast++ = '\r';
			break;

		case '~':
			getc(fl);
			if (pBufLast == buf) {
				CREATE(pAlloc, char, strlen(" ") + 2);
				sprintf(pAlloc, " ");
/* juice  4/7/95  this is a leak and hoses lines with nothing but tildes
			pAlloc  = " ";
*/
			} else {
				*pBufLast++ = '\0';
				CREATE(pAlloc, char, pBufLast - buf);
				memcpy(pAlloc, buf, pBufLast - buf);
			}
			return pAlloc;
		}
	}
	log_hd(buf);
	perror("fread_string: string too long");
	SUICIDE;
	return (NULL);
}

/* read contents of a text file, and place in buf */
int file_to_string(char *name, char *buf)
{
	FILE *fl;
	char tmp[100];

	*buf = '\0';

	if (!(fl = med_open(name, "r"))) {
		sprintf(log_buf, "file-to-string(%s)", name);
		perror(log_buf);
		*buf = '\0';
		SUICIDE;
		return (-1);
	}
	open_files++;

	do {
		fgets(tmp, 99, fl);

		if (!feof(fl)) {
			if (strlen(buf) + strlen(tmp) + 2 > MAX_STRING_LENGTH) {
				sprintf(log_buf, "fl->strng: (%ld)string too big (db.c,file_to_string)", strlen(buf) + strlen(tmp) + 2);
				log_hd(log_buf);
				*buf = '\0';
				SUICIDE;
				return (-1);
			}

			strcat(buf, tmp);
			*(buf + strlen(buf) + 1) = '\0';
			*(buf + strlen(buf)) = '\r';
		}
	}
	while (!feof(fl));

	med_close(fl);
	open_files--;

	return (0);
}

/* clear some of the the working variables of a char */
void reset_char(struct char_data *ch)
{
	int i, x, y;

	for (i = 0; i < MAX_WEAR; i++)	/* Intializing  */
		ch->equipment[i] = NULL;

	ch->master = NULL;
	for (x = 0; x < 3; x++)
		for (y = 0; y < 3; y++)
			ch->formation[x][y] = NULL;
	ch->carrying = NULL;
	ch->next = NULL;
	ch->next_fighting = NULL;
	ch->next_in_room = NULL;
	ch->specials.fighting = NULL;
	ch->specials.position = POSITION_STANDING;
	ch->specials.default_pos = POSITION_STANDING;
	ch->specials.carry_weight = 0;
	ch->specials.carry_items = 0;
	GET_AC(ch) = 100;
	ch->specials.wizInvis = FALSE;
	ch->specials.holyLite = FALSE;
	ch->specials.new_comm_flags = 0;
	ch->specials.solo = FALSE;
	ch->specials.home_number = 0;
	ch->specials.autoexit = 0;
	ch->specials.editzone = 0;
	ch->specials.wimpy = 0;
	if (GET_HIT(ch) <= 0)
		GET_HIT(ch) = 1;
	if (GET_MOVE(ch) <= 0)
		GET_MOVE(ch) = 1;
	if (GET_MANA(ch) <= 0)
		GET_MANA(ch) = 1;
	GET_GOLD(ch) += number(2000, 5000);
}

/*
 * Reset ch->p
 */

void clear_pData(struct char_data *ch)
{
	if (!ch->p)
		return;

	ch->p->iFlags = 0;
	ch->p->sneaked_room = 0;
	ch->p->siLastTown = 0;
	ch->p->queryfunc = NULL;
	ch->p->queryintfunc = NULL;
	*ch->p->queryprompt = '\0';
	*ch->p->queryprompt2 = '\0';
	ch->p->querycommand = 0;
	ch->p->queryintcommand = 0;
	ch->p->donated = 0;
	ch->p->iLastInBeforeSocial = 0;
	ch->p->iLastSocialX = 0;
	ch->p->iLastSocialY = 0;
	return;
}

/* initialize a new character only if class is set */
void init_char(struct char_data *ch)
{
	int i;
	extern int god;

	/*
	 * Boot with -g to make gods.
	 */
	if (god) {
		GET_EXP(ch) = 90000000;
		GET_LEVEL(ch) = 35;
	}

	set_title(ch);

	ch->player.short_descr = MED_NULL;
	ch->player.long_descr = MED_NULL;
	ch->player.description = MED_NULL;

	ch->player.time.birth = time(0);
	ch->player.time.played = 0;
	ch->player.time.logon = time(0);

	for (i = 0; i < MAX_TONGUE; i++)
		ch->player.talks[i] = 0;

	GET_STR(ch) = 9;
	GET_INT(ch) = 9;
	GET_WIS(ch) = 9;
	GET_DEX(ch) = 9;
	GET_CON(ch) = 9;

	/* make favors for sex */
	if (ch->player.sex == SEX_MALE) {
		ch->player.weight = number(120, 180);
		ch->player.height = number(160, 200);
	} else {
		ch->player.weight = number(100, 160);
		ch->player.height = number(150, 180);
	}

	ch->points.max_mana = 100;
	ch->points.mana = GET_MAX_MANA(ch);
	ch->points.hit = GET_MAX_HIT(ch);
	ch->points.move = GET_MAX_MOVE(ch);
	ch->points.armor = 100;

	for (i = 0; i < MAX_SKILLS; i++) {
		if (GET_LEVEL(ch) < 35) {
			ch->skills[i].learned = 0;
			ch->skills[i].recognise = FALSE;
		} else {
			ch->skills[i].learned = 100;
			ch->skills[i].recognise = FALSE;
		}
	}

	ch->specials.affected_by = 0;
	ch->specials.practices = 0;

	for (i = 0; i < 5; i++)
		ch->specials.apply_saving_throw[i] = 0;

	for (i = 0; i < 3; i++)
		GET_COND(ch, i) = (GET_LEVEL(ch) == 35 ? -1 : 35);

	reset_char(ch);
	ch->specials.autoexit = 1;
}

/* these will exist till I stop being lazy and take out all old refernces */
int real_mobile(int virtual)
{
	return (virtual);
}

int real_object(int virtual)
{
	return (virtual);
}

void build_trivia(FILE * fh)
{
	int counter = 0, bcounter = 0;
	char get_buffer[MAX_INPUT_LENGTH];

	while (!feof(fh)) {
		fgets(get_buffer, sizeof(get_buffer), fh);
		if (*get_buffer == '~')
			break;
		bcounter = 0;
		while (get_buffer[bcounter] != '~')
			bcounter++;
		get_buffer[bcounter] = '\0';
		strcat(get_buffer, "\r\n");
		trivia[counter] = strdup(get_buffer);
		counter++;
	}
	top_trivia = counter;
	*get_buffer = '\0';
	sprintf(get_buffer, "top_trivia = %d", top_trivia);
	log_hd(get_buffer);
}

void clear_char(struct char_data *ch)
{
	if (!IS_NPC(ch))
		memset((char *)ch, (char)'\0', (int)sizeof(struct char_data));

	ch->internal_use = -1;
	ch->in_room = NOWHERE;
	ch->specials.was_in_room = NOWHERE;
	ch->specials.position = POSITION_STANDING;
	ch->specials.default_pos = POSITION_STANDING;
	ch->specials.wizInvis = FALSE;
	ch->specials.holyLite = FALSE;
	ch->specials.afk_text[0] = 0;
	ch->specials.solo = FALSE;
	ch->specials.home_number = 0;
	GET_AC(ch) = 100;	/* Basic Armor */
	if (ch->points.max_mana < 100) {
		ch->points.max_mana = 100;
	}
	ch->ask_for_follow = NULL;
	ch->formation[0][0] = NULL;
	ch->formation[0][1] = NULL;
	ch->formation[0][2] = NULL;
	ch->formation[1][0] = NULL;
	ch->formation[1][1] = NULL;
	ch->formation[1][2] = NULL;
	ch->formation[2][0] = NULL;
	ch->formation[2][1] = NULL;
	ch->formation[2][2] = NULL;
	ch->affected = NULL;
	GET_BREATH(ch) = 100;
}

/* release memory allocated for a char struct */
void free_char(CHAR_DATA * ch)
{
	int iWear;
	struct affected_type *af = NULL;
	struct affected_type *next_affect = NULL;

	if (IS_NPC(ch) && ch->nr >= 0) {
		if (ch->player.name)
			if (ch->player.name != mobs[ch->nr]->player.name)
				ch->player.name = my_free(ch->player.name);
		if (ch->player.title)
			if (ch->player.title != mobs[ch->nr]->player.title)
				if (ch->player.title)
					ch->player.title = my_free(ch->player.title);
		if (ch->player.short_descr)
			if (ch->player.short_descr != mobs[ch->nr]->player.short_descr)
				ch->player.short_descr = my_free(ch->player.short_descr);
		if (ch->player.long_descr)
			if (ch->player.long_descr != mobs[ch->nr]->player.long_descr)
				ch->player.long_descr = my_free(ch->player.long_descr);
		if (ch->player.description)
			if (ch->player.description != mobs[ch->nr]->player.description)
				ch->player.description = my_free(ch->player.description);
	} else {
		ch->player.name = my_free(ch->player.name);
		ch->player.title = my_free(ch->player.title);
		ch->player.short_descr = my_free(ch->player.short_descr);
		ch->player.long_descr = my_free(ch->player.long_descr);
		ch->player.description = my_free(ch->player.description);
	}
	if (ch->p)
		ch->p = my_free(ch->p);
	for (iWear = 0; iWear < MAX_WEAR; iWear++) {
		if (ch->equipment[iWear])
			obj_to_char(unequip_char(ch, iWear), ch);
	}

	while (ch->carrying)
		extract_obj(ch->carrying);

	for (af = ch->affected; af; af = next_affect) {
		next_affect = af->next;
		affect_remove(ch, af);
	}

	ch = my_free(ch);
}
