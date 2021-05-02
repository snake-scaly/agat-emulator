cmake_minimum_required(VERSION 3.8)

project(AgatEmulator LANGUAGES C)
set(CMAKE_C_STANDARD 90)

option(COMPRESS_EXECUTABLE "Compress the emulator executable" YES)
option(BUILD_CHM_DOCS "Build CHM documentation" YES)

install(FILES CHANGES CHANGES_ru DESTINATION .)

add_subdirectory(help)
add_subdirectory(src)
