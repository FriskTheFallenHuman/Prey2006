#include "precompiled.h"
#pragma hdrstop

#ifndef IMGUI_DISABLE

#include <algorithm> // std::sort - TODO: replace with something custom..

#include "../libs/imgui/imgui_internal.h"

#include "Session_local.h" // sessLocal.GetActiveMenu()

#include "../sys/sys_imgui.h"

#include "../renderer/tr_local.h" // render cvars
#include "../sound/snd_local.h" // sound cvars
#include "async/ServerScan.h"
#include "async/AsyncNetwork.h"

static bool g_serverBrowserOpen = false;
static int g_selectedServer = -1;
static char g_filterBuf[128] = "";
static int g_sortMode = 0; // 0=ping,1=name,2=players,3=gametype,4=map,5=game

static const char* serverSortNames[] = { "Ping", "Name", "Players", "Gametype", "Map", "Game" };

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

// The main draw function. Other code can call this from the ImGui hooks.
void Com_DrawDhewm3ServerBrowser() {
	if ( !g_serverBrowserOpen ) {
		return;
	}

	idServerScan &serverScan = idAsyncNetwork::client.serverList;
	serverScan.RunFrame();	// Make sure the scan state advances each frame

	ImGui::SetNextWindowSize( ImVec2( 900, 480 ), ImGuiCond_FirstUseEver );
	if ( !ImGui::Begin( "Server Browser", &g_serverBrowserOpen, ImGuiWindowFlags_NoCollapse ) ) {
		ImGui::End();
		return;
	}

	// Top controls
	bool lanMode = idAsyncNetwork::LANServer.GetBool();
	if ( ImGui::Checkbox( "LAN", &lanMode ) ) {
		idAsyncNetwork::LANServer.SetBool( lanMode );
		RefreshServers();
	}
	ImGui::SameLine();
	if ( ImGui::Button( "Refresh" ) ) {
		RefreshServers();
	}
	ImGui::SameLine();
	if ( ImGui::Button( "Stop" ) ) {
		serverScan.SetState( idServerScan::IDLE );
	}
	ImGui::SameLine();
	if ( ImGui::Button( "Clear" ) ) {
		serverScan.Clear();
		g_selectedServer = -1;
	}

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

	if ( !g_serverBrowserOpen ) {
		D3::ImGuiHooks::CloseWindow( D3::ImGuiHooks::D3_ImGuiWin_ServerBrowser );
	}
}

void Com_OpenCloseDhewm3ServerBrowser( bool open ) {
	g_serverBrowserOpen = open;
}

void Com_Dhewm3ServerBrowser_f( const idCmdArgs& args ) {
	// toggle based on current mask
	int mask = D3::ImGuiHooks::GetOpenWindowsMask();
	if ( mask & D3::ImGuiHooks::D3_ImGuiWin_ServerBrowser ) {
		D3::ImGuiHooks::CloseWindow( D3::ImGuiHooks::D3_ImGuiWin_ServerBrowser );
	} else {
		D3::ImGuiHooks::OpenWindow( D3::ImGuiHooks::D3_ImGuiWin_ServerBrowser );
	}
}
#else // IMGUI_DISABLE - just a stub function

#include "Common.h"

void Com_Dhewm3ServerBrowser_f( const idCmdArgs &args ) {
	common->Warning( "Dear ImGui is disabled in this build, so the advance settings menu is not available!" );
}

#endif