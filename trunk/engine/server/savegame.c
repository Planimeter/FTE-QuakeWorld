#include "qwsvdef.h"

#ifndef CLIENTONLY

//#ifdef _DEBUG
#define NEWSAVEFORMAT
//#endif

extern cvar_t skill;
extern cvar_t deathmatch;
extern cvar_t coop;
extern cvar_t teamplay;

//Writes a SAVEGAME_COMMENT_LENGTH character comment describing the current 
void SV_SavegameComment (char *text)
{
	int		i;
	char	kills[20];

	char *mapname = sv.mapname;

	for (i=0 ; i<SAVEGAME_COMMENT_LENGTH ; i++)
		text[i] = ' ';
	if (!mapname)
		strcpy( text, "Unnamed_Level");
	else
	{
		i = strlen(mapname);
		if (i > SAVEGAME_COMMENT_LENGTH)
			i = SAVEGAME_COMMENT_LENGTH;
		memcpy (text, mapname, i);
	}
	if (ge)	//q2
	{
		sprintf (kills,"");
	}
	else
		sprintf (kills,"kills:%3i/%3i", (int)pr_global_struct->killed_monsters, (int)pr_global_struct->total_monsters);
	memcpy (text+22, kills, strlen(kills));
// convert space to _ to make stdio happy
	for (i=0 ; i<SAVEGAME_COMMENT_LENGTH ; i++)
	{
		if (text[i] == ' ')
			text[i] = '_';
		else if (text[i] == '\n')
			text[i] = '\0';
	}
	text[SAVEGAME_COMMENT_LENGTH] = '\0';
}

#ifndef NEWSAVEFORMAT
void SV_Savegame_f (void)
{
	int len;
	char *s = NULL;
	client_t *cl;
	int clnum;

	int version = SAVEGAME_VERSION;

	char	name[256];
	FILE	*f;
	int		i;
	char	comment[SAVEGAME_COMMENT_LENGTH+1];

	if (Cmd_Argc() != 2)
	{
		Con_TPrintf (STL_SAVESYNTAX);
		return;
	}

	if (strstr(Cmd_Argv(1), ".."))
	{
		Con_TPrintf (STL_NORELATIVEPATHS);
		return;
	}

	if (sv.state != ss_active)
	{
		Con_Printf("Can't apply: Server isn't running or is still loading\n");
		return;
	}

	sprintf (name, "%s/saves/%s", com_gamedir, Cmd_Argv(1));
	COM_DefaultExtension (name, ".sav");

	Con_TPrintf (STL_SAVEGAMETO, name);
	f = fopen (name, "w");
	if (!f)
	{
		Con_TPrintf (STL_ERRORCOULDNTOPEN);
		return;
	}

	//if there are 1 of 1 players connected
	if (sv.allocated_client_slots == 1 && svs.clients->state < cs_spawned)
	{//try to go for nq/zq compatability as this is a single player game.
		s = PR_SaveEnts(svprogfuncs, NULL, &len, 2);	//get the entity state now, so that we know if we can get the full state in a q1 format.
		if (s)
		{
			if (progstype == PROG_QW)
				version = 6;
			else
				version = 5;
		}
	}

	
	fprintf (f, "%i\n", version);
	SV_SavegameComment (comment);
	fprintf (f, "%s\n", comment);

	if (version != SAVEGAME_VERSION)
	{
		for (i=0; i<NUM_SPAWN_PARMS ; i++)
				fprintf (f, "%f\n", svs.clients->spawn_parms[i]);	//client 1.
		fprintf (f, "%f\n", skill.value);
	}
	else
	{
		fprintf(f, "%i\n", sv.allocated_client_slots);
		for (cl = svs.clients, clnum=0; clnum < sv.allocated_client_slots; cl++,clnum++)
		{
			if (cl->state < cs_spawned && !cl->istobeloaded)	//don't save if they are still connecting
			{
				fprintf(f, "\"\"\n");
				continue;
			}

			fprintf(f, "\"%s\"\n", cl->name);
			for (i=0; i<NUM_SPAWN_PARMS ; i++)
				fprintf (f, "%f\n", cl->spawn_parms[i]);
		}
		fprintf (f, "%i\n", progstype);
		fprintf (f, "%f\n", skill.value);
		fprintf (f, "%f\n", deathmatch.value);
		fprintf (f, "%f\n", coop.value);
		fprintf (f, "%f\n", teamplay.value);
	}
	fprintf (f, "%s\n", sv.name);
	fprintf (f, "%f\n",sv.time);

// write the light styles

	for (i=0 ; i<MAX_LIGHTSTYLES ; i++)
	{
		if (sv.lightstyles[i])
			fprintf (f, "%s\n", sv.lightstyles[i]);
		else
			fprintf (f,"m\n");
	}

	if (!s)
		s = PR_SaveEnts(svprogfuncs, NULL, &len, 1);
	fprintf(f, "%s\n", s);
	svprogfuncs->parms->memfree(s);

	fclose (f);
	Con_TPrintf (STL_SAVEDONE);

	SV_BroadcastTPrintf(2, STL_GAMESAVED);
}


//FIXME: Multiplayer save probably won't work with spectators.

void SV_Loadgame_f(void)
{
	char	filename[MAX_OSPATH];
	FILE	*f;
	char	mapname[MAX_QPATH];
	float	time, tfloat;
	char	str[32768];
	int		i;
	edict_t	*ent;
	int		version;
	int pt;

	int slots;
	int current_skill;

	client_t *cl;
	int clnum;
	char plname[32];

	int filelen, filepos;
	char *file;

	if (Cmd_Argc() != 2)
	{
		Con_TPrintf (STL_LOADSYNTAX);
		return;
	}

//	if (sv.state != ss_active)
//	{
//		Con_Printf("Can't apply: Server isn't running or is still loading\n");
//		return;
//	}

	sprintf (filename, "%s/saves/%s", com_gamedir, Cmd_Argv(1));
	COM_DefaultExtension (filename, ".sav");
	
// we can't call SCR_BeginLoadingPlaque, because too much stack space has
// been used.  The menu calls it before stuffing loadgame command
//	SCR_BeginLoadingPlaque ();

	Con_TPrintf (STL_LOADGAMEFROM, filename);
	f = fopen (filename, "rb");
	if (!f)
	{
		Con_TPrintf (STL_ERRORCOULDNTOPEN);
		return;
	}

	fscanf (f, "%i\n", &version);
	if (version != SAVEGAME_VERSION && version != 5 && version != 6)	//5 for NQ, 6 for ZQ/FQ
	{
		fclose (f);
		Con_TPrintf (STL_BADSAVEVERSION, version, SAVEGAME_VERSION);
		return;
	}
	fscanf (f, "%s\n", str);
	if (version == 5)
	{
		Con_Printf("loading single player game\n");
	}
	else if (version == 6)	//this is fuhquake's single player games
	{
		Con_Printf("loading single player qw game\n");
	}
	else
		Con_Printf("loading FTE saved game\n");

	

	for (clnum = 0; clnum < MAX_CLIENTS; clnum++)	//clear the server for the level change.
	{
		cl = &svs.clients[clnum];
		if (cl->state <= cs_zombie)
			continue;

		MSG_WriteByte (&cl->netchan.message, svc_stufftext);
		MSG_WriteString (&cl->netchan.message, "disconnect;wait;reconnect\n");	//kindly ask the client to come again.
		cl->drop = true;
	}
	SV_SendMessagesToAll();

	if (version == 5 || version == 6)
	{
		slots = 1;
		for (clnum = 1; clnum < MAX_CLIENTS; clnum++)	//kick all players fully. Load only player 1.
		{
			cl = &svs.clients[clnum];
			cl->istobeloaded = false;
			cl->state = cs_free;
		}
		cl = &svs.clients[0];
#ifdef SERVERONLY
		strcpy(cl->name, "");
#else
		strcpy(cl->name, name.string);
#endif
		cl->state = cs_zombie;
		cl->connection_started = realtime+20;
		cl->istobeloaded = true;

		for (i=0 ; i<NUM_SPAWN_PARMS ; i++)
			fscanf (f, "%f\n", &cl->spawn_parms[i]);
	}
	else	//fte QuakeWorld saves ALL the clients on the server.
	{
		fscanf (f, "%f\n", &tfloat);
		slots = tfloat;
		if (!slots)	//err
		{
			fclose (f);
			Con_Printf ("Corrupted save game");
			return;
		}
		for (clnum = 0; clnum < sv.allocated_client_slots; clnum++)	//work out which players we had when we saved, and hope they accepted the reconnect.
		{
			cl = &svs.clients[clnum];
			fscanf(f, "%s\n", plname);

			cl->istobeloaded = false;

			cl->state = cs_free;

			COM_Parse(plname);

			if (!*com_token)
				continue;

			strcpy(cl->name, com_token);
			cl->state = cs_zombie;
			cl->connection_started = realtime+20;
			cl->istobeloaded = true;		

			for (i=0 ; i<NUM_SPAWN_PARMS ; i++)
				fscanf (f, "%f\n", &cl->spawn_parms[i]);
		}
		for (clnum = sv.allocated_client_slots; clnum < MAX_CLIENTS; clnum++)
		{	//cleanup.
			cl = &svs.clients[clnum];
			cl->istobeloaded = false;
			cl->state = cs_free;
		}
	}
	if (version == 5 || version == 6)
	{
		fscanf (f, "%f\n", &tfloat);
		current_skill = (int)(tfloat + 0.1);
		Cvar_Set ("skill", va("%i", current_skill));
		Cvar_SetValue ("deathmatch", 0);
		Cvar_SetValue ("coop", 0);
		Cvar_SetValue ("teamplay", 0);

		if (version == 5)
		{
			progstype = PROG_NQ;
			Cvar_Set ("progs", "progs.dat");	//NQ's progs.
		}
		else
		{
			progstype = PROG_QW;
			Cvar_Set ("progs", "spprogs.dat");	//zquake's single player qw progs.
		}
		pt = 0;
	}
	else
	{
		fscanf (f, "%f\n", &tfloat);
		pt = tfloat;

	// this silliness is so we can load 1.06 save files, which have float skill values
		fscanf (f, "%f\n", &tfloat);
		current_skill = (int)(tfloat + 0.1);
		Cvar_Set ("skill", va("%i", current_skill));

		fscanf (f, "%f\n", &tfloat);
		Cvar_SetValue ("deathmatch", tfloat);
		fscanf (f, "%f\n", &tfloat);
		Cvar_SetValue ("coop", tfloat);
		fscanf (f, "%f\n", &tfloat);
		Cvar_SetValue ("teamplay", tfloat);
	}
	fscanf (f, "%s\n",mapname);
	fscanf (f, "%f\n",&time);	
	
	SV_SpawnServer (mapname, NULL, false, false);	//always inits MAX_CLIENTS slots. That's okay, because we can cut the max easily.
	if (sv.state != ss_active)
	{
		fclose (f);
		Con_TPrintf (STL_LOADFAILED);
		return;
	}

	sv.allocated_client_slots = slots;

// load the light styles

	for (i=0 ; i<MAX_LIGHTSTYLES ; i++)
	{
		fscanf (f, "%s\n", str);
		if (sv.lightstyles[i])
			Z_Free(sv.lightstyles[i]);
		sv.lightstyles[i] = Z_Malloc (strlen(str)+1);
		strcpy (sv.lightstyles[i], str);
	}

// load the edicts out of the savegame file
// the rest of the file is sent directly to the progs engine.

	if (version == 5 || version == 6)
		Q_InitProgs();	//reinitialize progs entirly.
	else
	{
		Q_SetProgsParms(false);
		svs.numprogs = 0;

		PR_Configure(svprogfuncs, NULL, -1, MAX_PROGS);
		PR_RegisterFields();
		PR_InitEnts(svprogfuncs, sv.max_edicts);	//just in case the max edicts isn't set.
		progstype = pt;	//presumably the progs.dat will be what they were before.
	}

	filepos = ftell(f);	
	fseek(f, 0, SEEK_END);
	filelen = ftell(f);
	fseek(f, filepos, SEEK_SET);
	filelen -= filepos;
	file = BZ_Malloc(filelen+1+8);
	memset(file, 0, filelen+1+8);
	strcpy(file, "loadgame");
	clnum=fread(file+8, 1, filelen, f);
	file[filelen+8]='\0';	
	pr_edict_size=svprogfuncs->load_ents(svprogfuncs, file, 0);
	BZ_Free(file);

	PR_LoadGlabalStruct();

	sv.time = time;

	pr_global_struct->time = sv.time;
	pr_global_struct->time = sv.time;

	fclose (f);

	SV_ClearWorld ();

	for (i=0 ; i<sv.num_edicts ; i++)
	{
		ent = EDICT_NUM(svprogfuncs, i);

		if (!ent)
			break;
		if (ent->isfree)
			continue;

		SV_LinkEdict (ent, false);
	}

	for (i=0 ; i<MAX_CLIENTS ; i++)
	{
		ent = EDICT_NUM(svprogfuncs, i+1);
		svs.clients[i].edict = ent;
	}
}
#endif




#define CACHEGAME_VERSION 512




void SV_FlushLevelCache(void)
{
	levelcache_t *cache;

	while(svs.levcache)
	{
		cache = svs.levcache->next;
		Z_Free(svs.levcache);
		svs.levcache = cache;
	}

}

qboolean SV_LoadLevelCache(char *level, char *startspot, qboolean ignoreplayers)
{
	eval_t *eval, *e2;

	char	name[MAX_OSPATH];
	FILE	*f;
	char	mapname[MAX_QPATH];
	float	time, tfloat;
	char	str[32768];
	int		i,j;
	edict_t	*ent;
	int		version;

	int current_skill;

	int clnum;

	int pt;

	int filelen, filepos;
	char *file;
	gametype_e gametype;

	levelcache_t *cache;

	cache = svs.levcache;
	while(cache)
	{
		if (!strcmp(cache->mapname, level))
			break;

		cache = cache->next;
	}
	if (!cache)
		return false;	//not visited yet. Ignore the existing caches as fakes.

	gametype = cache->gametype;

	sprintf (name, "%s/saves/%s", com_gamedir, level);
	COM_DefaultExtension (name, ".lvc");

//	Con_TPrintf (STL_LOADGAMEFROM, name);

#ifdef Q2SERVER
	if (gametype == GT_QUAKE2)
	{
		SV_SpawnServer (level, startspot, false, false);

		SV_ClearWorld();
		if (!ge)
		{
			Con_Printf("Incorrect gamecode type.\n");
			return false;
		}

		ge->ReadLevel(name);

		for (i=0 ; i<100 ; i++)	//run for 10 secs to iron out a few bugs.
			ge->RunFrame ();
		return true;
	}
#endif
	
// we can't call SCR_BeginLoadingPlaque, because too much stack space has
// been used.  The menu calls it before stuffing loadgame command
//	SCR_BeginLoadingPlaque ();

	f = fopen (name, "rb");
	if (!f)
	{
		Con_TPrintf (STL_ERRORCOULDNTOPEN);
		return false;
	}

	fscanf (f, "%i\n", &version);
	if (version != CACHEGAME_VERSION)
	{
		fclose (f);
		Con_TPrintf (STL_BADSAVEVERSION, version, CACHEGAME_VERSION);
		return false;
	}
	fscanf (f, "%s\n", str);

	SV_SendMessagesToAll();

	fscanf (f, "%f\n", &tfloat);
	pt = tfloat;

// this silliness is so we can load 1.06 save files, which have float skill values
	fscanf (f, "%f\n", &tfloat);
	current_skill = (int)(tfloat + 0.1);
	Cvar_Set (&skill, va("%i", current_skill));

	fscanf (f, "%f\n", &tfloat);
	Cvar_SetValue (&deathmatch, tfloat);
	fscanf (f, "%f\n", &tfloat);
	Cvar_SetValue (&coop, tfloat);
	fscanf (f, "%f\n", &tfloat);
	Cvar_SetValue (&teamplay, tfloat);

	fscanf (f, "%s\n",mapname);
	fscanf (f, "%f\n",&time);	
	
	SV_SpawnServer (mapname, startspot, false, false);
	if (svs.gametype != gametype)
	{
		Con_Printf("Incorrect gamecode type. Cannot load game.\n");
		return false;
	}
	if (sv.state != ss_active)
	{
		fclose (f);
		Con_TPrintf (STL_LOADFAILED);
		return false;
	}

//	sv.paused = true;		// pause until all clients connect
//	sv.loadgame = true;

// load the light styles

	for (i=0 ; i<MAX_LIGHTSTYLES ; i++)
	{
		fscanf (f, "%s\n", str);
		if (sv.lightstyles[i])
			Z_Free(sv.lightstyles[i]);
		sv.lightstyles[i] = Z_Malloc (strlen(str)+1);
		strcpy (sv.lightstyles[i], str);
	}

// load the edicts out of the savegame file
// the rest of the file is sent directly to the progs engine.

	Q_SetProgsParms(false);

	PR_Configure(svprogfuncs, -1, MAX_PROGS);
	PR_RegisterFields();
	PR_InitEnts(svprogfuncs, sv.max_edicts);

	filepos = ftell(f);	
	fseek(f, 0, SEEK_END);
	filelen = ftell(f);
	fseek(f, filepos, SEEK_SET);
	filelen -= filepos;
	file = BZ_Malloc(filelen+1);
	memset(file, 0, filelen+1);
	clnum=fread(file, 1, filelen, f);
	file[filelen]='\0';	
	pr_edict_size=svprogfuncs->load_ents(svprogfuncs, file, 0);
	BZ_Free(file);

	progstype = pt;

	PR_LoadGlabalStruct();

	pr_global_struct->time = sv.time = time;

	fclose (f);

	SV_ClearWorld ();

	for (i=0 ; i<MAX_CLIENTS ; i++)
	{
		ent = EDICT_NUM(svprogfuncs, i+1);
		svs.clients[i].edict = ent;
	}

	if (!ignoreplayers)
	{
		eval = PR_FindGlobal(svprogfuncs, "startspot", 0);
		if (eval) eval->_int = (int)PR_NewString(svprogfuncs, startspot);

		eval = PR_FindGlobal(svprogfuncs, "ClientReEnter", 0);
		if (eval)
		for (i=0 ; i<MAX_CLIENTS ; i++)
		{
			if (svs.clients[i].spawninfo) 
			{
				globalvars_t *pr_globals = PR_globals(svprogfuncs, PR_CURRENT);
				ent = svs.clients[i].edict;
				j = strlen(svs.clients[i].spawninfo);
				svprogfuncs->restoreent(svprogfuncs, svs.clients[i].spawninfo, &j, ent);

				e2 = svprogfuncs->GetEdictFieldValue(svprogfuncs, ent, "stats_restored", NULL);
				if (e2)
					e2->_float = 1;
				for (j=0 ; j< NUM_SPAWN_PARMS ; j++)
					(&pr_global_struct->parm1)[j] = host_client->spawn_parms[j];
				pr_global_struct->time = sv.time;
				pr_global_struct->self = EDICT_TO_PROG(svprogfuncs, ent);
				ent->area.next = ent->area.prev = NULL;
				G_FLOAT(OFS_PARM0) = sv.time-host_client->spawninfotime;
				PR_ExecuteProgram(svprogfuncs, eval->function);
			}
		}
	}

	for (i=0 ; i<sv.num_edicts ; i++)
	{
		ent = EDICT_NUM(svprogfuncs, i);
		if (ent->isfree)
			continue;

		SV_LinkEdict (ent, false);	// force retouch even for stationary
	}

	return true;	//yay
}

void SV_SaveLevelCache(qboolean dontharmgame)
{
	int len;
	char *s;
	client_t *cl;
	int clnum;

	char	name[256];
	FILE	*f;
	int		i;
	char	comment[SAVEGAME_COMMENT_LENGTH+1];

	levelcache_t *cache;
	if (!sv.state)
		return;

	cache = svs.levcache;
	while(cache)
	{
		if (!strcmp(cache->mapname, sv.name))
			break;

		cache = cache->next;
	}
	if (!cache)	//not visited yet. Let us know that we went there.
	{
		cache = Z_Malloc(sizeof(levelcache_t)+strlen(sv.name)+1);
		cache->mapname = (char *)(cache+1);
		strcpy(cache->mapname, sv.name);

		cache->gametype = svs.gametype;
		cache->next = svs.levcache;
		svs.levcache = cache;
	}

	
	sprintf (name, "%s/saves/%s", com_gamedir, cache->mapname);
	COM_DefaultExtension (name, ".lvc");

	if (!dontharmgame)	//save game in progress
		Con_TPrintf (STL_SAVEGAMETO, name);

#ifdef Q2SERVER
	if (ge)
	{
		char	path[256];
		strcpy(path, name);
		path[COM_SkipPath(name)-name] = '\0';
		Sys_mkdir(path);
		ge->WriteLevel(name);
		return;
	}
#endif


	f = fopen (name, "wb");
	if (!f)
	{
		Con_TPrintf (STL_ERRORCOULDNTOPEN);
		return;
	}

	fprintf (f, "%i\n", CACHEGAME_VERSION);
	SV_SavegameComment (comment);
	fprintf (f, "%s\n", comment);
	for (cl = svs.clients, clnum=0; clnum < MAX_CLIENTS; cl++,clnum++)//fake dropping
	{
		if ((cl->state < cs_spawned && !cl->istobeloaded) || dontharmgame)	//don't drop if they are still connecting
		{
			continue;
		}
		else if (!cl->spectator)
		{
			// call the prog function for removing a client
			// this will set the body to a dead frame, among other things
			pr_global_struct->self = EDICT_TO_PROG(svprogfuncs, cl->edict);
			PR_ExecuteProgram (svprogfuncs, pr_global_struct->ClientDisconnect);
		}
		else if (SpectatorDisconnect)
		{
			// call the prog function for removing a client
			// this will set the body to a dead frame, among other things
			pr_global_struct->self = EDICT_TO_PROG(svprogfuncs, cl->edict);
			PR_ExecuteProgram (svprogfuncs, SpectatorDisconnect);
		}
	}
	fprintf (f, "%d\n", progstype);
	fprintf (f, "%f\n", skill.value);
	fprintf (f, "%f\n", deathmatch.value);
	fprintf (f, "%f\n", coop.value);
	fprintf (f, "%f\n", teamplay.value);
	fprintf (f, "%s\n", sv.name);
	fprintf (f, "%f\n",sv.time);

// write the light styles

	for (i=0 ; i<MAX_LIGHTSTYLES ; i++)
	{
		if (sv.lightstyles[i])
			fprintf (f, "%s\n", sv.lightstyles[i]);
		else
			fprintf (f,"m\n");
	}

	s = PR_SaveEnts(svprogfuncs, NULL, &len, 1);
	fprintf(f, "%s\n", s);
	svprogfuncs->parms->memfree(s);

	fclose (f);
}

#ifdef NEWSAVEFORMAT

#define FTESAVEGAME_VERSION 25000

void SV_Savegame_f (void)
{
	extern cvar_t	nomonsters;
	extern cvar_t	gamecfg;
	extern cvar_t	scratch1;
	extern cvar_t	scratch2;
	extern cvar_t	scratch3;
	extern cvar_t	scratch4;
	extern cvar_t	savedgamecfg;
	extern cvar_t	saved1;
	extern cvar_t	saved2;
	extern cvar_t	saved3;
	extern cvar_t	saved4;
	extern cvar_t	temp1;
	extern cvar_t	noexit;
	extern cvar_t	pr_maxedicts;
	extern cvar_t	progs;


	client_t *cl;
	int clnum;
	char	comment[SAVEGAME_COMMENT_LENGTH+1];
	FILE *f, *f2;
	char filename[MAX_OSPATH];
	int len;
	char *buffer=NULL;
	int buflen=0;
	char *savename;
	levelcache_t *cache;
	char str[MAX_LOCALINFO_STRING+1];

	if (!sv.state)
	{
		Con_Printf("Server is not active - unable to save\n");
		return;
	}

	savename = Cmd_Argv(1);

	if (!*savename || strstr(savename, ".."))
		savename = "quicksav";

	sprintf (filename, "%s/saves/%s/info.fsv", com_gamedir, savename);
	COM_CreatePath(filename);
	f = fopen(filename, "wt");
	if (!f)
	{
		Con_Printf("Couldn't open file %s\n", filename);
		return;
	}
	SV_SavegameComment(comment);
	fprintf (f, "%d\n", FTESAVEGAME_VERSION+svs.gametype);
	fprintf (f, "%s\n", comment);

	fprintf(f, "%i\n", sv.allocated_client_slots);
	for (cl = svs.clients, clnum=0; clnum < sv.allocated_client_slots; cl++,clnum++)
	{
		if (cl->state < cs_spawned && !cl->istobeloaded)	//don't save if they are still connecting
		{
			fprintf(f, "\n");
			continue;
		}
		fprintf(f, "%s\n", cl->name);

		if (*cl->name)
			for (len = 0; len < NUM_SPAWN_PARMS; len++)
				fprintf(f, "%i (%f)\n", *(int*)&cl->spawn_parms[len], cl->spawn_parms[len]);	//write ints as not everyone passes a float in the parms.
																					//write floats too so you can use it to debug.
	}

	Q_strncpyz(str, svs.info, sizeof(str));
	Info_RemovePrefixedKeys(str, '*');
	fprintf (f, "%s\"\n",	str);

	Q_strncpyz(str, localinfo, sizeof(str));
	Info_RemovePrefixedKeys(str, '*');
	fprintf (f, "%s\n",	str);

	fprintf (f, "{\n");	//all game vars. FIXME: Should save the ones that have been retrieved/set by progs.
	fprintf (f, "skill			\"%s\"\n",	skill.string);
	fprintf (f, "deathmatch		\"%s\"\n",	deathmatch.string);
	fprintf (f, "coop			\"%s\"\n",	coop.string);
	fprintf (f, "teamplay		\"%s\"\n",	teamplay.string);

	fprintf (f, "nomonsters		\"%s\"\n",	nomonsters.string);
	fprintf (f, "gamecfg\t		\"%s\"\n",	gamecfg.string);
	fprintf (f, "scratch1		\"%s\"\n",	scratch1.string);
	fprintf (f, "scratch2		\"%s\"\n",	scratch2.string);
	fprintf (f, "scratch3		\"%s\"\n",	scratch3.string);
	fprintf (f, "scratch4		\"%s\"\n",	scratch4.string);
	fprintf (f, "savedgamecfg\t	\"%s\"\n",	savedgamecfg.string);
	fprintf (f, "saved1			\"%s\"\n",	saved1.string);
	fprintf (f, "saved2			\"%s\"\n",	saved2.string);
	fprintf (f, "saved3			\"%s\"\n",	saved3.string);
	fprintf (f, "saved4			\"%s\"\n",	saved4.string);
	fprintf (f, "temp1			\"%s\"\n",	temp1.string);
	fprintf (f, "noexit			\"%s\"\n",	noexit.string);
	fprintf (f, "pr_maxedicts\t	\"%s\"\n",	pr_maxedicts.string);
	fprintf (f, "progs			\"%s\"\n",	progs.string);
	fprintf (f, "set nextserver		\"%s\"\n",	Cvar_Get("nextserver", "", 0, "")->string);
	fprintf (f, "}\n");

	SV_SaveLevelCache(true);	//add the current level. Note that this can cause reentry problems.

	cache = svs.levcache;	//state from previous levels - just copy it all accross.
	fprintf(f, "{\n");
	while(cache)
	{
		fprintf(f, "%s\n", cache->mapname);

		sprintf (filename, "%s/saves/%s.lvc", com_gamedir, cache->mapname);
		f2 = fopen(filename, "rb");
		if (!f2)
			break;
		fseek(f2, 0, SEEK_END);
		len = ftell(f2);
		if (!len)
		{
			Con_Printf("WARNING: %s was empty\n");
			fclose(f2);
			cache = cache->next;
			continue;
		}
		fseek(f2, 0, SEEK_SET);
		if (!buffer || buflen < len)
		{
			if (buffer) BZ_Free(buffer);
			buffer = BZ_Malloc(len);
			buflen = len;
		}
		fread(buffer, len, 1, f2);
		fclose(f2);

		sprintf (filename, "%s/saves/%s/%s.lvc", com_gamedir, savename, cache->mapname);
		f2 = fopen(filename, "wb");
		if (!f2)
			break;
		fwrite(buffer, len, 1, f2);
		fclose(f2);

		cache = cache->next;
	}
	fprintf(f, "}\n");

	fprintf (f, "%s\n", sv.name);

	fclose(f);
}

void SV_Loadgame_f (void)
{
	levelcache_t *cache;
	char str[MAX_LOCALINFO_STRING+1], *trim;
	char savename[MAX_QPATH];
	FILE *f,	*fi,	*fo;
	char filename[MAX_OSPATH];
	int version;
	int clnum;
	int slots;
	client_t *cl;
	gametype_e gametype;

	int len, buflen=0;
	char *buffer=NULL;

	Q_strncpyz(savename, Cmd_Argv(1), sizeof(savename));

	if (!*savename || strstr(savename, ".."))
		strcpy(savename, "quicksav");

	sprintf (filename, "%s/saves/%s/info.fsv", com_gamedir, savename);
	f = fopen (filename, "rt");
	if (!f)
	{
		Con_TPrintf (STL_ERRORCOULDNTOPEN);
		return;
	}

	fgets(str, sizeof(str)-1, f);
	version = atoi(str);
	if (version < FTESAVEGAME_VERSION || version >= FTESAVEGAME_VERSION+GT_MAX)
	{
		fclose (f);
		Con_TPrintf (STL_BADSAVEVERSION, version, FTESAVEGAME_VERSION);
		return;
	}
	gametype = version - FTESAVEGAME_VERSION;
	fgets(str, sizeof(str)-1, f);
#ifndef SERVERONLY
	if (!cls.state)
#endif
		Con_TPrintf (STL_LOADGAMEFROM, filename);


	for (clnum = 0; clnum < MAX_CLIENTS; clnum++)	//clear the server for the level change.
	{
		cl = &svs.clients[clnum];
		if (cl->state <= cs_zombie)
			continue;

		if (cl->isq2client)
			MSG_WriteByte (&cl->netchan.message, svcq2_stufftext);
		else
			MSG_WriteByte (&cl->netchan.message, svc_stufftext);
		MSG_WriteString (&cl->netchan.message, "echo Loading Game;disconnect;wait;wait;reconnect\n");	//kindly ask the client to come again.
		cl->istobeloaded = false;
	}

	SV_SendMessagesToAll();

	fgets(str, sizeof(str)-1, f);
	slots = atoi(str);
	for (cl = svs.clients, clnum=0; clnum < slots; cl++,clnum++)
	{
		if (cl->state > cs_zombie)
			SV_DropClient(cl);

		fgets(str, sizeof(str)-1, f);
		str[sizeof(cl->name)-1] = '\0';
		for (trim = str+strlen(str)-1; trim>=str && *trim <= ' '; trim--)
			*trim='\0';
		for (trim = str; *trim <= ' ' && *trim; trim++)
			;
		strcpy(cl->name, str);
		if (*str)
		{
			cl->state = cs_zombie;
			cl->connection_started = realtime+20;
			cl->istobeloaded = true;
			memset(&cl->netchan, 0, sizeof(cl->netchan));

			for (len = 0; len < NUM_SPAWN_PARMS; len++)
			{
				fgets(str, sizeof(str)-1, f);
				for (trim = str+strlen(str)-1; trim>=str && *trim <= ' '; trim--)
					*trim='\0';
				for (trim = str; *trim <= ' ' && *trim; trim++)

				if (*str == '(')
					cl->spawn_parms[len] = atof(str);
				else
				{
					version = atoi(str);
					cl->spawn_parms[len] = *(float *)&version;
				}
			}
		}
	}

	
	fgets(str, sizeof(str)-1, f);
	for (trim = str+strlen(str)-1; trim>=str && *trim <= ' '; trim--)
		*trim='\0';
	for (trim = str; *trim <= ' ' && *trim; trim++)
		;
	Info_RemovePrefixedKeys(str, '*');	//just in case
	Info_RemoveNonStarKeys(svs.info);
	len = strlen(svs.info);
	Q_strncpyz(svs.info+len, str, sizeof(svs.info)-len);

	fgets(str, sizeof(str)-1, f);
	for (trim = str+strlen(str)-1; trim>=str && *trim <= ' '; trim--)
		*trim='\0';
	for (trim = str; *trim <= ' ' && *trim; trim++)
		;
	Info_RemovePrefixedKeys(str, '*');	//just in case
	Info_RemoveNonStarKeys(localinfo);
	len = strlen(localinfo);
	Q_strncpyz(localinfo+len, str, sizeof(localinfo)-len);

	fgets(str, sizeof(str)-1, f);
	for (trim = str+strlen(str)-1; trim>=str && *trim <= ' '; trim--)
		*trim='\0';
	for (trim = str; *trim <= ' ' && *trim; trim++)
		;
	if (strcmp(str, "{"))
		SV_Error("Corrupt saved game\n");
	while(1)
	{
		if (!fgets(str, sizeof(str)-1, f))
			SV_Error("Corrupt saved game\n");
		for (trim = str+strlen(str)-1; trim>=str && *trim <= ' '; trim--)
			*trim='\0';
		for (trim = str; *trim <= ' ' && *trim; trim++)
			;
		if (!strcmp(str, "}"))
			break;
		else if (*str)
			Cmd_ExecuteString(str, RESTRICT_RCON);
	}

	SV_FlushLevelCache();

	fgets(str, sizeof(str)-1, f);
	for (trim = str+strlen(str)-1; trim>=str && *trim <= ' '; trim--)
		*trim='\0';
	for (trim = str; *trim <= ' ' && *trim; trim++)
		;
	if (strcmp(str, "{"))
		SV_Error("Corrupt saved game\n");
	while(1)
	{
		if (!fgets(str, sizeof(str)-1, f))
			SV_Error("Corrupt saved game\n");
		for (trim = str+strlen(str)-1; trim>=str && *trim <= ' '; trim--)
			*trim='\0';
		for (trim = str; *trim <= ' ' && *trim; trim++)
			;
		if (!strcmp(str, "}"))
			break;
		if (!*str)
			continue;

		cache = Z_Malloc(sizeof(levelcache_t)+strlen(str)+1);
		cache->mapname = (char *)(cache+1);
		strcpy(cache->mapname, str);
		cache->gametype = gametype;

		cache->next = svs.levcache;





		sprintf (filename, "%s/saves/%s/%s.lvc", com_gamedir, savename, cache->mapname);
		fi = fopen(filename, "rb");
		if (!fi)
		{
			Z_Free(cache);
			continue;
		}
		fseek(fi, 0, SEEK_END);
		len = ftell(fi);
		fseek(fi, 0, SEEK_SET);
		if (!buffer || buflen < len)
		{
			if (buffer) BZ_Free(buffer);
			buffer = BZ_Malloc(len);
			buflen = len;
		}
		fread(buffer, len, 1, fi);
		fclose(fi);

		sprintf (filename, "%s/saves/%s.lvc", com_gamedir, cache->mapname);
		fo = fopen(filename, "wb");
		if (!fo)
		{
			Z_Free(cache);
			continue;
		}
		fwrite(buffer, len, 1, fo);
		fclose(fo);

		svs.levcache = cache;
	}
	if (buffer)
		Z_Free(buffer);

	fgets(str, sizeof(str)-1, f);
	for (trim = str+strlen(str)-1; trim>=str && *trim <= ' '; trim--)
		*trim='\0';
	for (trim = str; *trim <= ' ' && *trim; trim++)
		;

	fclose(f);

	SV_LoadLevelCache(str, "", true);
	sv.allocated_client_slots = slots;
}
#endif

#endif
