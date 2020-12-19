add_definitions(-DUNICODE -D_UNICODE)
file(GLOB SOURCES *.c *.h ../dialogs/dialog.c ../dialogs/resize.c
	../localize.c ../list.c ../msgloop.c langsel.rc)
add_executable(langsel WIN32 ${SOURCES})
target_link_libraries(langsel user32)
install(TARGETS langsel DESTINATION release)
