#include "qwsvdef.h"

#ifndef CLIENTONLY

static sizebuf_t	*writedest;
static client_t		*cldest;
static int majortype;
static int minortype;
static int protocollen;

static qbyte buffer[MAX_QWMSGLEN];
static int bufferlen;
static int nullterms;

static int multicastpos;	//writecoord*3 offset
static int multicasttype;
static int requireextension;
static qboolean ignoreprotocol;

#define svc_setfrags 14
#define svc_updatecolors 17

#define svc_clearviewflags 41	//hexen2.

//these are present in the darkplaces engine.
//I wanna knick their mods.
#define svcdp_skybox	37

#define	svcdp_showlmp			35		// [string] slotname [string] lmpfilename [short] x [short] y
#define	svcdp_hidelmp			36		// [string] slotname


#define	TE_EXPLOSION3_NEH		16 // [vector] origin [coord] red [coord] green [coord] blue	(fixme: ignored)
#define TE_LIGHTNING4_NEH		17 // [string] model [entity] entity [vector] start [vector] end
#define TE_EXPLOSIONSMALL2		20	//	org.

client_t *Write_GetClient(void);
sizebuf_t *QWWriteDest (int dest);
#ifdef NQPROT
sizebuf_t *NQWriteDest (int dest);
#endif

void NPP_SetInfo(client_t *cl, char *key, char *value)
{
	int i;
	Info_SetValueForKey (cl->userinfo, key, value, sizeof(cl->userinfo));
	if (!*Info_ValueForKey (cl->userinfo, "name"))
		cl->name[0] = '\0';
	else // process any changed values
		SV_ExtractFromUserinfo (cl);

	i = cl - svs.clients;
	MSG_WriteByte (&sv.reliable_datagram, svc_setinfo);
	MSG_WriteByte (&sv.reliable_datagram, i);
	MSG_WriteString (&sv.reliable_datagram, key);
	MSG_WriteString (&sv.reliable_datagram, Info_ValueForKey(cl->userinfo, key));
}

void NPP_NQFlush(void)
{
	if (!bufferlen)
		return;


	switch(majortype)
	{
	case svc_cdtrack:
		if (bufferlen!=protocollen)
			Con_Printf("NQFlush: svc_cdtrack wasn't the right length\n");
		else
			bufferlen-=1;
		break;
		//ignore these.
	case svc_print:
	case svcdp_skybox:
	case svc_setfrags:
		bufferlen = 0;
		break;
	case svc_updatename:
		bufferlen = 0;
		NPP_SetInfo(&svs.clients[buffer[1]], "name", buffer+2);
		break;
	case svc_updatecolors:
		bufferlen = 0;
		NPP_SetInfo(&svs.clients[buffer[1]], "bottomcolor", va("%i", buffer[2]&15));
		NPP_SetInfo(&svs.clients[buffer[1]], "topcolor", va("%i", buffer[2]/16));
		break;
	case svc_intermission:
//		if (writedest == &sv.reliable_datagram)
		{
			client_t *cl;
			int i;
			for (i = 0, cl = svs.clients; i < sv.allocated_client_slots; i++, cl++)
			{
				if (cl->state == cs_spawned && ISQWCLIENT(cl))
				{
					if (cl->zquake_extensions & Z_EXT_SERVERTIME)
					{
						ClientReliableCheckBlock(cl, 6);
						ClientReliableWrite_Byte(cl, svc_updatestatlong);
						ClientReliableWrite_Byte(cl, STAT_TIME);
						ClientReliableWrite_Long(cl, (int)(sv.time * 1000));
						cl->nextservertimeupdate = sv.time+10;
					}

					ClientReliableCheckBlock(cl, 16);
					ClientReliableWrite_Byte(cl, svc_intermission);
					ClientReliableWrite_Coord(cl, cl->edict->v->origin[0]);
					ClientReliableWrite_Coord(cl, cl->edict->v->origin[1]);
					ClientReliableWrite_Coord(cl, cl->edict->v->origin[2]+cl->edict->v->view_ofs[2]);
					ClientReliableWrite_Angle(cl, cl->edict->v->angles[0]);
					ClientReliableWrite_Angle(cl, cl->edict->v->angles[1]);
					ClientReliableWrite_Angle(cl, cl->edict->v->angles[2]);

				}
			}
			bufferlen = 0;
			protocollen=0;
			writedest = NULL;
		}
		break;
//	case svc_finale:
//		bufferlen = 0;
//		break;
	case svc_setview:
//		requireextension = PEXT_SETVIEW;

		if (cldest)	//catch it to work with all clients
		{
			cldest->viewent = *(unsigned short*)&buffer[1];
//			bufferlen = 0;
			if (cldest->viewent == (cldest - svs.clients)+1)
				cldest->viewent = 0;	//self is the same as none
		}
		bufferlen = 0;
		break;
	case svcdp_hidelmp:
		requireextension = PEXT_SHOWPIC;
		buffer[0] = svc_hidepic;
		break;
	case svcdp_showlmp:
		requireextension = PEXT_SHOWPIC;
		memmove(buffer+2, buffer+1, bufferlen-1);
		bufferlen++;
		buffer[0] = svc_showpic;
		buffer[1] = 0;	//top left
		//pad the bytes to shorts.
		buffer[bufferlen] = buffer[bufferlen-1];
		buffer[bufferlen-1] = 0;
		buffer[bufferlen+1] = 0;
		bufferlen+=2;
		break;

	case svc_temp_entity:
		switch (buffer[1])
		{
		case NQTE_EXPLOSION2:	//happens with rogue.
			bufferlen -= 2;	//trim the colour
			buffer[1] = TE_EXPLOSION;
			break;
		}
		break;
	}
	if (ignoreprotocol)
	{
		ignoreprotocol=false;
		bufferlen = 0;
	}




	if (cldest)
	{
		if (!requireextension || cldest->fteprotocolextensions & requireextension)
		if (bufferlen && ISQWCLIENT(cldest))
		{
			ClientReliableCheckBlock(cldest, bufferlen);
			ClientReliableWrite_SZ(cldest, buffer, bufferlen);
		}
		cldest = NULL;
	}
	else
	{
		if (multicastpos && (writedest == &sv.datagram || writedest == &sv.multicast))
			writedest = &sv.multicast;
		else
			multicastpos = 0;
		if (bufferlen)
			SZ_Write(writedest, buffer, bufferlen);

		if (multicastpos)
		{
			vec3_t org;
			coorddata cd;

			memcpy(&cd, &buffer[multicastpos+sizeofcoord*0], sizeofcoord);
			org[0] = MSG_FromCoord(cd, sizeofcoord);
			memcpy(&cd, &buffer[multicastpos+sizeofcoord*1], sizeofcoord);
			org[1] = MSG_FromCoord(cd, sizeofcoord);
			memcpy(&cd, &buffer[multicastpos+sizeofcoord*2], sizeofcoord);
			org[2] = MSG_FromCoord(cd, sizeofcoord);

			SV_MulticastProtExt(org, multicasttype, FULLDIMENSIONMASK, requireextension, 0);
		}
		writedest = NULL;
	}
	bufferlen = 0;
	protocollen=0;
	nullterms = 0;
	multicastpos=0;
	requireextension=0;
}
void NPP_NQCheckFlush(void)
{
	if (bufferlen >= protocollen && protocollen && !nullterms)
		NPP_NQFlush();
}

void NPP_NQCheckDest(int dest)
{
	if (dest == MSG_ONE)
	{
		client_t *cl = Write_GetClient();
		if (!cl)
		{
			Con_Printf("Not a client\n");
			return;
		}
		if ((cldest && cldest != cl) || writedest)
		{
			Con_Printf("MSG destination changed in the middle of a packet %i.\n", (int)*buffer);
			NPP_NQFlush();
		}
		cldest = cl;

	}
	else
	{
		sizebuf_t	*ndest = QWWriteDest(dest);
		if (cldest || (writedest && writedest != ndest))
		{
			Con_Printf("NQCheckDest: MSG destination changed in the middle of a packet %i.\n", (int)*buffer);
			NPP_NQFlush();
		}
		writedest = ndest;
	}
}
void NPP_AddData(void *data, int len)
{
	if (bufferlen+len > sizeof(buffer))
		Sys_Error("Preparse buffer was filled\n");
	memcpy(buffer+bufferlen, data, len);
	bufferlen+=len;
}

void NPP_NQWriteByte(int dest, qbyte data)	//replacement write func (nq to qw)
{
	NPP_NQCheckDest(dest);

#ifdef NQPROT
	if (dest == MSG_ONE) 
	{
		client_t *cl = Write_GetClient();
		if (!cl)
		{
			Con_Printf("msg_entity: not a client\n");
			return;
		}
		else
		{
			if (cl->protocol == SCP_BAD) // is a bot
				return;
			else if (!ISQWCLIENT(cl))
			{
				ClientReliableCheckBlock(cl, sizeof(qbyte));
				ClientReliableWrite_Byte(cl, data);
				return;
			}
		}
	}
	else
		MSG_WriteByte (NQWriteDest(dest), data);
#endif

	if (!bufferlen)	//new message section
	{
		switch(data)
		{
		case svcdp_showlmp:
		case svcdp_hidelmp:
			break;
		case svc_temp_entity:
			break;
		case svc_setangle:
			protocollen = sizeof(qbyte)*4;
			break;
		case svc_setview:
			protocollen = sizeof(qbyte)*1 + sizeof(short);
			break;
		case svc_updatename:
			nullterms = 1;
			break;
		case svc_setfrags:
			protocollen = 4;	//or this
			break;
		case svc_updatecolors:
			protocollen = 3;	//or even this
			break;
		case svc_print:
			protocollen = 3;
			nullterms = 1;
			break;
		case svc_cdtrack:
			protocollen = sizeof(qbyte)*3;
			break;
		case svc_killedmonster:
			protocollen = 1;
			break;
		case svc_foundsecret:
			protocollen = 1;
			break;
		case svc_intermission:
			if (progstype == PROG_H2)
				protocollen = 2;
			else
				protocollen = 1;
			break;
		case svc_finale:
			protocollen = 2;
			break;
		case svcdp_skybox:
			protocollen = 2;//it's just a string
			break;
		case svc_updatestat:	//insta fixup
			data = svc_updatestatlong;	//ho hum... let it through (should check size later.)
			protocollen = 5;
			break;
		case svc_stufftext:
		case svc_centerprint:
			nullterms = 1;
			break;
		case svc_clearviewflags:
			protocollen = 2;
			ignoreprotocol = true;
			break;
		case svc_cutscene:
			ignoreprotocol = true;
			break;
		default:
			Con_Printf("NQWriteByte: bad protocol %i\n", (int)data);
			protocollen = sizeof(buffer);
			break;
		}
		majortype = data;
	}
	if (bufferlen == 1 && !protocollen)	//some of them depend on the following bytes for size.
	{
		switch(majortype)
		{
		case svc_temp_entity:
			switch(data)
			{
			case NQTE_BEAM:
				data = TE_LIGHTNING1;	//QW doesn't do te_beam. Replace with lightning1.
						//fallthrough
			case TE_LIGHTNING1:
			case TE_LIGHTNING2:
			case TE_LIGHTNING3:
				multicastpos=4;
				multicasttype=MULTICAST_PHS;
				protocollen = sizeofcoord*6+sizeof(short)+sizeof(qbyte)*2;
				break;
			case TE_GUNSHOT:
				multicastpos=3;
				multicasttype=MULTICAST_PVS;
				//we need to emit annother qbyte here. QuakeWorld has a number of particles.
				//emit it here and we don't need to remember to play with temp_entity later
				NPP_AddData(&data, sizeof(qbyte));
				data = 1;
				protocollen = sizeofcoord*3+sizeof(qbyte)*3;
				break;
			case TE_EXPLOSION:
			case TE_SPIKE:
			case TE_SUPERSPIKE:
				multicastpos=2;
				multicasttype=MULTICAST_PHS_R;
				protocollen = sizeofcoord*3+sizeof(qbyte)*2;
				break;
			case TE_TAREXPLOSION:
			case TE_WIZSPIKE:
			case TE_KNIGHTSPIKE:
			case TE_LAVASPLASH:
			case TE_TELEPORT:
				multicastpos=2;
				multicasttype=MULTICAST_PVS;
				protocollen = sizeofcoord*3+sizeof(qbyte)*2;
				break;
			case TE_EXPLOSION3_NEH:
				protocollen = sizeof(qbyte) + sizeofcoord*6;
				ignoreprotocol = true;
				break;
			case NQTE_EXPLOSION2:
				protocollen = sizeof(qbyte)*4 + sizeofcoord*3;
				multicastpos=2;
				multicasttype=MULTICAST_PHS_R;
				break;
			case TE_EXPLOSIONSMALL2:
				data = TE_EXPLOSION;
				protocollen = sizeof(qbyte)*2 + sizeofcoord*3;
				multicastpos=2;
				multicasttype=MULTICAST_PHS;
				break;
			case TE_RAILTRAIL:
				protocollen = sizeofcoord*6+sizeof(qbyte)*1;
				multicastpos=2;
				multicasttype=MULTICAST_PHS;
				break;
			case TE_STREAM_CHAIN:
			case TE_STREAM_SUNSTAFF1:
			case TE_STREAM_SUNSTAFF2:
			case TE_STREAM_LIGHTNING:
			case TE_STREAM_ICECHUNKS:
			case TE_STREAM_GAZE:
			case TE_STREAM_FAMINE:
				protocollen = sizeofcoord*6+sizeof(short)+sizeof(qbyte)*(2+2);
				multicastpos = 8;
				multicasttype=MULTICAST_PHS;
				break;
			case TE_STREAM_COLORBEAM:
				protocollen = sizeofcoord*6+sizeof(short)+sizeof(qbyte)*(3+2);
				multicastpos = 8;
				multicasttype=MULTICAST_PHS;
				break;

			case DPTE_FLAMEJET:	//TE_FLAMEJET
				protocollen = sizeofcoord*6 +sizeof(qbyte)*3;
				multicastpos = 2;
				multicasttype=MULTICAST_PVS;
				break;

			case DPTE_TEI_G3:
				protocollen = sizeofcoord*9+sizeof(qbyte)*2;
				multicastpos = 2;
				multicasttype=MULTICAST_PHS;
				break;

			case DPTE_SMOKE:
				protocollen = sizeofcoord*6+sizeof(qbyte)*3;
				multicastpos = 2;
				multicasttype=MULTICAST_PHS;
				break;

			case DPTE_TEI_BIGEXPLOSION:
				protocollen = sizeofcoord*3+sizeof(qbyte)*2;
				multicastpos = 2;
				multicasttype=MULTICAST_PHS;
				break;

			case DPTE_TEI_PLASMAHIT:
				protocollen = sizeofcoord*6+sizeof(qbyte)*3;
				multicastpos = 2;
				multicasttype=MULTICAST_PHS;
				break;

			default:
				protocollen = sizeof(buffer);
				Con_Printf("NQWriteByte: bad tempentity\n");
				PR_StackTrace(svprogfuncs);
				break;
			}
			break;
		case svc_updatename:
		case svc_stufftext:
		case svc_centerprint:
			break;
		default:
			Con_Printf("NQWriteByte: Non-Implemented svc\n");
			protocollen = sizeof(buffer);
			break;
		}
	}
	if (!protocollen)	//these protocols take strings, and are thus dynamically sized.
	{
		switch(majortype)
		{
		case svc_updatename:
			if (bufferlen < 2)
				break;	//don't truncate the name if the mod is sending the slot number
		case svcdp_hidelmp:
		case svc_stufftext:
		case svc_centerprint:
			if (!data)
				protocollen = bufferlen;
			break;
		case svcdp_showlmp:			// [string] slotname [string] lmpfilename [byte] x [byte] y
									//note: nehara uses bytes!
									//and the rest of dp uses shorts. how nasty is that?
			if (!data)
			{	//second string, plus 2 bytes.
				int i;
				for (i = 0; i < bufferlen; i++)
					if (!buffer[i])
						protocollen = bufferlen+2;
			}
			break;
		}
	}

	NPP_AddData(&data, sizeof(qbyte));
	if (!data && bufferlen>=protocollen)
		if (nullterms)
			nullterms--;
	NPP_NQCheckFlush();
}

void NPP_NQWriteChar(int dest, char data)	//replacement write func (nq to qw)
{
	NPP_NQWriteByte(dest, (qbyte)data);
	return;
	/*
	NPP_NQCheckDest(dest);
	if (!bufferlen)
	{
		NPP_NQWriteByte(dest, (qbyte)data);
		return;
	}

#ifdef NQPROT
	if (dest == MSG_ONE) {
		client_t *cl = Write_GetClient();
		if (cl && cl->nqprot)
		{
			ClientReliableCheckBlock(cl, sizeof(char));
			ClientReliableWrite_Char(cl, data);
		}
	} else
		MSG_WriteChar (NQWriteDest(dest), data);
#endif

	NPP_AddData(&data, sizeof(char));
	NPP_NQCheckFlush();*/
}

void NPP_NQWriteShort(int dest, short data)	//replacement write func (nq to qw)
{
	union {
		qbyte b[2];
		short s;
	} u;
	u.s = LittleShort(data);
	NPP_NQWriteByte(dest, u.b[0]);
	NPP_NQWriteByte(dest, u.b[1]);
}

void NPP_NQWriteLong(int dest, long data)	//replacement write func (nq to qw)
{
	union {
		qbyte b[4];
		int l;
	} u;
	u.l = LittleLong(data);
	NPP_NQWriteByte(dest, u.b[0]);
	NPP_NQWriteByte(dest, u.b[1]);
	NPP_NQWriteByte(dest, u.b[2]);
	NPP_NQWriteByte(dest, u.b[3]);
}
void NPP_NQWriteAngle(int dest, float in)	//replacement write func (nq to qw)
{
	char data = (int)(in*256/360) & 255;
	NPP_NQCheckDest(dest);
	if (!bufferlen)
		Con_Printf("NQWriteAngle: Messages should start with WriteByte\n");

#ifdef NQPROT
	if (dest == MSG_ONE)
	{
		client_t *cl = Write_GetClient();
		if (!cl)
		{
			Con_Printf("msg_entity: not a client\n");
			return;
		}
		else
		{
			if (cl->protocol == SCP_BAD)
				return;
			else if (!ISQWCLIENT(cl))
			{
				ClientReliableCheckBlock(cl, sizeof(char));
				ClientReliableWrite_Angle(cl, in);
				return;
			}
		}
	}
	else
		MSG_WriteAngle (NQWriteDest(dest), in);
#endif

	NPP_AddData(&data, sizeof(char));
	NPP_NQCheckFlush();
}
void NPP_NQWriteCoord(int dest, float in)	//replacement write func (nq to qw)
{
	short datas = (int)(in*8);
	float dataf = in;
	NPP_NQCheckDest(dest);
	if (!bufferlen)
		Con_Printf("NQWriteCoord: Messages should start with WriteByte\n");

#ifdef NQPROT
	if (dest == MSG_ONE)
	{
		client_t *cl = Write_GetClient();
		if (!cl)
		{
			Con_Printf("msg_entity: not a client\n");
			return;
		}
		else
		{
			if (cl->protocol == SCP_BAD)
				return;
			else if (!ISQWCLIENT(cl))
			{
				ClientReliableCheckBlock(cl, sizeof(float));
				ClientReliableWrite_Coord(cl, in);
				return;
			}
		}
	}
	else
		MSG_WriteCoord (NQWriteDest(dest), in);
#endif

	if (sizeofcoord==4)
	{
		dataf = LittleFloat(dataf);
		NPP_AddData(&dataf, sizeof(float));
	}
	else
	{
		datas = LittleShort(datas);
		NPP_AddData(&datas, sizeof(short));
	}
	NPP_NQCheckFlush();
}
void NPP_NQWriteString(int dest, char *data)	//replacement write func (nq to qw)
{
	NPP_NQCheckDest(dest);
	if (!bufferlen)
	{
		Con_Printf("NQWriteString: Messages should start with WriteByte\n");
	}

#ifdef NQPROT
	if (dest == MSG_ONE)
	{
		client_t *cl = Write_GetClient();
		if (!cl)
		{
			Con_Printf("msg_entity: not a client\n");
			return;
		}
		else
		{
			if (cl->protocol == SCP_BAD)
				return;
			else if (!ISQWCLIENT(cl))
			{
				ClientReliableCheckBlock(cl, strlen(data)+1);
				ClientReliableWrite_String(cl, data);
				return;
			}
		}
	}
	else
		MSG_WriteString (NQWriteDest(dest), data);
#endif

	NPP_AddData(data, strlen(data)+1);

	if (!protocollen)	//these protocols take strings, and are thus dynamically sized.
	{
		switch(majortype)
		{
		case svc_updatename:
		case svc_stufftext:
		case svc_centerprint:
			protocollen = bufferlen;
			break;
		}
	}

	if (nullterms)
		nullterms--;
	NPP_NQCheckFlush();
}
void NPP_NQWriteEntity(int dest, short data)	//replacement write func (nq to qw)
{
	NPP_NQCheckDest(dest);
	if (!bufferlen)
		Con_Printf("NQWriteEntity: Messages should start with WriteByte\n");

	if (majortype == svc_temp_entity && data >= 0 && data <= sv.allocated_client_slots)
		if (svs.clients[data-1].viewent)
			data = svs.clients[data-1].viewent;

#ifdef NQPROT
	if (dest == MSG_ONE)
	{
		client_t *cl = Write_GetClient();
		if (!cl)
		{
			Con_Printf("msg_entity: not a client\n");
			return;
		}
		else
		{
			if (cl->protocol == SCP_BAD)
				return;
			else if (!ISQWCLIENT(cl))
			{
				ClientReliableCheckBlock(cl, sizeof(short));
				ClientReliableWrite_Short(cl, data);
				return;
			}
		}
	}
	else
		MSG_WriteShort (NQWriteDest(dest), data);
#endif

	NPP_AddData(&data, sizeof(short));
	NPP_NQCheckFlush();
}
















#ifdef NQPROT


//qw to nq translation is only useful if we allow nq clients to connect.

void NPP_QWFlush(void)
{
	qbyte b;
	if (!bufferlen)
		return;

	switch(majortype)
	{
	case svc_updatename:	//not a standard feature, but hey, if a progs wants bots.
		bufferlen = 0;
		NPP_SetInfo(&svs.clients[buffer[1]], "name", buffer+2);
		break;
	case svc_updatecolors:
		bufferlen = 0;
		NPP_SetInfo(&svs.clients[buffer[1]], "bottomcolor", va("%i", buffer[2]&15));
		NPP_SetInfo(&svs.clients[buffer[1]], "topcolor", va("%i", buffer[2]/16));
		break;
	case svc_cdtrack:
		if (bufferlen!=protocollen)
			Con_Printf("QWFlush: svc_cdtrack wasn't the right length\n");
		else
		{
			b = 0;
			NPP_AddData(&b, sizeof(qbyte));
		}
		break;
		//ignore these.
	case svc_intermission:
//		if (writedest == &sv.reliable_datagram)
		{
			client_t *cl;
			int i;
			for (i = 0, cl = svs.clients; i < sv.allocated_client_slots; i++, cl++)
			{
				if (cl->state == cs_spawned && !ISQWCLIENT(cl))
				{
					vec3_t org, ang;

					if (cl->zquake_extensions & Z_EXT_SERVERTIME)
					{
						ClientReliableCheckBlock(cl, 6);
						ClientReliableWrite_Byte(cl, svc_updatestatlong);
						ClientReliableWrite_Byte(cl, STAT_TIME);
						ClientReliableWrite_Long(cl, (int)(sv.time * 1000));
						cl->nextservertimeupdate = sv.time+10;
					}

					ClientReliableCheckBlock(cl, 1);
					ClientReliableWrite_Byte(cl, svc_intermission);

					org[0] = (*(short*)&buffer[1])/8.0f;
					org[1] = (*(short*)&buffer[1+2])/8.0f;
					org[2] = (*(short*)&buffer[1+4])/8.0f;

					ang[0] = (*(qbyte*)&buffer[7])*360.0/255;
					ang[1] = (*(qbyte*)&buffer[7+1])*360.0/255;
					ang[2] = (*(qbyte*)&buffer[7+2])*360.0/255;

					//move nq players to origin + angle
					VectorCopy(org, cl->edict->v->origin);
					VectorCopy(ang, cl->edict->v->angles);
					cl->edict->v->angles[0]*=-1;
				}
			}
		}
		bufferlen = 0;
		protocollen=0;
		writedest = NULL;
//	case svc_finale:
//		bufferlen = 0;
		break;
	case svc_setview:
		requireextension = PEXT_SETVIEW;
//		bufferlen = 0;
		break;
	case svc_muzzleflash:
		//we need to make a fake muzzleflash position.
		multicastpos = 4;
		{
			short data;
			float org[3];
			edict_t *ent = EDICT_NUM(svprogfuncs, LittleShort((*(short*)&buffer[1])));
			VectorCopy(ent->v->origin, org);

			data = LittleShort((short)(org[0]*8));
			NPP_AddData(&data, sizeof(short));
			data = LittleShort((short)(org[1]*8));
			NPP_AddData(&data, sizeof(short));
			data = LittleShort((short)(org[2]*8));
			NPP_AddData(&data, sizeof(short));

		}
		bufferlen = 0;	//can't send this to nq. :(
		break;
	case svc_smallkick:
	case svc_bigkick:
		bufferlen = 0;
		break;
	case svc_updateuserinfo:
		if (buffer[6])
		{
			unsigned int j = buffer[1];
			if (j < MAX_CLIENTS)
			{
				Q_strncpyz(svs.clients[j].userinfo, (buffer+6), sizeof(svs.clients[j].userinfo));
				if (*Info_ValueForKey(svs.clients[j].userinfo, "name"))
					SV_ExtractFromUserinfo(&svs.clients[j]);
				else
					*svs.clients[j].name = '\0';
			}
		}
		else
		{
			unsigned int j = buffer[1];
			if (j < MAX_CLIENTS)
			{
				*svs.clients[j].name = '\0';
				*svs.clients[j].userinfo = '\0';
			}
		}

		break;
	case svc_temp_entity:
		switch(minortype)
		{
		case TE_LIGHTNINGBLOOD:
		case TE_BLOOD:		//needs to be converted to a particle
			{
				vec3_t org;
				qbyte count;
				qbyte colour;
				char dir[3];
				short s;
				int v;
				int i;
				qbyte svc;
				svc = svc_particle;
				org[0] = (*(short*)&buffer[multicastpos])/8.0f;
				org[1] = (*(short*)&buffer[multicastpos+2])/8.0f;
				org[2] = (*(short*)&buffer[multicastpos+4])/8.0f;
				count = buffer[2]*20;
				if (minortype == TE_LIGHTNINGBLOOD)
					colour = 225;
				else
					colour = 73;

				for (i=0 ; i<3 ; i++)
				{
					v = 0*16;
					if (v > 127)
						v = 127;
					else if (v < -128)
						v = -128;
					dir[i] = v;
				}

				bufferlen = 0;		//restart
				protocollen = 1000;

				multicastpos = 1;

				NPP_AddData(&svc, sizeof(qbyte));
				for (i = 0; i < 3; i++)
				{
					if (sizeofcoord == 4)
						NPP_AddData(&org[i], sizeof(float));
					else
					{
						s = org[i]*8;
						NPP_AddData(&s, sizeof(short));
					}
				}
				NPP_AddData(&dir[0], sizeof(char));
				NPP_AddData(&dir[1], sizeof(char));
				NPP_AddData(&dir[2], sizeof(char));
				NPP_AddData(&count, sizeof(qbyte));
				NPP_AddData(&colour, sizeof(qbyte));
			}
			break;
		case TE_GUNSHOT:	//needs byte 3 removed
			if (bufferlen >= 3)
			{
				memmove(buffer+2, buffer+3, bufferlen-3);
				bufferlen--;
			}
			break;
		}
	}
	if (ignoreprotocol)
	{
		ignoreprotocol=false;
		bufferlen = 0;
	}




	if (cldest)
	{
		if (!requireextension || cldest->fteprotocolextensions & requireextension)
		if (bufferlen && !ISQWCLIENT(cldest))
		{
			ClientReliableCheckBlock(cldest, bufferlen);
			ClientReliableWrite_SZ(cldest, buffer, bufferlen);
		}
		cldest = NULL;
	}
	else
	{
		if (multicastpos && (writedest == &sv.nqdatagram || writedest == &sv.nqmulticast))
			writedest = &sv.nqmulticast;
		else
			multicastpos = 0;
		if (bufferlen)
			SZ_Write(writedest, buffer, bufferlen);

		if (multicastpos)
		{
			int qwsize;
			vec3_t org;
			coorddata cd;

			memcpy(&cd, &buffer[multicastpos+sizeofcoord*0], sizeofcoord);
			org[0] = MSG_FromCoord(cd, sizeofcoord);
			memcpy(&cd, &buffer[multicastpos+sizeofcoord*1], sizeofcoord);
			org[1] = MSG_FromCoord(cd, sizeofcoord);
			memcpy(&cd, &buffer[multicastpos+sizeofcoord*2], sizeofcoord);
			org[2] = MSG_FromCoord(cd, sizeofcoord);

			qwsize = sv.multicast.cursize;
			sv.multicast.cursize = 0;
			SV_MulticastProtExt(org, multicasttype, FULLDIMENSIONMASK, requireextension, 0);
			sv.multicast.cursize = qwsize;
		}
		writedest = NULL;
	}
	bufferlen = 0;
	nullterms=0;
	protocollen=0;
	multicastpos=0;
	requireextension=0;
}
void NPP_QWCheckFlush(void)
{
	if (bufferlen >= protocollen && protocollen && !nullterms)
		NPP_QWFlush();
}

void NPP_QWCheckDest(int dest)
{
	if (dest == MSG_ONE)
	{
/*		client_t *cl = Write_GetClient();
		if (!cl)
		{
			Con_Printf("Not a client\n");
			return;
		}
		if ((cldest && cldest != cl) || writedest)
		{
			Con_Printf("MSG destination changed in the middle of a packet %i.\n", (int)*buffer);
			NPP_QWFlush();
		}
		cldest = cl;
*/
		cldest = NULL;
	}
	else
	{
		sizebuf_t	*ndest = NQWriteDest(dest);
		if (cldest || (writedest && writedest != ndest))
		{
			Con_Printf("QWCheckDest: MSG destination changed in the middle of a packet %i.\n", (int)*buffer);
			NPP_QWFlush();
		}
		writedest = ndest;
	}
}




void NPP_QWWriteByte(int dest, qbyte data)	//replacement write func (nq to qw)
{
	NPP_QWCheckDest(dest);

#ifdef NQPROT
	if (dest == MSG_ONE)
	{
		client_t *cl = Write_GetClient();
		if (!cl)
		{
			Con_Printf("msg_entity: not a client\n");
			return;
		}
		else
		{
			if (cl->protocol == SCP_BAD) // is a bot
				return;
			else if (ISQWCLIENT(cl))
			{
				ClientReliableCheckBlock(cl, sizeof(qbyte));
				ClientReliableWrite_Byte(cl, data);
				return;
			}
		}
	} 
	else
		MSG_WriteByte (QWWriteDest(dest), data);
#endif
	if (!bufferlen)	//new message section
	{
		switch(data)
		{
		case svc_temp_entity:
			break;
		case svc_setangle:
			protocollen = sizeof(qbyte)*4;
			break;
		case svc_setview:
			protocollen = sizeof(qbyte)*1 + sizeof(short);
			break;
		case svc_cdtrack:
			protocollen = sizeof(qbyte)*2;
			break;
		case svc_killedmonster:
			protocollen = 1;
			break;
		case svc_foundsecret:
			protocollen = 1;
			break;
		case svc_intermission:
			protocollen = 10;
			break;
		case svc_finale:
			protocollen = 2;
			break;
		case svc_updatepl:
		case svc_muzzleflash:
			protocollen = 3;
			break;
		case svc_smallkick:
		case svc_bigkick:
			protocollen = 1;
			break;
		case svc_print:
			protocollen = 2;
			nullterms=1;
			break;
		case svc_setinfo:
			protocollen = 2;
			nullterms = 2;
			break;
		case svc_centerprint:
		case svc_stufftext:
			protocollen = 1;
			nullterms=1;
			break;
		case svc_updatestat:
			protocollen = 3;
			break;
		case svc_updateping:
		case svc_updatefrags:
			protocollen = 4;
			break;
		case svc_updateentertime:
			protocollen = 6;
			break;
		case svc_updateuserinfo:
			protocollen = 6;
			nullterms = 1;
			break;
		case svc_updatestatlong:
			protocollen = 6;
			break;
		case svc_setpause:
			protocollen = 2;
			break;
		default:
			Con_Printf("QWWriteByte: bad protocol %i\n", (int)data);
			protocollen = sizeof(buffer);
			break;
		}
		majortype = data;
	}
	if (bufferlen == 1 && !protocollen)	//some of them depend on the following bytes for size.
	{
		switch(majortype)
		{
		case svc_temp_entity:
			minortype = data;
			switch(data)
			{
			case TE_LIGHTNING1:
			case TE_LIGHTNING2:
			case TE_LIGHTNING3:
				multicastpos=4;
				multicasttype=MULTICAST_PHS;
				protocollen = sizeofcoord*6+sizeof(short)+sizeof(qbyte)*2;
				break;
			case TE_BLOOD:		//needs to be converted to a particle
			case TE_GUNSHOT:	//needs qbyte 2 removed
				multicastpos=3;
				multicasttype=MULTICAST_PVS;
				protocollen = sizeofcoord*3+sizeof(qbyte)*3;
				break;
			case TE_LIGHTNINGBLOOD:
			case TE_EXPLOSION:
			case TE_SPIKE:
			case TE_SUPERSPIKE:
				multicastpos=2;
				multicasttype=MULTICAST_PHS_R;
				protocollen = sizeofcoord*3+sizeof(qbyte)*2;
				break;
			case TE_TAREXPLOSION:
			case TE_WIZSPIKE:
			case TE_KNIGHTSPIKE:
			case TE_LAVASPLASH:
			case TE_TELEPORT:
				multicastpos=2;
				multicasttype=MULTICAST_PVS;
				protocollen = sizeofcoord*3+sizeof(qbyte)*2;
				break;
			case TE_RAILTRAIL:
				multicastpos=1;
				multicasttype=MULTICAST_PVS;
				protocollen = sizeofcoord*3+sizeof(qbyte)*1;
				break;
			default:
				protocollen = sizeof(buffer);
				Con_Printf("QWWriteByte: bad tempentity - %i\n", data);
				break;
			}
			break;
		default:
			Con_Printf("QWWriteByte: Non-Implemented svc\n");
			protocollen = sizeof(buffer);
			break;
		}
	}

	NPP_AddData(&data, sizeof(qbyte));
	if (!data && bufferlen>=protocollen)
		if (nullterms)
			nullterms--;
	NPP_QWCheckFlush();
}

void NPP_QWWriteChar(int dest, char data)	//replacement write func (nq to qw)
{
	NPP_QWWriteByte(dest, (qbyte)data);
}

void NPP_QWWriteShort(int dest, short data)	//replacement write func (nq to qw)
{
	union {
		qbyte b[2];
		short s;
	} u;
	u.s = LittleShort(data);
	NPP_QWWriteByte(dest, u.b[0]);
	NPP_QWWriteByte(dest, u.b[1]);
}

void NPP_QWWriteFloat(int dest, float data)	//replacement write func (nq to qw)
{
	union {
		qbyte b[4];
		float f;
	} u;
	u.f = LittleFloat(data);
	NPP_QWWriteByte(dest, u.b[0]);
	NPP_QWWriteByte(dest, u.b[1]);
	NPP_QWWriteByte(dest, u.b[2]);
	NPP_QWWriteByte(dest, u.b[3]);
}

void NPP_QWWriteLong(int dest, long data)	//replacement write func (nq to qw)
{
	union {
		qbyte b[4];
		int l;
	} u;
	u.l = LittleLong(data);
	NPP_QWWriteByte(dest, u.b[0]);
	NPP_QWWriteByte(dest, u.b[1]);
	NPP_QWWriteByte(dest, u.b[2]);
	NPP_QWWriteByte(dest, u.b[3]);
}
void NPP_QWWriteAngle(int dest, float in)	//replacement write func (nq to qw)
{
	if (sizeofangle==1)
	{
		char data = (int)(in*256/360) & 255;
		NPP_QWWriteChar(dest, data);
	}
	else
	{
		short data = (int)(in*0xffff/360) & 0xffff;
		NPP_QWWriteShort(dest, data);
	}
}
void NPP_QWWriteCoord(int dest, float in)	//replacement write func (nq to qw)
{
	if (sizeofcoord==4)
	{
		NPP_QWWriteFloat(dest, in);
	}
	else
	{
		short datas = (int)(in*8);
		NPP_QWWriteShort(dest, datas);
	}
}
void NPP_QWWriteString(int dest, char *data)	//replacement write func (nq to qw)
{
#if 0
	//the slow but guarenteed routine
	while(*data)
		NPP_QWWriteByte(dest, *data++);
	NPP_QWWriteByte(dest, 0);	//and the null terminator
#else
	//the fast-track, less reliable routine


	NPP_QWCheckDest(dest);

#ifdef NQPROT
	if (dest == MSG_ONE)
	{
		client_t *cl = Write_GetClient();
		if (!cl)
		{
			Con_Printf("msg_entity: not a client\n");
			return;
		}
		else
		{
			if (cl->protocol == SCP_BAD)
				return;
			else if (ISQWCLIENT(cl))
			{
				ClientReliableCheckBlock(cl, strlen(data)+1);
				ClientReliableWrite_String(cl, data);
				return;
			}
		}
	}
	else
		MSG_WriteString (QWWriteDest(dest), data);
#endif

	if (!bufferlen)
		Con_Printf("QWWriteString: Messages should start with WriteByte (last was %i)\n", majortype);

	NPP_AddData(data, strlen(data)+1);
	if (nullterms)
		nullterms--;
	NPP_QWCheckFlush();
#endif
}
void NPP_QWWriteEntity(int dest, short data)	//replacement write func (nq to qw)
{
	union {
		qbyte b[2];
		short s;
	} u;
	u.s = LittleShort(data);
	NPP_QWWriteByte(dest, u.b[0]);
	NPP_QWWriteByte(dest, u.b[1]);
}



#endif

























#define DF_ORIGIN	1
#define DF_ANGLES	(1<<3)
#define DF_EFFECTS	(1<<6)
#define DF_SKINNUM	(1<<7)
#define DF_DEAD		(1<<8)
#define DF_GIB		(1<<9)
#define DF_WEAPONFRAME (1<<10)
#define DF_MODEL	(1<<11)

#define	PF_MSEC			(1<<0)
#define	PF_COMMAND		(1<<1)
#define	PF_VELOCITY1	(1<<2)
#define	PF_VELOCITY2	(1<<3)
#define	PF_VELOCITY3	(1<<4)
#define	PF_MODEL		(1<<5)
#define	PF_SKINNUM		(1<<6)
#define	PF_EFFECTS		(1<<7)
#define	PF_WEAPONFRAME	(1<<8)		// only sent for view player
#define	PF_DEAD			(1<<9)		// don't block movement any more
#define	PF_GIB			(1<<10)		// offset the view height differently




int sv_demo_spikeindex;




void NPP_MVDFlush(void)
{
	if (!bufferlen)
		return;

	switch(majortype)
	{
	case svc_spawnbaseline:
		SV_FlushDemoSignon();
		cldest = NULL;
		writedest = &sv.demosignon;

		if (1)
		{
			int entnum, i;
			mvdentity_state_t *ent;

			if (!sv.demobaselines)
			{
				sv.demobaselines = (mvdentity_state_t*)BZ_Malloc(sizeof(mvdentity_state_t)*MAX_EDICTS);
				sv.demostatevalid = true;
			}
			entnum = buffer[1] + (buffer[2]<<8);
//			if (entnum < MAX_CLIENTS)
//				break;
			ent = &sv.demobaselines[entnum];

			ent->modelindex = buffer[3];
			ent->frame = buffer[4];
			ent->colormap = buffer[5];
			ent->skinnum = buffer[6];

			for (i=0 ; i<3 ; i++)
			{
				ent->origin[i] = (short)(buffer[7+i*3] + (buffer[8+i*3]<<8))/8.0f;
				ent->angles[i] = buffer[9+i*3];
			}
		}
		break;
	case svc_spawnstatic:
		SV_FlushDemoSignon();
		cldest = NULL;
		writedest = &sv.demosignon;
		break;
	case svc_modellist:
		ignoreprotocol=true;
		{
			int i;
			int rpos, s;
			i = buffer[1];
			rpos = 2;
			while (1)
			{
				i++;
				s = rpos;
				while(buffer[rpos])
					rpos++;

				if (rpos == s)	//end
					break;

				strcpy(sv.demmodel_precache[i], buffer+s);
				if (!strcmp(sv.demmodel_precache[i], "progs/spike.mdl"))
					sv_demo_spikeindex = i;
				rpos++;
			}
		}
		break;
	case svc_soundlist:
		ignoreprotocol=true;
		{
			int i;
			int rpos, s;
			i = buffer[1];
			rpos = 2;
			while (1)
			{
				i++;
				s = rpos;
				while(buffer[rpos])
					rpos++;

				if (rpos == s)	//end
					break;

				strcpy(sv.demsound_precache[i], buffer+s);
				rpos++;
			}
		}
		break;
	case svc_serverdata:
		{
			int i;
			sv_demo_spikeindex = 0;	//new map, new precaches.

			i = 9;
			strcpy(sv.demgamedir, buffer+i);
			for(;i < bufferlen && buffer[i];i++)
				;
			i++;
			i+=4;
			Q_strncpyz(sv.demfullmapname, buffer+i, sizeof(sv.demfullmapname));
			for(;i < bufferlen && buffer[i];i++)
				;
			i+=4*10;
		}
		ignoreprotocol=true;
		break;
	case svc_lightstyle:
		sv.demolightstyles[buffer[1]] = Hunk_Alloc(strlen(buffer+2)+1);
		strcpy(sv.demolightstyles[buffer[1]], buffer+2);
		break;

	case svc_updatestat:
	case svc_updatestatlong:	//make sure we update the running players stats properly.
		{
			int v, s;
			if (majortype == svc_updatestat)
				v = buffer[2];
			else
				v = buffer[2] | (buffer[3]<<8) | (buffer[4]<<16) | (buffer[5]<<24);
			s = buffer[1];

			if (sv.lastto < 32)	//dem_multicast could be used at the wrong time...
				sv.recordedplayer[sv.lastto].stats[s] = v;

			ignoreprotocol=true;
		}
		break;
	case svc_packetentities:
	case svc_deltapacketentities:	//read the delta in to the array.
		ignoreprotocol=true;		//a bug exists in that the delta MUST have been reliably recorded.
		{
			int i;
			int entnum;
			mvdentity_state_t *ents;
			unsigned short s;
			if (!sv.demostate)
			{
				sv.demostate = BZ_Malloc(sizeof(mvdentity_state_t)*MAX_EDICTS);
				sv.demostatevalid = true;
			}
			i = majortype-svc_packetentities+1;
			while (1)
			{
				s = buffer[i] + buffer[i+1]*256;
				i+=2;
				if (!s)
				{
					break;
				}
				else
				{
					entnum = s&511;
					s &= ~511;

					if (entnum > sv.demomaxents)
						sv.demomaxents = entnum;

					ents = &sv.demostate[entnum];
					if (s & U_REMOVE)
					{	//this entity went from the last packet
						ents->modelindex = 0;
						ents->effects = 0;
						continue;
					}

					if (!ents->modelindex && !ents->effects && sv.demobaselines)
					{	//new entity, reset to baseline
						memcpy(ents, &sv.demobaselines[entnum], sizeof(mvdentity_state_t));
					}

					if (s & U_MOREBITS)
					{
						s |= buffer[i];
						i++;
					}

					if (s & U_MODEL)
					{
						ents->modelindex = buffer[i];
						i++;
					}

					if (s & U_FRAME)
					{
						ents->frame = buffer[i];
						i++;
					}

					if (s & U_COLORMAP)
					{
						ents->colormap = buffer[i];
						i++;
					}

					if (s & U_SKIN)
					{
						ents->skinnum = buffer[i];
						i++;
					}

					if (s & U_EFFECTS)
					{
						ents->effects = buffer[i];
						i++;
					}

					if (s & U_ORIGIN1)
					{
						ents->origin[0] = (short)(buffer[i]+buffer[i+1]*256) /8.0f;
						i+=2;
					}

					if (s & U_ANGLE1)
					{
						ents->angles[0] = (unsigned char)(buffer[i]);//	* (360.0/256);
						i++;
					}

					if (s & U_ORIGIN2)
					{
						ents->origin[1] = (short)(buffer[i]+buffer[i+1]*256) /8.0f;
						i+=2;
					}

					if (s & U_ANGLE2)
					{
						ents->angles[1] = (unsigned char)(buffer[i]);//	* (360.0/256);
						i++;
					}

					if (s & U_ORIGIN3)
					{
						ents->origin[2] = (short)(buffer[i]+buffer[i+1]*256) /8.0f;
						i+=2;
					}

					if (s & U_ANGLE3)
					{
						ents->angles[2] = (unsigned char)(buffer[i]);//	* (360.0/256);
						i++;
					}
				}
			}
		}
		break;
	case svc_playerinfo:
		ignoreprotocol=true;
		{
			int i, j;
			unsigned short flags;
			mvdentity_state_t *ents;
			int playernum;
			vec3_t oldang;

			if (!sv.demostate)
				sv.demostate = BZ_Malloc(sizeof(entity_state_t)*MAX_EDICTS);

			sv.demostatevalid = true;

			flags = buffer[2] + buffer[3]*256;

			playernum = buffer[1];
			ents = &sv.demostate[playernum+1];
			ents->frame = buffer[4];

//			ents->colormap=playernum+1;

			VectorCopy(ents->origin, sv.recordedplayer[playernum].oldorg);
			VectorCopy(ents->angles, sv.recordedplayer[playernum].oldang);

			i = 5;
			for (j=0 ; j<3 ; j++)
				if (flags & (DF_ORIGIN << j))
				{
					ents->origin[j] = (signed short)(buffer[i] + (buffer[i+1]<<8))/8.0f;
					i+=2;
				}

			VectorCopy(ents->angles, oldang);
			for (j=0 ; j<3 ; j++)
				if (flags & (DF_ANGLES << j))
				{
					//FIXME: angle truncation here.
					ents->angles[j] = (char)((int)(buffer[i] + (buffer[i+1]<<8))/256.0f);
					i+=2;
				}

			if (flags & (DF_ANGLES << 0))	//'stupid quake bug' I believe is the correct quote...
				ents->angles[0] = ents->angles[0]*-1/3.0f; //also scale pitch down as well as invert

			if (flags & DF_MODEL)
			{
				ents->modelindex = buffer[i];
				i+=1;
			}
			if (flags & DF_SKINNUM)
			{
				ents->skinnum = buffer[i];
				i+=1;
			}
			if (flags & DF_EFFECTS)
			{
				ents->effects = buffer[i];
				i+=1;
			}
			if (flags & DF_WEAPONFRAME)
			{	//mvds are deltas remember, this is really the only place where that fact is all that important.
				sv.recordedplayer[playernum].weaponframe = buffer[i];
				i+=1;
			}

			sv.recordedplayer[playernum].updatetime = realtime;

			ignoreprotocol=true;
		}
		break;

	case svc_nails:
	case svc_nails2:
		sv.numdemospikes = buffer[1];
		{
			qboolean hasid = (majortype==svc_nails2);
			char *bits;
			int i;
			bits = buffer+2;
			for (i = 0; i < sv.numdemospikes; i++)
			{
				if (hasid)
				{
					sv.demospikes[i].id = *bits;
					bits++;
				}
				else
					sv.demospikes[i].id = 0;
				sv.demospikes[i].modelindex = sv_demo_spikeindex;
				sv.demospikes[i].org[0] = ( ( bits[0] + ((bits[1]&15)<<8) ) <<1) - 4096;
				sv.demospikes[i].org[1] = ( ( (bits[1]>>4) + (bits[2]<<4) ) <<1) - 4096;
				sv.demospikes[i].org[2] = ( ( bits[3] + ((bits[4]&15)<<8) ) <<1) - 4096;
				sv.demospikes[i].pitch = (bits[4]>>4);
				sv.demospikes[i].yaw = bits[5];

				bits+=6;
			}
		}
		ignoreprotocol=true;
		break;

	case svc_stufftext:
		ignoreprotocol = true;
		Cmd_TokenizeString(buffer+1, false, false);
		if (!stricmp(Cmd_Argv(0), "fullserverinfo"))
		{
			Q_strncpyz(sv.demoinfo, Cmd_Argv(1), sizeof(sv.demoinfo));
			break;
		}
		break;
	case svc_updateping:
		{
			int j;
			j = buffer[1];
			sv.recordedplayer[j].ping = buffer[2] | (buffer[3]<<8);
		}
		break;
	case svc_updatepl:
		{
			int j;
			j = buffer[1];
			sv.recordedplayer[j].pl = buffer[2] | (buffer[3]<<8);
		}
		break;
	case svc_updatefrags:
		{
			int j;
			j = buffer[1];
			sv.recordedplayer[j].frags = buffer[2] | (buffer[3]<<8);
		}
		break;
	case svc_setinfo:
//		ignoreprotocol = true;
		{
			int j;
			j = buffer[1];
			Info_SetValueForStarKey(sv.recordedplayer[j].userinfo, buffer+2, buffer+2+strlen(buffer+2)+1, sizeof(sv.recordedplayer[j].userinfo));
		}
		break;
	case svc_updateuserinfo:
//		ignoreprotocol = true;
		{
			unsigned int j;
			j = buffer[1];
			if (j < MAX_CLIENTS)
			{
				sv.recordedplayer[j].userid = buffer[2] | (buffer[3]<<8) | (buffer[4]<<16) | (buffer[5]<<24);
				Q_strncpyz(sv.recordedplayer[j].userinfo, buffer+6, sizeof(sv.recordedplayer[j].userinfo));
			}
		}
		break;
	case svc_setangle:	//FIXME: forward on to trackers.
		ignoreprotocol = true;
		break;

	case svc_disconnect:
		svs.spawncount++;
		SV_BroadcastCommand("changing\n");
		SV_BroadcastCommand ("reconnect\n");
		ignoreprotocol = true;
		break;
	}
	if (ignoreprotocol)
	{
		ignoreprotocol=false;
		bufferlen = 0;
	}




	if (cldest)
	{
		if (!requireextension || cldest->fteprotocolextensions & requireextension)
		if (bufferlen)
		{
			ClientReliableCheckBlock(cldest, bufferlen);
			ClientReliableWrite_SZ(cldest, buffer, bufferlen);
		}
		cldest = NULL;
	}
	else
	{
		if (multicastpos && (writedest == &sv.datagram || writedest == &sv.multicast))
			writedest = &sv.multicast;
		else
			multicastpos = 0;

		if (writedest == &sv.reliable_datagram)
		{
			writedest = &sv.multicast;
			multicasttype = MULTICAST_ALL_R;
		}

		if (bufferlen)
			SZ_Write(writedest, buffer, bufferlen);

		if (multicastpos)
		{
			vec3_t org;
			coorddata cd;

			memcpy(&cd, &buffer[multicastpos+sizeofcoord*0], sizeofcoord);
			org[0] = MSG_FromCoord(cd, sizeofcoord);
			memcpy(&cd, &buffer[multicastpos+sizeofcoord*1], sizeofcoord);
			org[1] = MSG_FromCoord(cd, sizeofcoord);
			memcpy(&cd, &buffer[multicastpos+sizeofcoord*2], sizeofcoord);
			org[2] = MSG_FromCoord(cd, sizeofcoord);

			SV_MulticastProtExt(org, multicasttype, FULLDIMENSIONMASK, requireextension, 0);
		}
		else if (writedest == &sv.multicast)
			SV_MulticastProtExt(vec3_origin, multicasttype, FULLDIMENSIONMASK, requireextension, 0);
		writedest = NULL;
	}
	bufferlen = 0;
	protocollen=0;
	multicastpos=0;
	requireextension=0;
}
void NPP_MVDForceFlush(void)
{
	if (bufferlen)
	{
		Con_Printf("Forcing flush mvd->qw prot\n");
		Con_Printf("(last was %i)\n", (int)majortype);
		NPP_MVDFlush();
	}
}
void NPP_MVDCheckFlush(void)
{
	if (bufferlen >= protocollen && protocollen)
		NPP_MVDFlush();
}

void NPP_MVDCheckDest(client_t *cl, int broadcast)
{
	if (!broadcast)
	{
		if (!cl)
		{
			Con_Printf("Not a client\n");
			return;
		}
		if ((cldest && cldest != cl) || writedest)
		{
			Con_Printf("MSG destination changed in the middle of a packet.\n");
			NPP_MVDFlush();
		}
		cldest = cl;
	}
	else
	{
		sizebuf_t	*ndest = &sv.reliable_datagram;
		if (cldest || (writedest && writedest != ndest))
		{
			Con_Printf("MSG destination changed in the middle of a packet.\n");
			NPP_MVDFlush();
		}
		writedest = ndest;
	}
}

void NPP_MVDWriteByte(qbyte data, client_t *to, int broadcast)	//replacement write func (nq to qw)
{
	int i;
	NPP_MVDCheckDest(to, broadcast);

	if (!bufferlen)	//new message section
	{
		switch(data)
		{
		case svc_temp_entity://depends on following bytes
			break;
		case svc_serverinfo:
		case svc_print:
		case svc_sound:
		case svc_serverdata:
		case svc_stufftext:
		case svc_modellist:
		case svc_soundlist:
		case svc_updateuserinfo:
		case svc_playerinfo:
		case svc_packetentities:
		case svc_deltapacketentities:
		case svc_lightstyle:
		case svc_nails:
		case svc_nails2:
		case svc_centerprint:
		case svc_setinfo:
			break;
		case svc_setangle:
			if (sv.mvdplayback)
				protocollen = sizeof(qbyte)*5;	//MVDSV writes an extra client num too.
			else
				protocollen = sizeof(qbyte)*4;	//MVDSV writes an extra client num too.
			break;
		case svc_setview:
			protocollen = sizeof(qbyte)*1 + sizeof(short);
			break;
		case svc_cdtrack:
			protocollen = sizeof(qbyte)*2;
			break;
		case svc_killedmonster:
			protocollen = 1;
			break;
		case svc_foundsecret:
			protocollen = 1;
			break;
//		case svc_intermission:
//			protocollen = 1;
//			break;
//		case svc_finale:
//			protocollen = 2;
//			break;
		case svc_muzzleflash:
			protocollen = 3;
			break;
		case svc_smallkick:
		case svc_bigkick:
			protocollen = 1;
			break;

		case svc_spawnstaticsound:
			protocollen = 10;
			break;
		case svc_spawnstatic:
			protocollen = 14;
			break;
		case svc_spawnbaseline:
			protocollen = 16;
			break;
		case svc_updateping:
		case svc_updatefrags:
			protocollen = 4;
			break;
		case svc_updatestat:
		case svc_updatepl:
			protocollen = 3;
			break;
		case svc_updatestatlong:
			protocollen = 6;
			break;
		case svc_updateentertime:
			protocollen = 6;
			break;
		case svc_intermission:
			protocollen = 10;
			break;
		case svc_disconnect:
			protocollen = 2+strlen("EndOfDemo");
			break;
		case svc_chokecount:
			protocollen = 2;
			break;
		case svc_damage:
			protocollen = 9;
			break;
		default:
			Con_Printf("mvd: bad protocol %i\n", (int)data);
			Con_Printf("(last was %i)\n", (int)majortype);
			protocollen = sizeof(buffer);
			net_message.cursize=0;
			data = svc_nop;
			protocollen = 1;
			break;
		}
		majortype = data;
	}
	else if (!protocollen)
	{
		switch(majortype)
		{
		case svc_temp_entity:
			if (bufferlen == 1)
			{
				minortype = data;
				switch(data)
				{
				case TE_LIGHTNING1:
				case TE_LIGHTNING2:
				case TE_LIGHTNING3:
					multicastpos=4;
					multicasttype=MULTICAST_PHS;
					protocollen = sizeof(short)*6+sizeof(short)+sizeof(qbyte)*2;
					break;
				case TE_BLOOD:		//needs to be converted to a particle
				case TE_GUNSHOT:	//needs qbyte 2 removed
					multicastpos=3;
					multicasttype=MULTICAST_PVS;
					protocollen = sizeof(short)*3+sizeof(qbyte)*3;
					break;
				case TE_LIGHTNINGBLOOD:
				case TE_EXPLOSION:
				case TE_SPIKE:
				case TE_SUPERSPIKE:
					multicastpos=2;
					multicasttype=MULTICAST_PHS_R;
					protocollen = sizeof(short)*3+sizeof(qbyte)*2;
					break;
				case TE_TAREXPLOSION:
				case TE_WIZSPIKE:
				case TE_KNIGHTSPIKE:
				case TE_LAVASPLASH:
				case TE_TELEPORT:
					multicastpos=2;
					multicasttype=MULTICAST_PVS;
					protocollen = sizeof(short)*3+sizeof(qbyte)*2;
					break;
				default:
					protocollen = sizeof(buffer);
					Con_Printf("bad tempentity\n");
					break;
				}
			}
			break;
		case svc_serverdata:
			if (bufferlen > 9)
			{
				i = 9;
				for(;i < bufferlen && buffer[i];i++)
					;
				i++;
				i+=4;
				for(;i < bufferlen && buffer[i];i++)
					;
				i+=4*10;
				if (i <= bufferlen)
					protocollen = i;
			}

			break;
		case svc_stufftext:
			if (!data)	//terminated by a null term
				protocollen = bufferlen;
			break;

		case svc_modellist:
		case svc_soundlist:
			if (!data)
				if (!buffer[bufferlen-1])	//two null bytes marks the last string
					protocollen = bufferlen+2;
			break;

		case svc_updateuserinfo:	//6 bytes then a string
			if (!data && bufferlen>=6)
				protocollen = bufferlen+1;
			break;

		case svc_playerinfo:
			if (bufferlen==4)
			{
				unsigned short pflags;
				int j;
				ignoreprotocol=true;
				pflags = buffer[2] + (buffer[3]*256);	//little endian

				protocollen = 4+1;

				for (j=0 ; j<3 ; j++)
					if (pflags & (DF_ORIGIN << j))
						protocollen += 2;

				for (j=0 ; j<3 ; j++)
					if (pflags & (DF_ANGLES << j))
						protocollen += 2;


				if (pflags & DF_MODEL)
					protocollen += 1;

				if (pflags & DF_SKINNUM)
					protocollen += 1;

				if (pflags & DF_EFFECTS)
					protocollen += 1;

				if (pflags & DF_WEAPONFRAME)
					protocollen += 1;

/*
				if (pflags & PF_MSEC)
					protocollen+=1;

				if (pflags & PF_COMMAND)
				{
					Con_Printf("svc_playerinfo PF_COMMAND not expected in mvd\n");
					protocollen+=500;
				}

				for (i=0 ; i<3 ; i++)
					if (pflags & (PF_VELOCITY1<<i) )
						protocollen+=2;

				if (pflags & PF_MODEL)
					protocollen+=1;

				if (pflags & PF_SKINNUM)
					protocollen+=1;

				if (pflags & PF_EFFECTS)
					protocollen+=1;

				if (pflags & PF_WEAPONFRAME)
					protocollen+=1;
*/
			}
			break;

		case svc_packetentities:
		case svc_deltapacketentities:
			if (!data)	//two last bytes are 0.
			{
				unsigned short s;
				i = majortype-svc_packetentities+1;
				buffer[bufferlen] = data;
				bufferlen++;
				while (1)
				{
					s = buffer[i] + buffer[i+1]*256;
					i+=2;
					if (i > bufferlen)
						break;
					if (!s)
					{
						if (i <= bufferlen)
							protocollen = bufferlen;
						break;
					}
					else
					{
						s &= ~511;
						if (s & U_MOREBITS)
						{
							s |= buffer[i];
							i++;
						}

						if (s & U_MODEL)
							i++;

						if (s & U_FRAME)
							i++;

						if (s & U_COLORMAP)
							i++;

						if (s & U_SKIN)
							i++;

						if (s & U_EFFECTS)
							i++;

						if (s & U_ORIGIN1)
							i+=2;

						if (s & U_ANGLE1)
							i++;

						if (s & U_ORIGIN2)
							i+=2;

						if (s & U_ANGLE2)
							i++;

						if (s & U_ORIGIN3)
							i+=2;

						if (s & U_ANGLE3)
							i++;
					}
				}
				bufferlen--;
			}
			break;

		case svc_lightstyle:
			if (!data && bufferlen>=2)
				protocollen = bufferlen+1;
			break;

		case svc_serverinfo:	//looking for two null terminators
			if (!data)	//gotta be the second.
			{
				for (i = 0; i < bufferlen; i++)
				{
					if (!buffer[i])	//we already wrote one.
					{
						protocollen = bufferlen+1;	//so this is the last qbyte
						break;
					}
				}
			}
			break;

		case svc_sound:
			if (bufferlen == 3) //decide after 3
			{
				unsigned short s;
				i = 10;
				s = buffer[1] + buffer[2]*256;
				if (s & SND_VOLUME)
					i++;
				if (s & SND_ATTENUATION)
					i++;
				protocollen = i;
			}
			break;

		case svc_setinfo:
			if (!data && bufferlen>2)
			{
				for (i = 2; i < bufferlen; i++)
				{
					if (!buffer[i])
					{
						protocollen = bufferlen+1;
						break;
					}
				}
			}
			break;

		case svc_centerprint:
			if (!data && bufferlen>=1)
				protocollen = bufferlen+1;
			break;
		case svc_print:
			if (!data && bufferlen>=2)
				protocollen = bufferlen+1;
			break;

		case svc_nails:
			if (bufferlen == 1)
			{
				i = data;	//first qbyte is id, second, (the current, about to be written) is number of nails.
				protocollen = (i * 6) + 2;
			}
			break;
		case svc_nails2:
			if (bufferlen == 1)
			{
				i = data;	//first qbyte is id, second, (the current, about to be written) is number of nails.
				protocollen = (i * 7) + 2;
			}
			break;
		default:
			Con_Printf("mvd: bad protocol %i\n", (int)data);
			protocollen = sizeof(buffer);
			net_message.cursize=0;
			data = svc_nop;
			protocollen = 1;
			break;
		}
	}


	NPP_AddData(&data, sizeof(qbyte));
	NPP_MVDCheckFlush();
}


void NPP_Flush(void)
{
	if (progstype == PROG_NQ)
		NPP_NQFlush();
#ifdef NQPROT
	else
		NPP_QWFlush();
#endif
}
#endif
