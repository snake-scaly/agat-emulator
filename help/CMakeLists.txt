if(NOT BUILD_CHM_DOCS)
    return()
endif()

set(TMP_PROJECT_DIR ${CMAKE_CURRENT_BINARY_DIR}/source)
set(OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/release/help)

find_program(HHC hhc)
if(NOT HHC)
    message(FATAL_ERROR "HTML Help Compiler is required. You can set the \
BUILD_CHM_DOCS option to NO to skip CHM documentation generation, or set \
the HHC CMake variable to a path to the hhc executable.")
endif()

# HHC returns zero status on failure and status 1 on success. Need to reverse
# to run as a build step.
set(HHC_WRAPPER ${CMAKE_CURRENT_BINARY_DIR}/hhc-wrapper.cmd)
file(WRITE ${HHC_WRAPPER} "\
@echo off
\"${HHC}\" %*
if ERRORLEVEL 1 (
  exit /b 0
) else (
  exit /b 1
)
")

# HHC does not recognize "C:/" as the path root. Convert to backslashes.
file(TO_NATIVE_PATH ${TMP_PROJECT_DIR} TMP_PROJECT_DIR_DOS)

file(GLOB_RECURSE DEPS *)

# Copy project files to a temporary location.
# This is necessary because the output location is hard-coded in a .hhp project
# but we want it in the build directory.
add_custom_command(
    OUTPUT ${TMP_PROJECT_DIR}/TAG
    COMMAND ${CMAKE_COMMAND} -E
        copy_directory ${CMAKE_CURRENT_SOURCE_DIR} ${TMP_PROJECT_DIR}
    COMMAND ${CMAKE_COMMAND} -E touch ${TMP_PROJECT_DIR}/TAG
    DEPENDS ${DEPS})

add_custom_command(
    OUTPUT ${OUT_DIR}/russian.chm
    COMMAND ${CMAKE_COMMAND} -E make_directory ${OUT_DIR}
    COMMAND ${HHC_WRAPPER} ${TMP_PROJECT_DIR_DOS}/russian.hhp
    MAIN_DEPENDENCY ${TMP_PROJECT_DIR}/TAG)

add_custom_command(
    OUTPUT ${OUT_DIR}/english.chm
    COMMAND ${CMAKE_COMMAND} -E make_directory ${OUT_DIR}
    COMMAND ${HHC_WRAPPER} ${TMP_PROJECT_DIR_DOS}/english.hhp
    MAIN_DEPENDENCY ${TMP_PROJECT_DIR}/TAG)

add_custom_target(docs ALL
    DEPENDS
        ${OUT_DIR}/russian.chm
        ${OUT_DIR}/english.chm)

install(
    FILES
        ${OUT_DIR}/russian.chm
        ${OUT_DIR}/english.chm
    DESTINATION release/help)
