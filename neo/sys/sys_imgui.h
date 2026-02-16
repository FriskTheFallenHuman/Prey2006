

#ifndef _SYS_IMGUI_H_
#define _SYS_IMGUI_H_

#ifndef IMGUI_DISABLE
#include "../libs/imgui/imgui.h"
#endif

namespace D3 {
namespace ImGuiHooks {

enum D3ImGuiWindow {
	D3_ImGuiWin_None        = 0,
	D3_ImGuiWin_Settings    = 1, // advanced dhewm3 settings menu
	D3_ImGuiWin_Demo        = 2, // ImGui demo window
	D3_ImGuiWin_ServerBrowser = 4, // server browser window
	D3_ImGuiWin_Unused = 8, // <unused>
	D3_ImGuiWin_LightEditor = 16, // new ingame Light Editor
	D3_ImGuiWin_SoundEditor = 32, // new ingame Sound Editor
	D3_ImGuiWin_AfEditor    = 64, // new AF Editor
	D3_ImGuiWin_ParticleEditor = 128, // new Particle Editor
	D3_ImGuiWin_ScriptEditor = 256, // new Script Editor
	D3_ImGuiWin_DeclBrowser = 512, // new Decl Browser
	D3_ImGuiWin_MaterialEditor = 1024, // material editor
	D3_ImGuiWin_PDAEditor = 2048, // PDA editor

	D3_ImGuiWin_AnyEditor = D3_ImGuiWin_LightEditor
		| D3_ImGuiWin_SoundEditor
		| D3_ImGuiWin_AfEditor
		| D3_ImGuiWin_ParticleEditor
		| D3_ImGuiWin_ScriptEditor
		| D3_ImGuiWin_DeclBrowser
		| D3_ImGuiWin_MaterialEditor
		| D3_ImGuiWin_PDAEditor // to decide whether to call DrawToolWindows()
};

#ifndef IMGUI_DISABLE

extern ImGuiContext* imguiCtx; // this is only here so IsImguiEnabled() can use it inline

inline bool IsImguiEnabled()
{
	return imguiCtx != NULL;
}

// using void* instead of SDL_Window and SDL_GLContext to avoid dragging SDL headers into sys_imgui.h
extern bool Init(void* sdlWindow, void* sdlGlContext);

extern void Shutdown();

extern void OpenWindow( D3ImGuiWindow win );

extern void CloseWindow( D3ImGuiWindow win );

// enum D3ImGuiWindow values of all currently open imgui windows or-ed together
// (0 if none are open)
extern int GetOpenWindowsMask();

// called with every SDL event by Sys_GetEvent()
// returns true if ImGui has handled the event (so it shouldn't be handled by D3)
extern bool ProcessEvent(const void* sdlEvent);

// for binding keys from an ImGui-based menu: send input events to dhewm3
// even if ImGui window has focus
extern void SetKeyBindMode( bool enable );

// returns true if the system cursor should be shown because an ImGui menu is active
extern bool ShouldShowCursor();

// NewFrame() is called once per D3 frame, after all events have been gotten
// => ProcessEvent() has already been called (probably multiple times)
extern void NewFrame();

// called at the end of the D3 frame, when all other D3 rendering is done
// renders ImGui menus then
extern void EndFrame();

extern float GetScale();
extern void SetScale( float scale );

// show a red overlay-window at the center of the screen that contains
// a warning symbol (triangle with !) and the given text
// disappears after a few seconds or when a key is pressed or the mouse is moved
extern void ShowWarningOverlay( const char* text );

// show a little overlay-window at the upper left of the screen showing the given text
// disappears after a few seconds or when a key is pressed or the mouse is moved
extern void ShowInfoOverlay( const char* text );

// add a tooltip with the given text to the last item, if it's hovered
extern void AddTooltip( const char* text );

// add a little "(?)" after the last item, and show a tooltip with the given text when it's hovered
extern void AddDescrTooltip( const char* description );

extern const char* GetLocalizedString( const char* id, const char* fallback );

enum Style {
	Prey,
	ImGui_Default,
	User
};

// set the overall style for ImGui: Both shape (sizes, roundings, etc) and colors
extern void SetImGuiStyle( Style style );

// set the default dhewm3 imgui style colors
extern void SetPreyStyleColors( ImGuiStyle* dst = nullptr );
extern void SetUserStyleColors();

// write current style settings (incl. colors) as userStyle
extern bool WriteUserStyle();

// copy current style to clipboard
extern void CopyCurrentStyle( bool onlyChanges );

#else // IMGUI_DISABLE - just stub out everything

inline bool IsImguiEnabled()
{
	return false;
}

// using void* instead of SDL_Window and SDL_GLContext to avoid dragging SDL headers into sys_imgui.h
inline bool Init(void* sdlWindow, void* sdlGlContext)
{
	return false;
}

inline void Shutdown() {}

inline bool ProcessEvent(const void* sdlEvent) { return false; }

inline void SetKeyBindMode( bool enable ) {}

inline bool ShouldShowCursor() { return false; }

inline void NewFrame() {}

inline void EndFrame() {}

inline void OpenWindow( D3ImGuiWindow win ) {}

inline void CloseWindow( D3ImGuiWindow win ) {}

inline int GetOpenWindowsMask() { return 0; }

inline float GetScale() { return 1.0f; }
inline void SetScale( float scale ) {}

inline void ShowWarningOverlay( const char* text ) {}

inline bool WriteUserStyle() { return false; }

#endif

}} //namespace D3::ImGuiHooks


#endif /* _SYS_IMGUI_H_ */
