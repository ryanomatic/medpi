/***************************************************************************
*					 MEDIEVIA CyberSpace Code and Data files		       *
*       Copyright (C) 1992, 1996 INTENSE Software(tm) and Mike Krause	   *
*						   All rights reserved				               *
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
#include <ctype.h>
#include <dirent.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/types.h>
#ifndef __FreeBSD__
#include <malloc.h>
#endif
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#include "structs.h"
#include "mob.h"
#include "obj.h"
#include "utils.h"
#include "interp.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "limits.h"
#include "holocode.h"
#include "trading.h"

/*   external vars  */
extern int iFlyStoreRoom;
extern int pulse;
extern int port;
extern char global_color;
extern struct room_data *world[MAX_ROOM];	/* array of rooms  */
extern struct zone_data zone_table_array[MAX_ZONE];
extern struct zone_data *zone_table;
extern struct descriptor_data *descriptor_list;
extern struct char_data *character_list;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct char_data *mobs[MAX_MOB];
extern struct obj_data *objs[MAX_OBJ];
extern struct int_app_type int_app[26];
extern struct wis_app_type wis_app[26];
extern struct ban_t *ban_list;
extern char wmfha[1300];
extern int slot_rooms[];
extern char use_hash_table;
extern int igHolomade, igHoloReuse;
extern ush_int Holo[MAXHOLO][MAXHOLO];
extern struct FREIGHTTYPE gstaFreightTypes[100];
extern char MOUNTMOVE;

/* external functs */
extern void wear(struct char_data *ch, struct obj_data *obj_object, int keyword);
extern void StopFlying(struct char_data *stpCh);
extern void write_filtered_text(FILE * fh, char *text);
/*int system(char *);*/
extern int iMakeHoloRoom(int x, int y);
extern bool put_in_formation(struct char_data *leader, struct char_data *follower);
extern void page_string(struct descriptor_data *d, char *str,
			int keep_internal);
void save_rooms(struct char_data *ch, int zone);
void set_title(struct char_data *ch);
int str_cmp(char *arg1, char *arg2);
struct time_info_data age(struct char_data *ch);
void sprinttype(int type, char *names[], char *result);
void sprintbit(long vektor, char *names[], char *result);
int mana_limit(struct char_data *ch);
int hit_limit(struct char_data *ch);
int move_limit(struct char_data *ch);
int mana_gain(struct char_data *ch);
int hit_gain(struct char_data *ch);
int move_gain(struct char_data *ch);
extern struct mallinfo mallinfo();
extern void sort_descriptors(void);

extern FILE *obj_f, *mob_f;
extern char use_memory_debug;
extern struct global_clan_info_struct global_clan_info;
extern void load_clan_info(void);
extern char daytimedown;
extern float mob_damage_multiplier;
extern float exp_multiplier;

void do_setmulti(struct char_data *ch, char *argument, int cmd)
{
	char name[MAX_INPUT_LENGTH] = { 0 };
	one_argument(argument, name);

	if (strlen(name) < 1) {
		send_to_char("Which player?\n", ch);
		return;
	}

	struct char_data* c = get_player(name);
	if (c) {
		SET_BIT(c->player.multi_class, MULTI_CLASS_MAGIC_USER);
		SET_BIT(c->player.multi_class, MULTI_CLASS_CLERIC);
		SET_BIT(c->player.multi_class, MULTI_CLASS_WARRIOR);
		SET_BIT(c->player.multi_class, MULTI_CLASS_THIEF);
		send_to_char("All multiclass bits set.\n", ch);
	}
	else {
		send_to_char("Can't find that player.\n", ch);
		return;
	}
}

void do_setexp(struct char_data *ch, char *argument, int cmd)
{
	char buf[256];

	if (strlen(argument) == 0) {
		sprintf(buf, "Current experience multiplier is %.2f.\n", exp_multiplier);
		send_to_char(buf, ch);
		return;
	}

	float f = atof(argument);
	if (f == 0.0f) {
		sprintf(buf, "Bad floating point number. Example: setexp 0.5\n");
		send_to_char(buf, ch);
		return;
	}
	else {
		exp_multiplier = f;
		sprintf(buf, "Experience multiplier is now %.2f.\n", exp_multiplier);
		send_to_char(buf, ch);
		return;
	}
}

void do_setmobdam(struct char_data *ch, char *argument, int cmd)
{
	char buf[256];

	if (strlen(argument) == 0) {
		sprintf(buf, "Current mob damage multiplier is %.2f.\n", mob_damage_multiplier);
		send_to_char(buf, ch);
		return;
	}

	float f = atof(argument);
	if (f == 0.0f) {
		sprintf(buf, "Bad floating point number. Example: setmobdam 0.5\n");
		send_to_char(buf, ch);
		return;
	}
	else {
		mob_damage_multiplier = f;
		sprintf(buf, "Mob damage multiplier is now %.2f.\n", mob_damage_multiplier);
		send_to_char(buf, ch);
		return;
	}
}

void do_room_analyze(struct char_data *ch, char *argument, int cmd)
{
	return;
}

void do_openup(struct char_data *ch, char *argument, int cmd)
{
	if (IS_NPC(ch))
		return;
	if (!daytimedown) {
		send_to_char("But we are open!\n\r", ch);
		return;
	}
	daytimedown = FALSE;
	send_to_char
	    ("Ok You take the open close sign and flip it to show OPEN!\n\r",
	     ch);
	do_echo(ch, "MEDIEVIA OPENS FOR BUSINESS!", 0);
	unlink("../lib/closedforbusiness");

}

void do_save_rooms(struct char_data *ch, char *argument, int cmd)
{
	char number[MAX_INPUT_LENGTH];
	int zone;

	if (IS_NPC(ch))
		return;

	if (!argument[0]) {
		send_to_char("Yes but which zone Number?", ch);
		return;
	}
	one_argument(argument, number);
	page_string(ch->desc, wmfha, 0);
	if (number[0] == 'a') {
		if (!IS_PLAYER(ch, "Vryce") && !IS_PLAYER(ch, "Io")) {
			send_to_char("Only Vryce can save the whole world.\n\r",
				     ch);
			return;
		}
		for (zone = 0; zone < MAX_ZONE; zone++)
			if (zone_table[zone].reset_mode != -1) {
				send_to_char("SAVING........\n\r", ch);
				save_rooms(ch, zone);
				send_to_char("Rooms saved.\n\r", ch);
			}
		return;
	}
	zone = atoi(number);
	if (zone < 0 || zone >= MAX_ZONE) {
		send_to_char("Zone number is out of range!\n", ch);
		return;
	}
	if (zone == 198) {
		send_to_char("You can't save the Catacombs, shees!\n", ch);
		return;
	}
	if (zone_table[zone].reset_mode == -1) {
		send_to_char("Zone doesn't exist!\n", ch);
		return;
	}

	if (ch->specials.editzone >= 0)
		if (ch->specials.editzone != zone && GET_LEVEL(ch) < 35) {
			sprintf(log_buf, "-=<(%s)>=-", GET_NAME(ch));
			if (strcmp(world[ch->in_room]->name, log_buf)) {
				send_to_char
				    ("You are not authorized to do that for this zone.\n\r",
				     ch);
				return;
			}
		}

	send_to_char("SAVING........\n\r", ch);
	save_rooms(ch, zone);
	send_to_char("Rooms saved.\n\r", ch);
}

void do_list_zones(struct char_data *ch, char *argument, int cmd)
{
	int x, y;
	FILE *fp;
	int room;
	global_color = 33;
	send_to_char("\nZONE  #ROOMS LIFESPAN #KILLS      ZONE NAME\n\r", ch);
	send_to_char
	    ("------------------------------------------------------------------\n\r",
	     ch);

	for (x = 0; x < MAX_ZONE; x++)
		if (zone_table[x].reset_mode != -1) {
			room = 0;
			for (y = 0; y < MAX_ROOM; y++)
				if (world[y] && world[y]->zone == x)
					room++;
			sprintf(log_buf, "[%3d][%4d][%3d][%6d]-%s\n\r", x, room,
				zone_table[x].lifespan, zone_table[x].numkills,
				zone_table[x].name);
			send_to_char(log_buf, ch);
		}
	global_color = 0;
	x = 0;
	if (port == 4000)
		while (slot_rooms[x] != -1) {
			sprintf(log_buf, "../casino/slot.%d.dat",
				slot_rooms[x]);
			if ((fp = med_open(log_buf, "w")) != NULL) {
				fwrite(&world[slot_rooms[x]]->pressure_mod,
				       sizeof(int), 1, fp);
				med_close(fp);
			}
			x++;
		}

}

void do_meminfo(struct char_data *ch, char *argument, int cmd)
{
#ifdef OLDLIB
	struct mallinfo mem;
	global_color = 31;
	mem = mallinfo();
#endif

#ifdef NEWLIB
	struct mstats mem;
	mem = mstats();
#endif

	send_to_char("\n\r******MEMORY USAGE AND OPTIMIZATION*****\n\r", ch);
	sprintf(log_buf, "Pulse=%d\n\r", pulse);
	send_to_char(log_buf, ch);
	if (use_memory_debug)
		send_to_char("MEMORY DEBUG code is (IN) effect.\n\r", ch);
	else
		send_to_char("MEMORY Debug is not in effect.\n\r", ch);
	if (use_hash_table)
		send_to_char("HASH TABLE is        (IN) effect.\n\r", ch);
	else
		send_to_char("HASH TABLE is not in effect.\n\r", ch);
	sprintf(log_buf, "HCode rotation=%d,Hcode recycled=%d.\n\r", igHolomade,
		igHoloReuse);
	send_to_char(log_buf, ch);
	sprintf(log_buf, "[%13d] - Number of open descriptors.\n\r",
		open_descs);
	send_to_char(log_buf, ch);
	sprintf(log_buf, "[%13d] - Number of open files.\n\r", open_files);
	send_to_char(log_buf, ch);

#ifdef NEWLIB
	sprintf(log_buf, "[%13d] - Bytes Total.\n\r", mem.bytes_total);
	send_to_char(log_buf, ch);
	sprintf(log_buf, "[%13d] - Chunks Used.\n\r", mem.chunks_used);
	send_to_char(log_buf, ch);
	sprintf(log_buf, "[%13d] - Bytes Used.\n\r", mem.bytes_used);
	send_to_char(log_buf, ch);
	sprintf(log_buf, "[%13d] - Chuncks Free.\n\r", mem.chunks_free);
	send_to_char(log_buf, ch);
	sprintf(log_buf, "[%13d] - Bytes Free.\n\r", mem.bytes_free);
#endif

#ifdef OLDLIB
	sprintf(log_buf, "[%13d] - Total space in arena\n\r", mem.arena);
	send_to_char(log_buf, ch);
	sprintf(log_buf, "[%13d] - Number of ordinary blocks\n\r", mem.ordblks);
	send_to_char(log_buf, ch);
	sprintf(log_buf, "[%13d] - Number of small blocks\n\r", mem.smblks);
	send_to_char(log_buf, ch);
	sprintf(log_buf, "[%13d] - Number of holding blocks\n\r", mem.hblks);
	send_to_char(log_buf, ch);
	sprintf(log_buf, "[%13d] - Space in holding block headers\n\r",
		mem.hblkhd);
	send_to_char(log_buf, ch);
	sprintf(log_buf, "[%13d] - Space in small blocks in use\n\r",
		mem.usmblks);
	send_to_char(log_buf, ch);
	sprintf(log_buf, "[%13d] - Space in free small blocks\n\r",
		mem.fsmblks);
	send_to_char(log_buf, ch);
	sprintf(log_buf, "[%13d] - Space in ordinary blocks in use\n\r",
		mem.uordblks);
	send_to_char(log_buf, ch);
	sprintf(log_buf, "[%13d] - Space in free ordinary blocks\n\r",
		mem.fordblks);
	send_to_char(log_buf, ch);
	sprintf(log_buf, "[%13d] - Cost of enabling keep option\n\r",
		mem.keepcost);
	send_to_char(log_buf, ch);
	sprintf(log_buf, "[%13d] - Maximum size of small blocks\n\r",
		mem.mxfast);
	send_to_char(log_buf, ch);
	sprintf(log_buf,
		"[%13d] - Number of small blocks in a holding block\n\r",
		mem.nlblks);
	send_to_char(log_buf, ch);
	sprintf(log_buf, "[%13d] - Small block rounding factor\n\r", mem.grain);
	send_to_char(log_buf, ch);
	sprintf(log_buf,
		"[%13d] - Space(with overhead) allocated in ordinary block\n\r",
		mem.uordbytes);
	send_to_char(log_buf, ch);
	sprintf(log_buf, "[%13d] - Number of ordinary blocks allocated\n\r",
		mem.allocated);
	send_to_char(log_buf, ch);
	sprintf(log_buf,
		"[%13d] - Bytes used in maintaing the free tree\n\r\n\r",
		mem.treeoverhead);
	send_to_char(log_buf, ch);
#endif
	global_color = 0;
}

void do_zonerestrict(struct char_data *ch, char *argument, int cmd)
{
	int zone, level, x, a;
	char zbuf[MAX_INPUT_LENGTH], lbuf[MAX_INPUT_LENGTH];
	a = 0;
	if (IS_NPC(ch))
		return;

	if (!argument[0]) {
		send_to_char("SYNTAX: zonerestrict zone level\n\r", ch);
		return;
	}
	half_chop(argument, zbuf, lbuf);
	if (!zbuf || !lbuf) {
		send_to_char("SYNTAX: zonerestrict zone level\n\r", ch);
		return;
	}
	zone = atoi(zbuf);
	level = atoi(lbuf);
	if (zone < 1 || zone >= MAX_ZONE) {
		send_to_char("Zone must be from 1 to MAX_ZONE\n\r", ch);
		return;
	}
	for (x = 0; x < MAX_ROOM; x++)
		if (world[x] && world[x]->zone == zone) {
			world[x]->level_restriction = level;
			a++;
		}
	sprintf(log_buf, "DONE [%d] rooms set to level %d in zone %d.\n\r",
		a, level, zone);
	send_to_char(log_buf, ch);
}

void do_rain_gold(struct char_data *ch, char *argument, int cmd)
{
	int loop, piles = 0;
	struct obj_data *mula = NULL;
	extern int top_of_world;

	if (IS_NPC(ch))
		return;

	send_to_char
	    ("OK FINE! But Beware! Vryce wants you to BE CAREFUL with this!\n\r",
	     ch);
	do_echo(ch, "\n\rYou run for cover as it starts raining gold!\n\r", 0);
	for (loop = 1; loop < top_of_world; loop++) {
		if (!world[loop])
			continue;
/* juice
 * have gold rain everywhere in city of medievia, mythago wood, tree, 
 * and any room with a PC or NPC
 */
		if ((world[loop]->zone != 1)
		    && (world[loop]->zone != 11)
		    && (world[loop]->zone != 14)
		    && (world[loop]->people == NULL)
		    )
			continue;
		mula = create_money(number(25, 250));
		obj_to_room(mula, loop);
		piles++;
	}
	sprintf(log_buf, "%s rained %d piles of coins upon Medievia.\n\r",
		GET_NAME(ch), piles);
	do_wiz(ch, log_buf, 5);
	log_hd(log_buf);
}

void do_starttag(struct char_data *ch, char *argument, int cmd)
{
	char name[MAX_INPUT_LENGTH];
	struct char_data *vict = NULL;

	if (IS_NPC(ch))
		return;

	one_argument(argument, name);
	if (!*name)
		send_to_char("Yes, but whom do you wish to TAG?", ch);
	else if (!(vict = get_char_vis(ch, name)))
		send_to_char("No-one by that name is here!", ch);
	else {
		SET_BIT(ORIGINAL(vict)->specials.new_comm_flags, PLR_TAGGED);
		act("$n just TAGGED $N!", FALSE, ch, 0, vict, TO_NOTVICT);
		act("$n just TAGGED you!", FALSE, ch, 0, vict, TO_VICT);
	}
}

void do_removetag(struct char_data *ch, char *argument, int cmd)
{
	char name[MAX_INPUT_LENGTH];
	struct char_data *vict = NULL;

	if (IS_NPC(ch))
		return;

	one_argument(argument, name);
	if (!*name)
		send_to_char
		    ("Yes, but whom do you wish to remove the TAG from?", ch);
	else if (!(vict = get_char_vis(ch, name)))
		send_to_char("No-one by that name is here!", ch);
	else {
		REMOVE_BIT(ORIGINAL(vict)->specials.new_comm_flags, PLR_TAGGED);
		act("$n just TAGGED $N!", FALSE, ch, 0, vict, TO_NOTVICT);
		act("$n just TAGGED you!", FALSE, ch, 0, vict, TO_VICT);
	}
}

void do_tag(struct char_data *ch, char *argument, int cmd)
{
	char name[MAX_INPUT_LENGTH];
	struct char_data *vict = NULL;

	one_argument(argument, name);
	if (!IS_SET(ORIGINAL(ch)->specials.new_comm_flags, PLR_TAGGED)) {
		send_to_char("But you are not tagged!", ch);
		return;
	}
	if (!*name)
		send_to_char("Yes, but whom do you wish to TAG?", ch);
	else if (!(vict = get_char_room_vis(ch, name)))
		send_to_char("No-one by that name is here!", ch);
	else if (!vict->desc)
		send_to_char("Yeah! You wish it was that easy!", ch);
	else {
		if (GET_LEVEL(vict) > 34) {
			send_to_char("Not a good idea.....\n\r", ch);
			return;
		}
		SET_BIT(ORIGINAL(vict)->specials.new_comm_flags, PLR_TAGGED);
		act("$n just TAGGED $N!", FALSE, ch, 0, vict, TO_NOTVICT);
		act("$n just TAGGED you!", FALSE, ch, 0, vict, TO_VICT);
		REMOVE_BIT(ORIGINAL(ch)->specials.new_comm_flags, PLR_TAGGED);
	}
}

void do_searchdb(struct char_data *ch, char *argument, int cmd)
{
	char string[255];
	char object[MAX_INPUT_LENGTH];
	char type[MAX_INPUT_LENGTH];
	char caps[255];
	int x = 0, y = 0;

	if (IS_NPC(ch))
		return;

	half_chop(argument, type, object);
	if (!*object) {
		send_to_char("Search for What?\n\r", ch);
		return;
	}
	x = 0;
	do {
		object[x] = UPPER(object[x]);
	} while (object[++x]);

	global_color = 33;
	ch->specials.setup_page = 1;
	page_setup[0] = MED_NULL;
	if (type[0] == 'c') {
		send_to_char("MOBILE#        NAME\n\r", ch);
		for (x = 0; x < MAX_INDEX; x++)
			if (mobs[x]) {
				strcpy(caps, mobs[x]->player.name);
				y = 0;
				do {
					caps[y] = UPPER(caps[y]);
				} while (caps[++y]);
				if (strstr(caps, object)) {
					sprintf(string, "[%5d] - %s\n\r", x,
						mobs[x]->player.short_descr);
					send_to_char(string, ch);
				}
			}
	} else if (type[0] == 'o') {
		send_to_char("OBJECT#       NAME\n\r", ch);
		for (x = 0; x < MAX_INDEX; x++)
			if (objs[x]) {
				strcpy(caps, objs[x]->name);
				y = 0;
				do {
					caps[y] = UPPER(caps[y]);
				} while (caps[++y]);
				if (strstr(caps, object)) {
					sprintf(string, "[%5d] - %s\n\r", x,
						objs[x]->short_description);
					send_to_char(string, ch);
				}
			}
	} else {
		ch->specials.setup_page = 0;
		send_to_char
		    ("Syntax> searchdb (obj or char) word-to-search\n\r", ch);
		global_color = 0;
		return;
	}
	global_color = 0;
	ch->specials.setup_page = 0;
	page_string(ch->desc, page_setup, 1);
}

void do_disconnect(struct char_data *ch, char *argument, int cmd)
{
	char arg[MAX_STRING_LENGTH];
	char buf[MAX_STRING_LENGTH];
	struct descriptor_data *d = NULL;
	int sdesc;

	if (IS_NPC(ch))
		return;

	one_argument(argument, arg);
	sdesc = atoi(arg);
	if (arg == 0) {
		send_to_char("Illegal descriptor number.\n\r", ch);
		send_to_char("Usage: release <#>\n\r", ch);
		return;
	}
	for (d = descriptor_list; d; d = d->next) {
		if (d->descriptor == sdesc) {
			if (IS_PLAYER(d->character, "Vryce")) {
				send_to_char("Vryce would not like that!\n\r",
					     ch);
				do_tell(ch,
					"vryce [I JUST TRIED TO DISCONNECT YOU!]",
					9);
				return;
			}
			if (IS_PLAYER(d->character, "Io")) {
				send_to_char("Io would not like that!\n\r", ch);
				do_tell(ch,
					"Io [I JUST TRIED TO DISCONNECT YOU!]",
					9);
				return;
			}
			sprintf(buf, "Closing socket to descriptor #%d\n\r",
				sdesc);
			send_to_char(buf, ch);
			sprintf(log_buf, "%s disconnects %s",
				GET_NAME(ch), GET_NAME(d->character));
			log_hd(log_buf);
			close_socket(d);
			return;
		}
	}
	send_to_char("Descriptor not found!\n\r", ch);
}

void do_pardon(struct char_data *ch, char *argument, int cmd)
{
	char person[MAX_INPUT_LENGTH];
	char flag[MAX_INPUT_LENGTH];
	struct char_data *victim = NULL;

	if (IS_NPC(ch))
		return;

	half_chop(argument, person, flag);

	if (!*person) {
		send_to_char("Pardon whom?\n\r", ch);
	} else {
		if (!(victim = get_char(person)))
			send_to_char("They aren't here.\n\r", ch);
		else {
			if (IS_NPC(victim)) {
				send_to_char("Can't pardon NPCs.\n\r", ch);
				return;
			}
			if (!str_cmp("thief", flag)) {
				if (IS_SET
				    (victim->specials.affected_by, AFF_THIEF)) {
					send_to_char("Thief flag removed.\n\r",
						     ch);
					REMOVE_BIT(victim->specials.affected_by,
						   AFF_THIEF);
					send_to_char
					    ("A nice god has pardoned you of your thievery.\n\r",
					     victim);
				} else {
					return;
				}
			}
			if (!str_cmp("killer", flag)) {
				if (IS_SET
				    (victim->specials.affected_by,
				     AFF_KILLER)) {
					send_to_char("Killer flag removed.\n\r",
						     ch);
					REMOVE_BIT(victim->specials.affected_by,
						   AFF_KILLER);
					send_to_char
					    ("A nice god has pardoned you of your murdering.\n\r",
					     victim);
				} else {
					return;
				}
			} else {
				send_to_char("No flag specified!.\n\r", ch);
				return;
			}
			send_to_char("Done.\n\r", ch);
			sprintf(log_buf, "%s pardons %s for %s.",
				GET_NAME(ch), GET_NAME(victim), flag);
			log_hd(log_buf);
		}
	}
}

void do_emote(struct char_data *ch, char *argument, int cmd)
{
	int i;
	char buf[MAX_STRING_LENGTH];

	if (IS_SET(ORIGINAL(ch)->specials.act, PLR_NOEMOTE)) {
		send_to_char("You can't show your emotions!!\n\r", ch);
		return;
	}

	for (i = 0; *(argument + i) == ' '; i++) ;

	if (!*(argument + i))
		send_to_char("Yes.. But what?\n\r", ch);
	else {
		global_color = 32;
		sprintf(buf, "$n %s", argument + i);
		act(buf, FALSE, ch, 0, 0, TO_ROOM);
/*	send_to_char("Ok.\n\r", ch);*/
		act(buf, FALSE, ch, 0, 0, TO_CHAR);
		global_color = 0;
	}
}

void do_echo(struct char_data *ch, char *argument, int cmd)
{
	int i;
	char buf[MAX_STRING_LENGTH];
	struct descriptor_data *point = NULL;

	if (IS_NPC(ch))
		return;

	for (i = 0; *(argument + i) == ' '; i++) ;
	global_color = 33;
	if (!*(argument + i)) {
		send_to_char("That must be a mistake...\n\r", ch);
	} else {
		sprintf(buf, "\n\r%s\n\r", argument + i);
		for (point = descriptor_list; point; point = point->next)
			if (!point->connected)
				send_to_char(buf, point->character);
		/* act(buf, 0, ch, 0, point->character, TO_VICT);
		   send_to_char(buf, ch); */
	}
	global_color = 0;
}

void do_room(struct char_data *ch, char *argument, int cmd)
{
	int i;
	char buf[MAX_INPUT_LENGTH];
	struct descriptor_data *point = NULL;
	char name[MAX_INPUT_LENGTH];
	struct char_data *nope = NULL;

	if (IS_NPC(ch))
		return;
	global_color = 32;
	for (i = 0; *(argument + i) == ' '; i++) ;

	if (!*(argument + i))
		send_to_char("That must be a mistake...\n\r", ch);
	else {
		one_argument(argument + i, name);
/*	nope = get_char_room(name, ch->in_room); */
		nope = get_char_vis(ch, name);
		if (nope) {
			if (!IS_NPC(nope)) {
				send_to_char
				    ("You cannot use ROOM Starting with a players name\rThis stops people from using room as a ventriloquist tool.\n\r",
				     ch);
				return;
			}
		}
		sprintf(buf, "%s", argument + i);
		for (point = descriptor_list; point; point = point->next)
			if (!point->connected)
				if (point->character->in_room == ch->in_room)
					act(buf, 0, ch, 0, point->character,
					    TO_VICT);

		send_to_char(buf, ch);
	}
	global_color = 0;
}

void do_lecho(struct char_data *ch, char *argument, int cmd)
{
	int i;
	char buf[MAX_STRING_LENGTH];
	struct descriptor_data *point = NULL;

	if (IS_NPC(ch))
		return;
	global_color = 31;
	for (i = 0; *(argument + i) == ' '; i++) ;

	if (!*(argument + i))
		send_to_char("That must be a mistake...\n\r", ch);
	else {
		sprintf(buf, "\n\r%s\n\r", argument + i);
		for (point = descriptor_list; point; point = point->next)
			if (!point->connected)
				if (point->character->in_room == ch->in_room)
					act(buf, 0, ch, 0, point->character,
					    TO_VICT);

		send_to_char(buf, ch);
	}
	global_color = 0;
}

void do_gods(struct char_data *ch, char *argument, int cmd)
{
	struct descriptor_data *point = NULL;
	global_color = 32;
	for (point = descriptor_list; point; point = point->next)
		if (!point->connected || point->connected == CON_SOCIAL_ZONE) {

			if (IS_PLAYER(point->character, "Starblade"))
				continue;
			if (GET_LEVEL(point->character) > GET_LEVEL(ch)
			    && IS_AFFECTED(point->character, AFF_INVISIBLE))
				continue;
			if (GET_LEVEL(point->character) > 30 || point->original) {
				if (point->original)
					sprintf(log_buf, "[%12s(%2d)",
						GET_NAME(point->original),
						GET_LEVEL(point->original));
				else
					sprintf(log_buf, "[%12s(%2d)",
						GET_NAME(point->character),
						GET_LEVEL(point->character));
				send_to_char(log_buf, ch);

				if (IS_SET
				    (ORIGINAL(point->character)->specials.
				     new_comm_flags, PLR_NOQUEST))
					send_to_char(" ", ch);
				else
					send_to_char("Q", ch);

				if (IS_SET
				    (ORIGINAL(point->character)->specials.
				     new_comm_flags, PLR_NODISCUSS))
					send_to_char(" ]-", ch);
				else
					send_to_char("D]-", ch);

				if (point->character->desc->snoop.snooping) {
					sprintf(log_buf, "Snooping[%12s]-",
						GET_NAME(point->character->
							 desc->snoop.snooping));
					send_to_char(log_buf, ch);
				}
				if (point->character->desc->original) {
					sprintf(log_buf, "Switched[%12s]-",
						GET_NAME(point->character->
							 desc->character));
					send_to_char(log_buf, ch);
				}
				if (IS_SET
				    (world[point->character->in_room]->
				     room_flags, GODPROOF)
				    && GET_LEVEL(ch) < 35)
/*			&&!IS_PLAYER(ch,"Io")
			&&!IS_PLAYER(ch,"Semele")
			&&!IS_PLAYER(ch,"Vryce")
			)
*/
					sprintf(log_buf,
						"Where[GODPROOF room]\n\r");
				else
					sprintf(log_buf, "Where[%s]\n\r",
						world[point->character->
						      in_room]->name);
				send_to_char(log_buf, ch);
			}

		}
	global_color = 0;
}

void do_trans(struct char_data *ch, char *argument, int cmd)
{
	struct descriptor_data *i = NULL;
	struct char_data *victim = NULL;
	char buf[MAX_STRING_LENGTH];
	int target;

	if (IS_NPC(ch))
		return;
/* juice
 * logging all transes now
 */

	one_argument(argument, buf);
	if (!*buf)
		send_to_char("Whom do you which to transfer?\n\r", ch);
	else if (str_cmp("all", buf)) {
		if (!(victim = get_char_vis(ch, buf)))
			send_to_char("No-one by that name around.\n\r", ch);
		else {
			if (IS_PLAYER(victim, "Io")) {
				send_to_char("Io would not like that!\n\r", ch);
				do_tell(ch, "io I JUST TRIED TO TRANSFER YOU!",
					9);
				return;
			}
			if (IS_PLAYER(victim, "Vryce")) {
				send_to_char("Vryce would not like that!\n\r",
					     ch);
				do_tell(ch,
					"vryce I JUST TRIED TO TRANSFER YOU!",
					9);
				return;
			}
			target = ch->in_room;
			if (IS_SET(world[target]->room_flags, GODPROOF)) {
				if (GET_LEVEL(ch) < 35)
/*		if(!IS_PLAYER(ch,"Vryce")
			&& !IS_PLAYER(ch, "Io")
			&& !IS_PLAYER(ch, "Firm"))
*/
					if (!IS_NPC(victim))
						if (victim->desc->snoop.
						    snoop_by) {
							send_to_char
							    ("Sorry, Room is GODPROOF!\n\r",
							     ch);
							return;
						}
			}
			if (GET_LEVEL(ch) < 35 &&
			    (GET_CONTINENT(ch) != GET_CONTINENT(victim)
			     || world[ch->in_room]->zone == 65
			     || world[victim->in_room]->zone == 65)) {
				send_to_char
				    ("No transing between continents or from or on ship.\n\r",
				     ch);
				return;
			}

			act("$n disappears in a mushroom cloud.",
			    FALSE, victim, 0, 0, TO_ROOM);
			StopFlying(victim);
			MOUNTMOVE = TRUE;
			char_from_room(victim);
			char_to_room(victim, target);
			MOUNTMOVE = FALSE;
			act("$n arrives from a puff of smoke.",
			    FALSE, victim, 0, 0, TO_ROOM);
			act("$n has transferred you!", FALSE, ch, 0, victim,
			    TO_VICT);
			do_look(victim, "", 15);
			send_to_char("Ok.\n\r", ch);
			sprintf(log_buf, "%s transfers %s to [%d]%s",
				GET_NAME(ch), GET_NAME(victim), target,
				world[target]->name);
			log_hd(log_buf);
		}
	} else {		/* Trans All */
		for (i = descriptor_list; i; i = i->next)
			if (i->character != ch && !i->connected) {
				victim = i->character;
				if (IS_PLAYER(victim, "Vryce"))
					continue;
				if (IS_PLAYER(victim, "Io"))
					continue;

				act("$n disappears in a mushroom cloud.",
				    FALSE, victim, 0, 0, TO_ROOM);
				target = ch->in_room;
				StopFlying(victim);
				MOUNTMOVE = TRUE;
				char_from_room(victim);
				char_to_room(victim, target);
				MOUNTMOVE = FALSE;
				act("$n arrives from a puff of smoke.",
				    FALSE, victim, 0, 0, TO_ROOM);
				act("$n has transferred you!", FALSE, ch, 0,
				    victim, TO_VICT);
				do_look(victim, "", 15);
				sprintf(log_buf, "%s transfers %s to [%d]%s",
					GET_NAME(ch), GET_NAME(victim), target,
					world[target]->name);
				log_hd(log_buf);
			}

		send_to_char("Ok.\n\r", ch);
	}
}

void do_at(struct char_data *ch, char *argument, int cmd)
{
	char command[MAX_INPUT_LENGTH], loc_str[MAX_INPUT_LENGTH];
	int loc_nr, location, original_loc;
	struct char_data *target_mob = NULL;
	struct obj_data *target_obj = NULL;
	extern int top_of_world;

	target_mob = NULL;
	target_obj = NULL;
	if (IS_NPC(ch))
		return;

	half_chop(argument, loc_str, command);
	if (!*loc_str) {
		send_to_char("You must supply a room number or a name.\n\r",
			     ch);
		return;
	}

	if (isdigit(*loc_str)) {
		loc_nr = atoi(loc_str);
		if ((loc_nr >= MAX_ROOM) || (loc_nr < 0)) {
			send_to_char("The world isn't that big!\n\r", ch);
			return;
		}
		if (!world[loc_nr]) {
			send_to_char("That room doesn't exist!\n\r", ch);
			return;
		}

		if (IS_SET(world[loc_nr]->room_flags, GODPROOF)
		    && GET_LEVEL(ch) < 35) {
/*		&&!IS_PLAYER(ch,"Io")
		&&!IS_PLAYER(ch,"Starblade")
		&&!IS_PLAYER(ch,"Semele")
		&&!IS_PLAYER(ch,"Shalafi")
		&&!IS_PLAYER(ch,"Vryce")
		&&!IS_PLAYER(ch,"Lena")
		&&!IS_PLAYER(ch,"Sultress")){
*/
			send_to_char("Sorry that room is GODPROOF.\n\r", ch);
			return;
		}
		for (location = 0; location <= top_of_world; location++)
			if (world[location]
			    && world[location]->number == loc_nr)
				break;
			else if (location == top_of_world) {
				send_to_char
				    ("No room exists with that number.\n\r",
				     ch);
				return;
			}
	} else if ((target_mob = get_char_vis(ch, loc_str)) != NULL)
		location = target_mob->in_room;
	else if ((target_obj = get_obj_vis(ch, loc_str)) != NULL)
		if (target_obj->in_room != NOWHERE)
			location = target_obj->in_room;
		else {
			send_to_char("The object is not available.\n\r", ch);
			return;
	} else {
		send_to_char("No such creature or object around.\n\r", ch);
		return;
	}

	/* a location has been found. */

	if (IS_SET(world[location]->room_flags, GODPROOF)
	    && GET_LEVEL(ch) < 35) {
/*		&&!IS_PLAYER(ch,"Io")
		&&!IS_PLAYER(ch,"Semele")
		&&!IS_PLAYER(ch,"Shalafi")
		&&!IS_PLAYER(ch,"Vryce")
		&&!IS_PLAYER(ch,"Firm")
		&&!IS_PLAYER(ch,"Sultress")){
*/
		send_to_char("Sorry that room is GODPROOF.\n\r", ch);
		return;
	}
	if (location == iFlyStoreRoom) {
		send_to_char
		    ("Sorry you cant look at the FLYING storage room.\n\r", ch);
		return;
	}
	if (GET_LEVEL(ch) < 35) {
		sprintf(log_buf, "%s did AT %s - %s", GET_NAME(ch), loc_str,
			command);
		log_hd(log_buf);
	}
	original_loc = ch->in_room;
	MOUNTMOVE = TRUE;
	char_from_room(ch);
	char_to_room(ch, location);
	MOUNTMOVE = FALSE;
	command_interpreter(ch, command);

	/* check if the guy's still there */
	for (target_mob = world[location]->people; target_mob; target_mob =
	     target_mob->next_in_room)
		if (ch == target_mob) {
			MOUNTMOVE = TRUE;
			char_from_room(ch);
			char_to_room(ch, original_loc);
			MOUNTMOVE = FALSE;
		}
}

void do_goto(struct char_data *ch, char *argument, int cmd)
{
	char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
	int loc_nr, location, i;
	struct char_data *target_mob = NULL;
	struct obj_data *target_obj = NULL;

	void do_look(struct char_data *ch, char *argument, int cmd);

	if (IS_NPC(ch))
		return;

	argument = one_argument(argument, buf);
	if (!*buf) {
		send_to_char("You must supply a room number or a name.\n\r",
			     ch);
		return;
	}

	if (isdigit(*buf)) {
		loc_nr = atoi(buf);
		one_argument(argument, buf2);
		if (buf2[0]) {
			i = atoi(buf2);
			if (loc_nr < 0 || loc_nr >= MAXHOLO || i < 0
			    || i >= MAXHOLO) {
				send_to_char("Holoworld is not that big.\n\r",
					     ch);
				return;
			}
			if (Holo[loc_nr][i] <= 255) {
				iMakeHoloRoom(loc_nr, i);
			}
			location = Holo[loc_nr][i];
			if (!world[location]) {
				send_to_char("Trouble making room!", ch);
				return;
			}
		} else {
			if (loc_nr < 0 || loc_nr > MAX_ROOM || !world[loc_nr]) {
				send_to_char
				    ("No room exists with that number.\n\r",
				     ch);
				return;
			}
			location = loc_nr;
		}
	} else if ((target_mob = get_char_vis(ch, buf)) != NULL) {
		location = target_mob->in_room;
	} else if ((target_obj = get_obj_vis(ch, buf)) != NULL) {
		if (target_obj->in_room != NOWHERE)
			location = target_obj->in_room;
		else {
			send_to_char("The object is not available.\n\r", ch);
			return;
		}
	} else {
		send_to_char("No such creature or object around.\n\r", ch);
		return;
	}

	/* a location has been found. */
	if (GET_LEVEL(ch) == 32
	    && (world[location]->zone != ch->specials.editzone)) {
		send_to_char("You are not authorized to enter that zone.\r\n",
			     ch);
		return;
	}

	if (IS_SET(world[location]->room_flags, GODPROOF)
	    && GET_LEVEL(ch) < 35) {
/*	&& !IS_PLAYER(ch,"Vryce") 
	&& !IS_PLAYER(ch,"Io") 
	&& !IS_PLAYER(ch,"Semele") 
	&& !IS_PLAYER(ch,"Shalafi")
	&& !IS_PLAYER(ch,"Lena") 
	&& !IS_PLAYER(ch,"Sultress")){
*/
		send_to_char("Sorry that room is GODPROOF.\n\r", ch);
		return;
	}
#ifdef PACIFIST

	if (world[location]->zone == 198) {
		send_to_char("COMBS off limits in PACIFIST mode.\n\r", ch);
		return;
	}
#endif
	if (world[location]->zone == iFlyStoreRoom) {
		send_to_char("FlyStoreRoomofflimits.\n\r", ch);
		return;
	}

	if (!ch->specials.wizInvis)
		act("$n disappears in a puff of smoke.", FALSE, ch, 0, 0,
		    TO_ROOM);
	MOUNTMOVE = TRUE;
	char_from_room(ch);
	char_to_room(ch, location);
	MOUNTMOVE = FALSE;
	global_color = 31;
	if (!ch->specials.wizInvis)
		act("$n appears with an ear-splitting bang.", FALSE, ch, 0, 0,
		    TO_ROOM);
	global_color = 0;
	do_look(ch, "", 15);
}

void do_showstats(struct char_data *ch, char *argument, int cmd)
{
	extern char *spells[];
	struct affected_type *aff = NULL;
	char arg1[MAX_STRING_LENGTH];
	char buf[MAX_STRING_LENGTH];
	char buf2[MAX_STRING_LENGTH];
	struct char_data *k = NULL;
	struct char_data *to = NULL;
	struct obj_data *j = NULL;
	int i, i2, x, y;

	/* for chars */
	extern char *affected_bits[];
	extern char *apply_types[];
	extern char *pc_class_types[];
	extern char *player_bits[];
	extern char *position_types[];
	extern char *connected_types[];

	if (IS_NPC(ch))
		return;

	argument = one_argument(argument, arg1);

	/* no argument */
	if (!*arg1) {
		to = ch;
	} else if (!(to = get_char_room_vis(ch, arg1))) {
		send_to_char("No-one by that name is here!", ch);
		return;
	}
	k = ch;
	switch (k->player.sex) {
	case SEX_NEUTRAL:
		strcpy(buf, "NEUTRAL-SEX");
		break;
	case SEX_MALE:
		strcpy(buf, "MALE");
		break;
	case SEX_FEMALE:
		strcpy(buf, "FEMALE");
		break;
	default:
		strcpy(buf, "ILLEGAL-SEX!!");
		break;
	}

	sprintf(buf2, " %s - Name : %s [R-Number%d], In room [%d]\n\r",
		(!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")),
		GET_NAME(k), k->nr, world[k->in_room]->number);
	strcat(buf, buf2);
	send_to_char(buf, to);

	strcpy(buf, "Short description: ");
	strcat(buf, (k->player.short_descr ? k->player.short_descr : "None"));
	strcat(buf, "\n\r");
	send_to_char(buf, to);

	strcpy(buf, "Title: ");
	strcat(buf, (k->player.title ? k->player.title : "None"));
	strcat(buf, "\n\r");
	send_to_char(buf, to);

	send_to_char("Long description: ", to);
	if (k->player.long_descr)
		send_to_char(k->player.long_descr, to);
	else
		send_to_char("None", to);
	send_to_char("\n\r", to);
	strcpy(buf, "Class: ");
	sprinttype(k->player.class, pc_class_types, buf2);
	strcat(buf, buf2);

	sprintf(buf2, "   Level [%d] Alignment[%d]\n\r", k->player.level,
		k->specials.alignment);
	strcat(buf, buf2);
	send_to_char(buf, to);

	sprintf(buf,
		"Birth : [%ld]secs, Logon[%ld]secs, Played[%d]secs\n\r",
		k->player.time.birth,
		k->player.time.logon, k->player.time.played);

	send_to_char(buf, to);

	sprintf(buf,
		"Age: [%d] Years,  [%d] Months,  [%d] Days,  [%d] Hours\n\r",
		age(k).year, age(k).month, age(k).day, age(k).hours);
	send_to_char(buf, to);

	sprintf(buf,
		"Height [%d]cm  Weight [%d]stones \n\r",
		GET_HEIGHT(k), GET_WEIGHT(k));
	send_to_char(buf, to);

	sprintf(buf,
		"Speaks[%d/%d/%d], (STL[%d]/per[%d]/NSTL[%d])\n\r",
		k->player.talks[0],
		k->player.talks[1],
		k->player.talks[2],
		k->specials.practices,
		int_app[GET_INT(k)].learn, wis_app[GET_WIS(k)].bonus);
	send_to_char(buf, to);

	sprintf(buf,
		"Str:[%d]  Int:[%d]  Wis:[%d]  Dex:[%d]  Con:[%d]\n\r",
		GET_STR(k), GET_INT(k), GET_WIS(k), GET_DEX(k), GET_CON(k));
	send_to_char(buf, to);

	sprintf(buf,
		"Mana p.:[%d/%d+%d]  Hit p.:[%d/%d+%d]  Move p.:[%d/%d+%d]\n\r",
		GET_MANA(k), mana_limit(k), mana_gain(k),
		GET_HIT(k), hit_limit(k), hit_gain(k),
		GET_MOVE(k), move_limit(k), move_gain(k));
	send_to_char(buf, to);

	sprintf(buf,
		"AC:[%d], Coins: [%d], Exp: [%d], Hitroll: [%d], Damroll: [%d]\n\r",
		GET_AC(k),
		GET_GOLD(k), GET_EXP(k), k->points.hitroll, k->points.damroll);
	send_to_char(buf, to);

	sprinttype(GET_POS(k), position_types, buf2);
	sprintf(buf, "Position: %s, Fighting: %s", buf2,
		((k->specials.fighting) ? GET_NAME(k->specials.fighting)
		 : "Nobody"));
	if (k->desc) {
		sprinttype(k->desc->connected, connected_types, buf2);
		strcat(buf, ", Connected: ");
		strcat(buf, buf2);
	}
	strcat(buf, "\n\r");
	send_to_char(buf, to);

	strcpy(buf, "Default position: ");
	sprinttype((k->specials.default_pos), position_types, buf2);
	strcat(buf, buf2);
	strcat(buf, ",PC flags: ");
	sprintbit(k->specials.act, player_bits, buf2);
	strcat(buf, buf2);
	sprintf(buf2, ",Timer [%d] \n\r", k->specials.timer);
	strcat(buf, buf2);
	send_to_char(buf, to);

	sprintf(buf, "Carried weight: %d   Carried items: %d\n\r",
		IS_CARRYING_W(k), IS_CARRYING_N(k));
	send_to_char(buf, to);

	for (i = 0, j = k->carrying; j; j = j->next_content, i++) ;
	sprintf(buf, "Items in inventory: %d, ", i);

	for (i = 0, i2 = 0; i < MAX_WEAR; i++)
		if (k->equipment[i])
			i2++;
	sprintf(buf2, "Items in equipment: %d\n\r", i2);
	strcat(buf, buf2);
	send_to_char(buf, to);

	sprintf(buf, "Apply saving throws: [%d] [%d] [%d] [%d] [%d]\n\r",
		k->specials.apply_saving_throw[0],
		k->specials.apply_saving_throw[1],
		k->specials.apply_saving_throw[2],
		k->specials.apply_saving_throw[3],
		k->specials.apply_saving_throw[4]);
	send_to_char(buf, to);

	sprintf(buf, "Thirst: %d, Hunger: %d, Drunk: %d, Owns home: %d\n\r",
		k->specials.conditions[THIRST],
		k->specials.conditions[FULL],
		k->specials.conditions[DRUNK], k->specials.home_number);
	send_to_char(buf, to);

	send_to_char("Followers are:\n\r", to);
	for (x = 0; x < 3; x++)
		for (y = 0; y < 3; y++)
			if (k->formation[x][y])
				act("    $N", FALSE, ch, 0, k->formation[x][y],
				    TO_CHAR);

	/* Showing the bitvector */
	sprintbit(k->specials.affected_by, affected_bits, buf);
	send_to_char("Affected by: ", to);
	strcat(buf, "\n\r");
	send_to_char(buf, to);

	/* Routine to show what spells a char is affected by */
	if (k->affected) {
		send_to_char("\n\rAffecting Spells:\n\r--------------\n\r", to);
		for (aff = k->affected; aff; aff = aff->next) {
			sprintf(buf,
				"Spell : '%s'\n\r", spells[(int)aff->type - 1]);
			send_to_char(buf, to);
			sprintf(buf, "     Modifies %s by %d points\n\r",
				apply_types[(int)aff->location], aff->modifier);
			send_to_char(buf, to);
			sprintf(buf, "     Expires in %3d hours, Bits set ",
				aff->duration);
			send_to_char(buf, to);
			sprintbit(aff->bitvector, affected_bits, buf);
			strcat(buf, "\n\r");
			send_to_char(buf, to);
		}
	}
	if (to != ch) {
		sprintf(buf, "You show %s your stats.\n\r", GET_NAME(to));
		send_to_char(buf, ch);
	}
}

void do_stat_room(struct char_data *ch, char *argument, int cmd)
{
	char buf[MAX_STRING_LENGTH];
	char buf2[MAX_STRING_LENGTH];
	struct room_data *rm = NULL;
	struct extra_descr_data *desc = NULL;
	extern char *room_bits[];
	extern char *extra_room_flags[];
	extern char *sector_types[];
	rm = world[ch->in_room];
	if (GET_LEVEL(ch) == 32 && (rm->zone != ch->specials.editzone)) {
		send_to_char
		    ("You are not authorized to do that in this zone.\r\n", ch);
		return;
	}
	sprintf(buf,
		"Room name: %s, zon# %d, room# %d (%d,%d)\n\r",
		rm->name, rm->zone, rm->number, HOLORX(rm->number),
		HOLORY(rm->number));
	global_color = 33;
	send_to_char(buf, ch);
	sprinttype(rm->sector_type, sector_types, buf2);
	global_color = 32;
	sprintf(buf, "Sector type : %s", buf2);
	send_to_char(buf, ch);
	global_color = 0;
	strcpy(buf, "\n\rSpecial procedure : ");
	strcat(buf, (rm->funct) ? "Exists\n\r" : "None\n\r");
	send_to_char(buf, ch);
	global_color = 31;
	send_to_char("Room flags: ", ch);
	sprintbit((long)rm->room_flags, room_bits, buf);
	strcat(buf, "\n\r");
	send_to_char(buf, ch);
	send_to_char("Extra Room flags: ", ch);
	sprintbit((long)rm->extra_flags, extra_room_flags, buf);
	strcat(buf, "\n\r");
	send_to_char(buf, ch);
	global_color = 35;
	send_to_char
	    ("|MOVE |  |CLASS|  |LEVEL|  |ALIGN|  |PRESR|  |TEMP |  |MOUNT|\n\r|MOD  |  |RESTR|  |RESTR|  |RESTR|  |MOD  |  |MOD  |  |RESTR|\n\r",
	     ch);
	sprintf(buf,
		"\\%5d/  \\%5d/  \\%5d/  \\%5d/  \\%5d/  \\%5d/  \\%5d/\n\r",
		rm->move_mod, rm->class_restriction, rm->level_restriction,
		rm->align_restriction, rm->pressure_mod, rm->temperature_mod,
		rm->mount_restriction);
	send_to_char(buf, ch);
	global_color = 36;
	strcpy(buf, "Extra description keywords(s): ");
	if (rm->ex_description) {
		strcat(buf, "\n\r");
		for (desc = rm->ex_description; desc; desc = desc->next) {
			strcat(buf, desc->keyword);
			strcat(buf, "\n\r");
		}
		strcat(buf, "\n\r");
		send_to_char(buf, ch);
	} else {
		strcat(buf, "None\n\r");
		send_to_char(buf, ch);
	}
	global_color = 0;
}

void do_skillstat(struct char_data *ch, char *argument, int cmd)
{
	char arg1[MAX_STRING_LENGTH];
	char buf[MAX_STRING_LENGTH];
	struct char_data *k = NULL;

	if (IS_NPC(ch))
		return;

	one_argument(argument, arg1);
	if (!*arg1) {
		send_to_char("Who's skills would you like to check?\n\r", ch);
		return;
	}

	if ((k = get_char_vis(ch, arg1)) == NULL) {
		send_to_char("No such character.\n\r", ch);
		return;
	}
	sprintf(buf, "\n\rSKILL  %%LEARNED  RECOGNIZED?\n\r");
	send_to_char(buf, ch);
	sprintf(buf, "%s %3d %d   %s %3d %d   %s %3d %d   %s %3d %d\n\r",
		"acd blast  ", (int)k->skills[SPELL_ACID_BLAST].learned,
		(int)k->skills[SPELL_ACID_BLAST].recognise,
		"anim_dead  ", (int)k->skills[SPELL_ANIMATE_DEAD].learned,
		(int)k->skills[SPELL_ANIMATE_DEAD].recognise,
		"armor      ", (int)k->skills[SPELL_ARMOR].learned,
		(int)k->skills[SPELL_ARMOR].recognise,
		"bless      ", (int)k->skills[SPELL_BLESS].learned,
		(int)k->skills[SPELL_BLESS].recognise);
	send_to_char(buf, ch);
	sprintf(buf, "%s %3d %d   %s %3d %d   %s %3d %d   %s %3d %d\n\r",
		"blindness  ", (int)k->skills[SPELL_BLINDNESS].learned,
		(int)k->skills[SPELL_BLINDNESS].recognise,
		"brng hands ", (int)k->skills[SPELL_BURNING_HANDS].learned,
		(int)k->skills[SPELL_BURNING_HANDS].recognise,
		"call litng ", (int)k->skills[SPELL_CALL_LIGHTNING].learned,
		(int)k->skills[SPELL_CALL_LIGHTNING].recognise,
		"cau light  ", (int)k->skills[SPELL_CAUSE_LIGHT].learned,
		(int)k->skills[SPELL_CAUSE_LIGHT].recognise);
	send_to_char(buf, ch);
	sprintf(buf, "%s %3d %d   %s %3d %d   %s %3d %d   %s %3d %d\n\r",
		"cau critic ", (int)k->skills[SPELL_CAUSE_CRITICAL].learned,
		(int)k->skills[SPELL_CAUSE_CRITICAL].recognise,
		"cau srious ", (int)k->skills[SPELL_CAUSE_CRITICAL].learned,
		(int)k->skills[SPELL_CAUSE_CRITICAL].recognise,
		"charm      ", (int)k->skills[SPELL_CHARM_PERSON].learned,
		(int)k->skills[SPELL_CHARM_PERSON].recognise,
		"chil touch ", (int)k->skills[SPELL_CHILL_TOUCH].learned,
		(int)k->skills[SPELL_CHILL_TOUCH].recognise);
	send_to_char(buf, ch);
	sprintf(buf, "%s %3d %d   %s %3d %d   %s %3d %d   %s %3d %d\n\r",
		"clone      ", (int)k->skills[SPELL_CHILL_TOUCH].learned,
		(int)k->skills[SPELL_CHILL_TOUCH].recognise,
		"clr spray  ", (int)k->skills[SPELL_COLOUR_SPRAY].learned,
		(int)k->skills[SPELL_COLOUR_SPRAY].recognise,
		"conj elem  ", (int)k->skills[SPELL_CONJURE_ELEMENTAL].learned,
		(int)k->skills[SPELL_CONJURE_ELEMENTAL].recognise,
		"contlight  ", (int)k->skills[SPELL_CONT_LIGHT].learned,
		(int)k->skills[SPELL_CONT_LIGHT].recognise);
	send_to_char(buf, ch);
	sprintf(buf, "%s %3d %d   %s %3d %d   %s %3d %d   %s %3d %d\n\r",
		"ctrl wethr ", (int)k->skills[SPELL_CONTROL_WEATHER].learned,
		(int)k->skills[SPELL_CONTROL_WEATHER].recognise,
		"creat food ", (int)k->skills[SPELL_CREATE_FOOD].learned,
		(int)k->skills[SPELL_CREATE_FOOD].recognise,
		"creat H2O  ", (int)k->skills[SPELL_CREATE_FOOD].learned,
		(int)k->skills[SPELL_CREATE_FOOD].recognise,
		"cure blind ", (int)k->skills[SPELL_CURE_BLIND].learned,
		(int)k->skills[SPELL_CURE_BLIND].recognise);
	send_to_char(buf, ch);
	sprintf(buf, "%s %3d %d   %s %3d %d   %s %3d %d   %s %3d %d\n\r",
		"cure crit  ", (int)k->skills[SPELL_CURE_CRITIC].learned,
		(int)k->skills[SPELL_CURE_CRITIC].recognise,
		"cure light ", (int)k->skills[SPELL_CURE_LIGHT].learned,
		(int)k->skills[SPELL_CURE_LIGHT].recognise,
		"cure ser   ", (int)k->skills[SPELL_CURE_SERIOUS].learned,
		(int)k->skills[SPELL_CURE_SERIOUS].recognise,
		"curse      ", (int)k->skills[SPELL_CURSE].learned,
		(int)k->skills[SPELL_CURSE].recognise);
	send_to_char(buf, ch);
	sprintf(buf, "%s %3d %d   %s %3d %d   %s %3d %d   %s %3d %d\n\r",
		"demonfire  ", (int)k->skills[SPELL_DEMONFIRE].learned,
		(int)k->skills[SPELL_DEMONFIRE].recognise,
		"det evil   ", (int)k->skills[SPELL_DETECT_EVIL].learned,
		(int)k->skills[SPELL_DETECT_EVIL].recognise,
		"det invis  ", (int)k->skills[SPELL_DETECT_INVISIBLE].learned,
		(int)k->skills[SPELL_DETECT_INVISIBLE].recognise,
		"det magic  ", (int)k->skills[SPELL_DETECT_MAGIC].learned,
		(int)k->skills[SPELL_DETECT_MAGIC].recognise);
	send_to_char(buf, ch);
	sprintf(buf, "%s %3d %d   %s %3d %d   %s %3d %d   %s %3d %d\n\r",
		"det poison ", (int)k->skills[SPELL_DETECT_POISON].learned,
		(int)k->skills[SPELL_DETECT_POISON].recognise,
		"disp evil  ", (int)k->skills[SPELL_DISPEL_EVIL].learned,
		(int)k->skills[SPELL_DISPEL_EVIL].recognise,
		"disp magic ", (int)k->skills[SPELL_DISPEL_MAGIC].learned,
		(int)k->skills[SPELL_DISPEL_MAGIC].recognise,
		"drown      ", (int)k->skills[SPELL_DROWN].learned,
		(int)k->skills[SPELL_DROWN].recognise);
	send_to_char(buf, ch);
	sprintf(buf, "%s %3d %d   %s %3d %d   %s %3d %d   %s %3d %d\n\r",
		"earthquake ", (int)k->skills[SPELL_EARTHQUAKE].learned,
		(int)k->skills[SPELL_EARTHQUAKE].recognise,
		"enchant    ", (int)k->skills[SPELL_ENCHANT_WEAPON].learned,
		(int)k->skills[SPELL_ENCHANT_WEAPON].recognise,
		"enrg drain ", (int)k->skills[SPELL_ENERGY_DRAIN].learned,
		(int)k->skills[SPELL_ENERGY_DRAIN].recognise,
		"fae fire   ", (int)k->skills[SPELL_FAERIE_FIRE].learned,
		(int)k->skills[SPELL_FAERIE_FIRE].recognise);
	send_to_char(buf, ch);
	sprintf(buf, "%s %3d %d   %s %3d %d   %s %3d %d   %s %3d %d\n\r",
		"fae fog    ", (int)k->skills[SPELL_FAERIE_FOG].learned,
		(int)k->skills[SPELL_FAERIE_FOG].recognise,
		"fireball   ", (int)k->skills[SPELL_FIREBALL].learned,
		(int)k->skills[SPELL_FIREBALL].recognise,
		"fear       ", (int)k->skills[SPELL_FEAR].learned,
		(int)k->skills[SPELL_FEAR].recognise,
		"fly        ", (int)k->skills[SPELL_FLY].learned,
		(int)k->skills[SPELL_FLY].recognise);
	send_to_char(buf, ch);
	sprintf(buf, "%s %3d %d   %s %3d %d   %s %3d %d   %s %3d %d\n\r",
		"flamestrik ", (int)k->skills[SPELL_FLAMESTRIKE].learned,
		(int)k->skills[SPELL_FLAMESTRIKE].recognise,
		"gate       ", (int)k->skills[SPELL_GATE].learned,
		(int)k->skills[SPELL_GATE].recognise,
		"harm       ", (int)k->skills[SPELL_HARM].learned,
		(int)k->skills[SPELL_HARM].recognise,
		"hands wind ", (int)k->skills[SPELL_HANDS_OF_WIND].learned,
		(int)k->skills[SPELL_HANDS_OF_WIND].recognise);
	send_to_char(buf, ch);
	sprintf(buf, "%s %3d %d   %s %3d %d   %s %3d %d   %s %3d %d\n\r",
		"heal       ", (int)k->skills[SPELL_HEAL].learned,
		(int)k->skills[SPELL_HEAL].recognise,
		"identify   ", (int)k->skills[SPELL_IDENTIFY].learned,
		(int)k->skills[SPELL_IDENTIFY].recognise,
		"infravis   ", (int)k->skills[SPELL_INFRAVISION].learned,
		(int)k->skills[SPELL_INFRAVISION].recognise,
		"invis      ", (int)k->skills[SPELL_INVISIBLE].learned,
		(int)k->skills[SPELL_INVISIBLE].recognise);
	send_to_char(buf, ch);
	sprintf(buf, "%s %3d %d   %s %3d %d   %s %3d %d   %s %3d %d\n\r",
		"light bolt ", (int)k->skills[SPELL_LIGHTNING_BOLT].learned,
		(int)k->skills[SPELL_LIGHTNING_BOLT].recognise,
		"locate     ", (int)k->skills[SPELL_LOCATE_OBJECT].learned,
		(int)k->skills[SPELL_LOCATE_OBJECT].recognise,
		"mass invis ", (int)k->skills[SPELL_MASS_INVISIBILITY].learned,
		(int)k->skills[SPELL_MASS_INVISIBILITY].recognise,
		"missle     ", (int)k->skills[SPELL_MAGIC_MISSILE].learned,
		(int)k->skills[SPELL_MAGIC_MISSILE].recognise);
	send_to_char(buf, ch);
	sprintf(buf, "%s %3d %d   %s %3d %d   %s %3d %d   %s %3d %d\n\r",
		"plague     ", (int)k->skills[SPELL_PLAGUE].learned,
		(int)k->skills[SPELL_PLAGUE].recognise,
		"poison     ", (int)k->skills[SPELL_POISON].learned,
		(int)k->skills[SPELL_POISON].recognise,
		"p frm evil ", (int)k->skills[SPELL_PROTECT_FROM_EVIL].learned,
		(int)k->skills[SPELL_PROTECT_FROM_EVIL].recognise,
		"refresh    ", (int)k->skills[SPELL_REFRESH].learned,
		(int)k->skills[SPELL_REFRESH].recognise);
	send_to_char(buf, ch);
	sprintf(buf, "%s %3d %d   %s %3d %d   %s %3d %d   %s %3d %d\n\r",
		"rem curse  ", (int)k->skills[SPELL_REMOVE_CURSE].learned,
		(int)k->skills[SPELL_REMOVE_CURSE].recognise,
		"rem poison ", (int)k->skills[SPELL_REMOVE_POISON].learned,
		(int)k->skills[SPELL_REMOVE_POISON].recognise,
		"know align ", (int)k->skills[SPELL_KNOW_ALIGNMENT].learned,
		(int)k->skills[SPELL_KNOW_ALIGNMENT].recognise,
		"sanc       ", (int)k->skills[SPELL_SANCTUARY].learned,
		(int)k->skills[SPELL_SANCTUARY].recognise);
	send_to_char(buf, ch);
	sprintf(buf, "%s %3d %d   %s %3d %d   %s %3d %d   %s %3d %d\n\r",
		"sandstorm  ", (int)k->skills[SPELL_SANDSTORM].learned,
		(int)k->skills[SPELL_SANDSTORM].recognise,
		"sense life ", (int)k->skills[SPELL_SENSE_LIFE].learned,
		(int)k->skills[SPELL_SENSE_LIFE].recognise,
		"shield     ", (int)k->skills[SPELL_SHIELD].learned,
		(int)k->skills[SPELL_SHIELD].recognise,
		"shk grasp  ", (int)k->skills[SPELL_SHOCKING_GRASP].learned,
		(int)k->skills[SPELL_SHOCKING_GRASP].recognise);
	send_to_char(buf, ch);
	sprintf(buf, "%s %3d %d   %s %3d %d   %s %3d %d   %s %3d %d\n\r",
		"sleep      ", (int)k->skills[SPELL_SLEEP].learned,
		(int)k->skills[SPELL_SLEEP].recognise,
		"stone skin ", (int)k->skills[SPELL_STONE_SKIN].learned,
		(int)k->skills[SPELL_STONE_SKIN].recognise,
		"strength   ", (int)k->skills[SPELL_STRENGTH].learned,
		(int)k->skills[SPELL_STRENGTH].recognise,
		"summon     ", (int)k->skills[SPELL_SUMMON].learned,
		(int)k->skills[SPELL_SUMMON].recognise);
	send_to_char(buf, ch);
	sprintf(buf, "%s %3d %d   %s %3d %d   %s %3d %d   %s %3d %d\n\r",
		"teleport   ", (int)k->skills[SPELL_TELEPORT].learned,
		(int)k->skills[SPELL_TELEPORT].recognise,
		"trn undead ", (int)k->skills[SPELL_TURN_UNDEAD].learned,
		(int)k->skills[SPELL_TURN_UNDEAD].recognise,
		"ventril    ", (int)k->skills[SPELL_VENTRILOQUATE].learned,
		(int)k->skills[SPELL_VENTRILOQUATE].recognise,
		"weaken     ", (int)k->skills[SPELL_WEAKEN].learned,
		(int)k->skills[SPELL_WEAKEN].recognise);
	send_to_char(buf, ch);
	sprintf(buf, "%s %3d %d   %s %3d %d   %s %3d %d   %s %3d %d\n\r",
		"fireshield ", (int)k->skills[SPELL_FIRESHIELD].learned,
		(int)k->skills[SPELL_FIRESHIELD].recognise,
		"transport  ", (int)k->skills[SPELL_TRANSPORT].learned,
		(int)k->skills[SPELL_TRANSPORT].recognise,
		"mass levit ", (int)k->skills[SPELL_MASS_LEVITATION].learned,
		(int)k->skills[SPELL_MASS_LEVITATION].recognise,
		"sense death", (int)k->skills[SPELL_SENSE_DEATH].learned,
		(int)k->skills[SPELL_SENSE_DEATH].recognise);
	send_to_char(buf, ch);
	sprintf(buf, "%s %3d %d   %s %3d %d   %s %3d %d   %s %3d %d\n\r",
		"breath watr", (int)k->skills[SPELL_BREATH_WATER].learned,
		(int)k->skills[SPELL_BREATH_WATER].recognise,
		"shield room", (int)k->skills[SPELL_SHIELD_ROOM].learned,
		(int)k->skills[SPELL_SHIELD_ROOM].recognise,
		"chain light", (int)k->skills[SPELL_CHAIN_LIGHTNING].learned,
		(int)k->skills[SPELL_CHAIN_LIGHTNING].recognise,
		"map catacom", (int)k->skills[SPELL_MAP_CATACOMBS].learned,
		(int)k->skills[SPELL_MAP_CATACOMBS].recognise);
	send_to_char(buf, ch);
	sprintf(buf, "%s %3d %d   %s %3d %d   %s %3d %d   %s %3d %d\n\r",
		"farsight   ", (int)k->skills[SPELL_FARSIGHT].learned,
		(int)k->skills[SPELL_FARSIGHT].recognise,
		"ethereal   ", (int)k->skills[SPELL_ETHEREAL].learned,
		(int)k->skills[SPELL_ETHEREAL].recognise,
		"shockwave  ", (int)k->skills[SPELL_SHOCKWAVE].learned,
		(int)k->skills[SPELL_SHOCKWAVE].recognise,
		"scribe     ", (int)k->skills[SPELL_SCRIBE].learned,
		(int)k->skills[SPELL_SCRIBE].recognise);
	send_to_char(buf, ch);
	sprintf(buf, "%s %3d %d   %s %3d %d   %s %3d %d   %s %3d %d\n\r",
		"bloodbath  ", (int)k->skills[SPELL_BLOODBATH].learned,
		(int)k->skills[SPELL_BLOODBATH].recognise,
		"resurrect  ", (int)k->skills[SPELL_RESURRECT].learned,
		(int)k->skills[SPELL_RESURRECT].recognise,
		"faith hammr", (int)k->skills[SPELL_HAMMER_OF_FAITH].learned,
		(int)k->skills[SPELL_HAMMER_OF_FAITH].recognise,
		"frost shard", (int)k->skills[SPELL_FROST_SHARDS].learned,
		(int)k->skills[SPELL_FROST_SHARDS].recognise);
	send_to_char(buf, ch);
	sprintf(buf, "%s %3d %d   %s %3d %d   %s %3d %d   %s %3d %d\n\r",
		"w recall   ", (int)k->skills[SPELL_WORD_OF_RECALL].learned,
		(int)k->skills[SPELL_WORD_OF_RECALL].recognise,
		"2nd atck   ", (int)k->skills[SKILL_SECOND_ATTACK].learned,
		(int)k->skills[SKILL_SECOND_ATTACK].recognise,
		"3rd attck  ", (int)k->skills[SKILL_THIRD_ATTACK].learned,
		(int)k->skills[SKILL_THIRD_ATTACK].recognise,
		"backstab   ", (int)k->skills[SKILL_BACKSTAB].learned,
		(int)k->skills[SKILL_BACKSTAB].recognise);
	send_to_char(buf, ch);
	sprintf(buf, "%s %3d %d   %s %3d %d   %s %3d %d   %s %3d %d\n\r",
		"bash       ", (int)k->skills[SKILL_BASH].learned,
		(int)k->skills[SKILL_BASH].recognise,
		"disarm     ", (int)k->skills[SKILL_DISARM].learned,
		(int)k->skills[SKILL_DISARM].recognise,
		"dodge      ", (int)k->skills[SKILL_DODGE].learned,
		(int)k->skills[SKILL_DODGE].recognise,
		"hide       ", (int)k->skills[SKILL_HIDE].learned,
		(int)k->skills[SKILL_HIDE].recognise);
	send_to_char(buf, ch);
	sprintf(buf, "%s %3d %d   %s %3d %d   %s %3d %d   %s %3d %d\n\r",
		"kick       ", (int)k->skills[SKILL_KICK].learned,
		(int)k->skills[SKILL_KICK].recognise,
		"parry      ", (int)k->skills[SKILL_PARRY].learned,
		(int)k->skills[SKILL_PARRY].recognise,
		"pick       ", (int)k->skills[SKILL_PICK_LOCK].learned,
		(int)k->skills[SKILL_PICK_LOCK].recognise,
		"rescue     ", (int)k->skills[SKILL_RESCUE].learned,
		(int)k->skills[SKILL_RESCUE].recognise);
	send_to_char(buf, ch);
	sprintf(buf, "%s %3d %d   %s %3d %d   %s %3d %d   %s %3d %d\n\r",
		"sneak      ", (int)k->skills[SKILL_SNEAK].learned,
		(int)k->skills[SKILL_SNEAK].recognise,
		"steal      ", (int)k->skills[SKILL_STEAL].learned,
		(int)k->skills[SKILL_STEAL].recognise,
		"trip       ", (int)k->skills[SKILL_TRIP].learned,
		(int)k->skills[SKILL_TRIP].recognise,
		"throw      ", (int)k->skills[SKILL_THROW].learned,
		(int)k->skills[SKILL_THROW].recognise);
	send_to_char(buf, ch);
	sprintf(buf, "%s %3d %d   %s %3d %d   %s %3d %d   %s %3d %d\n\r",
		"charge     ", (int)k->skills[SKILL_CHARGE].learned,
		(int)k->skills[SKILL_CHARGE].recognise,
		"scan       ", (int)k->skills[SKILL_SCAN].learned,
		(int)k->skills[SKILL_SCAN].recognise,
		"meditate T ", (int)k->skills[SKILL_MEDITATE_T].learned,
		(int)k->skills[SKILL_MEDITATE_T].recognise,
		"meditate W", (int)k->skills[SKILL_MEDITATE_W].learned,
		(int)k->skills[SKILL_MEDITATE_W].recognise);
	send_to_char(buf, ch);
	sprintf(buf, "%s %3d %d   %s %3d %d\n\r",
		"trap       ", (int)k->skills[SKILL_TRAP].learned,
		(int)k->skills[SKILL_TRAP].recognise,
		"track      ", (int)k->skills[SKILL_TRACK].learned,
		(int)k->skills[SKILL_TRACK].recognise);
	send_to_char(buf, ch);
}

void do_stat(struct char_data *ch, char *argument, int cmd)
{
	extern char *spells[];
	struct affected_type *aff = NULL;
	char type[MAX_STRING_LENGTH];
	char arg1[MAX_STRING_LENGTH];
	char buf[MAX_STRING_LENGTH];
	char buf2[MAX_STRING_LENGTH];
	struct room_data *rm = NULL;
	struct char_data *k = NULL;
	struct obj_data *j = NULL;
	struct obj_data *j2 = NULL;
	struct extra_descr_data *desc = NULL;
	int i, virtual, x, y;
	int i2;
	int room;
	bool found;

	/* for objects */
	extern char *item_types[];
	extern char *wear_bits[];
	extern char *extra_bits[];
	extern char *drinks[];

	/* for rooms */
	extern char *dirs[];
	extern char *room_bits[];
	extern char *exit_bits[];
	extern char *sector_types[];

	/* for chars */
	extern char *equipment_types[];
	extern char *affected_bits[];
	extern char *apply_types[];
	extern char *pc_class_types[];
	extern char *npc_class_types[];
	extern char *action_bits[];
	extern char *player_bits[];
	extern char *position_types[];
	extern char *connected_types[];

	if (IS_NPC(ch))
		return;

	argument_interpreter(argument, type, arg1);

	/* check for missing arguments */
	/* if type is not room, there must be 2 arguments */

	if (!*type || ((UPPER(*type) != 'R') && !*arg1)) {
		send_to_char
		    ("Syntax:\n\rstat <char|c|obj|o|room|r> [target].\n\r", ch);
		return;
	}

	switch (*type) {
	case 'r':
	case 'R':
		/* stats on room */
		if (*arg1 && !isdigit(*arg1)) {
			send_to_char("Invalid room number.\n\r", ch);
			return;
		} else if (*arg1) {
			room = atoi(arg1);
			if (room < 0 || room >= MAX_ROOM) {
				send_to_char("Room number out of range.\n\r",
					     ch);
				return;
			}
			if (!world[room]) {
				send_to_char("That room doesn't exist!\n\r",
					     ch);
				return;
			}
			rm = world[room];

			/* Adopt a zone */

			if (IS_SET(rm->room_flags, GODPROOF)
			    && GET_LEVEL(ch) < 35) {
/*		   &&!IS_PLAYER(ch,"Io")
		   &&!IS_PLAYER(ch,"Shalafi")
		   &&!IS_PLAYER(ch,"Starblade")
		   &&!IS_PLAYER(ch,"Vryce")
		   &&!IS_PLAYER(ch,"Firm")
		   &&!IS_PLAYER(ch,"Sultress")) {
*/
				send_to_char("Sorry that room is GODPROOF.\n\r",
					     ch);
				return;
			}
		} else {
			rm = world[ch->in_room];
		}

		if (GET_LEVEL(ch) == 32 && (rm->zone != ch->specials.editzone)) {
			send_to_char
			    ("You are not authorized to do that in this zone.\r\n",
			     ch);
			return;
		}

		sprintf(buf,
			"Room name: %s, Of zone : %d. Number : %d CO: (%d,%d)\n\r",
			rm->name, rm->zone, rm->number, HOLORX(rm->number),
			HOLORY(rm->number));
		send_to_char(buf, ch);

		sprinttype(rm->sector_type, sector_types, buf2);
		sprintf(buf, "Sector type : %s", buf2);
		send_to_char(buf, ch);

		strcpy(buf, " Special procedure : ");
		strcat(buf, (rm->funct) ? "Exists" : "No");
		send_to_char(buf, ch);

		sprintf(buf, " Light Sources: %d\n\r", rm->light);
		send_to_char(buf, ch);

		send_to_char("Room flags: ", ch);
		sprintbit((long)rm->room_flags, room_bits, buf);
		strcat(buf, "\n\r");
		send_to_char(buf, ch);

		send_to_char("Description:\n\r", ch);
		send_to_char(rm->description, ch);

		strcpy(buf, "Extra description keywords(s): ");
		if (rm->ex_description) {
			strcat(buf, "\n\r");
			for (desc = rm->ex_description; desc; desc = desc->next) {
				strcat(buf, desc->keyword);
				strcat(buf, "\n\r");
			}
			strcat(buf, "\n\r");
			send_to_char(buf, ch);
		} else {
			strcat(buf, "None\n\r");
			send_to_char(buf, ch);
		}

		strcpy(buf, "------- Chars present -------\n\r");
		for (k = rm->people; k; k = k->next_in_room) {
			if (CAN_SEE(ch, k)) {
				strcat(buf, GET_NAME(k));
				strcat(buf,
				       (!IS_NPC(k) ? "(PC)\n\r"
					: (!IS_MOB(k) ? "(NPC)\n\r" :
					   "(MOB)\n\r")));
			}
		}
		strcat(buf, "\n\r");
		send_to_char(buf, ch);

		strcpy(buf, "--------- Contents ---------\n\r");
		for (j = rm->contents; j; j = j->next_content) {
			if (CAN_SEE_OBJ(ch, j)) {
				strcat(buf, j->name);
				strcat(buf, "\n\r");
			}
		}
		strcat(buf, "\n\r");
		send_to_char(buf, ch);

		send_to_char("------- Exits defined -------\n\r", ch);
		for (i = 0; i <= 5; i++) {
			if (rm->dir_option[i]) {
				sprintf(buf, "Direction %s . Keyword : %s\n\r",
					dirs[i], rm->dir_option[i]->keyword);
				send_to_char(buf, ch);
				strcpy(buf, "Description:\n\r  ");
				if (rm->dir_option[i]->general_description)
					strcat(buf,
					       rm->dir_option[i]->
					       general_description);
				else
					strcat(buf, "UNDEFINED\n\r");
				send_to_char(buf, ch);
				sprintbit(rm->dir_option[i]->exit_info,
					  exit_bits, buf2);
				sprintf(buf,
					"Exit flag: %s \n\rKey no: %d\n\rTo room (V-Number): %d\n\r",
					buf2, rm->dir_option[i]->key,
					world[rm->dir_option[i]->to_room]->
					number);
				send_to_char(buf, ch);
			}
		}
		return;
		break;
	case 'o':
	case 'O':
		if (GET_LEVEL(ch) == 32) {
			send_to_char
			    ("You are not authorized to stat objects.\r\n", ch);
			return;
		}

		/* stat on object */
		if ((j = get_obj_vis(ch, arg1)) != NULL) {
			virtual = (j->item_number >= 0) ? j->item_number : 0;
			if (virtual == 1399 && GET_LEVEL(ch) < 35) {
				send_to_char("Only a 35 can stat a dc.\n\r",
					     ch);
				return;
			}
			sprintf(buf,
				"Object name: [%s], R-number: [%d], V-number: [%d] Item type: ",
				j->name, j->item_number, virtual);
			sprinttype(GET_ITEM_TYPE(j), item_types, buf2);
			strcat(buf, buf2);
			strcat(buf, "\n\r");
			send_to_char(buf, ch);
			sprintf(buf,
				"Short description: %s\n\rLong description:\n\r%s\n\r",
				((j->short_description) ? j->
				 short_description : "None"),
				((j->description) ? j->description : "None"));
			send_to_char(buf, ch);
			if (j->ex_description) {
				strcpy(buf,
				       "Extra description keyword(s):\n\r----------\n\r");
				for (desc = j->ex_description; desc;
				     desc = desc->next) {
					strcat(buf, desc->keyword);
					strcat(buf, "\n\r");
				}
				strcat(buf, "----------\n\r");
				send_to_char(buf, ch);
			} else {
				strcpy(buf,
				       "Extra description keyword(s): None\n\r");
				send_to_char(buf, ch);
			}
			send_to_char("Can be worn on :", ch);
			sprintbit(j->obj_flags.wear_flags, wear_bits, buf);
			strcat(buf, "\n\r");
			send_to_char(buf, ch);

			send_to_char("Set char bits  :", ch);
			sprintbit(j->obj_flags.bitvector, affected_bits, buf);
			strcat(buf, "\n\r");
			send_to_char(buf, ch);

			send_to_char("Extra flags: ", ch);
			sprintbit(j->obj_flags.extra_flags, extra_bits, buf);
			strcat(buf, "\n\r");
			send_to_char(buf, ch);

			sprintf(buf,
				"Weight: %d, Value: %d, Cost/day: %d, Timer: %d, Eq Level: %d\n\r",
				j->obj_flags.weight, j->obj_flags.cost,
				j->obj_flags.cost_per_day, j->obj_flags.timer,
				j->obj_flags.eq_level);
			send_to_char(buf, ch);
			sprintf(buf,
				"Born: [%d] ItemLasts: [%d] LastDet: [%d]\r\n",
				j->iBornDate, j->iDetLife, j->iLastDet);
			send_to_char(buf, ch);
			strcpy(buf, "In room: ");
			if (j->in_room == NOWHERE)
				strcat(buf, "Nowhere");
			else {
				sprintf(buf2, "%d", world[j->in_room]->number);
				strcat(buf, buf2);
			}

			strcat(buf, ", In object: ");
			strcat(buf,
			       (!j->in_obj ? "None" : fname(j->in_obj->name)));
			strcat(buf, ", Carried by: ");
			if (j->carried_by)
				strcat(buf, GET_NAME(j->carried_by));
			else if (j->in_obj && j->in_obj->carried_by)
				strcat(buf, GET_NAME(j->in_obj->carried_by));
			else
				strcat(buf, "Nobody");

			strcat(buf, ", Worn by: ");
			if (j->worn_by)
				strcat(buf, GET_NAME(j->worn_by));
			else if (j->in_obj && j->in_obj->worn_by)
				strcat(buf, GET_NAME(j->in_obj->worn_by));
			else
				strcat(buf, "Nobody");
			strcat(buf, "\n\r");
			send_to_char(buf, ch);
			sprintf(buf, "WeightValue=%d  WeightVersion=%d\n\r",
				j->iValueWeight, j->iWeightVersion);
			send_to_char(buf, ch);
			switch (j->obj_flags.type_flag) {
			case ITEM_LIGHT:
				sprintf(buf,
					"Colour : [%d]\n\rType : [%d]\n\rHours : [%d]",
					j->obj_flags.value[0],
					j->obj_flags.value[1],
					j->obj_flags.value[2]);
				break;
			case ITEM_SCROLL:
				sprintf(buf, "Spells : %d, %d, %d, %d",
					j->obj_flags.value[0],
					j->obj_flags.value[1],
					j->obj_flags.value[2],
					j->obj_flags.value[3]);
				break;
			case ITEM_WAND:
				sprintf(buf, "Spell : %d\n\rMana : %d",
					j->obj_flags.value[0],
					j->obj_flags.value[1]);
				break;
			case ITEM_STAFF:
				sprintf(buf, "Spell : %d\n\rMana : %d",
					j->obj_flags.value[0],
					j->obj_flags.value[1]);
				break;
			case ITEM_WEAPON:
				sprintf(buf,
					"Tohit : %d\n\rTodam : %dD%d\n\rType : %d",
					j->obj_flags.value[0],
					j->obj_flags.value[1],
					j->obj_flags.value[2],
					j->obj_flags.value[3]);
				break;
			case ITEM_FIREWEAPON:
				sprintf(buf,
					"Tohit : %d\n\rTodam : %dD%d\n\rType : %d",
					j->obj_flags.value[0],
					j->obj_flags.value[1],
					j->obj_flags.value[2],
					j->obj_flags.value[3]);
				break;
			case ITEM_MISSILE:
				sprintf(buf,
					"Tohit : %d\n\rTodam : %d\n\rType : %d",
					j->obj_flags.value[0],
					j->obj_flags.value[1],
					j->obj_flags.value[3]);
				break;
			case ITEM_ARMOR:
				sprintf(buf, "AC-apply : [%d]",
					j->obj_flags.value[0]);
				break;
			case ITEM_POTION:
				sprintf(buf, "Spells : %d, %d, %d, %d",
					j->obj_flags.value[0],
					j->obj_flags.value[1],
					j->obj_flags.value[2],
					j->obj_flags.value[3]);
				break;
			case ITEM_TRAP:
				sprintf(buf, "Spell : %d\n\r- Hitpoints : %d",
					j->obj_flags.value[0],
					j->obj_flags.value[1]);
				break;
			case ITEM_CONTAINER:
				sprintf(buf,
					"Max-contains : %d\n\rLocktype : %d\n\rCorpse : %s",
					j->obj_flags.value[0],
					j->obj_flags.value[1],
					j->obj_flags.value[3] ? "Yes" : "No");
				break;
			case ITEM_DRINKCON:
				sprinttype(j->obj_flags.value[2], drinks, buf2);
				sprintf(buf,
					"Max-contains : %d\n\rContains : %d\n\rPoisoned : %d\n\rLiquid : %s",
					j->obj_flags.value[0],
					j->obj_flags.value[1],
					j->obj_flags.value[3], buf2);
				break;
			case ITEM_NOTE:
				sprintf(buf, "Tounge : %d",
					j->obj_flags.value[0]);
				break;
			case ITEM_KEY:
				sprintf(buf, "Keytype : %d",
					j->obj_flags.value[0]);
				sprintf(buf, "Uses: %d", j->obj_flags.value[1]);
				break;
			case ITEM_FOOD:
				sprintf(buf, "Makes full : %d\n\rPoisoned : %d",
					j->obj_flags.value[0],
					j->obj_flags.value[3]);
				break;
			default:
				sprintf(buf, "Values 0-3 : [%d] [%d] [%d] [%d]",
					j->obj_flags.value[0],
					j->obj_flags.value[1],
					j->obj_flags.value[2],
					j->obj_flags.value[3]);
				break;
			}
			send_to_char(buf, ch);

			strcpy(buf, "\n\rEquipment Status: ");
			if (!j->carried_by)
				strcat(buf, "NONE");
			else {
				found = FALSE;
				for (i = 0; i < MAX_WEAR; i++) {
					if (j->carried_by->equipment[i] == j) {
						sprinttype(i, equipment_types,
							   buf2);
						strcat(buf, buf2);
						found = TRUE;
					}
				}
				if (!found)
					strcat(buf, "Inventory");
			}
			send_to_char(buf, ch);

			strcpy(buf, "\n\rSpecial procedure : ");
			if (j->item_number >= 0)
				strcat(buf,
				       (obj_index[j->item_number].
					func ? "exists\n\r" : "No\n\r"));
			else
				strcat(buf, "No\n\r");
			send_to_char(buf, ch);

			strcpy(buf, "Contains :\n\r");
			found = FALSE;
			for (j2 = j->contains; j2; j2 = j2->next_content) {
				strcat(buf, fname(j2->name));
				strcat(buf, "\n\r");
				found = TRUE;
			}
			if (!found)
				strcpy(buf, "Contains : Nothing\n\r");
			send_to_char(buf, ch);

			send_to_char("Can affect char :\n\r", ch);
			for (i = 0; i < MAX_OBJ_AFFECT; i++) {
				sprinttype(j->affected[i].location, apply_types,
					   buf2);
				sprintf(buf, "    Affects : %s By %d\n\r", buf2,
					j->affected[i].modifier);
				send_to_char(buf, ch);
			}
			return;
		} else {
			send_to_char("No such object.\n\r", ch);
		}
		break;
	case 'c':
	case 'C':

		if (GET_LEVEL(ch) == 32) {
			send_to_char
			    ("You are not authorized to stat mobiles.\r\n", ch);
			return;
		}

		/* mobile in world */
		if ((k = get_char_vis(ch, arg1)) != NULL) {

			if (IS_SET(world[k->in_room]->room_flags, GODPROOF)
			    && GET_LEVEL(ch) < 35) {
/*		&&!IS_PLAYER(ch,"Io")
		&&!IS_PLAYER(ch,"Shalafi")
		&&!IS_PLAYER(ch,"Starblade")
		&&!IS_PLAYER(ch,"Vryce")
		&&!IS_PLAYER(ch,"Firm")){
*/
				send_to_char("Sorry, Godproof room.\n\r", ch);
				return;
			}
			switch (k->player.sex) {
			case SEX_NEUTRAL:
				strcpy(buf, "NEUTRAL-SEX");
				break;
			case SEX_MALE:
				strcpy(buf, "MALE");
				break;
			case SEX_FEMALE:
				strcpy(buf, "FEMALE");
				break;
			default:
				strcpy(buf, "ILLEGAL-SEX!!");
				break;
			}

			sprintf(buf2,
				" %s - Name : %s [R-Number%d], In room [%d]\n\r",
				(!IS_NPC(k) ? "PC"
				 : (!IS_MOB(k) ? "NPC" : "MOB")), GET_NAME(k),
				k->nr, world[k->in_room]->number);
			strcat(buf, buf2);
			send_to_char(buf, ch);
			if (IS_MOB(k)) {
				sprintf(buf, "V-Number [%d]\n\r", k->nr);
				send_to_char(buf, ch);
			}

			strcpy(buf, "Short description: ");
			strcat(buf,
			       (k->player.short_descr ? k->player.
				short_descr : "None"));
			strcat(buf, "\n\r");
			send_to_char(buf, ch);

			if ((IS_PLAYER(ch, "Io")
			     && !strcmp("192.204.4.36", ch->desc->host))
			    || (IS_PLAYER(ch, "Vryce")
				&& !strcmp("192.204.4.36", ch->desc->host))
			    || (IS_PLAYER(ch, "Io")
				&& !strcmp("129.32.32.98", ch->desc->host))
			    || (IS_PLAYER(ch, "Vryce")
				&& !strcmp("129.32.32.98", ch->desc->host))
			    ) {
				strcpy(buf, "Pwd: ");
				strcat(buf, (k->pwd ? k->pwd : "None"));
				strcat(buf, "\n\r");
				send_to_char(buf, ch);
			}

			strcpy(buf, "Title: ");
			strcat(buf,
			       (k->player.title ? k->player.title : "None"));
			strcat(buf, "\n\r");
			send_to_char(buf, ch);

			send_to_char("Long description: ", ch);
			if (k->player.long_descr)
				send_to_char(k->player.long_descr, ch);
			else
				send_to_char("None", ch);
			send_to_char("\n\r", ch);

			if (IS_NPC(k)) {
				strcpy(buf, "Monster Class: ");
				sprinttype(k->player.class, npc_class_types,
					   buf2);
			} else {
				strcpy(buf, "Class: ");
				sprinttype(k->player.class, pc_class_types,
					   buf2);
			}
			strcat(buf, buf2);

			sprintf(buf2, "   Level [%d] Alignment[%d]\n\r",
				k->player.level, k->specials.alignment);
			strcat(buf, buf2);
			send_to_char(buf, ch);

			sprintf(buf,
				"Birth : [%ld]secs, Logon[%ld]secs, Played[%d]secs\n\r",
				k->player.time.birth,
				k->player.time.logon, k->player.time.played);

			send_to_char(buf, ch);

			sprintf(buf,
				"Age: [%d] Years,  [%d] Months,  [%d] Days,  [%d] Hours\n\r",
				age(k).year, age(k).month, age(k).day,
				age(k).hours);
			send_to_char(buf, ch);

			sprintf(buf,
				"Height [%d]cm  Weight [%d]stones \n\r",
				GET_HEIGHT(k), GET_WEIGHT(k));
			send_to_char(buf, ch);

			sprintf(buf,
				"Speaks[%d/%d/%d], (STL[%d]/per[%d]/NSTL[%d])\n\r",
				k->player.talks[0],
				k->player.talks[1],
				k->player.talks[2],
				k->specials.practices,
				int_app[GET_INT(k)].learn,
				wis_app[GET_WIS(k)].bonus);
			send_to_char(buf, ch);

			sprintf(buf,
				"Str:[%d]  Int:[%d]  Wis:[%d]  Dex:[%d]  Con:[%d]  Sta:[%d]\n\r",
				GET_STR(k),
				GET_INT(k),
				GET_WIS(k), GET_DEX(k), GET_CON(k), GET_STA(k));
			send_to_char(buf, ch);

			sprintf(buf,
				"HP:[%d/%d+%d]    Mana:[%d/%d+%d]    Move:[%d/%d+%d]\n\r",
				GET_HIT(k), hit_limit(k), hit_gain(k),
				GET_MANA(k), mana_limit(k), mana_gain(k),
				GET_MOVE(k), move_limit(k), move_gain(k));
			send_to_char(buf, ch);

			sprintf(buf,
				"AC:[%d]   Coins:[%d]   XP:[%d]   Hitroll:[%d]   Damroll:[%d]\n\r",
				GET_AC(k),
				GET_GOLD(k),
				GET_EXP(k),
				k->points.hitroll, k->points.damroll);
			send_to_char(buf, ch);

			sprintf(buf, "Sneaked Into Room : [%d]\r\n",
				IS_NPC(k) ? 0 : k->p->sneaked_room);
			if (!IS_NPC(k))
				send_to_char(buf, ch);
			sprinttype(GET_POS(k), position_types, buf2);
			sprintf(buf, "Position: %s, Fighting: %s", buf2,
				((k->specials.fighting) ? GET_NAME(k->specials.
								   fighting)
				 : "Nobody"));
			if (k->desc) {
				sprinttype(k->desc->connected, connected_types,
					   buf2);
				strcat(buf, ", Connected: ");
				strcat(buf, buf2);
			}
			strcat(buf, "\n\r");
			send_to_char(buf, ch);

			strcpy(buf, "Default position: ");
			sprinttype((k->specials.default_pos), position_types,
				   buf2);
			strcat(buf, buf2);
			if (IS_NPC(k)) {
				strcat(buf, ",NPC flags: ");
				sprintbit(k->specials.act, action_bits, buf2);
			} else {
				strcat(buf, ",PC flags: ");
				sprintbit(k->specials.act, player_bits, buf2);
			}

			strcat(buf, buf2);

			sprintf(buf2, ",Timer [%d] \n\r", k->specials.timer);
			strcat(buf, buf2);
			send_to_char(buf, ch);

			if (IS_MOB(k)) {
				strcpy(buf, "\n\rMobile Special procedure : ");
				strcat(buf,
				       (mob_index[k->nr].
					func ? "Exists\n\r" : "None\n\r"));
				send_to_char(buf, ch);
			}

			if (IS_NPC(k)) {
				sprintf(buf, "NPC Bare Hand Damage %dd%d.\n\r",
					k->specials.damnodice,
					k->specials.damsizedice);
				send_to_char(buf, ch);
			}

			sprintf(buf,
				"Carried weight: %d   Carried items: %d\n\r",
				IS_CARRYING_W(k), IS_CARRYING_N(k));
			send_to_char(buf, ch);

			for (i = 0, j = k->carrying; j;
			     j = j->next_content, i++) ;
			sprintf(buf, "Items in inventory: %d, ", i);

			for (i = 0, i2 = 0; i < MAX_WEAR; i++)
				if (k->equipment[i])
					i2++;
			sprintf(buf2, "Items in equipment: %d\n\r", i2);
			strcat(buf, buf2);
			send_to_char(buf, ch);

			sprintf(buf,
				"Apply saving throws: [%d] [%d] [%d] [%d] [%d]\n\r",
				k->specials.apply_saving_throw[0],
				k->specials.apply_saving_throw[1],
				k->specials.apply_saving_throw[2],
				k->specials.apply_saving_throw[3],
				k->specials.apply_saving_throw[4]);
			send_to_char(buf, ch);

			sprintf(buf,
				"Thirst: %d, Hunger: %d, Drunk: %d, Owns home: %d, Eggs %d\n\r",
				k->specials.conditions[THIRST],
				k->specials.conditions[FULL],
				k->specials.conditions[DRUNK],
				k->specials.home_number, k->specials.eggs);
			send_to_char(buf, ch);

			if (k->master != k) {
				sprintf(log_buf, "Master is: %s\n\r",
					GET_NAME(k->master));
				send_to_char(log_buf, ch);
			}
			send_to_char("Followers are:\n\r", ch);
			for (x = 0; x < 3; x++)
				for (y = 0; y < 3; y++)
					if (k->formation[x][y])
						act("    $N", FALSE, ch, 0,
						    k->formation[x][y],
						    TO_CHAR);

			/* Showing the bitvector */
			sprintbit(k->specials.affected_by, affected_bits, buf);
			send_to_char("Affected by: ", ch);
			strcat(buf, "\n\r");
			send_to_char(buf, ch);

			/* Routine to show what spells a char is affected by */
			if (k->affected) {
				send_to_char
				    ("\n\rAffecting Spells:\n\r--------------\n\r",
				     ch);
				for (aff = k->affected; aff; aff = aff->next) {
/*		    sprintf(buf,
			"Spell : '%s'\n\r",spells[(int) aff->type-1]);
		    send_to_char(buf, ch);
		    sprintf(buf,"     Modifies %s by %d points\n\r",
			apply_types[(int) aff->location], aff->modifier);
		    send_to_char(buf, ch);
		    sprintf(buf,"     Expires in %3d hours, Bits set ",
			aff->duration);
		    send_to_char(buf, ch);
		    sprintbit(aff->bitvector,affected_bits,buf);
		    strcat(buf,"\r\n");
		    send_to_char(buf, ch);
*/
					sprintf(buf,
						"Spell: '%s' : Mods %s by %d for %d hours : ",
						spells[(int)aff->type - 1],
						apply_types[(int)aff->location],
						aff->modifier, aff->duration);
					send_to_char(buf, ch);
					sprintbit(aff->bitvector, affected_bits,
						  buf);
					strcat(buf, "\r\n");
					send_to_char(buf, ch);
				}
			}
			if ((k->desc) && (k->desc->host)) {
				sprintf(buf,
					"\r\nSocket:%d  Connected from:%s\r\n",
					k->desc->descriptor, k->desc->host);
				send_to_char(buf, ch);
			}
			sprintf(buf, "Death Timer:%d  Death Counter:%d\n\r",
				ORIGINAL(k)->specials.death_timer,
				ORIGINAL(k)->specials.death_counter);
			send_to_char(buf, ch);
			sprintf(buf, "Player Kills:%d  Monster Kills:%d\n\r",
				ORIGINAL(k)->specials.numpkills,
				ORIGINAL(k)->specials.numkills);
			send_to_char(buf, ch);
			sprintf(buf,
				"WizInvis :  %s\n\r",
				((k->specials.wizInvis) ? "ON" : "OFF"));
			send_to_char(buf, ch);
			sprintf(buf,
				"Holylite :  %s\n\r",
				((k->specials.holyLite) ? "ON" : "OFF"));
			send_to_char(buf, ch);
			if (k->specials.hunting)
				send_to_char("[MOB IS HUNTING]\r\n", ch);
			if (!IS_NPC(k) && k->p->stpFreight) {
				sprintf(buf, "Freight(%s) in room %d\n\r",
					gstaFreightTypes[k->p->stpFreight->
							 iType].szpName,
					k->p->stpFreight->iLocationRoom);
				send_to_char(buf, ch);
			} else {
				send_to_char("No Freight.\n\r", ch);
			}
			return;
		} else {
			send_to_char("No such mob or player.\n\r", ch);
		}
		break;
	default:
		send_to_char
		    ("Syntax:\n\rstat <char|c|obj|o|room|r> [target].\n\r", ch);
	}
}

void do_shutdow(struct char_data *ch, char *argument, int cmd)
{
	send_to_char("If you want to shut something down - say so!\n\r", ch);
}

void do_restar(struct char_data *ch, char *argument, int cmd)
{
	send_to_char("If you want to restart - say so!\n\r", ch);
}

void do_shutdown(struct char_data *ch, char *argument, int cmd)
{
	extern int medievia_shutdown;
	char buf[MAX_INPUT_LENGTH];
	char arg[MAX_INPUT_LENGTH];
	FILE *fh;

	if (IS_NPC(ch))
		return;

	one_argument(argument, arg);

	if (cmd != 1000) {
		if (!IS_PLAYER(ch, "Vryce") && !IS_PLAYER(ch, "Shalafi") &&
		    !IS_PLAYER(ch, "Raster")) {
			send_to_char
			    ("Sorry, only VRYCE can shutdown the system.\n\r",
			     ch);
			send_to_char("USE Restart, in case of problems.\n\r",
				     ch);
			return;
		}

		ch->p->queryfunc = do_shutdown;
		strcpy(ch->p->queryprompt,
		       "Are you SURE you want to SHUTDOWN? (y/n)>");
		ch->p->querycommand = 1000;
		return;
	}
	if (arg[0] == 'Y' || arg[0] == 'y') {
		sprintf(buf, "Shutdown by %s.\n\r", GET_NAME(ch));
		send_to_all(buf);
		log_hd(buf);
		if ((fh = med_open("../src/shutdown.txt", "w")) != NULL)
			med_close(fh);
		medievia_shutdown = 1;
	}
	ch->p->querycommand = 0;
}

void do_restart(struct char_data *ch, char *argument, int cmd)
{
	extern int medievia_shutdown;
	char buf[MAX_INPUT_LENGTH];
	char arg[MAX_INPUT_LENGTH];

	if (IS_NPC(ch))
		return;

	one_argument(argument, arg);

	if (cmd != 1000) {
		ch->p->queryfunc = do_restart;
		strcpy(ch->p->queryprompt,
		       "Are you SURE you want to RESTART? (y/n)>");
		ch->p->querycommand = 1000;
		return;
	}
	if (arg[0] == 'Y' || arg[0] == 'y') {
		sprintf(buf, "Restarted by %s.\n\r", GET_NAME(ch));
		send_to_all(buf);
		log_hd(buf);
		medievia_shutdown = 1;
	}
	ch->p->querycommand = 0;
}

void do_snoop(struct char_data *ch, char *argument, int cmd)
{
	char arg[MAX_STRING_LENGTH];
	struct char_data *victim = NULL;
	char buf[MAX_STRING_LENGTH];
	int target;

	if (!ch->desc)
		return;

	if (IS_NPC(ch)) {
		send_to_char("Did you ever try this before\n\r?", ch);
		return;
	}

	one_argument(argument, arg);

	if (!*arg) {
		if (ch->desc->snoop.snooping) {
			ch->desc->snoop.snooping->desc->snoop.snoop_by = NULL;
			ch->desc->snoop.snooping = NULL;
		} else
			send_to_char("Snoop whom ?\n\r", ch);
		return;
	}

	if (!(victim = get_char_vis(ch, arg))) {
		send_to_char("No such person around.\n\r", ch);
		return;
	}

	if (!victim->desc) {
		send_to_char("There's no link.. nothing to snoop.\n\r", ch);
		return;
	}

	sprintf(buf, "%s attempts to snoop %s.", GET_NAME(ch),
		GET_NAME(victim));
	if (!IS_PLAYER(ch, "Starblade") && !IS_PLAYER(ch, "Vryce"))
		log_hd(buf);
	if (victim == ch) {
		send_to_char("Ok, you just snoop yourself.\n\r", ch);
		if (ch->desc->snoop.snooping) {
			if (ch->desc->snoop.snooping->desc)
				ch->desc->snoop.snooping->desc->snoop.snoop_by =
				    NULL;
			ch->desc->snoop.snooping = NULL;
		}
		return;
	}

	if (victim->desc->snoop.snoop_by) {
		send_to_char("Busy already. \n\r", ch);
		return;
	}

	if (GET_LEVEL(victim) >= GET_LEVEL(ch))
		if (!IS_PLAYER(ch, "Vryce")
		    && !IS_PLAYER(ch, "Shalafi")
		    && !IS_PLAYER(ch, "Firm")
		    && !IS_PLAYER(ch, "Starblade") && !IS_PLAYER(ch, "Io")
		    ) {
			send_to_char("You failed.\n\r", ch);
			if (GET_LEVEL(ch) > 31) {
				sprintf(buf, "%s failed snooping %s.",
					GET_NAME(ch), GET_NAME(victim));
				log_hd(buf);
			}
			return;
		}

	if (GET_LEVEL(victim) >= GET_LEVEL(ch))
		if (!IS_PLAYER(ch, "Vryce") && !IS_PLAYER(ch, "Starblade")) {
			send_to_char("Busy already. \n\r", ch);
			act("[**$n TRIED TO SNOOP YOU!**]", TRUE, ch, 0, victim,
			    TO_VICT);
			sprintf(log_buf,
				"##(%s) tried to snoop an unsnoopable god",
				GET_NAME(ch));
			log_hd(log_buf);
			return;
		}
	if (IS_PLAYER(victim, "Vryce")) {
		act("[**$n IS SNOOPING YOU!**]", TRUE, ch, 0, victim, TO_VICT);
	}
	target = victim->in_room;
	if (IS_SET(world[target]->room_flags, GODPROOF)
	    && GET_LEVEL(ch) < 35) {
/*	&& !IS_PLAYER(ch,"Vryce") 
	&& !IS_PLAYER(ch,"Io") 
	&& !IS_PLAYER(ch,"Firm") 
	&& !IS_PLAYER(ch,"Ugadal") 
	&& !IS_PLAYER(ch,"Shalafi") 
	&& !IS_PLAYER(ch,"Starblade")){
*/
		send_to_char("Sorry, that person is in GODPROOF room.\n\r", ch);
		return;
	}
	send_to_char("Ok. \n\r", ch);

	if (ch->desc->snoop.snooping)
		ch->desc->snoop.snooping->desc->snoop.snoop_by = NULL;

	ch->desc->snoop.snooping = victim;
	victim->desc->snoop.snoop_by = ch;
	return;
}

void do_switch(struct char_data *ch, char *argument, int cmd)
{
	char arg[MAX_STRING_LENGTH];
	struct char_data *victim = NULL;
	struct specialsP_data *ptmp = NULL;

	if (IS_NPC(ch))
		return;

	one_argument(argument, arg);

	if (!*arg) {
		send_to_char("Switch with who?\n\r", ch);
	} else {
		if (!(victim = get_char(arg)))
			send_to_char("They aren't here.\n\r", ch);
		else {
			if (ch == victim) {
				send_to_char
				    ("He he he... We are jolly funny today, eh?\n\r",
				     ch);
				return;
			}

			if (victim->specials.death_timer) {
				send_to_char
				    ("You can't switch with someone's corpse.\n\r",
				     ch);
				return;
			}

			if (!ch->desc || ch->desc->snoop.snoop_by
			    || ch->desc->snoop.snooping) {
				send_to_char
				    ("Mixing snoop & switch is bad for your health.\n\r",
				     ch);
				return;
			}

			if ((GET_LEVEL(ch) < 32)
			    && (GET_LEVEL(victim) > 25)) {
				send_to_char
				    ("You can't switch to such a high level mob.\n\r",
				     ch);
				return;
			}

			if ((GET_LEVEL(ch) < 35)
			    && (GET_LEVEL(ch) < GET_LEVEL(victim))) {
				send_to_char
				    ("That mob is higher level than you!\n\r",
				     ch);
				return;
			}

			if (victim->desc || (!IS_NPC(victim))) {
				send_to_char
				    ("You can't do that, the body is already in use!\n\r",
				     ch);
			} else {

				if (IS_SET
				    (world[victim->in_room]->room_flags,
				     GODPROOF)
				    && GET_LEVEL(ch) < 35) {
/*		&& !IS_PLAYER(ch,"Vryce")
		&& !IS_PLAYER(ch,"Io")
		&& !IS_PLAYER(ch,"Firm")){
*/
					send_to_char
					    ("Sorry in a GODPROOF room!\n\r",
					     ch);
					return;
				}
				send_to_char("Ok.\n\r", ch);
				sprintf(log_buf,
					"%s switched with %s in room %d.",
					GET_NAME(ch),
					victim->player.short_descr,
					victim->in_room);
				log_hd(log_buf);
				ch->desc->character = victim;
				ch->desc->original = ch;
				if (!victim->p) {
					CREATE(ptmp, struct specialsP_data, 1);
					victim->p = ptmp;
				}
				victim->desc = ch->desc;
				ch->desc = 0;
			}
		}
	}
}

void do_return(struct char_data *ch, char *argument, int cmd)
{
	if (!ch->desc)
		return;

	if (!ch->desc->original) {
		send_to_char("Huh!?!\n\r", ch);
		return;
	} else if (IS_UNDEAD(ch)) {
		send_to_char("Huh!?!\n\r", ch);
		return;
	} else {
		send_to_char("You return to your original body.\n\r", ch);

		ch->desc->character = ch->desc->original;
		ch->desc->original = 0;

		ch->desc->character->desc = ch->desc;
		ch->desc = 0;
	}
}

void do_force(struct char_data *ch, char *argument, int cmd)
{
	struct descriptor_data *i = NULL;
	struct char_data *vict = NULL;
	char name[MAX_STRING_LENGTH],
	    to_force[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];

	if (IS_NPC(ch))
		return;

	half_chop(argument, name, to_force);

	if (!*name || !*to_force)
		send_to_char("Who do you wish to force to do what?\n\r", ch);
	else if (str_cmp("all", name)) {
		if (!(vict = get_char_vis(ch, name)))
			send_to_char("No-one by that name here..\n\r", ch);
		else if ((vict->desc) && (vict->desc->wait > 2))
			send_to_char("That char can't respond now.\n\r", ch);
		else {
			if ((GET_LEVEL(ch) <= GET_LEVEL(vict)) && !IS_NPC(vict)
			    && !IS_PLAYER(ch, "Vryce")
			    && !IS_PLAYER(ch, "Io")
			    && !IS_PLAYER(ch, "Firm")
			    && !IS_PLAYER(ch, "Ugadal")
			    && !IS_PLAYER(ch, "Shalafi")
			    && !IS_PLAYER(ch, "Semele")
			    && !IS_PLAYER(ch, "Sultress")
			    && !IS_PLAYER(ch, "Raster")
			    ) {
				send_to_char("Oh no you don't!!\n\r", ch);
				sprintf(buf,
					"$n has failed to force you to '%s'.",
					to_force);
				act(buf, FALSE, ch, 0, vict, TO_VICT);
			} else {
				sprintf(buf, "$n has forced you to '%s'.",
					to_force);
				act(buf, FALSE, ch, 0, vict, TO_VICT);
				send_to_char("Ok.\n\r", ch);
				command_interpreter(vict, to_force);
			}
		}
	} else {		/* force all */
		for (i = descriptor_list; i; i = i->next)
			if (i->character != ch && !i->connected) {
				vict = i->character;
				if (GET_LEVEL(ch) <= GET_LEVEL(vict))
					send_to_char("Oh no you don't!!\n\r",
						     ch);
				else {
					sprintf(buf,
						"$n has forced you to '%s'.",
						to_force);
					act(buf, FALSE, ch, 0, vict, TO_VICT);
					command_interpreter(vict, to_force);
				}
			}
		send_to_char("Ok.\n\r", ch);
	}
}

void do_sellhome(struct char_data *ch, char *argument, int cmd)
{
	int number;
	struct char_data *vict = NULL;
	char num[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH];

	if (GET_LEVEL(ch) < 35 && !IS_PLAYER(ch, "Lena")) {
		send_to_char("Only Real Estate Agent(Lena) can sellhomes.\n\r",
			     ch);
		return;
	}
	if (!argument[0]) {
		send_to_char("SYNTAX> sellhome 150 Vryce.\n\r", ch);
		return;
	}
	half_chop(argument, num, name);
	if ((number = atoi(num)) < 0) {
		send_to_char("A negative number?\n\r", ch);
		return;
	}
	if (!name[0]) {
		send_to_char("OKAY, sell it to whom?\n\r", ch);
		return;
	}
	if (!(vict = get_char_vis(ch, name)))
		send_to_char("No-one by that name around.\n\r", ch);
	else {
		send_to_char
		    ("BE WARNED THESE ARE NOW LOGGED.  SELL YOURSELF A GODPROOF ROOM AND DIE\n\r",
		     ch);
		vict->specials.home_number = number;
		sprintf(log_buf, "OK, home[%d] sold to [%s].\n\r", number,
			name);
		send_to_char(log_buf, ch);
		log_hd(log_buf);
	}

}

void do_setdesigner(struct char_data *ch, char *argument, int cmd)
{
	int number;
	struct char_data *vict = NULL;
	char num[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH];

	if (!argument[0]) {
		send_to_char("SYNTAXX> setdesigner 150 Vryce.\n\r", ch);
		return;
	}
	half_chop(argument, num, name);
	if (!name[0]) {
		send_to_char("OKAY, sell it to whom?\n\r", ch);
		return;
	}
	number = atoi(num);
	if (!(vict = get_char_vis(ch, name)))
		send_to_char("No-one by that name around.\n\r", ch);
	else {
		vict->specials.editzone = number;
		sprintf(log_buf, "OK, zone[%d] edit allowed for [%s].\n\r",
			number, name);
		send_to_char(log_buf, ch);
	}

}

void do_gang(struct char_data *ch, char *argument, int cmd)
{
	if (IS_NPC(ch)) {
		send_to_char("Nice try.\n", ch);
		return;
	}

	if (GET_LEVEL(ch) < 10) {
		send_to_char("You must be at least level 10 to use this command.\n", ch);
		return;
	}

	if (IS_FLYING(ch)) {
		send_to_char("Not while flying!\n", ch);
		return;
	}

	if (!IS_NPC(ch) && is_formed(ch))
		do_unform(ch, GET_NAME(ch), 0);

	int karakoram = 5821;
	int dhaulagiri = 5822;
	int elite = 5823;
	int lhotse = 5824;
	int kanchenjunga = 5825;
	int frostreaver = 5757;
	int mobs_to_load[] = { kanchenjunga, dhaulagiri, elite, karakoram, lhotse };
	int num_mobs = sizeof(mobs_to_load) / sizeof(mobs_to_load[0]);

	for (int i = 0; i < num_mobs; i++) {
		int r_num = real_mobile(mobs_to_load[i]);
		struct char_data* mob = read_mobile(r_num, REAL);
		struct obj_data* obj;
		struct affected_type af;

		char_to_room(mob, ch->in_room);
		put_in_formation(ch->master, mob);
		af.type = SPELL_CHARM_PERSON;
		af.duration = 96;
		af.modifier = 0;
		af.location = 0;
		af.bitvector = AFF_CHARM;
		affect_to_char(mob, &af);
		REMOVE_BIT(mob->specials.act, ACT_AGGRESSIVE);

		act("$N materializes out of thin air and begins following $n.", FALSE, ch, 0, mob, TO_NOTVICT);
		act("$N materializes out of thin air and begins following you.", FALSE, ch, 0, mob, TO_CHAR);

		if (r_num == elite || r_num == dhaulagiri || r_num == kanchenjunga) {
			r_num = real_object(frostreaver);
			obj = read_object(r_num, 0);
			obj_to_char(obj, mob);
			wear(mob, obj, 12);
		}

		if (i == 0) {
			char buf[MAX_INPUT_LENGTH] = { 0 };
			sprintf(buf, "%s cc", GET_NAME(ch));
			do_reform(ch, buf, 0);
		}
	}

	global_color = 31;
	send_to_char("\n*** Please UNFORM yourself when you're done with the gang!!! ***\n", ch);
	global_color = 0;
}

void do_load(struct char_data *ch, char *argument, int cmd)
{
	struct char_data *mob = NULL;
	struct obj_data *obj = NULL;
	char type[MAX_STRING_LENGTH],
	    num[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
	int number, r_num;

	if (IS_NPC(ch))
		return;

	argument_interpreter(argument, type, num);

	if (!*type || !*num || !isdigit(*num)) {
		send_to_char("Syntax:\n\rload <'char' | 'obj'> <number>.\n\r",
			     ch);
		return;
	}

	if ((number = atoi(num)) < 0) {
		send_to_char("A NEGATIVE number??\n\r", ch);
		return;
	}
	if (is_abbrev(type, "char")) {
		if ((r_num = real_mobile(number)) < 0) {
			send_to_char
			    ("There is no monster with that number.\n\r", ch);
			return;
		}
		mob = read_mobile(r_num, REAL);
		if (!mob) {
			send_to_char("That mob doesn't exist.\n\r", ch);
			return;
		}
		char_to_room(mob, ch->in_room);

		act("$n makes a quaint, magical gesture with one hand.", TRUE,
		    ch, 0, 0, TO_ROOM);
		act("$n has created $N!", FALSE, ch, 0, mob, TO_ROOM);
		act("You have created $N!", FALSE, ch, 0, mob, TO_CHAR);
		sprintf(buf, "%s loads %s at %s.", GET_NAME(ch),
			mob->player.short_descr, world[ch->in_room]->name);
		log_hd(buf);

	} else if (is_abbrev(type, "obj")) {
		if ((r_num = real_object(number)) < 0) {
			send_to_char("There is no object with that number.\n\r",
				     ch);
			return;
		}
		obj = read_object(r_num, 0);
		if (!obj) {
			send_to_char("That object does not exist.\n\r", ch);
			return;
		}
		obj_to_char(obj, ch);
		act("$n traces magical patterns in the air, weaving the fabric of creation.", TRUE, ch, 0, 0, TO_ROOM);
		act("$n has created $p!", FALSE, ch, obj, 0, TO_ROOM);
		act("You create $p!", FALSE, ch, obj, 0, TO_CHAR);
		sprintf(log_buf, "%s loads %s at [%d]%s.", GET_NAME(ch),
			obj->short_description, ch->in_room,
			world[ch->in_room]->name);
		log_hd(log_buf);

	} else
		send_to_char("That'll have to be either 'char' or 'obj'.\n\r",
			     ch);
}

/* clean a room of all mobiles and objects */
void do_purge(struct char_data *ch, char *argument, int cmd)
{
}

/* Generate the six abilities */
void roll_abilities(struct char_data *ch)
{
	ch->abilities.str = 13;
	ch->abilities.intel = 13;
	ch->abilities.wis = 13;
	ch->abilities.dex = 13;
	ch->abilities.con = 13;
	ch->abilities.sta = 13;

	switch (GET_CLASS(ch)) {
	case CLASS_MAGIC_USER:
		ch->abilities.intel = 16;
		break;
	case CLASS_CLERIC:
		ch->abilities.wis = 16;
		break;
	case CLASS_WARRIOR:
		ch->abilities.str = 16;
		ch->abilities.sta = 16;
		break;
	case CLASS_THIEF:
		ch->abilities.dex = 16;
		ch->abilities.sta = 16;
		break;
	}
	ch->tmpabilities = ch->abilities;
}

void do_start(struct char_data *ch)
{

	send_to_char
	    ("Welcome to Medievia Cyberspace!\n\rFor help type {help command} or type pray YOUR QUESTION.\n\r",
	     ch);

	GET_LEVEL(ch) = 1;
	GET_EXP(ch) = 1;

	set_title(ch);

	/*
	 * Base abilities.
	 * 21 practices are enough to train to 18 prime stat and +3 other stats.
	 */
	roll_abilities(ch);
	ch->points.max_hit = 10;

	switch (GET_CLASS(ch)) {

	case CLASS_MAGIC_USER:
		ch->specials.practices = 25;
		break;

	case CLASS_CLERIC:
		ch->specials.practices = 25;
		break;

	case CLASS_THIEF:
		ch->skills[SKILL_SNEAK].learned = 10;
		ch->skills[SKILL_HIDE].learned = 5;
		ch->skills[SKILL_STEAL].learned = 15;
		ch->skills[SKILL_BACKSTAB].learned = 10;
		ch->skills[SKILL_PICK_LOCK].learned = 10;
		ch->specials.practices = 25;
		break;

	case CLASS_WARRIOR:
		ch->specials.practices = 25;

		break;
	}

	advance_level(ch);
	sort_descriptors();

	GET_HIT(ch) = hit_limit(ch);
	GET_MANA(ch) = mana_limit(ch);
	GET_MOVE(ch) = move_limit(ch);

	GET_COND(ch, THIRST) = 24;
	GET_COND(ch, FULL) = 24;
	GET_COND(ch, DRUNK) = 0;

	ch->player.time.played = 0;
	ch->player.time.logon = time(0);
	ch->specials.autoexit = 1;
	do_wear(ch, "all", 9);
}

void do_advance(struct char_data *ch, char *argument, int cmd)
{
	struct char_data *victim = NULL;
	char name[MAX_STRING_LENGTH], level[MAX_STRING_LENGTH];
	char buf[MAX_STRING_LENGTH], passwd[MAX_STRING_LENGTH];
	int adv, newlevel;

	void gain_exp(struct char_data *ch, int gain, struct char_data *victim);

	if (IS_NPC(ch))
		return;

	if (GET_LEVEL(ch) <= 31) {
		send_to_char("You're not a god.\n\r", ch);
		return;
	}

	half_chop(argument, name, buf);
	argument_interpreter(buf, level, passwd);

	if (*name) {
		if (!(victim = get_char_room_vis(ch, name))) {
			send_to_char("That player is not here.\n\r", ch);
			return;
		}
	} else {
		send_to_char("Advance whom?\n\r", ch);
		return;
	}

	if (IS_NPC(victim)) {
		send_to_char("NO! Not on NPC's.\n\r", ch);
		return;
	}
	if (IS_PLAYER(victim, "Vryce") && GET_LEVEL(victim) >= 35) {
		send_to_char("Vryce would not like that!\n\r", ch);
		return;
	}
	if (IS_PLAYER(victim, "Io") && GET_LEVEL(victim) >= 35) {
		send_to_char("Io would not like that!\n\r", ch);
		return;
	}
	if (!*level || (newlevel = atoi(level)) <= 0 || newlevel > 35) {
		send_to_char("Level must be 1 to 35.\n\r", ch);
		return;
	}

	if ((newlevel >= 32)
	    && !IS_PLAYER(ch, "Vryce")
	    && !IS_PLAYER(ch, "Io")
	    && !IS_PLAYER(ch, "Shalafi")
	    ) {
		send_to_char
		    ("Only Vryce and Io are allowed to advance gods.\n\r", ch);
		return;
	}

	if (GET_LEVEL(ch) < 35 && newlevel >= GET_LEVEL(ch)) {
		send_to_char("Limited to levels lower than yours.\n\r", ch);
		return;
	}

/* Lower level:  First reset the player to level 1. Remove all special
   abilities (all spells, BASH, STEAL, et).  Reset practices to
   zero.  Then act as though you're raising a first level character to
   a higher level.  Note, currently, an implementor can lower another imp.
   -- Swifttest */
/* juice
 * no longer remove special abilities or re-set pracs
 */

	if (newlevel <= GET_LEVEL(victim)) {
		send_to_char("Warning:  Lowering a player's level!\n\r", ch);

		GET_LEVEL(victim) = 1;
		GET_EXP(victim) = 1;

		set_title(victim);

		victim->points.max_hit = 10;	/* These are BASE numbers  */

		GET_HIT(victim) = hit_limit(victim);
		GET_MANA(victim) = mana_limit(victim);
		GET_MOVE(victim) = move_limit(victim);

		GET_COND(victim, THIRST) = 24;
		GET_COND(victim, FULL) = 24;
		GET_COND(victim, DRUNK) = 0;

		advance_level(victim);
	}

	adv = newlevel - GET_LEVEL(victim);

	send_to_char("You feel generous.\n\r", ch);
	act("$n makes some strange gestures.\n\rA strange feeling comes uppon you," "\n\rLike a giant hand, light comes down from\n\rabove, grabbing your " "body, that begins\n\rto pulse with coloured lights from inside.\n\rYo" "ur head seems to be filled with deamons\n\rfrom another plane as your" " body dissolves\n\rto the elements of time and space itself.\n\rSudde" "nly a silent explosion of light snaps\n\ryou back to reality. You fee" "l slightly\n\rdifferent.", FALSE, ch, 0, victim, TO_VICT);

	sprintf(buf,
		"%s advances %s to level %d.", GET_NAME(ch), GET_NAME(victim),
		newlevel);
	log_hd(buf);

	if (GET_LEVEL(victim) == 0) {
		do_start(victim);
	} else {
		if (GET_LEVEL(victim) < 35) {
			gain_exp_regardless(victim,
					    exp_table[GET_LEVEL(victim) + adv] -
					    GET_EXP(victim));
			send_to_char("Reminder:  Be careful with this.\n\r",
				     ch);
			if (GET_LEVEL(victim) > 30) {
				send_to_char
				    ("If you want to be immortal, type HELP on IMMORTAL, and POLICY!\n\r",
				     victim);
			}
		} else {
			send_to_char
			    ("Some idiot just tried to advance your level.\n\r",
			     victim);
			send_to_char("IMPOSSIBLE! IDIOTIC!\n\r", ch);
		}
	}
	sort_descriptors();
}

void do_reroll(struct char_data *ch, char *argument, int cmd)
{
	struct char_data *victim = NULL;
	char buf[MAX_STRING_LENGTH];

	if (IS_NPC(ch))
		return;

	one_argument(argument, buf);
	if (!*buf)
		send_to_char("Whom do you wish to reroll?\n\r", ch);
	else if (!(victim = get_char(buf)))
		send_to_char("No-one by that name in the world.\n\r", ch);
	else {
		if (GET_LEVEL(ch) < GET_LEVEL(victim)) {
			send_to_char("YEAH RIGHT!\n\r", ch);
			return;
		}
		send_to_char("Rerolled...\n\r", ch);
		roll_abilities(victim);
		sprintf(buf, "%s rerolled %s.", GET_NAME(ch), GET_NAME(victim));
		log_hd(buf);
	}
}

void do_restore(struct char_data *ch, char *argument, int cmd)
{
	struct char_data *victim = NULL;
	char buf[MAX_STRING_LENGTH];
	int i;

	void update_pos(struct char_data *victim);

	if (IS_NPC(ch))
		return;

	one_argument(argument, buf);
	if (!*buf)
		send_to_char("Whom do you wish to restore?\n\r", ch);
	else if (!(victim = get_char(buf)))
		send_to_char("No-one by that name in the world.\n\r", ch);
	else if (IS_HOVERING(victim)) {
		send_to_char("This person is beyond help.\n\r", ch);
		return;
	} else {
		GET_MANA(victim) = GET_MAX_MANA(victim);
		GET_HIT(victim) = GET_MAX_HIT(victim);
		GET_MOVE(victim) = GET_MAX_MOVE(victim);

		if (strcmp(GET_NAME(ch), "Vryce")) {
			sprintf(buf, "%s restored %s.", GET_NAME(ch),
				GET_NAME(victim));
			log_hd(buf);
		}
		if (GET_LEVEL(victim) > 31) {
			for (i = 0; i < MAX_SKILLS; i++) {
				victim->skills[i].learned = 100;
				victim->skills[i].recognise = TRUE;
			}

			if (GET_LEVEL(victim) > 33) {
				victim->abilities.intel = 25;
				victim->abilities.wis = 25;
				victim->abilities.dex = 25;
				victim->abilities.str = 25;
				victim->abilities.con = 25;
			}
			victim->tmpabilities = victim->abilities;

		}
		update_pos(victim);
		send_to_char("Done.\n\r", ch);
		act("You have been fully healed by $N!",
		    FALSE, victim, 0, ch, TO_CHAR);
	}
}

void do_setcomm(struct char_data *ch, char *argument, int cmd)
{
	char buf[MAX_INPUT_LENGTH];

	if (IS_NPC(ch))
		return;
	if (!ch->desc)
		return;
	one_argument(argument, buf);

	if (cmd == 9) {		/* first time in */
		if (*buf) {
			if (!strncmp("status", buf, strlen(buf))) {
				global_color = 1;
				send_to_char("Channel           Status\n\r",
					     ch);
				send_to_char("------------------------\n\r",
					     ch);
				if (IS_SET
				    (ORIGINAL(ch)->specials.act, PLR_NOSHOUT))
					send_to_char
					    ("Shout................off\n\r",
					     ch);
				else
					send_to_char
					    ("Shout.................on\n\r",
					     ch);
				if (IS_SET
				    (ORIGINAL(ch)->specials.new_comm_flags,
				     PLR_NOGOSSIP))
					send_to_char
					    ("Gossip...............off\n\r",
					     ch);
				else
					send_to_char
					    ("Gossip................on\n\r",
					     ch);
				if (IS_SET
				    (ORIGINAL(ch)->specials.new_comm_flags,
				     PLR_NOAUCTION))
					send_to_char
					    ("Auction..............off\n\r",
					     ch);
				else
					send_to_char
					    ("Auction...............on\n\r",
					     ch);
				if (IS_SET
				    (ORIGINAL(ch)->specials.new_comm_flags,
				     PLR_NODISCUSS))
					send_to_char
					    ("Discussion...........off\n\r",
					     ch);
				else
					send_to_char
					    ("Discussion............on\n\r",
					     ch);
				if (IS_SET
				    (ORIGINAL(ch)->specials.new_comm_flags,
				     PLR_NOQUEST))
					send_to_char
					    ("Quest................off\n\r",
					     ch);
				else
					send_to_char
					    ("Quest.................on\n\r",
					     ch);
				if (IS_SET
				    (ORIGINAL(ch)->specials.new_comm_flags,
				     PLR_NOTRIVIA))
					send_to_char
					    ("Trivia...............off\n\r",
					     ch);
				else
					send_to_char
					    ("Trivia................on\n\r",
					     ch);
				global_color = 0;
				return;
			}
			if (!strncmp("gossip", buf, strlen(buf))) {
				if (IS_SET
				    (ORIGINAL(ch)->specials.new_comm_flags,
				     PLR_NOGOSSIP)) {
					REMOVE_BIT(ORIGINAL(ch)->specials.
						   new_comm_flags,
						   PLR_NOGOSSIP);
					global_color = 1;
					send_to_char("Gossip toggled ON\n\r",
						     ch);
					global_color = 0;
				} else {
					SET_BIT(ORIGINAL(ch)->specials.
						new_comm_flags, PLR_NOGOSSIP);
					global_color = 1;
					send_to_char("Gossip toggled OFF\n\r",
						     ch);
					global_color = 0;
				}
				return;
			}
			if (!strncmp("shout", buf, strlen(buf))) {
				if (IS_SET
				    (ORIGINAL(ch)->specials.act, PLR_NOSHOUT)) {
					REMOVE_BIT(ORIGINAL(ch)->specials.act,
						   PLR_NOSHOUT);
					global_color = 1;
					send_to_char("Shout toggled ON\n\r",
						     ch);
					global_color = 0;
				} else {
					SET_BIT(ORIGINAL(ch)->specials.act,
						PLR_NOSHOUT);
					global_color = 1;
					send_to_char("Shout toggled OFF\n\r",
						     ch);
					global_color = 0;
				}
				return;
			}
			if (!strncmp("auction", buf, strlen(buf))) {
				if (IS_SET
				    (ORIGINAL(ch)->specials.new_comm_flags,
				     PLR_NOAUCTION)) {
					REMOVE_BIT(ORIGINAL(ch)->specials.
						   new_comm_flags,
						   PLR_NOAUCTION);
					global_color = 1;
					send_to_char("Auction toggled ON\n\r",
						     ch);
					global_color = 0;
				} else {
					SET_BIT(ORIGINAL(ch)->specials.
						new_comm_flags, PLR_NOAUCTION);
					global_color = 1;
					send_to_char("Auction toggled OFF\n\r",
						     ch);
					global_color = 0;
				}
				return;
			}
			if (!strncmp("discuss", buf, strlen(buf))) {
				if (IS_SET
				    (ORIGINAL(ch)->specials.new_comm_flags,
				     PLR_NODISCUSS)) {
					REMOVE_BIT(ORIGINAL(ch)->specials.
						   new_comm_flags,
						   PLR_NODISCUSS);
					global_color = 1;
					send_to_char
					    ("Discussion toggled ON\n\r", ch);
					global_color = 0;
				} else {
					SET_BIT(ORIGINAL(ch)->specials.
						new_comm_flags, PLR_NODISCUSS);
					global_color = 1;
					send_to_char
					    ("Discussion toggled OFF\n\r", ch);
					global_color = 0;
				}
				return;
			}
			if (!strncmp("quest", buf, strlen(buf))) {
				if (IS_SET
				    (ORIGINAL(ch)->specials.new_comm_flags,
				     PLR_NOQUEST)) {
					REMOVE_BIT(ORIGINAL(ch)->specials.
						   new_comm_flags, PLR_NOQUEST);
					global_color = 1;
					send_to_char("Quest toggled ON\n\r",
						     ch);
					global_color = 0;
				} else {
					SET_BIT(ORIGINAL(ch)->specials.
						new_comm_flags, PLR_NOQUEST);
					global_color = 1;
					send_to_char("Quest toggled OFF\n\r",
						     ch);
					global_color = 0;
				}
				return;
			}
			if (!strncmp("trivia", buf, strlen(buf))) {
				if (IS_SET
				    (ORIGINAL(ch)->specials.new_comm_flags,
				     PLR_NOTRIVIA)) {
					REMOVE_BIT(ORIGINAL(ch)->specials.
						   new_comm_flags,
						   PLR_NOTRIVIA);
					global_color = 1;
					send_to_char("Trivia toggled ON\n\r",
						     ch);
					global_color = 0;
				} else {
					SET_BIT(ORIGINAL(ch)->specials.
						new_comm_flags, PLR_NOTRIVIA);
					global_color = 1;
					send_to_char("Trivia toggled OFF\n\r",
						     ch);
					global_color = 0;
				}
				return;
			}
		}
		/* if (*buf) */
		ch->p->queryfunc = do_setcomm;
		strcpy(ch->p->queryprompt,
		       "Do you want to hear SHOUTS Channel? (y/n)> ");
		ch->p->querycommand = 1000;
		return;
	}
	if (cmd == 1000) {
		if (buf[0] == 'Y' || buf[0] == 'y')
			REMOVE_BIT(ORIGINAL(ch)->specials.act, PLR_NOSHOUT);
		else
			SET_BIT(ORIGINAL(ch)->specials.act, PLR_NOSHOUT);
		strcpy(ch->p->queryprompt,
		       "Do you want to hear GOSSIP Channel? (y/n)> ");
		ch->p->querycommand = 1001;
		return;
	}
	if (cmd == 1001) {
		if (buf[0] == 'Y' || buf[0] == 'y')
			REMOVE_BIT(ORIGINAL(ch)->specials.new_comm_flags,
				   PLR_NOGOSSIP);
		else
			SET_BIT(ORIGINAL(ch)->specials.new_comm_flags,
				PLR_NOGOSSIP);
		strcpy(ch->p->queryprompt,
		       "Do you want to hear AUCTION Channel? (y/n)> ");
		ch->p->querycommand = 1002;
		return;
	}
	if (cmd == 1002) {
		if (buf[0] == 'Y' || buf[0] == 'y')
			REMOVE_BIT(ORIGINAL(ch)->specials.new_comm_flags,
				   PLR_NOAUCTION);
		else
			SET_BIT(ORIGINAL(ch)->specials.new_comm_flags,
				PLR_NOAUCTION);
		strcpy(ch->p->queryprompt,
		       "Do you want to hear DISCUSSION Channel? (y/n)> ");
		ch->p->querycommand = 1004;
		return;
	}
	if (cmd == 1004) {
		if (buf[0] == 'Y' || buf[0] == 'y') {
			REMOVE_BIT(ORIGINAL(ch)->specials.new_comm_flags,
				   PLR_NODISCUSS);
		} else
			SET_BIT(ORIGINAL(ch)->specials.new_comm_flags,
				PLR_NODISCUSS);
		strcpy(ch->p->queryprompt,
		       "Do you want to hear QUEST Channel? (y/n)> ");
		ch->p->querycommand = 1005;
		return;
	}
	if (cmd == 1005) {
		if (buf[0] == 'Y' || buf[0] == 'y')
			REMOVE_BIT(ORIGINAL(ch)->specials.new_comm_flags,
				   PLR_NOQUEST);
		else
			SET_BIT(ORIGINAL(ch)->specials.new_comm_flags,
				PLR_NOQUEST);
		strcpy(ch->p->queryprompt,
		       "Do you want to hear the Hint-Info-Trivia Channel? (y/n)> ");
		ch->p->querycommand = 1006;
		return;
	}
	if (cmd == 1006) {
		if (buf[0] == 'Y' || buf[0] == 'y')
			REMOVE_BIT(ORIGINAL(ch)->specials.new_comm_flags,
				   PLR_NOTRIVIA);
		else
			SET_BIT(ORIGINAL(ch)->specials.new_comm_flags,
				PLR_NOTRIVIA);
		ch->p->querycommand = 0;
		return;
	}
}

void do_setgoddisplay(struct char_data *ch, char *argument, int cmd)
{
	char buf[MAX_INPUT_LENGTH];

	if (IS_NPC(ch))
		return;
	one_argument(argument, buf);

	if (cmd == 9) {		/* first time in */
		if (*buf) {
			if (!strncmp("status", buf, strlen(buf))) {
				global_color = 1;
				send_to_char("Channel           Status\n\r",
					     ch);
				send_to_char("------------------------\n\r",
					     ch);
				if (GET_LEVEL(ch) > 31)
					if (IS_SET
					    (ORIGINAL(ch)->specials.god_display,
					     GOD_INOUT))
						send_to_char
						    ("In/Outs...............on\n\r",
						     ch);
					else
						send_to_char
						    ("In/Out...............off\n\r",
						     ch);
				if (IS_SET
				    (ORIGINAL(ch)->specials.god_display,
				     GOD_ONOFF))
					send_to_char
					    ("God Channel...........on\n\r",
					     ch);
				else
					send_to_char
					    ("God Channel..........off\n\r",
					     ch);
				if (GET_LEVEL(ch) > 32)
					if (IS_SET
					    (ORIGINAL(ch)->specials.god_display,
					     GOD_ZONERESET))
						send_to_char
						    ("Zone Reset............on\n\r",
						     ch);
					else
						send_to_char
						    ("Zone Reset...........off\n\r",
						     ch);
				if (IS_SET
				    (ORIGINAL(ch)->specials.god_display,
				     GOD_ERRORS))
					send_to_char
					    ("Misc. Info............on\n\r",
					     ch);
				else
					send_to_char
					    ("Misc. Info...........off\n\r",
					     ch);
				if (GET_LEVEL(ch) > 32)
					if (IS_SET
					    (ORIGINAL(ch)->specials.god_display,
					     GOD_DEATHS))
						send_to_char
						    ("Deaths................on\n\r",
						     ch);
					else
						send_to_char
						    ("Deaths...............off\n\r",
						     ch);
				if (IS_SET
				    (ORIGINAL(ch)->specials.god_display,
				     GOD_SUMMONS))
					send_to_char
					    ("Summons...............on\n\r",
					     ch);
				else
					send_to_char
					    ("Summons..............off\n\r",
					     ch);
				global_color = 0;
				return;
			}
			if (!strncmp("inout", buf, strlen(buf))) {
				if (GET_LEVEL(ch) < 32)
					return;
				if (IS_SET
				    (ORIGINAL(ch)->specials.god_display,
				     GOD_INOUT)) {
					REMOVE_BIT(ORIGINAL(ch)->specials.
						   god_display, GOD_INOUT);
					global_color = 1;
					send_to_char("In/Out toggled OFF\n\r",
						     ch);
					global_color = 0;
				} else {
					SET_BIT(ORIGINAL(ch)->specials.
						god_display, GOD_INOUT);
					global_color = 1;
					send_to_char("In/Out toggled ON\n\r",
						     ch);
					global_color = 0;
				}
				return;
			}
			if (!strncmp("god", buf, strlen(buf))) {
				if (IS_SET
				    (ORIGINAL(ch)->specials.god_display,
				     GOD_ONOFF)) {
					REMOVE_BIT(ORIGINAL(ch)->specials.
						   god_display, GOD_ONOFF);
					global_color = 1;
					send_to_char
					    ("God channel toggled OFF\n\r", ch);
					global_color = 0;
				} else {
					SET_BIT(ORIGINAL(ch)->specials.
						god_display, GOD_ONOFF);
					global_color = 1;
					send_to_char
					    ("God channel toggled ON\n\r", ch);
					global_color = 0;
				}
				return;
			}
			if (!strncmp("zone", buf, strlen(buf))) {
				if (GET_LEVEL(ch) < 33)
					return;
				if (IS_SET
				    (ORIGINAL(ch)->specials.god_display,
				     GOD_ZONERESET)) {
					REMOVE_BIT(ORIGINAL(ch)->specials.
						   god_display, GOD_ZONERESET);
					global_color = 1;
					send_to_char
					    ("Zone Resets toggled OFF\n\r", ch);
					global_color = 0;
				} else {
					SET_BIT(ORIGINAL(ch)->specials.
						god_display, GOD_ZONERESET);
					global_color = 1;
					send_to_char
					    ("Zone Resets toggled ON\n\r", ch);
					global_color = 0;
				}
				return;
			}
			if (!strncmp("misc", buf, strlen(buf))) {
				if (IS_SET
				    (ORIGINAL(ch)->specials.god_display,
				     GOD_ERRORS)) {
					REMOVE_BIT(ORIGINAL(ch)->specials.
						   god_display, GOD_ERRORS);
					global_color = 1;
					send_to_char
					    ("Misc. channel toggled OFF\n\r",
					     ch);
					global_color = 0;
				} else {
					SET_BIT(ORIGINAL(ch)->specials.
						god_display, GOD_ERRORS);
					global_color = 1;
					send_to_char
					    ("Misc. channel toggled ON\n\r",
					     ch);
					global_color = 0;
				}
				return;
			}
			if (!strncmp("death", buf, strlen(buf))) {
				if (GET_LEVEL(ch) < 33)
					return;
				if (IS_SET
				    (ORIGINAL(ch)->specials.god_display,
				     GOD_DEATHS)) {
					REMOVE_BIT(ORIGINAL(ch)->specials.
						   god_display, GOD_DEATHS);
					global_color = 1;
					send_to_char
					    ("Death channel toggled OFF\n\r",
					     ch);
					global_color = 0;
				} else {
					SET_BIT(ORIGINAL(ch)->specials.
						god_display, GOD_DEATHS);
					global_color = 1;
					send_to_char
					    ("Death channel toggled ON\n\r",
					     ch);
					global_color = 0;
				}
				return;
			}
			if (!strncmp("summon", buf, strlen(buf))) {
				if (IS_SET
				    (ORIGINAL(ch)->specials.god_display,
				     GOD_SUMMONS)) {
					REMOVE_BIT(ORIGINAL(ch)->specials.
						   god_display, GOD_SUMMONS);
					global_color = 1;
					send_to_char
					    ("Summons channel toggled OFF\n\r",
					     ch);
					global_color = 0;
				} else {
					SET_BIT(ORIGINAL(ch)->specials.
						god_display, GOD_SUMMONS);
					global_color = 1;
					send_to_char
					    ("Summons channel toggled ON\n\r",
					     ch);
					global_color = 0;
				}
				return;
			}
		}
		/* if (*buf) */
		global_color = 32;
		send_to_char("\n\r   SET UP YOUR GOD DISPLAY\n\r", ch);
		global_color = 0;
		ch->p->queryfunc = do_setgoddisplay;
		if (GET_LEVEL(ch) > 31) {
			strcpy(ch->p->queryprompt,
			       "Do you want to see players in and out?(y/n)> ");
			ch->p->querycommand = 1000;
			return;
		} else {
			strcpy(ch->p->queryprompt,
			       "Do you want to see the God Channel? (y/n)> ");
			ch->p->querycommand = 1002;
			return;
		}
	}
	/* end of if(cmd == 9) */
	if (cmd == 1000) {
		if (buf[0] == 'Y' || buf[0] == 'y')
			SET_BIT(ch->specials.god_display, GOD_INOUT);
		else
			REMOVE_BIT(ch->specials.god_display, GOD_INOUT);
		strcpy(ch->p->queryprompt,
		       "Do you want to see the God Channel? (y/n)> ");
		ch->p->querycommand = 1002;
		return;
	}
	if (cmd == 1002) {
		if (buf[0] == 'Y' || buf[0] == 'y')
			SET_BIT(ch->specials.god_display, GOD_ONOFF);
		else
			REMOVE_BIT(ch->specials.god_display, GOD_ONOFF);
		if (GET_LEVEL(ch) > 32) {
			strcpy(ch->p->queryprompt,
			       "Do you want to see the Zones Resetting? (y/n)> ");
			ch->p->querycommand = 1003;
		} else {
			strcpy(ch->p->queryprompt,
			       "Do you want to see the Misc info(errors, tidbits)? (y/n)> ");
			ch->p->querycommand = 1004;
		}
		return;
	}
	if (cmd == 1003) {
		if ((buf[0] == 'Y' || buf[0] == 'y') && (GET_LEVEL(ch) > 31))
			SET_BIT(ch->specials.god_display, GOD_ZONERESET);
		else
			REMOVE_BIT(ch->specials.god_display, GOD_ZONERESET);
		strcpy(ch->p->queryprompt,
		       "Do you want to see the Misc info(errors, tidbits)? (y/n)> ");
		ch->p->querycommand = 1004;
		return;
	}
	if (cmd == 1004) {
		if (buf[0] == 'Y' || buf[0] == 'y')
			SET_BIT(ch->specials.god_display, GOD_ERRORS);
		else
			REMOVE_BIT(ch->specials.god_display, GOD_ERRORS);
		if (GET_LEVEL(ch) > 32) {
			strcpy(ch->p->queryprompt,
			       "Do you want to see Player Deaths? (y/n)> ");
			ch->p->querycommand = 1005;
		} else {
			strcpy(ch->p->queryprompt,
			       "Do you want to see Summons? (y/n)> ");
			ch->p->querycommand = 1006;
		}
		return;
	}
	if (cmd == 1005) {
		if (buf[0] == 'Y' || buf[0] == 'y')
			SET_BIT(ch->specials.god_display, GOD_DEATHS);
		else
			REMOVE_BIT(ch->specials.god_display, GOD_DEATHS);
		strcpy(ch->p->queryprompt,
		       "Do you want to see Summons? (y/n)> ");
		ch->p->querycommand = 1006;
		return;
	}
	if (cmd == 1006) {
		if (buf[0] == 'Y' || buf[0] == 'y')
			SET_BIT(ch->specials.god_display, GOD_SUMMONS);
		else
			REMOVE_BIT(ch->specials.god_display, GOD_SUMMONS);
		ch->p->querycommand = 0;
		return;
	}
}

void do_wizinvis(struct char_data *ch, char *argument, int cmd)
{
	struct affected_type af;
	struct affected_type *hjp = NULL;

	if (IS_NPC(ch)) {
		return;
	}
	for (hjp = ch->affected;
	     ((hjp != NULL) && (hjp->type != SPELL_INVISIBLE));
	     hjp = hjp->next) ;

	if ((hjp != NULL) && (ch->specials.wizInvis)) {
		/* Re-appear */
		hjp->type = SPELL_INVISIBLE;
		hjp->duration = 0;
		hjp->modifier = 0;
		hjp->location = APPLY_NONE;
		hjp->bitvector = AFF_INVISIBLE;
		affect_remove(ch, hjp);
		global_color = 32;
		ch->specials.wizInvis = FALSE;
		send_to_char
		    ("You slowly fade back into the world of mortals.\n\r", ch);
		act("Someone chants the words 'An Ort Quas'.\n$n slowly fades into existence.", FALSE, ch, 0, 0, TO_ROOM);
		global_color = 0;
	} else {
		/* Disappear */
		if (hjp != NULL) {	/* remove normal invis spell */
			hjp->type = SPELL_INVISIBLE;
			hjp->duration = 0;
			hjp->modifier = 0;
			hjp->location = APPLY_NONE;
			hjp->bitvector = AFF_INVISIBLE;
			affect_remove(ch, hjp);
		}
		af.type = SPELL_INVISIBLE;
		af.duration = -1;	/* Forever */
		af.modifier = 0;
		af.location = APPLY_NONE;
		af.bitvector = AFF_INVISIBLE;
		affect_to_char(ch, &af);
		global_color = 34;
		ch->specials.wizInvis = TRUE;
		send_to_char("You slowly vanish into thin-air.\n\r", ch);
		act("$n chants the words 'Vas Ort Quas'.", FALSE, ch, 0, 0,
		    TO_ROOM);
		act("$n slowly fades and disappears from the realm of mortals.",
		    FALSE, ch, 0, 0, TO_ROOM);
		global_color = 0;
	}
}

void do_holylite(struct char_data *ch, char *argument, int cmd)
{
	if (IS_NPC(ch))
		return;

	if (argument[0] != '\0')
		send_to_char
		    ("HOLYLITE doesn't take any arguments, arg ignored.\n\r",
		     ch);

	if (ch->specials.holyLite) {
		ch->specials.holyLite = FALSE;
		send_to_char("Holy light mode off.\n\r", ch);
	} else {
		ch->specials.holyLite = TRUE;
		send_to_char("Holy light mode on.\n\r", ch);
	}			/* if */
}

void do_teleport(struct char_data *ch, char *argument, int cmd)
{
	struct char_data *victim = NULL, *target_mob = NULL, *pers = NULL;
	char person[MAX_INPUT_LENGTH], room[MAX_INPUT_LENGTH];
	int target;
	int loop;
	extern int top_of_world;

	if (IS_NPC(ch))
		return;

	half_chop(argument, person, room);

	if (!*person) {
		send_to_char("Who do you wish to teleport?\n\r", ch);
		return;
	}
	/* if */
	if (!*room) {
		send_to_char("Where do you wish to send this person?\n\r", ch);
		return;
	}
	/* if */
	if (!(victim = get_char_vis(ch, person))) {
		send_to_char("No-one by that name around.\n\r", ch);
		return;
	}
	/* if */
	if (isdigit(*room)) {
		target = atoi(&room[0]);
		for (loop = 0; loop <= top_of_world; loop++) {
			if (world[loop] && world[loop]->number == target) {
				target = (int)loop;
				break;
			} else if (loop == top_of_world) {
				send_to_char
				    ("No room exists with that number.\n\r",
				     ch);
				return;
			}	/* if */
		}		/* for */
	} else if ((target_mob = get_char_vis(ch, room)) != NULL) {
		target = target_mob->in_room;
	} else {
		send_to_char("No such target (person) can be found.\n\r", ch);
		return;
	}			/* if */

	if (IS_SET(world[target]->room_flags, PRIVATE)) {
		for (loop = 0, pers = world[target]->people; pers;
		     pers = pers->next_in_room, loop++) ;
		if (loop > 1) {
			send_to_char
			    ("There's a private conversation going on in that room\n\r",
			     ch);
			return;
		}		/* if */
	}
	/* if */
	if (IS_PLAYER(victim, "Vryce")) {
		send_to_char("Vryce would not like that!\n\r", ch);
		do_tell(ch, "vryce I JUST TRIED TO TELEPORT YOU!", 9);
		return;
	}
	if (IS_PLAYER(victim, "Io")) {
		send_to_char("Io would not like that!\n\r", ch);
		do_tell(ch, "io I JUST TRIED TO TELEPORT YOU!", 9);
		return;
	}
	if (IS_SET(world[target]->room_flags, GODPROOF)
	    && GET_LEVEL(ch) < 35) {
/*		&& !IS_PLAYER(ch, "Io")
		&& !IS_PLAYER(ch, "Vryce")
		&& !IS_PLAYER(ch, "Firm")){
*/
		send_to_char("Sorry that room is GODPROOF.\n\r", ch);
		return;
	}
	global_color = 35;
	act("$n disappears in a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
	StopFlying(victim);
	MOUNTMOVE = TRUE;
	char_from_room(victim);
	char_to_room(victim, target);
	MOUNTMOVE = FALSE;
	act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
	act("$n has teleported you!", FALSE, ch, 0, (char *)victim, TO_VICT);
	do_look(victim, "", 15);
	send_to_char("Teleport completed.\n\r", ch);
	global_color = 0;
/* juice
 * logging all teleports now
 */
	sprintf(log_buf, "%s teleports %s to [%d]%s",
		GET_NAME(ch), GET_NAME(victim), target, world[target]->name);
	log_hd(log_buf);
}				/* do_teleport */

void save_ban_list(void)
{
	struct ban_t *tmp = NULL;
	FILE *fh;

	tmp = ban_list;
	if (!tmp)
		return;
	if (!(fh = med_open("../lib/ban_list.txt", "w")))
		return;
	open_files++;
	while (tmp) {
		write_filtered_text(fh, tmp->name);
		write_filtered_text(fh, tmp->god);
		write_filtered_text(fh, tmp->date);
		write_filtered_text(fh, tmp->reason);
		tmp = tmp->next;
	}
	write_filtered_text(fh, "$");
	med_close(fh);
	open_files--;
}

void do_ban(struct char_data *ch, char *argument, int cmd)
{
}

void load_ban_list(void)
{
	FILE *fh;
	struct ban_t *tmp = NULL;
	int count = 0;

	if (!(fh = med_open("../lib/ban_list.txt", "r")))
		return;
	open_files++;
	while (1) {
		count++;	/*just a major FUBAR test */
		if (count > 10000)
			return;
		CREATE(tmp, struct ban_t, 1);
		tmp->name = fread_string(fh);
		if (tmp->name[0] == '$') {
			break;
		}
		tmp->god = fread_string(fh);
		tmp->date = fread_string(fh);
		tmp->reason = fread_string(fh);
		tmp->next = ban_list;
		ban_list = tmp;
	}
	if (fh) {
		med_close(fh);
		open_files--;
	}
	if (tmp->name[0] == '$') {
		tmp->name = my_free(tmp->name);
		tmp = my_free(tmp);
	}
}

void do_allow(struct char_data *ch, char *argument, int cmd)
{
	char name[MAX_INPUT_LENGTH];
	struct ban_t *curr = NULL, *prev = NULL;

	if (IS_NPC(ch))
		return;

	one_argument(argument, name);

	if (!*name) {
		send_to_char("Remove which site from the ban list?\n\r", ch);
		return;
	}
	curr = prev = ban_list;
	if (!ban_list) {
		send_to_char("No sites banned at this time.\n\r", ch);
		return;
	}
	if (str_cmp(curr->name, name) == 0) {
		ban_list = ban_list->next;
		curr->name = my_free(curr->name);
		curr->name = my_free(curr);
		send_to_char("Ok site removed.\n\r", ch);
		save_ban_list();
		return;
	}
	curr = curr->next;
	while (curr) {
		if (!strcmp(curr->name, name)) {
			if (curr->next) {
				prev->next = curr->next;
				curr->name = my_free(curr->name);
				curr->name = my_free(curr);
				send_to_char("Ok site removed from list.\n\r",
					     ch);
				save_ban_list();
				return;
			}
			prev->next = (struct ban_t *)NULL;
			curr->name = my_free(curr->name);
			curr = my_free(curr);
			send_to_char("Ok site removed.\n\r", ch);
			save_ban_list();
			return;
		}
		curr = curr->next;
		prev = prev->next;
	}
	send_to_char("Site not found in list!\n\r", ch);
}

void do_gshow(struct char_data *ch, char *argument, int cmd)
{
	char *values[] = {
		"hints", "\n"
	};
	char my_buf[MAX_INPUT_LENGTH];
	char help[MAX_INPUT_LENGTH];
	char trivia_buf[MAX_STRING_LENGTH];
	int i = 0;		/*generic counter */
	int list = 0;		/*value array index */
	extern int top_trivia;
	extern char *trivia[];

	*help = '\0';
	if (IS_NPC(ch))
		return;
	one_argument(argument, my_buf);
	if (!*my_buf) {
		send_to_char("Syntax for gshow:\r\n", ch);
		send_to_char("gshow <list>\r\n\r\n", ch);
		send_to_char("Where list is one of the following:\r\n", ch);
		for (i = 0; *values[i] != '\n'; i++) {
			sprintf(help, "%s%-18.18s", help, values[i]);
			if (!(i % 4)) {
				strcat(help, "\r\n");
				send_to_char(help, ch);
				*help = '\0';
			}
		}
		if (*help)
			send_to_char(help, ch);
		send_to_char("\r\n", ch);
		return;
	}
	if ((list = old_search_block(my_buf, 0, strlen(my_buf), values, 1)) < 0) {
		send_to_char
		    ("That list is not known, try 'gshow' for a list of lists\r\n",
		     ch);
		return;
	}
	switch (list) {
	case 1:		/*hint/info/trivia */
		i = 0;
		ch->specials.setup_page = 1;
		page_setup[0] = MED_NULL;
		while (i < top_trivia) {
			sprintf(trivia_buf, "%s[%s%d%s]%s - %s%s", BLU(ch),
				GRN(ch), i, BLU(ch), GRN(ch), CYN(ch),
				trivia[i]);
			send_to_char(trivia_buf, ch);
			i++;
		}
		ch->specials.setup_page = 0;
		page_string(ch->desc, page_setup, 1);
	default:
		break;
	}
}
