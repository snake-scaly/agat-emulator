# Resource libraries don't have entry points.
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /noentry")

add_library(russian MODULE russian.rc)
add_library(english MODULE english.rc)

# Tell CMake which linker to use for RC-only libraries.
set_target_properties(russian PROPERTIES LINKER_LANGUAGE C)
set_target_properties(english PROPERTIES LINKER_LANGUAGE C)

install(TARGETS russian english DESTINATION release/lang)
