/***************************************************************************
*                MEDIEVIA CyberSpace Code and Data files                   *
*       Copyright (C) 1992, 1995 INTENSE Software(tm) and Mike Krause      *
*                          All rights reserved                             *
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
#include "interp.h"

/*                  /External variables and functions\                    */
/*------------------------------------------------------------------------*/
extern struct room_data *world[MAX_ROOM];
extern struct char_data *mobs[MAX_MOB];
extern struct obj_data *objs[MAX_OBJ];
extern int top_of_world;
extern struct obj_data *object_list;
extern struct char_data *character_list;
extern char global_color;
extern struct zone_data zone_table_array[MAX_ZONE];
extern struct zone_data *zone_table;
extern int top_of_zone_table;
extern struct index_data mob_index_array[MAX_MOB];
extern struct index_data *mob_index;
extern struct index_data obj_index_array[MAX_OBJ];
extern struct index_data *obj_index;
extern int top_of_mobt;
extern int top_of_objt;
extern struct descriptor_data *descriptor_list;
extern int dice(int number, int size);
extern int number(int from, int to);
extern void space_to_underline(char *text);
extern void page_string(struct descriptor_data *d, char *str, int keep_internal);
extern void save_room_actions(struct char_data *ch);
extern int number_of_rooms;

/*		           /Action Variables\				  */
/*------------------------------------------------------------------------*/
struct room_actions *ractions[MAX_RACTION];  /*raction storage area*/
struct raction_timing *raction_seconds;
struct raction_oe_design raction_design[23];


void do_raction(int room, struct char_data *ch, int action_num);
void do_editroom_actions(struct char_data *ch, char *argument, int cmd);
/* METHODS */
#define PLAYER 1  /*Do things per roll per player*/
#define ROOM 2    /*Roll once, do to all at once */

/*TO ADD A NEW ACTION:  Up the array raction_design by 1.  Add the action
name and the menu number to raction_list using the next number.  Set up
the raction_design with the proper text to be seen by the action OE user
and the order of fields and which fields to use by the NEXT fields in
raction_design.  Then code what the action does in do_raction.  Keep in
mind you are adding just a primitive tool, the power of actions is the
SEQUENCE of stringing them together, not in singular events.  Make sure
you understand the WHOLE action system first, know the assumptions made
by the system and know the effects of every action.  The purpose of
actions is to allow coding of VR events, like a script in a movie where
the script changes due to player variables, luck and prior events.  The
goal is to allow this vr ability to be made completly online in an easy
to use method by designing basic actions and putting them together
online in a logic sequence, using plain english and code that asks the
proper questions.  Immediate testing and no compiling and using plain
prompted english input makes this method better than any other.
KEEP IN MIND that actions are stored as part of a room, loose the room,
you loose the actions.  Object and Mobile actions will follow.
You must also Up the define below MAX_ACTION_TYPE.*/

char *raction_list[]={
" 0-Chatter          ",
" 1-Stat Change      ",
" 2-Room Change      ",
" 3-Set Action Start ",
" 4-Have Item?       ",
" 5-Item in Inv?     ",
" 6-Item in Equip?   ",
" 7-Load Mobile      ",
" 8-Object to Mobile ",
" 9-Object to Player ",
"10-Load Object      ",
"11-Lock Door        ",
"12-Unlock Door      ",
"13-Open Door        ",
"14-Close Door       ",
"15-Force Mob        ",
"16-Force Player     ",
"17-Player Waits     ",
"18-Said This?       ",
"19-Room occupied?   ",
"20-Make two way exit",
"21-kill exit one way",
"22-zone chatter     ",
"$"
};

#define MAX_TYPE_ACTION 23
#define ARG1 1001
#define ARG2 1002
#define ARG3 1003
#define METHOD 1004
#define TEXT1 1005
#define TEXT2 1006
#define TEXT3 1007
#define END 666

void setup_next_prompt(struct char_data *ch, int next)
{
    switch(next){
	case 1001:
	    strcpy(ch->p->queryprompt,
	    raction_design[(int)ractions[ch->internal_use]->type].arg1);
	    ch->p->querycommand=next;
	    break;
	case 1002:
	    strcpy(ch->p->queryprompt,
	    raction_design[(int)ractions[ch->internal_use]->type].arg2);
	    ch->p->querycommand=next;
	    break;
	case 1003:
	    strcpy(ch->p->queryprompt,
	    raction_design[(int)ractions[ch->internal_use]->type].arg3);
	    ch->p->querycommand=next;
	    break;
	case 1004:
	    strcpy(ch->p->queryprompt,
	    raction_design[(int)ractions[ch->internal_use]->type].method);
	    ch->p->querycommand=next;
	    break;
	case 1005:
	    send_to_char(raction_design[(int)ractions[ch->internal_use]->type].text1,ch);
            global_color=31;
            act("$n starts editing the room Actions",TRUE,ch,0,0,TO_ROOM);
            global_color=0;
            ch->desc->str = &ractions[ch->internal_use]->text1;
            ch->desc->max_str = 2500;
	    ch->p->querycommand=next;
 	    strcpy(ch->p->queryprompt,"\n\rPRESS [RETURN]");
	    break;
	case 1006:
	    send_to_char(raction_design[(int)ractions[ch->internal_use]->type].text2,ch);
            global_color=31;
            act("$n starts editing the room Actions",TRUE,ch,0,0,TO_ROOM);
            global_color=0;
            ch->desc->str = &ractions[ch->internal_use]->text2;
            ch->desc->max_str = 2500;
	    ch->p->querycommand=next;
 	    strcpy(ch->p->queryprompt,"\n\rPRESS [RETURN]");
	    break;
	case 1007:
	    send_to_char(raction_design[(int)ractions[ch->internal_use]->type].text3,ch);
            global_color=31;
            act("$n starts editing the room Actions",TRUE,ch,0,0,TO_ROOM);
            global_color=0;
            ch->desc->str = &ractions[ch->internal_use]->text3;
            ch->desc->max_str = 2500;
	    ch->p->querycommand=next;
 	    strcpy(ch->p->queryprompt,"\n\rPRESS [RETURN]");
	    break;
	case 666:
	    do_editroom_actions(ch,"",9);
	    break;

    }

}
void setup_raction_design(void)
{
}

void do_editroom_actions(struct char_data *ch, char *argument, int cmd)
{
}


/*			    FIRE_RACTION				*/
/* fire_raction starts(fires up) an action at its most advanced state.
The r in raction means ROOM. o(object) & m(mobile) will follow.
It looks to see the actions method, and rolls the die to see if that
action will fire(if the action even needs a die throw).  It then nodes
off and fires subsequent actions calling itself(recursive) depending on
if it fired or not, calling the proper action.  Sorta like traveling 
down a tree and turning at the proper branches.  It does another
important thing, it checks to see if it should start action once for the
room as a whole, or roll and fire/notfire for each player in room.
It then makes sure after a roll_per_player scenario that further down that
path on the tree it will not do a major branch and refire an action
for each player in room for that single action.  That could cause some
possible looping feedback and hang the game.

It ends up calling more primitive function do_raction which actually does
the action.  The tree if imagined would end up looking like 
BRANCHES(fire_raction) and dead end leaves(do_action), with one
leaf sticking out of every fork in the branch*/

void fire_raction(int room, struct char_data *ch, int action_num)
{
struct room_actions *action=NULL;
struct char_data *player=NULL;

    if(!world[room])return; /*better safe than sorry*/
    action=ractions[action_num];
    if(!action)return; /*should never happen*/
    if(ch){ /*Disallow possible feedback */
       if(action->chance){
	   if(number(1,100)>action->chance){ 
		  /*didn't happen*/
	        if(action->notfired_next)
		   fire_raction(room,ch,action->notfired_next);
		return;
	   }
       }
               /*Its gonna happen*/
       do_raction(room,ch,action_num);	
       if(action->fired_next)
	   fire_raction(room,ch,action->fired_next);
       return;
    }
    /*METHOD ROOM: One chance roll to see if action fires if so it call*/
    /*the do_action and that should see that its a room method and do  */
    /*its deal to everyone.  Different actions will deal with this     */
    /*in different ways, many don't even care if a player even exists  */
    /*in the room.  Others may echo to room, while others may have     */
    /*text1,2&3 setup for to_room,to_char,to_vict and use act function.*/
    /*Some actions may even use arg1,2or3 in the OE editing to querry  */
    /*user as how to deal with method room.  For instance action       */
    /*load_obj may ask(should we just load one or one per player in the*/
    /*room), then set up arg2 and deal with it in do_action.  Point    */
    /*being that the creator of actions in OE should just believe the  */
    /*and have faith in the code to ask the proper questions per action*/
    /*depending on his answers so far.				       */
      
    if(IS_SET(action->method,ROOM)){
       if(action->chance){
	   if(number(1,100)>action->chance){ 
		  /*didn't happen*/
	        if(action->notfired_next)
		   fire_raction(room,0,action->notfired_next);
		return;
	   }
       }
               /*Its gonna happen*/
       do_raction(room,NULL,action_num);	
       if(action->fired_next)
	   fire_raction(room,0,action->fired_next);
       return;
    }
    /*METHOD PLAYER: Here we roll one chance of firing roll per player*/
    /*and the whole thing branches off in different directions for    */
    /*each player.  Like in a avalanche, some players may get hit by a*/
    /*boulder which calls other actions that send him tumbling down   */
    /*the mountain into different rooms and sustain damage and maybe  */
    /*making him wait to recover, while other players are missed and  */
    /*simply get some scratches.  There are no limits, in fact the    */
    /*avalanche itself could have a possibility of starting another   */
    /*avalanche further down the mountain, making climbing the thing  */
    /*at lower altitudes dangerous when the zone is busy.  You must   */
    /*remember that once an action sequence hits a METHOD PLAYER type,*/
    /*that all remaining actions down that path for each player will  */
    /*be of a singular variety and can no longer use a binary logic   */
    /*action like (have this?) to fire up another sequence to branch  */
    /*again for each player in room.  SIMPLE RIGHT? Mike ducks.       */

    if(IS_SET(action->method,PLAYER)){
	for(player=world[room]->people;player;player=player->next_in_room){
	    if(player){
               if(action->chance){
	           if(number(1,100)>action->chance){ 
		          /*didn't happen*/
	               if(action->notfired_next)
		          fire_raction(room,player,action->notfired_next);
	           }
               }
                       /*Its gonna happen*/
               do_raction(room,player,action_num);
	       if(action->fired_next)
		   fire_raction(room,player,action->fired_next);	
	    }
	}
 	return;
    }
}
void name_string(char *buf1, char *buf2)
{
int x=0;
    strcpy(buf1,buf2);
    while(buf1[x]){
	if(buf1[x]=='%')
	    buf1[x]='$';
	x++;
    }
}

void action_chatter(int room, struct char_data *ch, struct room_actions *ra)
{
char buf[MAX_STRING_LENGTH];

    if(ra->arg1)
	room=ra->arg1;
    if(!ch){
	send_to_room(ra->text1,room);
    }else{
    	name_string(buf,ra->text1);    
	act(buf,TRUE,ch,0,0,TO_ROOM);
	name_string(buf,ra->text2);
	act(buf,TRUE,ch,0,0,TO_CHAR);
    }
}
void action_zone_chatter(int room, struct char_data *ch, struct room_actions *ra) 
{

struct descriptor_data *p=NULL;

    for(p=descriptor_list;p;p=p->next)
    	if(p->character&&p->character->in_room>0
	&&world[p->character->in_room]->zone==ra->arg1)
	    send_to_char(ra->text1,p->character);
    
}

void action_make_exit(int room, struct char_data *ch, struct room_actions *ra) 
{
char t[10];
     
    switch(ra->arg2){
	case 0:
	    sprintf(t,"n %ld", ra->arg3);
	    break;
	case 1:
	    sprintf(t,"e %ld", ra->arg3);
	    break;
	case 2:
	    sprintf(t,"s %ld", ra->arg3);
	    break;
	case 3:
	    sprintf(t,"w %ld", ra->arg3);
	    break;
	case 4:
	    sprintf(t,"u %ld", ra->arg3);
	    break;
	case 5:
	    sprintf(t,"d %ld", ra->arg3);
	    break;
	default:
	    log_hd("##Bad direction in action_make_exit");
	    return;
    }
    do_makedoor(0,t,ra->arg1);
}

void action_delete_exit(int room, struct char_data *ch, struct room_actions *ra) 
{
char t[10];

    switch(ra->arg2){
	case 0:
	    strcpy(t,"n");
	    break;
	case 1:
	    strcpy(t,"e");
	    break;
	case 2:
	    strcpy(t,"s");
	    break;
	case 3:
	    strcpy(t,"w");
	    break;
	case 4:
	    strcpy(t,"u");
	    break;
	case 5:
	    strcpy(t,"d");
	    break;
	default:
	    log_hd("##Bad direction in action_delete_exit");
	    return;
    }
    do_deletedoor(0,t,ra->arg1);
}

void action_set_action_start(int room, struct char_data *ch, struct room_actions *ra)
{
    struct raction_timing *rt=NULL,*prt=NULL,*crt=NULL;
    long l;
    int n;
    if(!ractions[ra->arg1])
	return;
    if(ra->arg3<=ra->arg2)
	ra->arg3=0;
    if(!ra->arg3){ /*no randomness*/
	time(&l);
	l+=ra->arg2;
    }else{
	n=number((int)ra->arg2,(int)ra->arg3);
	time(&l);
	l+=n;
    }
	CREATE(rt, struct raction_timing, 1);
	rt->action=(int)ra->arg1;;
	rt->next=NULL;
	rt->time=l;
	if(!raction_seconds)
	   raction_seconds=rt;
	else{
	   crt=raction_seconds;
	   if(l<=crt->time){
		rt->next=crt;
		raction_seconds=rt;
	   }else{
		prt=crt;
		crt=prt->next;
		while(crt){
		   if(l<=crt->time){
			rt->next=crt;
			prt->next=rt;
			break;
		   }else{
			prt=crt;
			crt=prt->next;
		   }
		}
		if(!crt)
		   prt->next=rt;
	   }
	}
}
void do_raction(int room, struct char_data *ch, int action_num)
{
	switch(ractions[action_num]->type){
	    case 0:
		action_chatter(room, ch, ractions[action_num]);
		break;
	    case 3:
		action_set_action_start(room, ch, ractions[action_num]);
		break;
	    case 20:
		action_make_exit(room, ch, ractions[action_num]);
		break;
	    case 21:
		action_delete_exit(room, ch, ractions[action_num]);
		break;
	    case 22:
		action_zone_chatter(room, ch, ractions[action_num]);
		break;
	    default:
		break;
	}
}

