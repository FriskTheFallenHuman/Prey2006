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
	const char *name = srv.serverInfo.GetString( "si_hostname" );
	if ( !name || !name[0] ) {
		name = srv.serverInfo.GetString( "si_name" );
	}
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
	// Make sure the scan state advances each frame
	serverScan.RunFrame();

	ImGui::SetNextWindowSize( ImVec2( 900, 480 ), ImGuiCond_FirstUseEver );
	if ( !ImGui::Begin( "Server Browser", &g_serverBrowserOpen, ImGuiWindowFlags_NoCollapse ) ) {
		ImGui::End();
		return;
	}

	// Top controls
	// Single Refresh button with LAN checkbox (mirrors main menu behavior)
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

	ImGui::SameLine();
	ImGui::Text( "Filter:" );
	ImGui::SameLine();
	ImGui::PushItemWidth( 200 );
	ImGui::InputText( "##serverfilter", g_filterBuf, sizeof( g_filterBuf ) );
	ImGui::PopItemWidth();

	ImGui::SameLine( 0, 12 );
	ImGui::Text( "Sort:" );
	ImGui::SameLine();
	if ( ImGui::BeginCombo( "##sortcombo", serverSortNames[g_sortMode] ) ) {
		for ( int i = 0; i < IM_ARRAYSIZE( serverSortNames ); i++ ) {
			bool sel = ( g_sortMode == i );
				if ( ImGui::Selectable( serverSortNames[i], sel ) ) {
					g_sortMode = i;
					idAsyncNetwork::client.serverList.SetSorting( (serverSort_t)i );
				}
		}
		ImGui::EndCombo();
	}

	ImGui::Separator();

	ImGui::Columns( 2 );

	// Left: server list
	ImGui::BeginChild( "server_list", ImVec2( 0, 0 ), true );
	ImGui::Text( "Found: %d servers", serverScan.Num() );
	ImGui::Separator();

	for ( int i = 0; i < serverScan.Num(); i++ ) {
		networkServer_t &srv = serverScan[i];

		// basic filter by substring in name or map
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

		// format the row
			idStr row = va( "%s   [%d/%d]   %s   %dms", display, srv.clients, srv.serverInfo.GetInt( "si_maxplayers", 0 ), map ? map : "?", srv.ping );

		if ( ImGui::Selectable( row.c_str(), g_selectedServer == i ) ) {
			if ( g_selectedServer == i ) {
				console->Close();
				// double click: connect
				ConnectToServer( srv );
				D3::ImGuiHooks::CloseWindow( D3::ImGuiHooks::D3_ImGuiWin_ServerBrowser );
			}
			g_selectedServer = i;
		}
	}

	ImGui::EndChild();

	ImGui::NextColumn();

	// Right: details for selected server
	ImGui::BeginChild( "server_details", ImVec2( 0, 0 ), true );
	if ( g_selectedServer >= 0 && g_selectedServer < serverScan.Num() ) {
		networkServer_t &srv = serverScan[g_selectedServer];
		ImGui::TextWrapped( "Name: %s", GetServerDisplayName( srv ) );
		ImGui::TextWrapped( "Address: %s", Sys_NetAdrToString( srv.adr ) );
		ImGui::TextWrapped( "Map: %s", srv.serverInfo.GetString( "si_map" ) );
		ImGui::TextWrapped( "Game: %s", srv.serverInfo.GetString( "si_game" ) );
		ImGui::TextWrapped( "Players: %d/%d", srv.clients, srv.serverInfo.GetInt( "si_maxplayers", 0 ) );
		ImGui::TextWrapped( "Ping: %d ms", srv.ping );

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
		const char *keys[] = { "si_hostname", "si_map", "si_game", "si_maxplayers", "si_gametype", "si_password", NULL };
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
	} else {
		ImGui::TextWrapped( "No server selected" );
	}
	ImGui::EndChild();

	ImGui::Columns( 1 );

	ImGui::End();
}

void Com_OpenCloseDhewm3ServerBrowser( bool open ) {
	g_serverBrowserOpen = open;
}

void Com_Dhewm3ServerBrowser_f( const idCmdArgs& args ) {
	// Toggle the server browser window via console command using the central ImGui hooks
	if ( args.Argc() > 1 ) {
		const char *a = args.Argv(1);
		if ( !idStr::Icmp( a, "open" ) || !idStr::Icmp( a, "1" ) ) {
			D3::ImGuiHooks::OpenWindow( D3::ImGuiHooks::D3_ImGuiWin_ServerBrowser );
			return;
		}
		if ( !idStr::Icmp( a, "close" ) || !idStr::Icmp( a, "0" ) ) {
			D3::ImGuiHooks::CloseWindow( D3::ImGuiHooks::D3_ImGuiWin_ServerBrowser );
			return;
		}
	}
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