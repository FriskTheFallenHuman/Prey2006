cmake_minimum_required(VERSION 3.20)

# Only use vcpkg in win32 since there we dont have package managers
if(WIN32)
	# Check if vcpkg exist in our source folder
	if (EXISTS "${CMAKE_SOURCE_DIR}/../vcpkg/scripts/buildsystems/vcpkg.cmake")
		set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/../vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
	else()
		message(FATAL_ERROR "Vcpkg wasn't not found, aborting, make sure to git clone this with all submodules!")
	endif()
endif() # WIN32

# Enable folder groups
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER  "")
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMakeModules")

project(prey06 CXX C)

# build type
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "RelWithDebInfo")
endif()

set(ENGINE_BINARY "PREY06")
set(DEDICATED_BINARY "PREY06Ded")

include(CheckCXXCompilerFlag)
include(GNUInstallDirs OPTIONAL RESULT_VARIABLE GNUINSTALLDIRS)
include(GlobbedHeaders)

option(ENGINE						"Build the main executable." ON)
option(DEDICATED					"Build the dedicated server executable." ON)
if(WIN32)
	option(TOOLS						"Build the include tools into the main executable." ON)
endif()
option(BASE						"Build the base game code" ON)
option(ONATIVE						"Optimize for the host CPU" OFF)
option(REPRODUCIBLE_BUILD			"Replace __DATE__ and __TIME__ by hardcoded values for reproducible builds." OFF)
option(LINUX_RELEASE_BINS			"Set RPATH to \$ORIGIN/libs/ for Linux binary releases" OFF)
option(HARDLINK_GAME				"Compile gamecode into executable (no game DLLs)" OFF)

if(NOT MSVC) # GCC/clang or compatible, hopefully
	option(FORCE_COLORED_OUTPUT	"Always produce ANSI-colored compiler warnings/errors (GCC/Clang only; esp. useful with ninja)." OFF)
	option(ASAN					"Enable GCC/Clang Adress Sanitizer (ASan)" OFF) # TODO: MSVC might also support this, somehow?
	option(UBSAN					"Enable GCC/Clang Undefined Behavior Sanitizer (UBSan), implies HARDLINK_GAME" OFF)

	if(UBSAN AND NOT HARDLINK_GAME)
		message(STATUS "UBSAN requires linking the gamecode into the executable, will enable HARDLINK_GAME")
		set(HARDLINK_GAME ON)
	endif()
endif()

if (LINUX_RELEASE_BINS)
	message(STATUS "Setting RPATH to \$ORIGIN/libs/ so you can put dependencies in there")
	set(CMAKE_SKIP_RPATH OFF CACHE BOOL "Skip RPATH" FORCE)
	set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
	set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
	set(CMAKE_INSTALL_RPATH "$ORIGIN/libs")
	set(ldflags "${ldflags} -Wl,--large-address-aware,-z,origin") # not sure this is even needed, but shouldn't hurt
else()
	set(CMAKE_SKIP_RPATH ON CACHE BOOL "Skip RPATH" FORCE)
endif()

if(REPRODUCIBLE_BUILD)
	# don't use __DATE__ and __TIME__ macros so builds are reproducible
	add_definitions(-DID_REPRODUCIBLE_BUILD)
endif()

# Set these if we found either MFC or any UI toolkit, turn off TOOLS if we didn't found neither of theses
set(MFC_TOOLS OFF) # (Visual Studio + SDL2 only)

# Define our cpu variable
include(CPUDefine)

# Nicely tied our thirdparty libraries here
set(sys_thirdparty_libs)

# libs
if(UNIX)
find_package(PkgConfig REQUIRED)
endif()

find_package(OpenAL REQUIRED)
include_directories(${OPENAL_INCLUDE_DIR})
list(APPEND sys_thirdparty_libs ${OPENAL_LIBRARY})

if(UNIX)
	pkg_check_modules(SDL2 REQUIRED sdl2)
	include_directories(${SDL2_INCLUDE_DIRS})
	link_directories(${SDL2_LIBRARY_DIRS})
	set(SDL2_LIBRARY ${SDL2_LIBRARIES})
else()
	find_package(SDL2 REQUIRED)
	include_directories(${SDL2_INCLUDE_DIR})
	set(SDL2_LIBRARY SDL2::SDL2)
endif()
list(APPEND sys_thirdparty_libs ${SDL2_LIBRARY})

# Curl
find_package(CURL QUIET)
if(CURL_FOUND)
	set(ID_ENABLE_CURL ON)
	if(WIN32)
		set(CURL_INCLUDE_DIR ${CURL_INCLUDE_DIRS})
	endif()
	include_directories(${CURL_INCLUDE_DIR})
	message(STATUS "libcurl found and enabled")
	list(APPEND sys_thirdparty_libs ${CURL_LIBRARY})
else()
	message(WARNING "libcurl not found, server downloads won't be available (apart from that the engine will work)")
	set(ID_ENABLE_CURL OFF)
	set(CURL_LIBRARY "")
endif()

if(TOOLS)
	if(MSVC)
		# This is required for tools on windows.
		find_package(MFC)
		if(NOT MFC_FOUND)
			message(WARNING "MFC ('Microsoft Foundation Classes for C++') couldn't be found, but is needed for TOOLS!")
			message(WARNING "If you're using VS2013, you'll also need the 'Multibyte MFC Library for Visual Studio 2013': https://www.microsoft.com/en-us/download/details.aspx?id=40770 (VS2015 and 2017 include that in the default MFC package)")
		else()
			set(MFC_TOOLS ON)
		endif()
	endif()
endif()

set(D3_COMPILER_IS_CLANG FALSE)
set(D3_COMPILER_IS_GCC_OR_CLANG FALSE)

if(NOT MSVC)
	if(NOT WIN32)
		# libbacktrace support - TODO: might work with MinGW? we don't have a crash handler for win32 though..
		include(CheckCSourceCompiles)
		set(CMAKE_REQUIRED_LIBRARIES backtrace)
		check_c_source_compiles("#include <backtrace.h>
		int main() { backtrace_create_state(NULL, 0, NULL, NULL); return 0; }" HAVE_LIBBACKTRACE)
		unset(CMAKE_REQUIRED_LIBRARIES)

		if(HAVE_LIBBACKTRACE)
			set(sys_libs ${sys_libs} backtrace)
			add_definitions(-DD3_HAVE_LIBBACKTRACE)
			message(STATUS "Using libbacktrace")
		else()
			message(WARNING "libbacktrace wasn't found. It's not required but recommended, because it provides useful backtraces if engine/game crashes")
		endif()
	endif() # NOT WIN32

	# check if our SDL2 supports X11 in SDL_syswm so we can use it for DPI scaling
	set(CMAKE_REQUIRED_LIBRARIES SDL2)
	check_c_source_compiles("#include <SDL_syswm.h>
		int main() { SDL_SysWMinfo wmInfo = {}; wmInfo.info.x11.display = NULL; return 0; }" HAVE_SDL_X11)
	unset(CMAKE_REQUIRED_LIBRARIES)

	if (HAVE_SDL_X11)
		message(STATUS "This SDL2 has X11 support")
		add_definitions(-DD3_SDL_X11)
	endif()

	# check if this is some kind of clang (Clang, whatever)
	# (convert compiler ID to lowercase so we match Clang, clang, etc, regardless of case)
	string(TOLOWER ${CMAKE_CXX_COMPILER_ID} compiler_id_lower)
	if(compiler_id_lower MATCHES ".*clang.*")
		message(STATUS "Compiler \"${CMAKE_CXX_COMPILER_ID}\" detected as some kind of clang")
		set(D3_COMPILER_IS_CLANG TRUE)
		set(D3_COMPILER_IS_GCC_OR_CLANG TRUE)
	elseif(CMAKE_COMPILER_IS_GNUCC)
		set(D3_COMPILER_IS_GCC_OR_CLANG TRUE)
	endif()
	unset(compiler_id_lower)
endif() # not MSVC

# compiler specific flags
if(D3_COMPILER_IS_GCC_OR_CLANG)
	add_compile_options(-pipe)
	add_compile_options(-Wall)
	add_compile_options(-Wno-unknown-pragmas)
	add_compile_options(-Wno-narrowing)

	if(NOT CMAKE_CROSSCOMPILING AND ONATIVE)
		add_compile_options(-march=native)
	elseif(cpu STREQUAL "x86")
		add_compile_options(-march=pentium3)
	endif()

	if(FORCE_COLORED_OUTPUT)
		if(CMAKE_COMPILER_IS_GNUCC)
		   add_compile_options (-fdiagnostics-color=always)
		elseif (D3_COMPILER_IS_CLANG)
		   add_compile_options (-fcolor-diagnostics)
		endif ()
	endif ()

	set(CMAKE_C_FLAGS_DEBUG "-g -ggdb -D_DEBUG -O0")
	set(CMAKE_C_FLAGS_DEBUGALL "-g -ggdb -D_DEBUG")
	set(CMAKE_C_FLAGS_PROFILE "-g -ggdb -D_DEBUG -O1 -fno-omit-frame-pointer")
	set(CMAKE_C_FLAGS_RELEASE "-O2 -fno-math-errno -fno-trapping-math  -ffinite-math-only -fomit-frame-pointer")
	set(CMAKE_C_FLAGS_RELWITHDEBINFO "-g -ggdb -O2 -fno-math-errno -fno-trapping-math  -ffinite-math-only -fno-omit-frame-pointer")
	set(CMAKE_C_FLAGS_MINSIZEREL "-Os -fno-math-errno -fno-trapping-math  -ffinite-math-only -fomit-frame-pointer")

	set(CMAKE_CXX_FLAGS_DEBUGALL ${CMAKE_C_FLAGS_DEBUGALL})
	set(CMAKE_CXX_FLAGS_PROFILE ${CMAKE_C_FLAGS_PROFILE})

	add_compile_options(-fno-strict-aliasing)

	# dear idiot compilers, don't fuck up math code with useless FMA "optimizations"
	# (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=100839)
	CHECK_CXX_COMPILER_FLAG("-ffp-contract=off" cxx_has_fp-contract)
	if(cxx_has_fp-contract)
		add_compile_options(-ffp-contract=off)
	endif()

	if(ASAN)
		# if this doesn't work, ASan might not be available on your platform, don't set ASAN then..
		add_compile_options(-fsanitize=address)
		set(sys_libs ${sys_libs} -fsanitize=address)
	endif()

	if(UBSAN)
		# if this doesn't work, UBSan might not be available on your platform, don't set UBSAN then..
		add_compile_options(-fsanitize=undefined)
		set(sys_libs ${sys_libs} -fsanitize=undefined)
	endif()

	CHECK_CXX_COMPILER_FLAG("-fvisibility=hidden" cxx_has_fvisibility)
	if(NOT cxx_has_fvisibility)
		message(FATAL_ERROR "Compiler does not support -fvisibility")
	endif()
	add_compile_options(-fvisibility=hidden)

	# TODO fix these warnings
	add_compile_options(-Wno-sign-compare)
	add_compile_options(-Wno-switch)

	CHECK_CXX_COMPILER_FLAG("-Woverloaded-virtual" cxx_has_Woverload_virtual)
	if(cxx_has_Woverload_virtual)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Woverloaded-virtual")
	endif()

	# shut up about using memcpy() on classes, in the cases doom3 uses it it seems to be fine
	CHECK_CXX_COMPILER_FLAG("-Wno-class-memaccess" cxx_has_Wno-class-memaccess)
	if(cxx_has_Wno-class-memaccess)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-class-memaccess")
	endif()

	# ignore warnings about variables named "requires" for now (in C++20 it's a keyword,
	#  but currently we don't even use C++11 features)
	CHECK_CXX_COMPILER_FLAG("-Wno-c++20-compat" cxx_has_Wno-cpp20-compat)
	if(cxx_has_Wno-cpp20-compat)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-c++20-compat")
	endif()

	#if (NOT MSVC)
	#	find_package(OpenSSL REQUIRED)
	#endif()

	if(WIN32)
		# use 8MB stack instead of 1MB, so big models don't overflow it with _alloca()
		set(ldflags "${ldflags} -static-libgcc -static-libstdc++ -Wl, --large-address-aware --stack,8388608")
	elseif(os STREQUAL "linux")
		#set(sys_libs ${sys_libs} dl OpenSSL::SSL OpenSSL::Crypto rt)
		set(sys_libs ${sys_libs} dl rt)
	elseif(os STREQUAL "freebsd")
		find_library(EXECINFO_LIBRARIES execinfo /usr/lib /usr/local/lib)
		#set(sys_libs ${sys_libs} OpenSSL::SSL OpenSSL::Crypto rt ${EXECINFO_LIBRARIES})
		set(sys_libs ${sys_libs} rt ${EXECINFO_LIBRARIES})
	endif()
elseif(MSVC)
	add_compile_options(/MP) # parallel build (use all cores, or as many as configured in VS)
	add_compile_options(/W3) # TODO: was /W4, caused trouble with VS2019 (and/or its integrated CMake? or only HarrieVG's setup?)

	add_compile_options(/we4840) # treat as error when passing a class to a vararg-function (probably printf-like)
	add_compile_options(/we4306 /we4311 /we4312 /we4302)	# treat several kinds of truncating int<->pointer conversions as errors (for more 64bit-safety)
	# ignore the following warnings:
	add_compile_options(/wd4100) # unreferenced formal parameter
	add_compile_options(/wd4127) # conditional expression is constant
	add_compile_options(/wd4244) # possible loss of data
	add_compile_options(/wd4245) # signed/unsigned mismatch
	add_compile_options(/wd4267) # possible loss of data
	add_compile_options(/wd4714) # 'function' marked as __forceinline not inlined
	add_compile_options(/wd4996) # 'function': was declared deprecated
	add_compile_options(/wd4068) # unknown pragma
	add_compile_options(/wd4458) # declaration of 'variable' hides class member
	add_compile_options(/wd4018) # 'math symbol': signed/unsigned mismatch
	add_compile_options(/wd4018) # 'math symbol': signed/unsigned mismatch
	add_compile_options(/wd26495) # variable 'variable' is uninitialized. Always initialize a member variable (type.6).
	add_compile_options(/wd4146) # unary minus operator applied to unsigned type, result still unsigned
	add_compile_options(/wd4805) # 'symbol' unsafe mix of type 'type a' and type 'type b' in operation
	add_compile_options(/wd4273) # 'function': inconsistent dll linkage
	add_compile_options(/wd4838) # conversion from 'type' to 'type' requires a narrowing conversion

	# Ignore warnings for Dedicate server build
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ignore:4286 /ignore:4217")
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /ignore:4286 /ignore:4217")
	set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} /ignore:4286 /ignore:4217")


	add_definitions(-D_ALLOW_KEYWORD_MACROS) # because of the "#define private public" and "#define protected public" in TypeInfo.cpp

	set(CMAKE_C_FLAGS_DEBUG "-D_DEBUG /Od /Zi /MDd")
	set(CMAKE_C_FLAGS_RELEASE "/Ox /Oy /MD")
	set(CMAKE_C_FLAGS_RELWITHDEBINFO "/Ox /Oy /Zi /MD")
	set(CMAKE_C_FLAGS_MINSIZEREL "/Ox /Oy /Os /MD")

	# use 8MB stack instead of 1MB, so big models don't overflow it with _alloca()
	set(ldflags "${ldflags} /STACK:8388608")

	# let the 32bit app use more than 2gb of ram
	set(ldflags "${ldflags} /LARGEADDRESSAWARE")

	# Don't let the linker generate manifest files for shared dlls
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /MANIFEST:NO")
else()
	message(FATAL_ERROR "Unsupported compiler")
endif()

# Drop the binaries in the output/${os} folder
set(OUTPUT_FOLDER ${CMAKE_SOURCE_DIR}/../output/${os})

set(CMAKE_CXX_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_C_FLAGS_RELWITHDEBINFO})

# disable assertions in release builds
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG ${CMAKE_C_FLAGS_RELEASE}")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-DNDEBUG ${CMAKE_C_FLAGS_MINSIZEREL}")

# mingw and msvc
if(WIN32)
	add_definitions(-DWINVER=0x0501)
	add_definitions(-D_WIN32_WINNT=0x0501)

	set(sys_libs ${sys_libs}
		winmm
		iphlpapi
		wsock32
		ole32
	)
endif()

# optimize this source file even in debug builds, to make speed (esp. loading times) more bearable
# used for libs integrated here like stb_* and miniz(ip)
if(CMAKE_MAJOR_VERSION LESS 3 OR (CMAKE_MAJOR_VERSION EQUAL 3 AND CMAKE_MINOR_VERSION LESS 11))
	function(always_optimize_sourcefile srcfilename)
		if(MSVC)
			set_source_files_properties(${srcfilename} PROPERTIES COMPILE_FLAGS "/Ox")
		else()
			set_source_files_properties(${srcfilename} PROPERTIES COMPILE_FLAGS "-O2")
		endif()
	endfunction()
else()
	function(always_optimize_sourcefile srcfilename)
		if(MSVC)
			set_source_files_properties(${srcfilename} PROPERTIES COMPILE_OPTIONS "/Ox")
		else()
			set_source_files_properties(${srcfilename} PROPERTIES COMPILE_OPTIONS "-O2")
		endif()
	endfunction()
endif()

set(bindir		"${CMAKE_INSTALL_FULL_BINDIR}")
set(libdir		"${CMAKE_INSTALL_FULL_LIBDIR}/prey06")
set(datadir	"${CMAKE_INSTALL_FULL_DATADIR}/prey06")

configure_file(
	"${CMAKE_SOURCE_DIR}/config.h.in"
	"${CMAKE_BINARY_DIR}/config.h"
)

if(NOT MSVC)
	message(STATUS "Building ${CMAKE_BUILD_TYPE} for ${os}-${cpu}")
endif()

if(NOT WIN32)
	message(STATUS "The install target will use the following directories:")
	message(STATUS "  Binary directory:  ${bindir}")
	message(STATUS "  Library directory: ${libdir}")
	message(STATUS "  Data directory:    ${datadir}")
endif()

set(src_renderer
	renderer/Cinematic.cpp
	renderer/DeviceContext.cpp
	renderer/GuiModel.cpp
	renderer/Image_files.cpp
	renderer/Image_init.cpp
	renderer/Image_load.cpp
	renderer/Image_process.cpp
	renderer/Image_program.cpp
	renderer/Interaction.cpp
	renderer/Material.cpp
	renderer/MegaTexture.cpp
	renderer/Model.cpp
	renderer/ModelDecal.cpp
	renderer/ModelManager.cpp
	renderer/ModelOverlay.cpp
	renderer/Model_beam.cpp
	renderer/Model_hhBeam.cpp
	renderer/Model_ase.cpp
	renderer/Model_liquid.cpp
	renderer/Model_lwo.cpp
	renderer/Model_md3.cpp
	renderer/Model_md5.cpp
	renderer/Model_prt.cpp
	renderer/Model_sprite.cpp
	renderer/RenderEntity.cpp
	renderer/RenderSystem.cpp
	renderer/RenderSystem_init.cpp
	renderer/RenderWorld.cpp
	renderer/RenderWorld_demo.cpp
	renderer/RenderWorld_load.cpp
	renderer/RenderWorld_portals.cpp
	renderer/VertexCache.cpp
	renderer/draw_arb2.cpp
	renderer/draw_common.cpp
	renderer/stblib_impls.c
	renderer/tr_backend.cpp
	renderer/tr_deform.cpp
	renderer/tr_font.cpp
	renderer/tr_guisurf.cpp
	renderer/tr_light.cpp
	renderer/tr_lightrun.cpp
	renderer/tr_main.cpp
	renderer/tr_orderIndexes.cpp
	renderer/tr_polytope.cpp
	renderer/tr_render.cpp
	renderer/tr_rendertools.cpp
	renderer/tr_shadowbounds.cpp
	renderer/tr_stencilshadow.cpp
	renderer/tr_subview.cpp
	renderer/tr_trace.cpp
	renderer/tr_trisurf.cpp
	renderer/tr_turboshadow.cpp
)

# Don't use precompiled headers for renderer/stblib_impls.c source.
set_source_files_properties(
	renderer/stblib_impls.c
	PROPERTIES
	SKIP_PRECOMPILE_HEADERS ON
	LANGUAGE C
)

always_optimize_sourcefile(renderer/stblib_impls.c)

add_globbed_headers(src_renderer "renderer")

set(src_framework
	framework/CVarSystem.cpp
	framework/CmdSystem.cpp
	framework/Common.cpp
	framework/Compressor.cpp
	framework/Console.cpp
	framework/DemoFile.cpp
	framework/DeclAF.cpp
	framework/DeclEntityDef.cpp
	framework/DeclFX.cpp
	framework/DeclManager.cpp
	framework/DeclParticle.cpp
	framework/DeclPreyBeam.cpp
	framework/DeclSkin.cpp
	framework/DeclTable.cpp
	framework/EditField.cpp
	framework/EventLoop.cpp
	framework/File.cpp
	framework/FileSystem.cpp
	framework/KeyInput.cpp
	framework/UsercmdGen.cpp
	framework/Session_menu.cpp
	framework/Session.cpp
	framework/async/AsyncClient.cpp
	framework/async/AsyncNetwork.cpp
	framework/async/AsyncServer.cpp
	framework/async/MsgChannel.cpp
	framework/async/NetworkSystem.cpp
	framework/async/ServerScan.cpp
	framework/miniz/miniz.c
	framework/minizip/ioapi.c
	framework/minizip/unzip.cpp
)

# Don't use precompiled headers for miniz/minizip sources.
set_source_files_properties(
	framework/miniz/miniz.c
	framework/minizip/ioapi.c
	PROPERTIES
	SKIP_PRECOMPILE_HEADERS ON
	LANGUAGE C
)

always_optimize_sourcefile(framework/miniz/miniz.c)
always_optimize_sourcefile(framework/minizip/ioapi.c)
always_optimize_sourcefile(framework/minizip/unzip.cpp)

add_globbed_headers(src_framework "framework")

set(src_cm
	cm/CollisionModel_contacts.cpp
	cm/CollisionModel_contents.cpp
	cm/CollisionModel_debug.cpp
	cm/CollisionModel_files.cpp
	cm/CollisionModel_load.cpp
	cm/CollisionModel_rotate.cpp
	cm/CollisionModel_trace.cpp
	cm/CollisionModel_translate.cpp
)

add_globbed_headers(src_cm "cm")

set(src_dmap
	tools/compilers/dmap/dmap.cpp
	tools/compilers/dmap/facebsp.cpp
	tools/compilers/dmap/leakfile.cpp
	tools/compilers/dmap/map.cpp
	tools/compilers/dmap/optimize.cpp
	tools/compilers/dmap/output.cpp
	tools/compilers/dmap/portals.cpp
	tools/compilers/dmap/shadowopt3.cpp
	tools/compilers/dmap/tritjunction.cpp
	tools/compilers/dmap/tritools.cpp
	tools/compilers/dmap/ubrush.cpp
	tools/compilers/dmap/usurface.cpp
)

add_globbed_headers(src_dmap "tools/compilers/dmap")

set(src_aas
	tools/compilers/aas/AASBuild.cpp
	tools/compilers/aas/AASBuild_file.cpp
	tools/compilers/aas/AASBuild_gravity.cpp
	tools/compilers/aas/AASBuild_ledge.cpp
	tools/compilers/aas/AASBuild_merge.cpp
	tools/compilers/aas/AASCluster.cpp
	tools/compilers/aas/AASFile.cpp
	tools/compilers/aas/AASFile_optimize.cpp
	tools/compilers/aas/AASFile_sample.cpp
	tools/compilers/aas/AASReach.cpp
	tools/compilers/aas/AASFileManager.cpp
	tools/compilers/aas/Brush.cpp
	tools/compilers/aas/BrushBSP.cpp
)

add_globbed_headers(src_aas "tools/compilers/aas")

set(src_roq
	tools/compilers/roqvq/NSBitmapImageRep.cpp
	tools/compilers/roqvq/codec.cpp
	tools/compilers/roqvq/roq.cpp
	tools/compilers/roqvq/roqParam.cpp
)

add_globbed_headers(src_roq "tools/compilers/roqvq")

set(src_renderbump
	tools/compilers/renderbump/renderbump.cpp
)

add_globbed_headers(src_renderbump "tools/compilers/renderbump")

set(src_snd
	sound/snd_cache.cpp
	sound/snd_decoder.cpp
	sound/snd_efxfile.cpp
	sound/snd_emitter.cpp
	sound/snd_shader.cpp
	sound/snd_system.cpp
	sound/snd_wavefile.cpp
	sound/snd_world.cpp
	sound/stbvorbis_impl.c
)

# Don't use precompiled headers for sound/stbvorbis_impl.c source.
set_source_files_properties(
	sound/stbvorbis_impl.c
	PROPERTIES
	SKIP_PRECOMPILE_HEADERS ON
	LANGUAGE C
)

always_optimize_sourcefile(sound/stbvorbis_impl.c)

add_globbed_headers(src_snd "sound")

set(src_ui
	ui/BindWindow.cpp
	ui/ChoiceWindow.cpp
	ui/EditWindow.cpp
	ui/FieldWindow.cpp
	ui/GuiScript.cpp
	ui/ListGUI.cpp
	ui/ListWindow.cpp
	ui/MarkerWindow.cpp
	ui/RegExp.cpp
	ui/RenderWindow.cpp
	ui/SimpleWindow.cpp
	ui/SliderWindow.cpp
	ui/TabContainerWindow.cpp
	ui/TabWindow.cpp
	ui/UserInterface.cpp
	ui/Window.cpp
	ui/Winvar.cpp
)

add_globbed_headers(src_ui "ui")

set(src_idlib
	idlib/bv/Bounds.cpp
	idlib/bv/Frustum.cpp
	idlib/bv/Sphere.cpp
	idlib/bv/Box.cpp
	idlib/geometry/DrawVert.cpp
	idlib/geometry/Winding2D.cpp
	idlib/geometry/Surface_SweptSpline.cpp
	idlib/geometry/Winding.cpp
	idlib/geometry/Surface.cpp
	idlib/geometry/Surface_Patch.cpp
	idlib/geometry/TraceModel.cpp
	idlib/geometry/JointTransform.cpp
	idlib/hashing/CRC8.cpp
	idlib/hashing/CRC16.cpp
	idlib/hashing/CRC32.cpp
	idlib/hashing/Honeyman.cpp
	idlib/hashing/MD4.cpp
	idlib/hashing/MD5.cpp
	idlib/math/Angles.cpp
	idlib/math/Lcp.cpp
	idlib/math/Math.cpp
	idlib/math/Matrix.cpp
	idlib/math/Ode.cpp
	idlib/math/Plane.cpp
	idlib/math/Pluecker.cpp
	idlib/math/Polynomial.cpp
	idlib/math/prey_math.cpp
	idlib/math/Quat.cpp
	idlib/math/Rotation.cpp
	idlib/math/Simd.cpp
	idlib/math/Simd_Generic.cpp
	idlib/math/Simd_MMX.cpp
	idlib/math/Simd_SSE.cpp
	idlib/math/Simd_SSE2.cpp
	idlib/math/Simd_SSE3.cpp
	idlib/math/Vector.cpp
	idlib/BitMsg.cpp
	idlib/LangDict.cpp
	idlib/Lexer.cpp
	idlib/Lib.cpp
	idlib/containers/HashIndex.cpp
	idlib/Dict.cpp
	idlib/Str.cpp
	idlib/Parser.cpp
	idlib/precompiled.cpp
	idlib/MapFile.cpp
	idlib/CmdArgs.cpp
	idlib/Token.cpp
	idlib/Base64.cpp
	idlib/Timer.cpp
	idlib/Heap.cpp
)

add_globbed_headers(src_idlib "idlib")

set(src_game
	game/Actor.cpp
	game/AF.cpp
	game/AFEntity.cpp
	game/BrittleFracture.cpp
	game/Camera.cpp
	game/Entity.cpp
	game/EntityAdditions.cpp
	game/Fx.cpp
	game/GameEdit.cpp
	game/Game_local.cpp
	game/Game_network.cpp
	game/IK.cpp
	game/Item.cpp
	game/Light.cpp
	game/Misc.cpp
	game/Moveable.cpp
	game/Mover.cpp
	game/MultiplayerGame.cpp
	game/Player.cpp
	game/PlayerIcon.cpp
	game/PlayerView.cpp
	game/Projectile.cpp
	game/Pvs.cpp
	game/SecurityCamera.cpp
	game/SmokeParticles.cpp
	game/Sound.cpp
	game/Target.cpp
	game/Trigger.cpp
	game/Weapon.cpp
	game/WorldSpawn.cpp
	game/ai/AAS.cpp
	game/ai/AAS_debug.cpp
	game/ai/AAS_pathing.cpp
	game/ai/AAS_routing.cpp
	game/ai/AI.cpp
	game/ai/AI_events.cpp
	game/ai/AI_pathing.cpp
	game/anim/Anim.cpp
	game/anim/Anim_Blend.cpp
	game/anim/Anim_Testmodel.cpp
	game/gamesys/Class.cpp
	game/gamesys/DebugGraph.cpp
	game/gamesys/Event.cpp
	game/gamesys/SaveGame.cpp
	game/gamesys/SysCmds.cpp
	game/gamesys/SysCvar.cpp
	game/physics/Clip.cpp
	game/physics/Force.cpp
	game/physics/Force_Constant.cpp
	game/physics/Force_Drag.cpp
	game/physics/Force_Field.cpp
	game/physics/Force_Spring.cpp
	game/physics/Physics.cpp
	game/physics/Physics_Actor.cpp
	game/physics/Physics_AF.cpp
	game/physics/Physics_Base.cpp
	game/physics/Physics_Monster.cpp
	game/physics/Physics_Parametric.cpp
	game/physics/Physics_Player.cpp
	game/physics/Physics_PreyPlayer.cpp
	game/physics/Physics_RigidBody.cpp
	game/physics/Physics_Static.cpp
	game/physics/Physics_StaticMulti.cpp
	game/physics/Push.cpp
	game/script/Script_Compiler.cpp
	game/script/Script_Interpreter.cpp
	game/script/Script_Program.cpp
	game/script/Script_Thread.cpp
)

add_globbed_headers(src_game "game")

set(src_prey
	Prey/ai_centurion.cpp
	Prey/ai_crawler.cpp
	Prey/ai_creaturex.cpp
	Prey/ai_droid.cpp
	Prey/ai_gasbag_simple.cpp
	Prey/ai_harvester_simple.cpp
	Prey/ai_hunter_simple.cpp
	Prey/ai_inspector.cpp
	Prey/ai_jetpack_harvester_simple.cpp
	Prey/ai_keeper_simple.cpp
	Prey/ai_mutate.cpp
	Prey/ai_mutilatedhuman.cpp
	Prey/ai_passageway.cpp
	Prey/ai_possessedTommy.cpp
	Prey/ai_reaction.cpp
	Prey/ai_spawncase.cpp
	Prey/ai_speech.cpp
	Prey/ai_sphereboss.cpp
	Prey/anim_baseanim.cpp
	Prey/force_converge.cpp
	Prey/game_afs.cpp
	Prey/game_alarm.cpp
	Prey/game_anim.cpp
	Prey/game_animatedentity.cpp
	Prey/game_animatedgui.cpp
	Prey/game_animator.cpp
	Prey/game_animBlend.cpp
	Prey/game_animDriven.cpp
	Prey/game_arcadegame.cpp
	Prey/game_barrel.cpp
	Prey/game_bindController.cpp
	Prey/game_blackjack.cpp
	Prey/game_cards.cpp
	Prey/game_cilia.cpp
	Prey/game_console.cpp
	Prey/game_damagetester.cpp
	Prey/game_dda.cpp
	Prey/game_deathwraith.cpp
	Prey/game_debrisspawner.cpp
	Prey/game_dock.cpp
	Prey/game_dockedgun.cpp
	Prey/game_door.cpp
	Prey/game_eggspawner.cpp
	Prey/game_energynode.cpp
	Prey/game_entityfx.cpp
	Prey/game_entityspawner.cpp
	Prey/game_events.cpp
	Prey/game_fixedpod.cpp
	Prey/game_forcefield.cpp
	Prey/game_fxinfo.cpp
	Prey/game_gibbable.cpp
	Prey/game_gravityswitch.cpp
	Prey/game_guihand.cpp
	Prey/game_gun.cpp
	Prey/game_hand.cpp
	Prey/game_handcontrol.cpp
	Prey/game_healthbasin.cpp
	Prey/game_healthspore.cpp
	Prey/game_inventory.cpp
	Prey/game_itemautomatic.cpp
	Prey/game_itemcabinet.cpp
	Prey/game_jukebox.cpp
	Prey/game_jumpzone.cpp
	Prey/game_light.cpp
	Prey/game_lightfixture.cpp
	Prey/game_mine.cpp
	Prey/game_misc.cpp
	Prey/game_modeldoor.cpp
	Prey/game_modeltoggle.cpp
	Prey/game_monster_ai.cpp
	Prey/game_monster_ai_events.cpp
	Prey/game_mountedgun.cpp
	Prey/game_moveable.cpp
	Prey/game_mover.cpp
	Prey/game_note.cpp
	Prey/game_organtrigger.cpp
	Prey/game_player.cpp
	Prey/game_playerview.cpp
	Prey/game_pod.cpp
	Prey/game_podspawner.cpp
	Prey/game_poker.cpp
	Prey/game_portal.cpp
	Prey/game_portalframe.cpp
	Prey/game_proxdoor.cpp
	Prey/game_rail.cpp
	Prey/game_railshuttle.cpp
	Prey/game_renderentity.cpp
	Prey/game_safeDeathVolume.cpp
	Prey/game_securityeye.cpp
	Prey/game_shuttle.cpp
	Prey/game_shuttledock.cpp
	Prey/game_shuttletransport.cpp
	Prey/game_skybox.cpp
	Prey/game_slots.cpp
	Prey/game_sphere.cpp
	Prey/game_spherepart.cpp
	Prey/game_spring.cpp
	Prey/game_sunCorona.cpp
	Prey/game_talon.cpp
	Prey/game_targetproxy.cpp
	Prey/game_targets.cpp
	Prey/game_trackmover.cpp
	Prey/game_trigger.cpp
	Prey/game_tripwire.cpp
	Prey/game_utils.cpp
	Prey/game_vehicle.cpp
	Prey/game_vomiter.cpp
	Prey/game_weaponHandState.cpp
	Prey/game_woundmanager.cpp
	Prey/game_wraith.cpp
	Prey/game_zone.cpp
	Prey/particles_particles.cpp
	Prey/physics_delta.cpp
	Prey/physics_preyai.cpp
	Prey/physics_preyparametric.cpp
	Prey/physics_simple.cpp
	Prey/physics_vehicle.cpp
	Prey/prey_animator.cpp
	Prey/prey_baseweapons.cpp
	Prey/prey_beam.cpp
	Prey/prey_bonecontroller.cpp
	Prey/prey_camerainterpolator.cpp
	Prey/prey_firecontroller.cpp
	Prey/prey_game.cpp
	Prey/prey_items.cpp
	Prey/prey_liquid.cpp
	Prey/prey_local.cpp
	Prey/prey_projectile.cpp
	Prey/prey_projectileautocannon.cpp
	Prey/prey_projectilebounce.cpp
	Prey/prey_projectilebug.cpp
	Prey/prey_projectilebugtrigger.cpp
	Prey/prey_projectilecocoon.cpp
	Prey/prey_projectilecrawlergrenade.cpp
	Prey/prey_projectilefreezer.cpp
	Prey/prey_projectilegasbagpod.cpp
	Prey/prey_projectilehiderweapon.cpp
	Prey/prey_projectilemine.cpp
	Prey/prey_projectilerifle.cpp
	Prey/prey_projectilerocketlauncher.cpp
	Prey/prey_projectileshuttle.cpp
	Prey/prey_projectilesoulcannon.cpp
	Prey/prey_projectilespiritarrow.cpp
	Prey/prey_projectiletracking.cpp
	Prey/prey_projectiletrigger.cpp
	Prey/prey_projectilewrench.cpp
	Prey/prey_script_thread.cpp
	Prey/prey_sound.cpp
	Prey/prey_soundleadincontroller.cpp
	Prey/prey_spiritbridge.cpp
	Prey/prey_spiritproxy.cpp
	Prey/prey_spiritsecret.cpp
	Prey/prey_vehiclefirecontroller.cpp
	Prey/prey_weapon.cpp
	Prey/prey_weaponautocannon.cpp
	Prey/prey_weaponcrawlergrenade.cpp
	Prey/prey_weaponfirecontroller.cpp
	Prey/prey_weaponhider.cpp
	Prey/prey_weaponrifle.cpp
	Prey/prey_weaponrocketlauncher.cpp
	Prey/prey_weaponsoulstripper.cpp
	Prey/prey_weaponspiritbow.cpp
	Prey/sys_debugger.cpp
	Prey/sys_preycmds.cpp
)

add_globbed_headers(src_prey "Prey")

set(src_debuggerServer
	tools/debugger/DebuggerBreakpoint.h
	tools/debugger/DebuggerBreakpoint.cpp
	tools/debugger/DebuggerServer.h
	tools/debugger/DebuggerServer.cpp
	tools/debugger/DebuggerScript.h
	tools/debugger/DebuggerScript.cpp
	tools/debugger/DebuggerMessages.h
	tools/debugger/debugger.cpp
)

set(src_core
	${src_aas_file}
	${src_renderer}
	${src_framework}
	${src_cm}
	${src_dmap}
	${src_aas}
	${src_roq}
	${src_renderbump}
	${src_snd}
	${src_ui}
	${src_tools}
)

set(src_stub_openal		sys/stub/openal_stub.cpp)
set(src_stub_gl			sys/stub/stub_gl.cpp)


set(src_tools
	tools/guied/GEWindowWrapper_stub.cpp
)

# TODO: add_globbed_headers(src_tools "tools/guied")

# Begin normal (MFC) tools code
if (TOOLS AND MFC_TOOLS AND MSVC)
	set(ALLOWRESOURCES ON)
	# Common files.
	file(GLOB src_comafx "tools/comafx/*.cpp")
	add_globbed_headers(src_comafx "tools/comafx")
	file(GLOB_RECURSE src_tools_common "tools/common/*.cpp")
	add_globbed_headers(src_tools_common "tools/common")

	#Compilers.
	# DG: this is always added, why add it again? file(GLOB src_compiler_renderbump "neo/tools/compilers/renderbump/*.cpp")
	file(GLOB src_compiler_roqvq	  "tools/compilers/roqvq/*.cpp") # FIXME: some of those were added before
	add_globbed_headers(src_compiler_roqvq "tools/compilers/roqvq")
	#tools
	# Articulated Frame editor.
	file(GLOB src_afeditor "tools/af/*.cpp")
	add_globbed_headers(src_afeditor "tools/af")
	# Declaration editor
	file(GLOB src_decleditor "tools/decl/*.cpp")
	add_globbed_headers(src_decleditor "tools/decl")
	# GUI Editor.
	file(GLOB src_gui_editor "tools/guied/*.cpp")
	add_globbed_headers(src_gui_editor "tools/guied")
	# Material editor
	file(GLOB src_materialeditor "tools/materialeditor/*.cpp")
	add_globbed_headers(src_materialeditor "tools/materialeditor")
	# Particle Editor
	file(GLOB src_particleeditor "tools/particle/*.cpp")
	add_globbed_headers(src_particleeditor "tools/particle")
	# Radiant (Map editor)
	file(GLOB src_map_editor "tools/radiant/*.cpp")
	add_globbed_headers(src_map_editor "tools/radiant")
	# Script editor
	file(GLOB src_script_editor "tools/script/*.cpp")
	add_globbed_headers(src_script_editor "tools/script")
	# Script Debugger
	set(src_debuggerClient
		tools/debugger/DebuggerClient.h
		tools/debugger/DebuggerClient.cpp
		tools/debugger/DebuggerApp.h
		tools/debugger/DebuggerApp.cpp
		tools/debugger/DebuggerQuickWatchDlg.h
		tools/debugger/DebuggerQuickWatchDlg.cpp
		tools/debugger/DebuggerWindow.h
		tools/debugger/DebuggerWindow.cpp
		tools/debugger/DebuggerFindDlg.h
		tools/debugger/DebuggerFindDlg.cpp
	)
	set(src_script_debugger
		${src_debuggerServer}
		${src_debuggerClient}
	)
	# sound editor?
	file(GLOB src_sound_editor "tools/sound/*.cpp")
	add_globbed_headers(src_sound_editor "tools/sound")


	# The numerous tools in a nice list.
	list(APPEND src_editor_tools
		${src_comafx}
		${src_tools_common}
		${src_compiler_renderbump}
		${src_afeditor}
		${src_decleditor}
		${src_gui_editor}
		${src_materialeditor}
		${src_particleeditor}
		${src_map_editor}
		${src_script_editor}
		${src_sound_editor}
		${src_script_debugger}
		"tools/edit_public.h"
		)
	SET(CMAKE_MFC_FLAG 2)
	set(TOOLS_DEFINES "ID_ALLOW_TOOLS;_AFXDLL")
else()
	set(src_editor_tools "tools/edit_stub.cpp" "tools/edit_public.h")
	list(APPEND src_editor_tools
		${src_debuggerServer}
	)
endif()

if(WIN32)

	set(src_sys_base
		sys/cpu.cpp
		sys/threads.cpp
		sys/events.cpp
		sys/sys_local.cpp
		sys/win32/win_input.cpp
		sys/win32/win_main.cpp
		sys/win32/win_net.cpp
		sys/win32/win_shared.cpp
		sys/win32/win_syscon.cpp
		sys/win32/SDL_win32_main.c
	)

	# Don't use precompiled headers for our SDL Win32 c sources.
	set_source_files_properties(
		sys/win32/SDL_win32_main.c
		PROPERTIES
		SKIP_PRECOMPILE_HEADERS ON
		LANGUAGE C
	)

	# adding the few relevant headers in sys/ manually..
	set(src_sys_base ${src_sys_base}
		sys/platform.h
		sys/sys_local.h
		sys/sys_public.h
		sys/win32/win_local.h
	)

	set(src_sys_core
		sys/glimp.cpp
		"sys/win32/rc/windows_default.manifest"
		"sys/win32/rc/windows_dpiaware.manifest"
		"sys/win32/rc/windows_visualstyles.manifest"
	)

	if(TOOLS)
		file(GLOB src_sys_tools_images "sys/win32/rc/res/*.bmp" "sys/win32/rc/res/*.ico" "sys/win32/rc/res/*.cur")
		set(src_sys_core
			${src_sys_core}
			${src_sys_tools_images}
			"sys/win32/rc/prey_tools.rc"
			"sys/win32/rc/resource.h"
		)
	else()
		set(src_sys_core
			${src_sys_core}
			"sys/win32/rc/res/prey.ico"
			"sys/win32/rc/prey.rc"
			"sys/win32/rc/prey_resource.h"
		)
	endif()

else()
	set(src_sys_base
		sys/cpu.cpp
		sys/threads.cpp
		sys/events.cpp
		sys/sys_local.cpp
		sys/posix/posix_net.cpp
		sys/posix/posix_main.cpp
		sys/linux/main.cpp
	)

	set(src_sys_core
		sys/glimp.cpp
	)
endif()

include_directories(${CMAKE_BINARY_DIR})
include_directories(${CMAKE_SOURCE_DIR})

# ================================= Idlib ======================================

add_library(idlib STATIC ${src_idlib})
if(D3_COMPILER_IS_GCC_OR_CLANG AND NOT MINGW)
	set_target_properties(idlib PROPERTIES COMPILE_FLAGS "-fPIC")
endif()

if (MSVC)
	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX neo FILES ${src_idlib})
endif()

target_include_directories(idlib PRIVATE "${CMAKE_SOURCE_DIR}/idlib")
target_precompile_headers(idlib PRIVATE "${CMAKE_SOURCE_DIR}/idlib/precompiled.h")

set_target_properties(idlib PROPERTIES FOLDER "libs")

# ================================= Engine ======================================

if(HARDLINK_GAME)
	if(BASE)
		set(src_core ${src_core} ${src_game} ${src_prey})
		set(game_includes "${CMAKE_SOURCE_DIR}/game" "${CMAKE_SOURCE_DIR}/Prey")
	endif()
endif()

if(ENGINE)
	add_executable(${ENGINE_BINARY} WIN32
		${src_core}
		${src_sys_base}
		${src_sys_core}
		${src_editor_tools}
	)

	if(MSVC)
		# set MSVC default debug directory on executable
		set_target_properties(${ENGINE_BINARY} PROPERTIES VS_STARTUP_PROJECT ${ENGINE_BINARY})
		set_target_properties(${ENGINE_BINARY} PROPERTIES VS_DEBUGGER_COMMAND "${OUTPUT_FOLDER}/${ENGINE_BINARY}.exe")
		set_target_properties(${ENGINE_BINARY} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${OUTPUT_FOLDER}")
		set_target_properties(${ENGINE_BINARY} PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "+set developer 1")

		source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX neo FILES ${src_core} ${src_sys_base} ${src_sys_core} ${src_editor_tools})
	endif()

	target_precompile_headers(${ENGINE_BINARY} PRIVATE "${CMAKE_SOURCE_DIR}/idlib/precompiled.h")

	target_include_directories(${ENGINE_BINARY} PRIVATE "${CMAKE_SOURCE_DIR}/idlib")

	if(HARDLINK_GAME)
		set_target_properties(${ENGINE_BINARY} PROPERTIES COMPILE_DEFINITIONS "HUMANHEAD;${TOOLS_DEFINES}")
		target_include_directories(${ENGINE_BINARY} PRIVATE ${game_includes})
	else()
		set_target_properties(${ENGINE_BINARY} PROPERTIES COMPILE_DEFINITIONS "HUMANHEAD;__DOOM_DLL__;${TOOLS_DEFINES}")
	endif()

	set_target_properties(${ENGINE_BINARY} PROPERTIES LINK_FLAGS "${ldflags}")
	set_target_properties(${ENGINE_BINARY} PROPERTIES RUNTIME_OUTPUT_DIRECTORY $<1:>${OUTPUT_FOLDER})
	set_target_properties(${ENGINE_BINARY} PROPERTIES FOLDER "exes")

	target_link_libraries(${ENGINE_BINARY}
		idlib
		${sys_thirdparty_libs}
		${sys_libs}
	)

	if(NOT WIN32)
		install(TARGETS ${ENGINE_BINARY}
				RUNTIME DESTINATION "${bindir}"
				LIBRARY DESTINATION "${libdir}"
				ARCHIVE DESTINATION "${libdir}"
		)
	endif()
endif()

# ============================== Dedicate Server ================================

if(DEDICATED)
	add_executable(${DEDICATED_BINARY} WIN32
		${src_core}
		${src_stub_openal}
		${src_stub_gl}
		${src_sys_base}
		${src_debuggerServer}
	)

	if(MSVC)
		source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX neo FILES ${src_core} ${src_sys_base} ${src_stub_openal} ${src_stub_gl} ${src_debuggerServer})
	endif()

	target_precompile_headers(${DEDICATED_BINARY} PRIVATE "${CMAKE_SOURCE_DIR}/idlib/precompiled.h")

	target_include_directories(${DEDICATED_BINARY} PRIVATE "${CMAKE_SOURCE_DIR}/idlib")

	if(HARDLINK_GAME)
		set_target_properties(${DEDICATED_BINARY} PROPERTIES COMPILE_DEFINITIONS "HUMANHEAD;ID_DEDICATED")
		target_include_directories(${DEDICATED_BINARY} PRIVATE ${game_includes})
	else()
		set_target_properties(${DEDICATED_BINARY} PROPERTIES COMPILE_DEFINITIONS "HUMANHEAD;ID_DEDICATED;__DOOM_DLL__")
	endif()

	set_target_properties(${DEDICATED_BINARY} PROPERTIES LINK_FLAGS "${ldflags}")
	set_target_properties(${DEDICATED_BINARY} PROPERTIES RUNTIME_OUTPUT_DIRECTORY $<1:>${OUTPUT_FOLDER})
	set_target_properties(${DEDICATED_BINARY} PROPERTIES FOLDER "exes")

	list(REMOVE_ITEM sys_thirdparty_libs ${OPENAL_LIBRARY})

	target_link_libraries(${DEDICATED_BINARY}
		idlib
		${sys_thirdparty_libs}
		${sys_libs}
	)

	if(NOT WIN32)
		install(TARGETS ${DEDICATED_BINARY}
				RUNTIME DESTINATION "${bindir}"
				LIBRARY DESTINATION "${libdir}"
				ARCHIVE DESTINATION "${libdir}"
		)
	endif()
endif()

# ============================== Prey ================================

if(BASE AND NOT HARDLINK_GAME)
	add_library(base SHARED ${src_game} ${src_prey})

	if (MSVC)
		source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX neo FILES ${src_game} ${src_prey})
	endif()

	target_precompile_headers(base PRIVATE "${CMAKE_SOURCE_DIR}/idlib/precompiled.h")

	target_include_directories(base PRIVATE "${CMAKE_SOURCE_DIR}/idlib")

	set_target_properties(base PROPERTIES PREFIX "")
	set_target_properties(base PROPERTIES COMPILE_DEFINITIONS "GAME_DLL;HUMANHEAD")
	target_include_directories(base PRIVATE "${CMAKE_SOURCE_DIR}/game" "${CMAKE_SOURCE_DIR}/Prey")
	set_target_properties(base PROPERTIES LINK_FLAGS "${ldflags}")
	set_target_properties(base PROPERTIES INSTALL_NAME_DIR "@executable_path")
	target_link_libraries(base idlib)
	set_target_properties(base PROPERTIES RUNTIME_OUTPUT_DIRECTORY $<1:>${OUTPUT_FOLDER} LIBRARY_OUTPUT_DIRECTORY $<1:>${OUTPUT_FOLDER})
	set_target_properties(base PROPERTIES FOLDER "dlls")

	if(NOT WIN32)
		install(TARGETS base
				RUNTIME DESTINATION "${bindir}"
				LIBRARY DESTINATION "${libdir}"
				ARCHIVE DESTINATION "${libdir}"
		)
	endif()
endif()