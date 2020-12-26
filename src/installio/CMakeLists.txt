add_executable(installio installio.c)
target_link_libraries(installio advapi32 user32)
install(TARGETS installio DESTINATION release/drivers)
