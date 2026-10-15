# Applies PATCH to the current directory (a git checkout) unless it is applied already: FetchContent runs its
# PATCH_COMMAND again whenever it updates the sources.
execute_process(COMMAND git apply --reverse --check "${PATCH}" RESULT_VARIABLE applied OUTPUT_QUIET ERROR_QUIET)
if(applied EQUAL 0)
    message(STATUS "${PATCH}: already applied")
    return()
endif()
execute_process(COMMAND git apply "${PATCH}" RESULT_VARIABLE result)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "${PATCH} does not apply")
endif()
message(STATUS "${PATCH}: applied")
