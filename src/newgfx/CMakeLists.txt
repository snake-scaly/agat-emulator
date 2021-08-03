file(GLOB SOURCES *.c *.h)
add_library(newgfx ${SOURCES})
target_link_libraries(newgfx logging)
