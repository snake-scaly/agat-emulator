add_executable(cfgedit cfgedit.c params.c params.h bitmaps.c ../sysconf.c ../streams.c ../localize.c)
target_link_libraries(cfgedit user32 gdi32)
install(TARGETS cfgedit DESTINATION release)
