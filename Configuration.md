# Configuration

This document explains some dhewm3-specific configuration options.

For general Doom3 configuration see for example [this list of CVars](https://modwiki.dhewm3.org/CVars_%28Doom_3%29)
and [this list of Console Commands](https://modwiki.dhewm3.org/Commands_%28Doom_3%29).

**CVars** are set by entering `cvarName value` in the console, for example `com_showFPS 1`.  
They can also be set as commandline arguments when starting dhewm3, for example `./dhewm3 +set r_fullscreen 0`.

Just entering a CVar's name (without a value) will show its current value, its default value
and a short description of what it does.

Starting dhewm3 with the commandline argument `-h` (for example `dhewm3.exe -h`) will show some
useful commandline arguments, for example how to tell dhewm3 where the game data can be found on your system.

## The Console

Like most id Software games from Quake 1 on, Doom3 has a console that allows entering commands 
and setting Console Variables ("CVars"), often for advanced configuration or to aid development,
see also https://modwiki.dhewm3.org/Console.  

Unlike in original Doom3, in dhewm3 the console is always available (no need to set `com_allowconsole 1`
or similar), and **can be opened with the key combination `Shift + Esc`**.  
The classic "console key" (the one between `Esc`, `Tab` and `1`) should also still work with
most keyboard layouts. However you can disable that, so you can bind that key like any other key
(for example to select the chainsaw), by setting `in_ignoreConsoleKey 1`.

## Screenshot configuration

Doom3 always supported taking screenshots, but dhewm3 (from 1.5.3 on) supports using different
formats than TGA.  
This can be configured with the following CVars:

- `r_screenshotFormat` What format screenshots should be in:
  `0` = TGA (default), `1` = BMP, `2` = PNG, `3` = JPG
- `r_screenshotJpgQuality` Quality when using JPG screenshots (`1` - `100`). Lower value means smaller
   image file but worse quality (default is `75`)
- `r_screenshotPngCompression` Compression level when using PNG screenshots (`0` - `9`). Higher levels
  generate smaller files, but take noticeably longer; `3` (the default) seems to be a good compromise.

## Other CVars added in dhewm3

- `g_hitEffect` if set to `1` (the default), mess up player camera when taking damage.
   Set to `0` if you don't like that effect.

- `m_invertLook` allows inverting mouse look: `0`: don't invert (this is the default),
  `1`: invert up/down (flight controls), `2`: invert left/right, `3`: invert both directions

- `in_nograb` if set to `1`, the mouse isn't grabbed when ingame. Not overly useful for normal playing
  (unless maybe you play with a gamepad), but very useful for debugging. Default is `0`.

- `in_grabKeyboard` if enabled (`1`), grabs all keyboard input if mouse is grabbed, so keyboard shortcuts
  from the OS like Alt-Tab or Windows Key won't work and thus not accidentally interrupt your playing.
  Defaults to `0`.

- `in_kbd` allows you to set your keyboard layout so the console key works better. Mostly useful with SDL1.2
- `in_tty` tab completion and history for input from the **terminal** (on Unix-likes, like Linux, macOS, BSD, ...)

- `r_fullscreenDesktop` configures fullscreen windows (when `r_fullscreen` is `1`).  
  `0`: "real"/"exclusive" fullscreen mode, might switch screen resolution  
  `1`: "desktop" fullscreen mode, which keeps desktop resolution and is more like a borderless fullscreen window
- `r_windowResizable` if set to `1` (the default), the dhewm3 window (when in windowed mode..)
   can be freely resized. Needs SDL2; with 2.0.5 and newer it's applied immediately, otherwise when
   creating the window (startup or `vid_restart`).
- `r_fillWindowAlphaChan` Make sure alpha channel of windows default framebuffer is completely opaque
  at the end of each frame. Needed at least when using Wayland.  
  `1`: do this, `0`: don't do it, `-1`: let dhewm3 decide (default)

- `r_useCarmacksReverse` Use Z-Fail ("Carmack's Reverse") when rendering shadows (default `1`)
- `r_useStencilOpSeparate` Use glStencilOpSeparate() (if available) when rendering shadow (default `1`)
- `r_scaleMenusTo43` Render full-screen menus in 4:3 by adding black bars on the left/right if necessary (default `1`)

- `s_alReverbGain` reduce strength of OpenAL (EAX-like) EFX reverb effects, `0.0` - `1.0` (default `0.5`)
- `s_alHRTF` Enable [HRTF](https://en.wikipedia.org/w/index.php?title=Head-related_transfer_function)
   for better surround sound with stereo **headphones**. `0`: Disable, `1`: Enable, `-1`: Let OpenAL decide (default).  
   *Note* that OpenAL may automatically enable HRTF when it detects headphones, and it can happen that
   it detects regular stereo speakers as headphones (when they're plugged into a jack that's somehow
   labeled as headphone jack) - in that case you'll want to explicitly disable it.
   The *Audio Options* tab of the [dhewm3 Settings Menu](#dhewm3-settings-menu) shows OpenAL Info,
   including the current HRTF state (if supported by your OpenAL version).
- `s_alOutputLimiter` Configure OpenAL's output-limiter which temporarily reduces the overall volume
  when too many too loud sounds play at once, to avoid issues like clipping. `0`: Disable, `1`: Enable, `-1`: Let OpenAL decide (default)
- `s_scaleDownAndClamp` Clamp and reduce volume of all sounds to prevent clipping or temporary downscaling by OpenAL's output limiter (default `1`)