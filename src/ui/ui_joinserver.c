/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// ui_joinserver.c

#include "ui_local.h"

/// JOIN SERVER MENU

static menuframework_s	s_joinserver_menu;
static menuaction_s		s_joinserver_search_action;
static menuaction_s		s_joinserver_address_book_action;
static menulist_s		s_joinserver_server_list;
static menulist_s		s_joinserver_sort_box;

static char join_menu_title[64];

//#define join_menu_title		"-[ Join Server ]-"
//#define join_menu_title		"-[ Server Browser ]-"
#define ADDRESS_FILE		"addresses.txt"
#define MAX_MENU_SERVERS	256
#define MASTER_SERVER		"master.quakeservers.net"

// user readable information
static serverStatus_t localServers[MAX_MENU_SERVERS];
static char local_server_names[MAX_MENU_SERVERS][64];
static char local_server_addresses[MAX_MENU_SERVERS][64];
static char *server_shit[MAX_MENU_SERVERS + 1];
static int	sortedSList[MAX_MENU_SERVERS];

static int		m_num_servers = 0;
static int		m_num_adr_cvar = 0, m_num_adr_file = 0, m_num_addresses = 0;
static qboolean addressesLoaded = false;
static qboolean joinMenuInitialized = false;
qboolean masterQueryActive = false;

// Maszyna stanów pingowania
typedef enum { B_IDLE, B_PINGING } bstate_t;
static bstate_t browser_state = B_IDLE;
static int ping_index = 0;
static unsigned int next_ping_time = 0;

// Forward declarations dla funkcji statycznych
static void M_LoadServersFromFile(void);
static void M_LoadServers(void);
static void M_PingServers(void);
static void SearchServers(qboolean useMaster);
static void CopyServerTitle(char *buf, int size, const serverStatus_t *status);
static int SortServers(void const *a, void const *b);
void M_AddAddressToList(const char *address);

void M_ParseServerList(sizebuf_t *msg)
{
    netadr_t adr;
    byte *pos;

    adr.type = NA_IP;
    pos = msg->data + msg->readcount;

    while (msg->readcount + 6 <= msg->cursize) {
        adr.ip[0] = *pos++;
        adr.ip[1] = *pos++;
        adr.ip[2] = *pos++;
        adr.ip[3] = *pos++;
        // Poprawka parsowania portu i kolejności bajtów
        adr.port = *(unsigned short *)pos; 
        pos += 2;
        msg->readcount += 6;

        if (adr.port == 0) break;

        // POPRAWKA: Przekazujemy wskaźnik &adr
        M_AddAddressToList(NET_AdrToString(&adr));
    }
}

void M_AddAddressToList(const char *address)
{
    if (m_num_addresses >= MAX_MENU_SERVERS) return;
    if (!address || !address[0]) return;

    for (int i = 0; i < m_num_addresses; i++) {
        if (!strcmp(local_server_addresses[i], address)) return;
    }

    Q_strncpyz(local_server_addresses[m_num_addresses++], address, sizeof(local_server_addresses[0]));
    
    if (masterQueryActive && browser_state == B_IDLE) {
        browser_state = B_PINGING;
        next_ping_time = cls.realtime; 
    }
}

// W ui_joinserver.c, funkcja M_QueryMaster
static void M_QueryMaster(void)
{
	netadr_t adr;
	// Standardowy pakiet: 4x 0xFF, potem komenda tekstowa
	byte packet[] = {0xff, 0xff, 0xff, 0xff, 'g', 'e', 't', 's', 'e', 'r', 'v', 'e', 'r', 's', ' ', '3', '4', '\n'};

	if (!NET_StringToAdr(MASTER_SERVER, &adr)) {
		Com_Printf("Browser: ERROR - Could not resolve master address.\n");
		return;
	}
	if (adr.port == 0) adr.port = BigShort(27900);

	Com_Printf("Browser: Querying master %s (%s)\n", MASTER_SERVER, NET_AdrToString(&adr));

	// Wysyłamy dokładnie tyle bajtów, ile ma pakiet
	NET_SendPacket(NS_CLIENT, sizeof(packet), packet, &adr);
}

cvar_t	*menu_serversort;	//? should stay??

///
static void M_LoadServers (void)
{
    char name[32], *adrstring;
    int  i;

    // Pobieramy adresy z cvarów adr0...adr8
    for (i=0 ; i<9 ; i++)
    {
        Com_sprintf (name, sizeof(name), "adr%i", i);
        adrstring = Cvar_VariableString (name);
        if (!adrstring || !adrstring[0])
            continue;

        if (m_num_addresses < MAX_MENU_SERVERS) {
            Q_strncpyz(local_server_addresses[m_num_addresses++], adrstring, 64);
        }
    }
}

static void SearchServers(qboolean useMaster)
{
    m_num_servers = 0;
    m_num_addresses = 0;
    ping_index = 0;
    next_ping_time = 0;
    server_shit[0] = NULL;
    
    masterQueryActive = useMaster;

    if (useMaster) {
        // CZYSTY TRYB MASTER
        browser_state = B_IDLE; // NIE PINGUJEMY dopóki nie dostaniemy listy
        M_QueryMaster();
    } else {
        // TRYB LOKALNY
        browser_state = B_PINGING;
        M_LoadServersFromFile();
        M_LoadServers();
        M_PingServers();
    }
}

///

static void M_LoadServersFromFile(void)
{
    char *buffer = NULL;
    char *s, *p;
    int len;

    m_num_addresses = 0;
    m_num_adr_file = 0;

    len = FS_LoadFile(ADDRESS_FILE, (void **)&buffer);
    if (!buffer || len <= 0) {
        Com_DPrintf("M_LoadServersFromFile: " ADDRESS_FILE " not found or empty\n");
        return;
    }

    s = buffer;
    while (*s && m_num_adr_file < MAX_MENU_SERVERS) {
        p = COM_Parse(&s);
        if (!p || !p[0]) break;

        Q_strncpyz(local_server_addresses[m_num_adr_file], p, 64);
        m_num_adr_file++;

        while (*s && *s != '\n') s++;
        if (*s == '\n') s++;
    }

    m_num_addresses = m_num_adr_file;
    Com_Printf("Browser: Loaded %i addresses from %s\n", m_num_adr_file, ADDRESS_FILE);
    FS_FreeFile(buffer);
}

///
void CL_SendUIStatusRequests( netadr_t *adr );

static void M_PingServers (void)
{
	int			i;
	netadr_t	adr = {0};
	char		*adrstring;

	if(!m_num_addresses)
		return;

	NET_Config (NET_CLIENT);		// allow remote

	adr.type = NA_BROADCAST;
	adr.port = BigShort(PORT_SERVER);
	CL_SendUIStatusRequests(&adr);

	// send a packet to each address book entry
	for (i=0; i<m_num_addresses; i++)
	{
		adrstring = local_server_addresses[i];
		Com_Printf ("pinging %s...\n", adrstring);
		if (!NET_StringToAdr (adrstring, &adr))
		{
			Com_Printf ("Bad address: %s\n", adrstring);
			continue;
		}
		if (!adr.port)
			adr.port = BigShort(PORT_SERVER);

		CL_SendUIStatusRequests(&adr);
	}
}

static int SortServers( void const *a, void const *b )
{
	int anum, bnum;

	anum = *(int *) a;
	bnum = *(int *) b;

	if(localServers[anum].numPlayers > localServers[bnum].numPlayers)
		return -1;

	if(localServers[anum].numPlayers < localServers[bnum].numPlayers)
		return 1;

	return 0;
}

static int titleMax = 0;


///
static void CopyServerTitle(char *buf, int size, const serverStatus_t *status)
{
    char map[64], hostname[64];
    char *s;

    s = Info_ValueForKey(status->infostring, "hostname");
    Q_strncpyz(hostname, (s && s[0]) ? s : status->address, sizeof(hostname));

    s = Info_ValueForKey(status->infostring, "mapname");
    int max_clients = atoi(Info_ValueForKey(status->infostring, "maxclients"));
    
    Com_sprintf(map, sizeof(map), " %-10s %2i/%2i", 
                (s && s[0]) ? s : "unknown", 
                status->numPlayers, 
                max_clients);

    int map_len = strlen(map);
    int host_limit = size - map_len - 1;

    if (host_limit <= 0) { // Bufor za mały nawet na mapę
        Q_strncpyz(buf, "...", size);
        return;
    }

    Q_strncpyz(buf, hostname, host_limit + 1);
    // Dopełnienie spacjami dla wyrównania kolumn
    int current_len = strlen(buf);
    for (int i = current_len; i < host_limit; i++) {
        buf[i] = ' ';
    }
    buf[host_limit] = 0;

    strncat(buf, map, size - strlen(buf) - 1);
}

/*
void CopyServerTitle(char *buf, int size, const serverStatus_t *status)
{
	char map[64], *s;
	int clients, len, mlen, i, maxLen;

	s = Info_ValueForKey(status->infostring, "hostname");
	Q_strncpyz(buf, s, size);

	s = Info_ValueForKey(status->infostring, "mapname");
	clients = atoi(Info_ValueForKey(status->infostring, "maxclients"));
	Com_sprintf(map, sizeof(map), " %-10s %2i/%2i", s, status->numPlayers, clients );

	len = strlen(buf);
	mlen = strlen(map);
	maxLen = min(titleMax, size);

	if(len + mlen > maxLen) {
		if(maxLen < mlen-3)
			buf[0] = 0;
		else
			buf[maxLen-mlen-3] = 0;
		Q_strncatz(buf, "...", size);
	}
	else
	{
		for(i = len; i < maxLen-mlen; i++)
			buf[i] = ' ';

		buf[i] = 0;
	}

	Q_strncatz(buf, map, size);
}
*/
///

void M_AddToServerList (const serverStatus_t *status)
{
    int i;

    if(!joinMenuInitialized || m_num_servers >= MAX_MENU_SERVERS) return;

    for( i=0 ; i<m_num_servers ; i++ ) {
        if( !strcmp( status->address, localServers[i].address ) ) return;
    }

    localServers[m_num_servers] = *status;
    
    titleMax = s_joinserver_server_list.width / 8;
    if (titleMax > 60) titleMax = 60;

    CopyServerTitle(local_server_names[m_num_servers], sizeof(local_server_names[0]), &localServers[m_num_servers]);
    
    sortedSList[m_num_servers] = m_num_servers;
    m_num_servers++;

    if(menu_serversort->integer) {
        qsort(sortedSList, m_num_servers, sizeof(sortedSList[0]), SortServers);
    }

    // Bezpieczne wypełnianie listy wskaźników
    for(i = 0; i < m_num_servers; i++) {
        server_shit[i] = local_server_names[sortedSList[i]];
    }
    // Gwarantowany terminator wewnątrz granic tablicy (server_shit ma rozmiar MAX_MENU_SERVERS + 1)
    server_shit[m_num_servers] = NULL; 

    s_joinserver_server_list.count = m_num_servers;
}

///
static void JoinServerFunc( void *self )
{
	(void)self; // Wyciszenie ostrzeżenia
	char	buffer[128];
	int		index;

	index = s_joinserver_server_list.curvalue;

	if (index >= m_num_servers)
		return;

	Com_sprintf (buffer, sizeof(buffer), "connect %s\n", localServers[sortedSList[index]].address);
	Cbuf_AddText (buffer);
	M_ForceMenuOff ();
}

static void AddressBookFunc( void *self )
{
	(void)self; // Wyciszenie ostrzeżenia
	M_Menu_AddressBook_f();
}



static void SearchLocalGamesFunc( void *self )
{
	(void)self; // Wyciszenie ostrzeżenia
	SearchServers(masterQueryActive);
}


static void JoinServer_InfoDraw( void )
{
	char key[MAX_INFO_KEY];
	char value[MAX_INFO_VALUE];
	const char *info;
	int x = s_joinserver_server_list.width + 40;
	int y = 80;
	int index;
	serverStatus_t *server;
	playerStatus_t *player;
	int i;

	// Never draw on low resolutions
	if( viddef.width < 512 )
		return;

	index = s_joinserver_server_list.curvalue;
	if( index < 0 || index >= MAX_MENU_SERVERS )
		return;

	server = &localServers[sortedSList[index]];

	DrawAltString( x, y, "Name            Score Ping");
	y += 8;
	DrawAltString( x, y, "--------------- ----- ----");
	y += 10;

	if( !server->numPlayers ) {
		DrawAltString( x, y, "No players");
		y += 8;
	}
	else
 	{
 		for( i=0, player=server->players ; i<server->numPlayers ; i++, player++ ) {
			const char *pstr = va( "%-15s %5i %4i", player->name, player->score, player->ping );
			
			if (player->ping > 0 && player->ping < 100)
				DrawAltString( x, y, pstr ); // Wyróżnienie dla niskiego pingu
			else
				DrawString( x, y, pstr );

 			y += 8;
 		}
 	}

	y+=8;
	DrawAltString( x, y, "Server info");
	y += 8;
	DrawAltString( x, y, "--------------------------");
	y += 10;

	info = (const char *)server->infostring;
	while( *info ) {
		Info_NextPair( &info, key, value );

		if(!key[0] || !value[0])
			break;

		if( strlen( key ) > 15 ) {
			strcpy( key + 12, "..." );
		}

		DrawString( x, y, va( "%-8s", key ));
		DrawString( x + 16 * 8, y, va( "%-16s", value ));
		y += 8;
	}

}

static void JoinServer_MenuDraw( menuframework_s *self )
{
	if (browser_state == B_PINGING && cls.realtime > next_ping_time)
	{
		if (ping_index < m_num_addresses) {
			netadr_t adr;
			if (NET_StringToAdr(local_server_addresses[ping_index], &adr)) {
				if (!adr.port) adr.port = BigShort(PORT_SERVER);
				CL_SendUIStatusRequests(&adr);
			}
			ping_index++;
			next_ping_time = cls.realtime + 40; 
		} else {
			browser_state = B_IDLE;
		}
	}

	DrawString((viddef.width - (strlen(join_menu_title)*8))>>1, 10, join_menu_title);
	JoinServer_InfoDraw();
	Menu_Draw( self );
}

static void SortFunc( void *unused )
{
	Cvar_SetValue( "menu_serversort", s_joinserver_sort_box.curvalue );
}

static void JoinServer_MenuInit( void )
{
	int y, x;

	static const char *yesno_names[] =
	{
		"no",
		"yes",
		0
	};

	menu_serversort = Cvar_Get("menu_serversort", "1", CVAR_ARCHIVE);

	memset(&s_joinserver_menu, 0, sizeof(s_joinserver_menu));
	s_joinserver_menu.x = 0;
	s_joinserver_menu.nitems = 0;

	s_joinserver_server_list.generic.type		= MTYPE_LIST;
	s_joinserver_server_list.generic.flags		= QMF_LEFT_JUSTIFY;
	s_joinserver_server_list.generic.name		= NULL;
	s_joinserver_server_list.generic.callback	= JoinServerFunc;
	s_joinserver_server_list.generic.x			= 20;
	s_joinserver_server_list.generic.y			= 40;
	s_joinserver_server_list.width				= (viddef.width - 40) / 2;
	s_joinserver_server_list.height				= viddef.height - 110;
	s_joinserver_server_list.generic.statusbar = "press ENTER to connect";
	s_joinserver_server_list.itemnames			= (const char **)server_shit;
	s_joinserver_server_list.count				= 0;
	MenuList_Init(&s_joinserver_server_list);

	x = s_joinserver_server_list.width + 60;
	y = 40;

	s_joinserver_address_book_action.generic.type	= MTYPE_ACTION;
	s_joinserver_address_book_action.generic.name	= "address book";
	s_joinserver_address_book_action.generic.flags	= QMF_LEFT_JUSTIFY;
	s_joinserver_address_book_action.generic.x		= x;
	s_joinserver_address_book_action.generic.y		= y;
	s_joinserver_address_book_action.generic.callback = AddressBookFunc;

	s_joinserver_search_action.generic.type		= MTYPE_ACTION;
	s_joinserver_search_action.generic.name		= "refresh server list";
	s_joinserver_search_action.generic.flags	= QMF_LEFT_JUSTIFY;
	s_joinserver_search_action.generic.x		= x;
	s_joinserver_search_action.generic.y		= y+10;
	s_joinserver_search_action.generic.callback = SearchLocalGamesFunc;
	s_joinserver_search_action.generic.statusbar = "search for servers";

	s_joinserver_sort_box.generic.type		= MTYPE_SPINCONTROL;
	s_joinserver_sort_box.generic.flags		= QMF_LEFT_JUSTIFY;
	s_joinserver_sort_box.generic.cursor_offset = 24;
	s_joinserver_sort_box.generic.name		= "list sorting";
	s_joinserver_sort_box.generic.x			= x-10+strlen(s_joinserver_sort_box.generic.name)*8;
	s_joinserver_sort_box.generic.y			= y+20;
	s_joinserver_sort_box.generic.callback	= SortFunc;
	s_joinserver_sort_box.itemnames			= yesno_names;
	s_joinserver_sort_box.curvalue			= menu_serversort->integer;

	s_joinserver_menu.draw = JoinServer_MenuDraw;
	s_joinserver_menu.key = NULL;

	Menu_AddItem( &s_joinserver_menu, &s_joinserver_address_book_action );
	Menu_AddItem( &s_joinserver_menu, &s_joinserver_search_action );
	Menu_AddItem( &s_joinserver_menu, &s_joinserver_sort_box.generic );

	Menu_AddItem( &s_joinserver_menu, &s_joinserver_server_list );

	//Menu_Center( &s_joinserver_menu );

	joinMenuInitialized = true;
	
}

const char *JoinServer_MenuKey( int key )
{
	return Default_MenuKey( &s_joinserver_menu, key );
}
void M_Menu_JoinServer_f (qboolean useMaster)
{
	masterQueryActive = useMaster;
	if (useMaster)
		Q_strncpyz(join_menu_title, "-[ Master Server ]-", sizeof(join_menu_title));
	else
		Q_strncpyz(join_menu_title, "-[ Address Book ]-", sizeof(join_menu_title));

	JoinServer_MenuInit();
	SearchServers(useMaster);
	M_PushMenu( &s_joinserver_menu );
}