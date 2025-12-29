function( get_git_branch BRANCH_NAME )
    find_package( Git REQUIRED )
    if( GIT_FOUND )
        execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_BRANCH
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        set( ${BRANCH_NAME} ${GIT_BRANCH} PARENT_SCOPE )
    else()
        message( WARNING "Git not found, not setting ${BRANCH_NAME}" )
        set( ${BRANCH_NAME} "" PARENT_SCOPE )
    endif()
endfunction()