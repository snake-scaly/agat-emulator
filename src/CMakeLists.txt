if(NOT WIN32)
    message(FATAL_ERROR "This project is Windows-only")
endif(NOT WIN32)


set(SYSLIBS comctl32 user32 gdi32 comdlg32 shlwapi htmlhelp advapi32)
set(LIBS video cpu sound tape joystick fdd softcard clock printer ramcard mouse scsi)

add_definitions(-DKEY_SCANCODES -DDOUBLE_X -DDOUBLE_Y -D_CRT_SECURE_NO_DEPRECATE -D_WIN32_WINNT=0x500)
include_directories(. dialogs)

if(MSVC)
    # VC hack: use static runtime
    foreach(FLAG_VAR CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
            CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
        string(REGEX REPLACE /MD /MT ${FLAG_VAR} "${${FLAG_VAR}}")
    endforeach()
endif(MSVC)

file(GLOB INTERFACE_SOURCES *.c *.h dialogs/*.c dialogs/*.h debug/*.c debug/*.h resource.rc)
add_executable(interface ${INTERFACE_SOURCES})
target_link_libraries(interface ${LIBS} ${SYSLIBS})
set_target_properties(interface PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/release)

set(EMULATOR $<TARGET_FILE_DIR:interface>/emulator${CMAKE_EXECUTABLE_SUFFIX})

if(COMPRESS_EXECUTABLE)
    find_program(UPX upx)

    if(NOT UPX)
        message(FATAL_ERROR "UPX is required to compress the emulator \
executable. You can set the COMPRESS_EXECUTABLE option to NO to skip \
compression, or set the UPX CMake variable to a path to the UPX executable.")
    endif()

    set(COMPRESS_COMMAND COMMAND ${UPX} -9 --compress-icons=0 ${EMULATOR})
endif()

add_custom_command(TARGET interface POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:interface> ${EMULATOR}
    COMMAND editbin /subsystem:windows /release ${EMULATOR}
    ${COMPRESS_COMMAND}
)

install(
    DIRECTORY ../release ../setup
    DESTINATION .
    PATTERN .git EXCLUDE
    PATTERN help EXCLUDE
    PATTERN emulator.ini EXCLUDE)

install(PROGRAMS ${EMULATOR} DESTINATION release)

# libraries
add_subdirectory(clock)
add_subdirectory(cpu)
add_subdirectory(fdd)
add_subdirectory(joystick)
add_subdirectory(lang)
add_subdirectory(logging)
add_subdirectory(mouse)
add_subdirectory(printer)
add_subdirectory(ramcard)
add_subdirectory(scsi)
add_subdirectory(softcard)
add_subdirectory(sound)
add_subdirectory(tape)
add_subdirectory(video)

# tools
add_subdirectory(cfgedit)
add_subdirectory(installio)
add_subdirectory(langsel)
