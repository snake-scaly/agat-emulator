add_library(fdd fdd.c fdd1.c fddaa.c fddliberty.c)
add_executable(fdd1extr fdd1extr.c)
add_executable(genimage genimage.c)
install(TARGETS fdd1extr genimage DESTINATION release)
