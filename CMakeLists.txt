cmake_minimum_required(VERSION 3.10)

project(hmp3 VERSION 5.2.4 LANGUAGES C CXX)

option(BUILD_SHARED_LIBS "Build shared library" OFF)
option(HMP3_BUILD_PROGRAMS "Build hmp3 CLI encoder tool" OFF)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

set(HMP3_SRC_C
    hmp3/src/amodini2.c
    hmp3/src/cnt.c
    hmp3/src/cnts.c
    hmp3/src/detect.c
    hmp3/src/emap.c
    hmp3/src/emdct.c
    hmp3/src/filter2.c
    hmp3/src/hwin.c
    hmp3/src/l3init.c
    hmp3/src/l3math.c
    hmp3/src/l3pack.c
    hmp3/src/mhead.c
    hmp3/src/pcmhpm.c
    hmp3/src/pow34.c
    hmp3/src/sbt.c
    hmp3/src/setup.c
    hmp3/src/spdsmr.c
    hmp3/src/xhead.c
    hmp3/src/xhwin.c
    hmp3/src/xsbt.c
)

set(HMP3_SRC_CPP
    hmp3/src/bitallo.cpp
    hmp3/src/bitallo1.cpp
    hmp3/src/bitallo3.cpp
    hmp3/src/bitalloc.cpp
    hmp3/src/bitallos.cpp
    hmp3/src/bitallosc.cpp
    hmp3/src/mp3enc.cpp
    hmp3/src/srcc.cpp
    hmp3/src/srccf.cpp
    hmp3/src/hmp3_api.cpp
)

add_library(hmp3 ${HMP3_SRC_C} ${HMP3_SRC_CPP})

target_compile_definitions(hmp3
    PRIVATE
        IEEE_FLOAT
        _FILE_OFFSET_BITS=64
)

target_include_directories(hmp3
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/hmp3/src/pub>
        $<INSTALL_INTERFACE:include>
        $<INSTALL_INTERFACE:include/hmp3>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/hmp3/src
)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(hmp3 PRIVATE -O3)
endif()

if(WIN32)
    target_link_libraries(hmp3 PRIVATE m)
endif()

# Installation rules
install(TARGETS hmp3
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES hmp3/src/pub/hmp3.h
    DESTINATION include
)

install(FILES hmp3/src/pub/hmp3.h
    DESTINATION include/hmp3
)

# Pkg-config file generation
set(prefix "${CMAKE_INSTALL_PREFIX}")
set(exec_prefix "${CMAKE_INSTALL_PREFIX}")
set(libdir "${CMAKE_INSTALL_PREFIX}/lib")
set(includedir "${CMAKE_INSTALL_PREFIX}/include")
set(VERSION "${PROJECT_VERSION}")
configure_file(hmp3.pc.in "${CMAKE_CURRENT_BINARY_DIR}/hmp3.pc" @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/hmp3.pc" DESTINATION lib/pkgconfig)

# Optional CLI executable
if(HMP3_BUILD_PROGRAMS)
    add_executable(hmp3_cli hmp3/src/test/tomp3.cpp)
    set_target_properties(hmp3_cli PROPERTIES OUTPUT_NAME hmp3)
    target_link_libraries(hmp3_cli PRIVATE hmp3)
    target_include_directories(hmp3_cli PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/hmp3/src/pub
        ${CMAKE_CURRENT_SOURCE_DIR}/hmp3/src
    )
    install(TARGETS hmp3_cli RUNTIME DESTINATION bin)
endif()
