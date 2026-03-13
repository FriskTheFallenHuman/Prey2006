function(add_gamepak)
	cmake_parse_arguments(_A "" "GAME_TARGET;REPO_ROOT" "" ${ARGN})

	if(NOT _A_GAME_TARGET)
		message(FATAL_ERROR "GAME_TARGET is required")
	endif()

	# The root, that is it
	if(NOT _A_REPO_ROOT)
		get_filename_component(_A_REPO_ROOT "${CMAKE_SOURCE_DIR}/.." ABSOLUTE)
	endif()

	# OS Name
	if(DEFINED os)
		set(_os "${os}")
	elseif(WIN32)
		set(_os "windows")
	elseif(APPLE)
		set(_os "macosx")
	elseif(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
		set(_os "freebsd")
	else()
		set(_os "linux")
	endif()

	# Hardcode the package names for each IDs, again im too lazy todo this :p
	set(_pkg_windows "game00.pk4")
	set(_pkg_macosx  "game01.pk4")
	set(_pkg_linux   "game02.pk4")
	set(_pkg_freebsd "game03.pk4")
	set(_pkg "${_pkg_${_os}}")

	# Filetype for the game dll (either .dll or .so)
	get_target_property(_out_name "${_A_GAME_TARGET}" OUTPUT_NAME)
	if(WIN32)
		set(_dll "${_out_name}.dll")
	else()
		set(_dll "${_out_name}.so")
	endif()

	set(_out_root "${_A_REPO_ROOT}/output/${_os}")
	set(_out_base "${_out_root}/base")
	set(_out_libs "${_out_root}/libs")
	set(_src_base "${_A_REPO_ROOT}/base")
	set(_src_libs "${_A_REPO_ROOT}/libs")
	set(_pkg_path "${_out_base}/${_pkg}")

	# Hardcode the OS IDs, im too lazy to do this properly :p
	set(_id_windows 0)
	set(_id_macosx  1)
	set(_id_linux   2)
	set(_id_freebsd 3)
	set(_os_id "${_id_${_os}}")

	# The actual heart of this function, i opted to hardcode it here because
	# well you guested :p
	set(_script [=[
file(MAKE_DIRECTORY "@_out_base@")
file(MAKE_DIRECTORY "@_out_libs@")
 
# Zip base/ contents so i can fix for good our pure server issues :p
set(_pak007 "@_out_base@/pak007.pk4")
if(EXISTS "${_pak007}")
	file(REMOVE "${_pak007}")
endif()
 
file(GLOB_RECURSE _base_files LIST_DIRECTORIES false "@_src_base@/*")
set(_base_rel_files)
foreach(_f IN LISTS _base_files)
	get_filename_component(_fname "${_f}" NAME)
	if(_fname STREQUAL "@_dll@" OR _fname STREQUAL "binary.conf")
		continue()
	endif()
	file(RELATIVE_PATH _rel "@_src_base@" "${_f}")
	list(APPEND _base_rel_files "${_rel}")
endforeach()
 
if(_base_rel_files)
	execute_process(
        COMMAND "@CMAKE_COMMAND@" -E tar cf "${_pak007}"
                --format=zip -- ${_base_rel_files}
        WORKING_DIRECTORY "@_src_base@"
        RESULT_VARIABLE _pak007_result)

	if(NOT _pak007_result EQUAL 0)
		message(FATAL_ERROR "Failed to create pak007.pk4")
	endif()
	
	message(STATUS "Created Assets PK4 => ${_pak007}")
else()
	message(STATUS "Couldn't find any files in base/, skipping")
endif()

# Copy libs/ if present
if(EXISTS "@_src_libs@")
	file(COPY "@_src_libs@/" DESTINATION "@_out_libs@")
endif()

# Copy docs
foreach(_doc ".github/Changelog.md" ".github/Configuration.md" ".github/README.md")
	if(EXISTS "@_A_REPO_ROOT@/${_doc}")
		file(COPY "@_A_REPO_ROOT@/${_doc}" DESTINATION "@_out_root@")
	endif()
endforeach()

# Write binary.conf (only if not already placed there by the build)
if(NOT EXISTS "@_out_base@/binary.conf")
	file(WRITE "@_out_base@/binary.conf"
	"// add flags for supported operating systems in this pak
	// one id per line
	// name the file binary.conf and place it in the game pak
	// 0 windows
	// 1 macosx
	// 2 linux
	// 3 freebsd
	@_os_id@")
endif()

# Copy the game dll.
if(NOT EXISTS "${DLL_SRC}")
	message(FATAL_ERROR
		"Game DLL not found: ${DLL_SRC}\n"
		"Make sure the build completed successfully.")
endif()
file(COPY_FILE "${DLL_SRC}" "@_out_base@/@_dll@" ONLY_IF_DIFFERENT)

# Create the pk4 archive
if(EXISTS "@_pkg_path@")
	file(REMOVE "@_pkg_path@")
endif()
execute_process(
	COMMAND "@CMAKE_COMMAND@" -E tar cf "@_pkg_path@"
			--format=zip -- "binary.conf" "@_dll@"
	WORKING_DIRECTORY "@_out_base@"
	RESULT_VARIABLE _result)

if(NOT _result EQUAL 0)
	message(FATAL_ERROR "Failed To Create PK4 => @_pkg_path@")
endif()

message(STATUS "Created Game PK4 File => @_pkg_path@")

# Clean up staging files and MSVC debug artifacts
file(REMOVE "@_out_base@/binary.conf" "@_out_base@/@_dll@")
foreach(_pat "*.ilk" "*.pdb" "*.lib" "*.exp")
	file(GLOB _leftovers "@_out_base@/${_pat}")
	if(_leftovers)
		file(REMOVE ${_leftovers})
	endif()
endforeach()
]=])

	string(CONFIGURE "${_script}" _script_configured @ONLY)
	set(_script_path "${CMAKE_BINARY_DIR}/PackagePk4_run.cmake")
	file(GENERATE OUTPUT "${_script_path}" CONTENT "${_script_configured}")

	# Install the generated files
	install(FILES "${_out_base}/pak007.pk4" DESTINATION "${CMAKE_INSTALL_DATADIR}/base")
	install(FILES "${_pkg_path}"            DESTINATION "${CMAKE_INSTALL_DATADIR}/base")

	add_custom_target(package_pk4 ALL
		COMMAND "${CMAKE_COMMAND}"
		"-DDLL_SRC=$<TARGET_FILE:${_A_GAME_TARGET}>"
		-P "${_script_path}"
		DEPENDS "${_A_GAME_TARGET}"
		COMMENT "Packaging ${_pkg} for ${_os}"
		VERBATIM)
	set_target_properties(package_pk4 PROPERTIES FOLDER "packaging")

	message(STATUS "Setting Game PK4 For => ${_pkg} (${_os})")
endfunction()
