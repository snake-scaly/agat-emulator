set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /noentry")
add_library(russian MODULE russian.rc)
add_library(english MODULE english.rc)
install(TARGETS russian english DESTINATION release/lang)

# There seems to be a bug in CMake when generating NMake makefiles for RC files. This line is a work-around.
set(CMAKE_RC_CREATE_SHARED_MODULE ${CMAKE_C_CREATE_SHARED_MODULE})