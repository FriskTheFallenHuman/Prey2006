#include "precompiled.h"
#pragma hdrstop

#ifndef IMGUI_DISABLE

#include <algorithm> // std::sort - TODO: replace with something custom..
#include <vector>

#include "../libs/imgui/imgui_internal.h"

#include "Session_local.h" // sessLocal.GetActiveMenu()

#include "../sys/sys_imgui.h"

#include "../renderer/tr_local.h" // render cvars
#include "../sound/snd_local.h" // sound cvars
#include "async/ServerScan.h"
#include "async/AsyncNetwork.h"

bool showServerBrowserWindow = false;
bool showcreateServerWindow = false;
bool showplayerSetupwindow = false;

// Create Server state
static char g_createServerName[128] = "Prey Server";
static int g_createServerSelectedMapIndex = 0;
static int g_createServerGameTypeIndex = 0;
static int g_createServerMaxPlayers = 8;
//static char g_createServerPassword[128] = "";
static bool g_createServerDedicated = false;
static idStrList g_createServerMapNameList;
static idList<int> g_createServerMapIndexList;
static bool g_createServerNeedsMapRefresh = true;
//static bool g_createServerUsePassword = false;
//static char g_createServerRemoteConsolePassword[128] = "";
static bool g_createServerAllowServerMod = false;
static bool g_createServerReloadEngine = false;
static bool g_createServerRunMapCycle = false;
static char g_createServerMapCycleStr[256] = "";
static int g_createServerConfigServerRate = 0;
static bool g_createServerPure = false;
static bool g_createServerTeamDamage = false;
static int g_createServerFragLimit = 0;
static int g_createServerTimeLimit = 0;
static bool g_createServerWarmup = false;
static bool g_createServerAllowSpectators = true;
static const char* g_gameTypeNames[] = { "Deathmatch", "Team DM" };
static const int g_gameTypeCount = 2;
static bool g_createServerUpdateMap = false;

// Server Browser state
static int g_selectedServer = -1;
static char g_filterBuf[128] = "";
static int g_sortMode = 0; // 0=ping,1=name,2=players,3=gametype,4=map,5=game
static const char* serverSortNames[] = { "Ping", "Name", "Players", "Gametype", "Map", "Game" };

// Player Setup State
static char playerNameBuf[128] = {};
static idCVar* ui_nameVar = nullptr;
static int g_playerRateIndex = 2; // default to 16000
static bool g_playerAutoSwitch = false;
static bool g_playerAutoReload = false;
static idStrList g_playerModelNameList;
static idStrList g_playerModelPortraitList;
static idList<int> g_playerModelIndexList;
static int g_playerSelectedModelIndex = 0;

void Com_OpenDhewm3CreateServer();
void Com_OpenDhewm3PlayerSetup();

static const char* GetServerDisplayName( const networkServer_t &srv ) {
	const char *name = srv.serverInfo.GetString( "si_name" );
	if ( !name || !name[0] ) {
		return Sys_NetAdrToString( srv.adr );
	}
	return name;
}

static void ConnectToServer( const networkServer_t &srv ) {
	idStr adr = Sys_NetAdrToString( srv.adr );
	cmdSystem->BufferCommandText( CMD_EXEC_APPEND, va( "connect %s\n", adr.c_str() ) );
}

static void RefreshServers() {
	if ( idAsyncNetwork::LANServer.GetBool() ) {
		idAsyncNetwork::client.GetLANServers();
	} else {
		idAsyncNetwork::client.GetNETServers();
	}
}

static void RefreshCreateServerMapList() {
	g_createServerMapIndexList.Clear();
	g_createServerMapNameList.Clear();
	g_createServerSelectedMapIndex = 0;

	const char *gametype = g_gameTypeNames[g_createServerGameTypeIndex];
	if ( !gametype || !gametype[0] ) {
		gametype = "Deathmatch";
	}

	// Collect all maps that support this game type
	int num = fileSystem->GetNumMaps();
	for ( int i = 0; i < num; i++ ) {
		const idDict *dict = fileSystem->GetMapDecl( i );
		if ( dict && dict->GetBool( gametype ) ) {
			g_createServerMapIndexList.Append( i );
			
			// Get display name
			const char *mapName = dict->GetString( "name" );
			if ( mapName[0] == '\0' ) {
				mapName = dict->GetString( "path" );
			}
			mapName = common->GetLanguageDict()->GetString( mapName );
			g_createServerMapNameList.Append( mapName );
		}
	}

	// Ensure selected index is valid after rebuilding the lists
	if ( g_createServerMapIndexList.Num() == 0 ) {
		g_createServerSelectedMapIndex = 0;
	} else if ( g_createServerSelectedMapIndex < 0 ) {
		g_createServerSelectedMapIndex = 0;
	} else if ( g_createServerSelectedMapIndex >= g_createServerMapIndexList.Num() ) {
		g_createServerSelectedMapIndex = g_createServerMapIndexList.Num() - 1;
	}

	g_createServerNeedsMapRefresh = false;
}

static void RefreshPlayerModelList() {
	g_playerModelIndexList.Clear();
	g_playerModelNameList.Clear();
	g_playerModelPortraitList.Clear();

	// try to find multiplayer player def and enumerate model_mp entries
	const idDecl *playerDef = declManager->FindType( DECL_ENTITYDEF, GAME_PLAYERDEFNAME_MP, false );
	if ( playerDef ) {
		const idDict &playerDict = static_cast<const idDeclEntityDef *>( playerDef )->dict;
		const idKeyValue *kv = playerDict.MatchPrefix( "model_mp", NULL );
		while ( kv != NULL ) {
			idStr key = kv->GetKey();
			key.StripLeading( "model_mp" );
			int modelNum = atoi( key.c_str() );

			idStr portrait = playerDict.GetString( va( "mtr_modelPortrait%d", modelNum ), "guis/assets/menu/questionmark" );
			idStr name = playerDict.GetString( va( "text_modelname%d", modelNum ), va("Model %d", modelNum) );
			name = common->GetLanguageDict()->GetString( name.c_str() );

			g_playerModelIndexList.Append( modelNum );
			g_playerModelNameList.Append( name );
			g_playerModelPortraitList.Append( portrait );

			kv = playerDict.MatchPrefix( "model_mp", kv );
		}
	}
}

//
// Server Browser
//

// The main draw function. Other code can call this from the ImGui hooks.
void Com_DrawDhewm3ServerBrowser() {
	if ( !showServerBrowserWindow ) {
		showcreateServerWindow = false;
		showplayerSetupwindow = false;
		D3::ImGuiHooks::CloseWindow( D3::ImGuiHooks::D3_ImGuiWin_ServerBrowser );
		return;
	}

	idServerScan &serverScan = idAsyncNetwork::client.serverList;
	serverScan.RunFrame();	// Make sure the scan state advances each frame

	ImGui::SetNextWindowSizeConstraints( ImVec2(1270, 650), ImVec2(FLT_MAX, FLT_MAX) );
	ImGui::SetNextWindowSize( ImVec2( 900, 480 ), ImGuiCond_FirstUseEver );
	
	ImGui::Begin( "Server Browser", &showServerBrowserWindow, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoDocking );
		// Top: Menu Bar with items
		ImGui::BeginMenuBar();
			if ( ImGui::BeginMenu("Server") ) {
				if ( ImGui::MenuItem("Player Setup") ) {
					Com_OpenDhewm3PlayerSetup();
				}
		
				ImGui::Separator();

				bool lanMode = idAsyncNetwork::LANServer.GetBool();
				if ( ImGui::Checkbox("LAN", &lanMode) ) {
					idAsyncNetwork::LANServer.SetBool(lanMode);
					RefreshServers();
				}

				if ( ImGui::MenuItem("Refresh List") ) {
					RefreshServers();
				}

				if ( ImGui::MenuItem("Stop Searching") ) {
					serverScan.SetState( idServerScan::IDLE );
				}

				if ( ImGui::MenuItem("Clear List") ) {
					serverScan.Clear();
					g_selectedServer = -1;
				}

				ImGui::Separator();

				if ( ImGui::MenuItem("Create Server") ) {
					Com_OpenDhewm3CreateServer();
				}

				ImGui::EndMenu();
			}
		ImGui::EndMenuBar();

		ImGui::Columns( 2 );

		// Left: server list
		ImGui::BeginChild( "server_list", ImVec2( 0, 0 ), true );
			ImGui::Text( "Found: %d servers", serverScan.Num() );

			static char connect_addr[64] = "";

			const float desiredFilterW = 200.0f;
			const float minFilterW = 80.0f;
			const float spacing = ImGui::GetStyle().ItemSpacing.x;

			ImVec2 regionMin = ImGui::GetWindowContentRegionMin();
			ImVec2 regionMax = ImGui::GetWindowContentRegionMax();
			float contentW = regionMax.x - regionMin.x;
			float curX = ImGui::GetCursorPosX();
			ImVec2 labelSz = ImGui::CalcTextSize( "Connect..." );
			float btnW = labelSz.x + ImGui::GetStyle().FramePadding.x * 2.0f;

			float availableForFilter = contentW - (btnW + spacing) - (curX - regionMin.x);
			float filterW = desiredFilterW;
			if ( availableForFilter < desiredFilterW ) {
				filterW = availableForFilter;
				if ( filterW < minFilterW ) {
					filterW = minFilterW;
				}
			}

			// compute X position to start the button so both controls are right-aligned
			float required = btnW + spacing + filterW;
			float startX = regionMax.x - required;
			if ( startX < curX ) {
				startX = curX; // fall back to current position
			}

			ImGui::SameLine();
			ImGui::SetCursorPosX( startX );

			if ( ImGui::Button( "Connect..." ) ) {
				if ( g_selectedServer >= 0 && g_selectedServer < serverScan.Num() ) {
					const char *addr = Sys_NetAdrToString( serverScan[ g_selectedServer ].adr );
					if ( addr ) {
						strncpy( connect_addr, addr, sizeof( connect_addr ) - 1 );
						connect_addr[ sizeof( connect_addr ) - 1 ] = '\0';
					}
				}
				ImGui::OpenPopup( "Connect to..." );
			}

			if ( ImGui::BeginPopupModal( "Connect to...", NULL, ImGuiWindowFlags_AlwaysAutoResize ) ) {
				ImGui::TextWrapped( "Enter IP / hostname (optional :port):" );
				bool submit = false;
				if ( ImGui::InputTextWithHint( "##connectaddr", "127.0.0.1:27960", connect_addr, sizeof(connect_addr), ImGuiInputTextFlags_EnterReturnsTrue ) ) {
					submit = true;
				}
				ImGui::Spacing();
				if ( ImGui::Button( "Connect" ) ) {
					submit = true;
				}
				ImGui::SameLine();
				if ( ImGui::Button( "Cancel" ) ) {
					ImGui::CloseCurrentPopup();
				}
				if ( submit && connect_addr[0] ) {
					cmdSystem->BufferCommandText( CMD_EXEC_APPEND, va( "connect %s\n", connect_addr ) );
					D3::ImGuiHooks::CloseWindow( D3::ImGuiHooks::D3_ImGuiWin_ServerBrowser );
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}

			// Place the filter immediately to the right of the Connect button on the same line
			ImGui::SameLine();
			ImGui::PushItemWidth( filterW );
			ImGui::InputTextWithHint("##filterinput", "Map Name...", g_filterBuf, sizeof(g_filterBuf));
			ImGui::PopItemWidth();
			ImGui::Separator();

			// Table: Server Name | Players | Ping | Game | Map
			ImGuiTableFlags tableFlags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Sortable;
			if ( ImGui::BeginTable( "server_table", 5, tableFlags ) ) {
				ImGui::TableSetupColumn( "Server Name" );
				ImGui::TableSetupColumn( "Players" );
				ImGui::TableSetupColumn( "Ping" );
				ImGui::TableSetupColumn( "Game" );
				ImGui::TableSetupColumn( "Map" );
				ImGui::TableHeadersRow();

				ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs();
				if ( sortSpecs && sortSpecs->SpecsDirty ) {
					if ( sortSpecs->SpecsCount > 0 ) {
						int col = sortSpecs->Specs[0].ColumnIndex;
						switch ( col ) {
							case 0: idAsyncNetwork::client.serverList.SetSorting( SORT_SERVERNAME ); g_sortMode = 1; break;		// Server Name
							case 1: idAsyncNetwork::client.serverList.SetSorting( SORT_PLAYERS ); g_sortMode = 2; break;		// Players
							case 2: idAsyncNetwork::client.serverList.SetSorting( SORT_PING ); g_sortMode = 0; break;			// Ping
							case 3: idAsyncNetwork::client.serverList.SetSorting( SORT_GAME ); g_sortMode = 5; break;			// Game
							case 4: idAsyncNetwork::client.serverList.SetSorting( SORT_MAP ); g_sortMode = 4; break;			// Map
							default: break;
						}
					}
					sortSpecs->SpecsDirty = false;
				}

				for ( int i = 0; i < serverScan.Num(); i++ ) {
					networkServer_t &srv = serverScan[i];

					// Cheap ass filter
					const char *display = GetServerDisplayName( srv );
					const char *map = srv.serverInfo.GetString( "si_map" );
					if ( g_filterBuf[0] ) {
						idStr needle = g_filterBuf;
						idStr hay = display;
						idStr hay2 = map ? map : "";
						hay.ToLower(); hay2.ToLower(); needle.ToLower();
						if ( hay.Find( needle ) == -1 && hay2.Find( needle ) == -1 ) {
							continue;
						}
					}

					ImGui::TableNextRow();

					// HACK: i don't want to copy the game string from ServerScan, so i just get it from the cvar :p
					const char *gameStr = srv.serverInfo.GetString( "si_game" );
					if ( !gameStr || !gameStr[0] ) {
						gameStr = srv.serverInfo.GetString( "si_gametype" );
					}

					ImGui::TableSetColumnIndex( 0 );
					ImGui::PushID( i );
					if ( ImGui::Selectable( display, g_selectedServer == i, ImGuiSelectableFlags_SpanAllColumns ) ) {
						if ( g_selectedServer == i ) {
							ConnectToServer( srv );
							D3::ImGuiHooks::CloseWindow( D3::ImGuiHooks::D3_ImGuiWin_ServerBrowser );
						}
						g_selectedServer = i;
					}
					ImGui::PopID();
					ImGui::TableSetColumnIndex( 1 ); ImGui::Text( "%d/%d", srv.clients, srv.serverInfo.GetInt( "si_maxplayers", 0 ) );
					ImGui::TableSetColumnIndex( 2 ); ImGui::Text( "%d ms", srv.ping );
					ImGui::TableSetColumnIndex( 3 ); ImGui::TextUnformatted( gameStr ? gameStr : "?" );
					ImGui::TableSetColumnIndex( 4 ); ImGui::TextUnformatted( map ? map : "?" );
				}

				ImGui::EndTable();
			}
		ImGui::EndChild();

		ImGui::NextColumn();

		// Right: details for selected server
		ImGui::BeginChild( "server_details", ImVec2( 0, 0 ), true );
			if ( g_selectedServer >= 0 && g_selectedServer < serverScan.Num() ) {
				networkServer_t &srv = serverScan[g_selectedServer];

				ImGui::Columns( 2, "server_details_cols", false );
				ImGui::SetColumnWidth( 1, 340.0f );

				// HACK: i don't want to copy the game string from ServerScan, so i just get it from the cvar :p
				const char *gameStr = srv.serverInfo.GetString( "si_game" );
				if ( !gameStr || !gameStr[0] ) {
					gameStr = srv.serverInfo.GetString( "si_gametype" );
				}

				// Server info
				ImGui::BeginGroup();
				ImGui::TextWrapped( "Name: %s", GetServerDisplayName( srv ) );
				ImGui::TextWrapped( "Address: %s", Sys_NetAdrToString( srv.adr ) );
				ImGui::TextWrapped( "Map: %s", srv.serverInfo.GetString( "si_map" ) );
				ImGui::TextWrapped( "Game: %s", gameStr );
				ImGui::TextWrapped( "Players: %d/%d", srv.clients, srv.serverInfo.GetInt( "si_maxplayers", 0 ) );
				ImGui::TextWrapped( "Ping: %d ms", srv.ping );
				ImGui::EndGroup();

				// Show a map preview if any
				ImGui::NextColumn();
				char screenshot[MAX_STRING_CHARS];
				fileSystem->FindMapScreenshot( srv.serverInfo.GetString( "si_map" ), screenshot, MAX_STRING_CHARS );
				idImage *preview = globalImages->ImageFromFile( screenshot, TF_DEFAULT, true, TR_CLAMP, TD_DEFAULT );
				if ( preview && preview->texnum != idImage::TEXTURE_NOT_LOADED ) {
					float w = 320.0f;
					float h = w * 0.5625f; // 16:9
					ImGui::Image( (ImTextureID)(intptr_t)preview->texnum, ImVec2( w, h ), ImVec2( 0, 0 ), ImVec2( 1, 1 ) );
				} else {
					ImGui::TextWrapped( "No preview available" );
				}

				// Connect, copy and remove buttons
				ImGui::NextColumn();
				ImGui::Separator();
				if ( ImGui::Button( "Connect" ) ) {
					console->Close();
					ConnectToServer( srv );
					D3::ImGuiHooks::CloseWindow( D3::ImGuiHooks::D3_ImGuiWin_ServerBrowser );
				}
				ImGui::SameLine();
				if ( ImGui::Button( "Copy Address" ) ) {
					Sys_SetClipboardData( Sys_NetAdrToString( srv.adr ) );
				}
				ImGui::SameLine();
				if ( ImGui::Button( "Remove" ) ) {
					serverScan.RemoveIndex( g_selectedServer );
					g_selectedServer = -1;
				}

				ImGui::Separator();
				ImGui::TextWrapped( "Server Info:" );

				// show some common keys
				const char *keys[] = { "si_name", "si_map", "si_game", "si_maxplayers", "si_gametype", "si_password", "si_pure", "si_usePass", "si_serverURL", NULL};
				for ( int k = 0; keys[k]; k++ ) {
					const char *v = srv.serverInfo.GetString( keys[k] );
					if ( v && v[0] ) {
						ImGui::TextWrapped( "%-16s: %s", keys[k], v );
					}
				}

				// players list (if available)
				if ( srv.clients > 0 ) {
					ImGui::Separator();
					ImGui::TextWrapped( "Players:" );
					for ( int p = 0; p < srv.clients && p < MAX_ASYNC_CLIENTS; p++ ) {
						if ( srv.nickname[p][0] ) {
							ImGui::BulletText( "%s (%d ms)", srv.nickname[p], srv.pings[p] );
						}
					}
				}

				// end two-column details
				ImGui::Columns( 1 );
			} else {
				ImGui::TextWrapped( "No server selected" );
			}
		ImGui::EndChild();

		ImGui::Columns( 1 );
	ImGui::End();

	if ( !showServerBrowserWindow ) {
		showServerBrowserWindow = false;
	}
}

void Com_OpenCloseDhewm3ServerBrowser( bool open ) {
	if ( open ) {
		showServerBrowserWindow = true;
		RefreshServers();
	} else {
		showServerBrowserWindow = false;
	}
}

void Com_Dhewm3ServerBrowser_f( const idCmdArgs& args ) {
	bool menuOpen = (D3::ImGuiHooks::GetOpenWindowsMask() & D3::ImGuiHooks::D3_ImGuiWin_ServerBrowser) != 0;
	if ( !menuOpen ) {
		D3::ImGuiHooks::OpenWindow( D3::ImGuiHooks::D3_ImGuiWin_ServerBrowser );
	} else {
		if ( ImGui::IsWindowFocused( ImGuiFocusedFlags_AnyWindow ) ) {
			// if the settings window is open and an ImGui window has focus,
			// close the settings window when "serverBrowser" is executed
			D3::ImGuiHooks::CloseWindow( D3::ImGuiHooks::D3_ImGuiWin_ServerBrowser );
		} else {
			// if the settings window is open but no ImGui window has focus,
			// give focus to one of the ImGui windows.
			// useful to get the cursor back when ingame..
			ImGui::SetNextWindowFocus();
		}
	}
}

//
// Create Server
//
void Com_DrawDhewm3CreateServer() {
	if ( !showcreateServerWindow ) {
		return;
	}

	ImGui::SetNextWindowSizeConstraints( ImVec2( 800, 500 ), ImVec2( FLT_MAX, FLT_MAX ) );
	ImGui::SetNextWindowSize( ImVec2( 900, 600 ), ImGuiCond_FirstUseEver );

	ImGui::Begin("Create Server", &showcreateServerWindow, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking);
		// Refresh map list if game type changed
		if ( g_createServerNeedsMapRefresh ) {
			RefreshCreateServerMapList();
		}

		ImGui::Columns( 2 );

		// Lef Side: Configuration
		ImGui::BeginChild( "create_server_controls", ImVec2(0, 0), true );
			if ( ImGui::CollapsingHeader( "Basic Info", ImGuiTreeNodeFlags_Framed ) ) {
				// Server Name
				ImGui::TextUnformatted("Server Name:");
				ImGui::SameLine();
				ImGui::InputText("##servername", g_createServerName, sizeof(g_createServerName));

				ImGui::Separator();

				// Game Type
				ImGui::TextUnformatted("Game Type:");
				ImGui::SameLine();
				int prevGameType = g_createServerGameTypeIndex;
				ImGui::Combo("##gametype", &g_createServerGameTypeIndex, g_gameTypeNames, g_gameTypeCount);
				if ( prevGameType != g_createServerGameTypeIndex ) {
					g_createServerNeedsMapRefresh = true;
				}

				ImGui::Separator();

				// Map Selection
				ImGui::TextUnformatted("Map:");
				ImGui::SameLine();
				if ( g_createServerMapIndexList.Num() > 0 ) {
					int count = g_createServerMapNameList.Num();
					std::vector<const char*> namePtrs;
					namePtrs.reserve(count);
					for ( int i = 0; i < count; i++ ) {
						namePtrs.push_back( g_createServerMapNameList[i].c_str() );
					}
					ImGui::Combo("##mapselect", &g_createServerSelectedMapIndex, namePtrs.data(), count);
				} else {
					ImGui::TextWrapped("No maps available for this game type");
				}

				ImGui::Separator();

				// Max Players
				ImGui::SliderInt("Max Players##maxplayers", &g_createServerMaxPlayers, 1, MAX_ASYNC_CLIENTS);

				ImGui::Separator();

				// Team Damage
				ImGui::Checkbox( "Team Damage##si_teamDamage", &g_createServerTeamDamage );

				ImGui::Separator();

				// Frag Limit
				ImGui::InputInt( "Frag Limit##si_fragLimit", &g_createServerFragLimit );

				ImGui::Separator();

				// Time Limit (minutes)
				ImGui::InputInt( "Time Limit##si_timeLimit", &g_createServerTimeLimit );

				ImGui::Separator();

				// Warmup
				ImGui::Checkbox( "Warmup##si_warmup", &g_createServerWarmup );

				ImGui::Separator();

				// Allow Spectators
				ImGui::Checkbox( "Allow Spectators##si_spectators", &g_createServerAllowSpectators );

				ImGui::Separator();

				// Dedicated Server
				ImGui::Checkbox( "Dedicated Server##net_serverDedicated", &g_createServerDedicated );
				D3::ImGuiHooks::AddTooltip( "Note: Dedicated servers run without a local player." );
			}

			if ( ImGui::CollapsingHeader( "Advanced", ImGuiTreeNodeFlags_Framed ) ) {
				// Use Password Checkbox
				//ImGui::Checkbox( "Enable Password##si_usePass", &g_createServerUsePassword );

				// Password Input
				//ImGui::InputText( "Password##si_password", g_createServerPassword, sizeof( g_createServerPassword ), ImGuiInputTextFlags_Password );

				//ImGui::Separator();

				// Remote Console Password
				//ImGui::InputText( "RC Password##net_serverRemoteConsolePassword", g_createServerRemoteConsolePassword, sizeof( g_createServerRemoteConsolePassword ), ImGuiInputTextFlags_Password );
				//D3::ImGuiHooks::AddTooltip( "Password for remote console access via rcon" );

				//ImGui::Separator();

				// Allow Server-side Mods
				ImGui::Checkbox( "Allow Server-side Mods##net_serverAllowServerMod", &g_createServerAllowServerMod );

				ImGui::Separator();

				// Reload Engine on Map Change
				ImGui::Checkbox( "Reload Engine on Map Change##net_serverReloadEngine", &g_createServerReloadEngine );

				ImGui::Separator();

				// Do Map Cycle
				ImGui::Checkbox( "Do Map Cycle##g_runMapCycle", &g_createServerRunMapCycle );

				// Map Cycle File
				if ( g_createServerRunMapCycle ) {
					ImGui::TextUnformatted( "Map Cycle:" );
					ImGui::SameLine();
					ImGui::InputText( "##g_mapCycle", g_createServerMapCycleStr, sizeof( g_createServerMapCycleStr ) );
				}

				ImGui::Separator();

				// Pure Server
				ImGui::Checkbox( "Pure Server##si_pure", &g_createServerPure );
				D3::ImGuiHooks::AddTooltip( "Enforce all clients use unmodified game files" );

				ImGui::Separator();

				// Server Rate
				const char* rateNames[] = { "Disabled", "128 kbps", "256 kbps", "384 kbps", "512 kbps", "LAN/Auto" };
				ImGui::Combo( "Server Rate##gui_configServerRate", &g_createServerConfigServerRate, rateNames, IM_ARRAYSIZE(rateNames) );
				D3::ImGuiHooks::AddTooltip( "Choose a preset to configure server client rate limits" );
			}


			// Start Server Button
			if ( ImGui::Button( "Start Server", ImVec2(-1, 40) ) ) {
				if ( g_createServerMapIndexList.Num() > 0 && g_createServerSelectedMapIndex >= 0 && g_createServerSelectedMapIndex < g_createServerMapIndexList.Num() ) {
					const idDict *selectedMap = fileSystem->GetMapDecl( g_createServerMapIndexList[g_createServerSelectedMapIndex] );
					const char *mapPath = selectedMap ? selectedMap->GetString( "path" ) : "";

					cvarSystem->SetCVarString( "si_name", g_createServerName );
					cvarSystem->SetCVarString( "si_map", mapPath );
					cvarSystem->SetCVarString( "si_gametype", g_gameTypeNames[g_createServerGameTypeIndex] );
					cvarSystem->SetCVarInteger( "si_maxplayers", g_createServerMaxPlayers );
					cvarSystem->SetCVarInteger( "net_serverDedicated", g_createServerDedicated ? 1 : 0 );

					//cvarSystem->SetCVarString( "si_password", g_createServerPassword );
					//cvarSystem->SetCVarBool( "si_usePass", g_createServerUsePassword );

					//cvarSystem->SetCVarString( "net_serverRemoteConsolePassword", g_createServerRemoteConsolePassword );
					cvarSystem->SetCVarBool( "net_serverAllowServerMod", g_createServerAllowServerMod );
					cvarSystem->SetCVarBool( "net_serverReloadEngine", g_createServerReloadEngine );
					cvarSystem->SetCVarBool( "g_runMapCycle", g_createServerRunMapCycle );
					cvarSystem->SetCVarString( "g_mapCycle", g_createServerMapCycleStr );
					cvarSystem->SetCVarBool( "si_pure", g_createServerPure );
					cvarSystem->SetCVarInteger( "gui_configServerRate", g_createServerConfigServerRate );

					cvarSystem->SetCVarBool( "si_teamDamage", g_createServerTeamDamage );
					cvarSystem->SetCVarInteger( "si_fragLimit", g_createServerFragLimit );
					cvarSystem->SetCVarInteger( "si_timeLimit", g_createServerTimeLimit );
					cvarSystem->SetCVarBool( "si_warmup", g_createServerWarmup );
					cvarSystem->SetCVarBool( "si_spectators", g_createServerAllowSpectators );

					showcreateServerWindow = false;
					D3::ImGuiHooks::CloseWindow( D3::ImGuiHooks::D3_ImGuiWin_ServerBrowser );
					cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "SpawnServer\n" );
				}
			}
		ImGui::EndChild();

		ImGui::NextColumn();

		// Right Side: Map Preview & Data
		ImGui::BeginChild("create_server_preview", ImVec2(0, 0), true);
			if ( g_createServerMapIndexList.Num() > 0 && g_createServerSelectedMapIndex >= 0 && g_createServerSelectedMapIndex < g_createServerMapIndexList.Num() ) {
				const idDict *selectedMap = fileSystem->GetMapDecl( g_createServerMapIndexList[g_createServerSelectedMapIndex] );

				// Map Name and Description
				const char *displayName = selectedMap->GetString( "name" );
				if ( displayName[0] == '\0' ) {
					displayName = selectedMap->GetString( "path" );
				}
				displayName = common->GetLanguageDict()->GetString( displayName );
				ImGui::TextWrapped( "Map: %s", displayName );

				// Map path
				ImGui::TextWrapped( "Path: %s", selectedMap->GetString( "path" ) );

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				// Map Preview Screenshot
				char screenshot[MAX_STRING_CHARS];
				fileSystem->FindMapScreenshot( selectedMap->GetString( "path" ), screenshot, MAX_STRING_CHARS );
				idImage *preview = globalImages->ImageFromFile( screenshot, TF_DEFAULT, true, TR_CLAMP, TD_DEFAULT );
				if ( preview && preview->texnum != idImage::TEXTURE_NOT_LOADED ) {
					float w = 365.0f;
					float h = w * 0.5625f; // 16:9
					ImGui::Image( (ImTextureID)(intptr_t)preview->texnum, ImVec2(w, h), ImVec2(0, 0), ImVec2(1, 1) );
				} else {
					ImGui::TextWrapped( "No preview available" );
				}

				// Map Details
				ImGui::Spacing();
				ImGui::Separator();
				if ( ImGui::CollapsingHeader( "Map Details", ImGuiTreeNodeFlags_Framed ) ) {
					int kvCount = selectedMap->GetNumKeyVals();
					for ( int kvi = 0; kvi < kvCount; kvi++ ) {
						const idKeyValue *kv = selectedMap->GetKeyVal( kvi );
						if ( kv ) {
							ImGui::TextWrapped( "%s: %s", kv->GetKey().c_str(), kv->GetValue().c_str() );
						}
					}
				}
			} else {
				ImGui::TextWrapped( "Select a map to view preview" );
			}
		ImGui::EndChild();

		ImGui::Columns(1);
	ImGui::End();

	if ( !showServerBrowserWindow ) {
		showServerBrowserWindow = false;
	}
}

void Com_OpenDhewm3CreateServer() {
	showcreateServerWindow = true;
	if ( showcreateServerWindow ) {
		RefreshCreateServerMapList();
	}

	//strncpy( g_createServerPassword, cvarSystem->GetCVarString( "si_password" ), sizeof(g_createServerPassword)-1 );
	//g_createServerPassword[sizeof(g_createServerPassword)-1] = '\0';
	//g_createServerUsePassword = cvarSystem->GetCVarBool( "si_usePass" );
	//strncpy( g_createServerRemoteConsolePassword, cvarSystem->GetCVarString( "net_serverRemoteConsolePassword" ), sizeof(g_createServerRemoteConsolePassword)-1 );
	//g_createServerRemoteConsolePassword[sizeof(g_createServerRemoteConsolePassword)-1] = '\0';
	g_createServerAllowServerMod = cvarSystem->GetCVarBool( "net_serverAllowServerMod" );
	g_createServerReloadEngine = cvarSystem->GetCVarBool( "net_serverReloadEngine" );
	g_createServerRunMapCycle = cvarSystem->GetCVarBool( "g_runMapCycle" );
	strncpy( g_createServerMapCycleStr, cvarSystem->GetCVarString( "g_mapCycle" ), sizeof(g_createServerMapCycleStr)-1 );
	g_createServerMapCycleStr[sizeof(g_createServerMapCycleStr)-1] = '\0';
	g_createServerConfigServerRate = cvarSystem->GetCVarInteger( "gui_configServerRate" );
	g_createServerPure = cvarSystem->GetCVarBool( "si_pure" );
	g_createServerTeamDamage = cvarSystem->GetCVarBool( "si_teamDamage" );
	g_createServerFragLimit = cvarSystem->GetCVarInteger( "si_fragLimit" );
	g_createServerTimeLimit = cvarSystem->GetCVarInteger( "si_timeLimit" );
	g_createServerWarmup = cvarSystem->GetCVarBool( "si_warmup" );
	g_createServerAllowSpectators = cvarSystem->GetCVarBool( "si_spectators" );
}

//
// Player Setup
//
static int PlayerNameInputTextCallback(ImGuiInputTextCallbackData* data)
{
	if ( data->EventChar > 0xFF ) { // some unicode char that ISO8859-1 can't represent
		data->EventChar = 0;
		return 1;
	}

	if ( data->Buf ) {
		// we want at most 40 codepoints
		int newLen = D3_UTF8CutOffAfterNCodepoints( data->Buf, 40 );
		if ( newLen != data->BufTextLen ) {
			data->BufTextLen = newLen;
			data->BufDirty = true;
		}
	}

	return 0;
}

void Com_DrawDhewm3PlayerSetup() {
	if ( !showplayerSetupwindow ) {
		return;
	}

	ImGui::Begin("Player Setup", &showplayerSetupwindow, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking);
		ImGuiInputTextFlags flags = ImGuiInputTextFlags_CallbackEdit | ImGuiInputTextFlags_CallbackCharFilter;
		if ( ImGui::InputText("Player Name", playerNameBuf, sizeof(playerNameBuf), flags, PlayerNameInputTextCallback)
			&& playerNameBuf[0] != '\0' ) {
			char playerNameIso[128] = {};
			if (D3_UTF8toISO8859_1(playerNameBuf, playerNameIso, sizeof(playerNameIso), '!') != NULL) {
				playerNameIso[40] = '\0'; // limit to 40 chars, like the original menu
				ui_nameVar->SetString(playerNameIso);
				// update the playerNameBuf to reflect the name as it is now: limited to 40 chars
				// and possibly containing '!' from non-translatable unicode chars
				D3_ISO8859_1toUTF8(ui_nameVar->GetString(), playerNameBuf, sizeof(playerNameBuf));
			}
			else {
				D3::ImGuiHooks::ShowWarningOverlay("Player Name way too long (max 40 chars) or contains invalid UTF-8 encoding!");
				playerNameBuf[0] = '\0';
			}
		}
		D3::ImGuiHooks::AddTooltip("ui_name");

		// Data Rate
		ImGui::Separator();
		const char* rateNames[] = { "24000", "32000", "16000" };
		int prevRate = g_playerRateIndex;
		ImGui::Combo("Data Rate##datarate", &g_playerRateIndex, rateNames, IM_ARRAYSIZE(rateNames));
		if ( prevRate != g_playerRateIndex ) {
			const int rateVals[] = { 24000, 32000, 16000 };
			cvarSystem->SetCVarInteger( "net_clientMaxRate", rateVals[g_playerRateIndex] );
		}

		// Player Model Selector
		ImGui::Separator();
		if ( g_playerModelNameList.Num() > 0 ) {
			int count = g_playerModelNameList.Num();
			std::vector<const char*> namePtrs;
			namePtrs.reserve(count);
			for ( int i = 0; i < count; i++ ) {
				namePtrs.push_back( g_playerModelNameList[i].c_str() );
			}
			int prevModelSel = g_playerSelectedModelIndex;
			ImGui::Combo( "Player Model##playermodel", &g_playerSelectedModelIndex, namePtrs.data(), count );
			if ( prevModelSel != g_playerSelectedModelIndex ) {
				int modelNum = g_playerModelIndexList[ g_playerSelectedModelIndex ];
				cvarSystem->SetCVarInteger( "ui_modelNum", modelNum );
			}

			// Portrait preview
			const char *portrait = g_playerModelPortraitList.Num() > 0 ? g_playerModelPortraitList[ g_playerSelectedModelIndex ].c_str() : NULL;
			if ( portrait && portrait[0] ) {
				idImage *img = globalImages->ImageFromFile( portrait, TF_DEFAULT, true, TR_CLAMP, TD_DEFAULT );
				if ( img && img->texnum != idImage::TEXTURE_NOT_LOADED ) {
					//ImGui::SameLine();
					ImGui::Image( (ImTextureID)(intptr_t)img->texnum, ImVec2(128,128), ImVec2(0,0), ImVec2(1,1) );
				}
			}
		} else {
			ImGui::TextUnformatted("No player models found");
		}

		// Auto Weapon Switch
		ImGui::SameLine();
		if (ImGui::Checkbox("Auto Weapon Switch##ui_autoSwitch", &g_playerAutoSwitch)) {
			cvarSystem->SetCVarBool("ui_autoSwitch", g_playerAutoSwitch);
		}

		// Auto Weapon Reload
		ImGui::SameLine();
		if (ImGui::Checkbox("Auto Weapon Reload##ui_autoReload", &g_playerAutoReload)) {
			cvarSystem->SetCVarBool("ui_autoReload", g_playerAutoReload);
		}
	ImGui::End();

	if ( !showplayerSetupwindow ) {
		showplayerSetupwindow = false;
	}
}

void Com_OpenDhewm3PlayerSetup() {
	showplayerSetupwindow = true;

	ui_nameVar = cvarSystem->Find( "ui_name" );

	// Note: ImGui uses UTF-8 for strings, Doom3 uses ISO8859-1, so we need to translate
	if ( D3_ISO8859_1toUTF8( ui_nameVar->GetString(), playerNameBuf, sizeof(playerNameBuf) ) == nullptr ) {
		// returning NULL means playerNameBuf wasn't big enough - that shouldn't happen,
		// at least the player name input in the original menu only allowed 40 chars
		playerNameBuf[sizeof(playerNameBuf)-1] = '\0';
	}

	// Initialize data rate selection
	int curRate = cvarSystem->GetCVarInteger( "net_clientMaxRate" );
	const int rateVals[] = { 24000, 32000, 16000 };
	g_playerRateIndex = 2; // default to 16000
	for ( int i = 0; i < 3; i++ ) {
		if ( curRate == rateVals[i] ) {
			g_playerRateIndex = i;
			break;
		}
	}

	// Auto weapon switch
	g_playerAutoSwitch = cvarSystem->GetCVarBool( "ui_autoSwitch" );

	// Auto Weapon Reload
	g_playerAutoReload = cvarSystem->GetCVarBool( "ui_autoReload" );

	// Populate player model lists
	RefreshPlayerModelList();
	int uiModelNum = cvarSystem->GetCVarInteger( "ui_modelNum" );
	g_playerSelectedModelIndex = 0;
	for ( int i = 0; i < g_playerModelIndexList.Num(); i++ ) {
		if ( g_playerModelIndexList[i] == uiModelNum ) {
			g_playerSelectedModelIndex = i;
			break;
		}
	}
}

#else // IMGUI_DISABLE - just a stub function

#include "Common.h"

void Com_Dhewm3ServerBrowser_f( const idCmdArgs &args ) {
	common->Warning( "Dear ImGui is disabled in this build, so the server browser menu is not available!" );
}

#endif