/***************************************************************************
*					 MEDIEVIA CyberSpace Code and Data files		  	   *
*       Copyright (C) 1991, 1996 INTENSE Software(tm) and Mike Krause	   *
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
#include <unistd.h>
#include "structs.h"
#include "mob.h"
#include "obj.h"
#include "utils.h"
#include "db.h"
#include "handler.h"
#include "limits.h"
#include "spells.h"
#include "holocode.h"
#include "dragon.h"

extern struct room_data *world[MAX_ROOM];	/* array of rooms                  */
extern struct char_data *character_list;	/* global l-list of chars          */
extern char global_color;
extern struct descriptor_data *descriptor_list;
extern int dice(int number, int size);
extern struct char_data *mobs[MAX_MOB];
extern bool DigitString(char *szpText);
extern int number(int from, int to);
extern void remove_room_affect(struct room_affect *ra, char type);
extern void do_move(struct char_data *ch, char *argument, int cmd);
extern double dHoloDistance(int iX1, int iY1, int iX2, int iY2);
extern struct zone_data *zone_table;
extern ush_int Holo[MAXHOLO][MAXHOLO];
extern char MOUNTMOVE;
extern int iFlyStoreRoom;
extern int pulse;
extern int number_of_rooms;
extern struct HoloSurvey *survey_list;
extern struct HOLOROOMS *stpHoloRTemplates[256];

struct DRAGONSTRUCT gstaDragons[MAXDRAGONS];
int giNumPlayersInWilderness;
int giaDragonTypes[MAXDRAGONTYPES][2];
#define GOODDRAGON			0
#define EVILDRAGON			1

void RemoveDragonSurvey(int iSlot)
{
	struct HoloSurvey *stpS, *stpSRemove;

	if (!gstaDragons[iSlot].stpSurvey)
		return;
	if (survey_list == gstaDragons[iSlot].stpSurvey) {
		survey_list = gstaDragons[iSlot].stpSurvey->next;
		gstaDragons[iSlot].stpSurvey->description =
		    my_free(gstaDragons[iSlot].stpSurvey->description);
		gstaDragons[iSlot].stpSurvey =
		    my_free(gstaDragons[iSlot].stpSurvey);
	} else {
		for (stpS = survey_list; stpS->next; stpS = stpS->next) {
			if (stpS->next == gstaDragons[iSlot].stpSurvey)
				break;
		}
		if (stpS->next) {
			stpSRemove = stpS->next;
			stpS->next = stpS->next->next;
			stpSRemove->description =
			    my_free(stpSRemove->description);
			stpSRemove = my_free(stpSRemove);
		}
	}
}

void do_CallDragon(struct char_data *stpCh, char *szpArg, int iCmd)
{
	if (GET_LEVEL(stpCh) < 5) {
		send_to_char("You must be at least level 5 to call a dragon.\n", stpCh);
		return;
	}

	int iMob, x, y, xxx, yyy, iSlot;

	if (world[stpCh->in_room]->sector_type == SECT_INSIDE) {
		global_color = 32;
		send_to_char
		    ("You realize it is impossible to call a dragon from the indoors.\n\r",
		     stpCh);
		global_color = 0;
		return;
	}

	global_color = 32;
	act("$n uses $s powers to plea for a dragon's aid.", TRUE, stpCh, 0, 0,
	    TO_ROOM);
	send_to_char("You use your powers to plea for a dragon's aid.\n",
		     stpCh);
	global_color = 0;
	for (x = 0; x < MAXDRAGONTYPES; x++)
		if (!giaDragonTypes[x][GOODDRAGON])
			break;
	iMob = number(0, x - 1);
	for (iSlot = 0; iSlot < MAXDRAGONS; iSlot++)
		if (!gstaDragons[iSlot].stpDragon)
			break;
	if (iSlot == MAXDRAGONS) {
		global_color = 32;
		send_to_char("You get the feeling all dragons are busy.\n\r",
			     stpCh);
		global_color = 0;
		return;
	}
	gstaDragons[iSlot].stpDragon =
	    read_mobile(giaDragonTypes[iMob][GOODDRAGON], REAL);
	SET_BIT(gstaDragons[iSlot].stpDragon->player.siMoreFlags, DRAGON);
	if (!gstaDragons[iSlot].stpDragon)
		SUICIDE;
	gstaDragons[iSlot].iPulse = pulse;
	gstaDragons[iSlot].iStatus = RESPONDING;
	gstaDragons[iSlot].iRoom = 0;

	gstaDragons[iSlot].iX = HOLOX(stpCh) + 30;
	gstaDragons[iSlot].iY = HOLOY(stpCh) + 40;

	if (gstaDragons[iSlot].iX < 10)
		gstaDragons[iSlot].iX = 10;
	if (gstaDragons[iSlot].iX > MAXHOLO - 10)
		gstaDragons[iSlot].iX = MAXHOLO - 10;
	if (gstaDragons[iSlot].iY < 10)
		gstaDragons[iSlot].iY = 10;
	if (gstaDragons[iSlot].iY > MAXHOLO - 10)
		gstaDragons[iSlot].iY = MAXHOLO - 10;

	gstaDragons[iSlot].stpCaller = stpCh;

	CREATE(gstaDragons[iSlot].stpSurvey, struct HoloSurvey, 1);
	gstaDragons[iSlot].stpSurvey->iX = gstaDragons[iSlot].iX;
	gstaDragons[iSlot].stpSurvey->iY = gstaDragons[iSlot].iY;
	gstaDragons[iSlot].stpSurvey->dist = SIGHTDIST;
	CREATE(gstaDragons[iSlot].stpSurvey->description, char, 200);
	sprintf(gstaDragons[iSlot].stpSurvey->description,
		"you see %s responding to someones plea",
		gstaDragons[iSlot].stpDragon->player.short_descr);
	gstaDragons[iSlot].stpSurvey->next = survey_list;
	survey_list = gstaDragons[iSlot].stpSurvey;

	x = gstaDragons[iSlot].iX - SIGHTDIST;
	if (x < 1)
		x = 1;
	y = gstaDragons[iSlot].iY - SIGHTDIST;
	if (y < 1)
		y = 1;
	xxx = gstaDragons[iSlot].iX + SIGHTDIST;
	if (xxx > MAXHOLO - 1)
		xxx = MAXHOLO - 1;
	yyy = gstaDragons[iSlot].iY + SIGHTDIST;
	if (yyy > MAXHOLO - 1)
		yyy = MAXHOLO - 1;
	HoloAct(x, y, gstaDragons[iSlot].iX, gstaDragons[iSlot].iY, xxx, yyy,
		"$n appear responding to someones plea",
		TRUE, gstaDragons[iSlot].stpDragon, NULL, NULL, 33, "notice");
}

void RemoveDragon(struct char_data *stpDragon)
{
	int iSlot;

	for (iSlot = 0; iSlot < MAXDRAGONS; iSlot++) {
		if (gstaDragons[iSlot].stpDragon == stpDragon) {
			RemoveDragonSurvey(iSlot);
			gstaDragons[iSlot].stpDragon = NULL;
			return;
		}
	}
}

void Respond(int iSlot)
{
	int a = gstaDragons[iSlot].iX;
	int b = gstaDragons[iSlot].iY;

	struct descriptor_data *stpS;
	int iE, iW, iN, iS, x = 0, y = 0, xxx, yyy, i;

	for (stpS = descriptor_list; stpS; stpS = stpS->next) {
		if (stpS->character == gstaDragons[iSlot].stpCaller) {
			if (dHoloDistance
			    (HOLOX(gstaDragons[iSlot].stpCaller),
			     HOLOY(gstaDragons[iSlot].stpCaller),
			     gstaDragons[iSlot].iX,
			     gstaDragons[iSlot].iY) < 6) {
				gstaDragons[iSlot].iStatus = FLYING;
				char_to_room(gstaDragons[iSlot].stpDragon,
					     stpS->character->in_room);
				gstaDragons[iSlot].iX =
				    HOLOX(gstaDragons[x].stpDragon);
				gstaDragons[iSlot].iY =
				    HOLOY(gstaDragons[x].stpDragon);
				gstaDragons[iSlot].iRoom =
				    gstaDragons[x].stpDragon->in_room;
				RemoveDragonSurvey(iSlot);
				act("A wise-looking dragon crashes into the room, ready to be mounted.", TRUE, gstaDragons[iSlot].stpDragon, 0, 0, TO_ROOM);
				return;
			}
			iE = gstaDragons[iSlot].iX - HOLOX(stpS->character);
			iW = HOLOX(stpS->character) - gstaDragons[iSlot].iX;
			iS = gstaDragons[iSlot].iY - HOLOY(stpS->character);
			iN = HOLOY(stpS->character) - gstaDragons[iSlot].iY;
			if (iE >= iW && iE >= iS && iE >= iN)
				x = -5;
			if (iW >= iE && iW >= iS && iW >= iN)
				x = 5;
			if (iS >= iE && iS >= iW && iS >= iN)
				y = -5;
			if (iN >= iE && iN >= iW && iN >= iS)
				y = 5;
			gstaDragons[iSlot].iX += x;
			gstaDragons[iSlot].iY += y;
			gstaDragons[iSlot].stpSurvey->iX =
			    gstaDragons[iSlot].iX;
			gstaDragons[iSlot].stpSurvey->iY =
			    gstaDragons[iSlot].iY;
			return;
		}
	}
	extract_char(gstaDragons[iSlot].stpDragon, TRUE);
}

void Flying(int iSlot)
{
	int x, y, xxx, yyy;

	if (gstaDragons[iSlot].stpDragon->in_room != iFlyStoreRoom) {
		if ((pulse - gstaDragons[iSlot].iPulse) > FLYTIMEPULSE) {
			x = HOLOX(gstaDragons[iSlot].stpDragon) - SIGHTDIST;
			if (x < 1)
				x = 1;
			y = HOLOY(gstaDragons[iSlot].stpDragon) - SIGHTDIST;
			if (y < 1)
				y = 1;
			xxx = HOLOX(gstaDragons[iSlot].stpDragon) + SIGHTDIST;
			if (xxx > MAXHOLO - 1)
				xxx = MAXHOLO - 1;
			yyy = HOLOY(gstaDragons[iSlot].stpDragon) + SIGHTDIST;
			if (yyy > MAXHOLO - 1)
				yyy = MAXHOLO - 1;
			HoloAct(x, y, HOLOX(gstaDragons[iSlot].stpDragon),
				HOLOY(gstaDragons[iSlot].stpDragon), xxx, yyy,
				"$n fly up and away slowly..", TRUE,
				gstaDragons[iSlot].stpDragon, NULL, NULL, 31,
				"notice");
			extract_char(gstaDragons[iSlot].stpDragon, TRUE);
			return;
		}
		if (!gstaDragons[iSlot].stpDragon->specials.stpMount) {
			if (number(0, 100) < 4) {
				global_color = 34;
				act("$n looks around...waiting to be mounted..looks as if $e may just fly off.", TRUE, gstaDragons[iSlot].stpDragon, 0, 0, TO_ROOM);
				global_color = 0;
			}
			return;
		}
		if (number(0, 100) < 10) {
			global_color = 34;
			act("$n stretches $s wings...wanting to fly.", TRUE,
			    gstaDragons[iSlot].stpDragon, 0, 0, TO_ROOM);
			global_color = 0;
		}
	}
}

void DragonControl(void)
{
	int iNumDragons = 0, x;
	struct descriptor_data *stpPeople;

	for (x = 0; x < MAXDRAGONS; x++) {
		if (gstaDragons[x].stpDragon) {
			switch (gstaDragons[x].iStatus) {
			case RESPONDING:
				Respond(x);
				break;
			case FLYING:
				Flying(x);
				break;
			default:
				SUICIDE;
			}
		}
	}
}

void LoadDragonList(void)
{
	FILE *fl;
	char szTag[8192];
	int x, iSlot, iType, iMob;

	if (!(fl = fopen("../lib/medievia.dragons", "r"))) {
		perror("Opening medievia.dragons");
		SUICIDE;
	}
	for (x = 0; x < MAXDRAGONTYPES; x++) {
		giaDragonTypes[x][GOODDRAGON] = 0;
		giaDragonTypes[x][EVILDRAGON] = 0;
	}
	iSlot = 0;
	fprintf(stderr, "DRAGONS:  Loading Dragon Data...\n");
	iType = GOODDRAGON;
	while (1) {
		fscanf(fl, " %s ", szTag);
		if (szTag[0] == '$')
			break;
		if (!str_cmp(szTag, "GOODDRAGONS")) {
			iType = GOODDRAGON;
			iSlot = 0;
		} else if (!str_cmp(szTag, "EVILDRAGONS")) {
			iType = EVILDRAGON;
			iSlot = 0;
		} else if (DigitString(szTag)) {
			iMob = atoi(szTag);
			if (iMob < 0 || iMob > MAX_MOB)
				SUICIDE;
			if (!mobs[iMob])
				SUICIDE;
			if (iType == MAXDRAGONTYPES)
				SUICIDE;
			giaDragonTypes[iSlot][iType++] = iMob;
		} else {
			SUICIDE;
		}
	}
	fclose(fl);
}

void SetupDragons(void)
{
	int x;

	fprintf(stderr, "DRAGONS: Initializing Dragon Control Center...\n");
	for (x = 0; x < MAXDRAGONS; x++) {
		gstaDragons[x].stpDragon = NULL;
	}
	LoadDragonList();
}
