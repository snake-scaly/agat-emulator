set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /noentry")
add_library(russian MODULE russian.rc)
add_library(english MODULE english.rc)
install(TARGETS russian english DESTINATION release/lang)
