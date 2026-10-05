cmake_minimum_required(VERSION 3.21)

project(vectorpie_sdk VERSION 0.1.0 LANGUAGES C CXX)

# Builds, from WSL2 Debian 13 (trixie) after tools/setup-wsl.sh:
#   PC (Windows .exe):  cmake --preset pc && cmake --build --preset pc
#   Pi (Linux arm64):   cmake --preset pi && cmake --build --preset pi
# See README.md.

option(VP_WITH_SDL2 "Window output (SDL2). Off = board output only." ON)
option(VP_BUILD_TOOLS "Build the tools (vp-test)." ON)
option(VP_BUILD_EXAMPLES "Build the examples (poc)." ON)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# --- SDL2 ------------------------------------------------------------------

set(VP_SDL2_TARGET "")
if(VP_WITH_SDL2)
    find_package(SDL2 CONFIG QUIET)
    if(TARGET SDL2::SDL2)
        set(VP_SDL2_TARGET SDL2::SDL2)
    else()
        find_package(PkgConfig QUIET)
        if(PKG_CONFIG_FOUND)
            pkg_check_modules(SDL2PC IMPORTED_TARGET sdl2)
            if(SDL2PC_FOUND)
                set(VP_SDL2_TARGET PkgConfig::SDL2PC)
            endif()
        endif()
    endif()
    if(NOT VP_SDL2_TARGET)
        message(FATAL_ERROR
            "SDL2 not found. Run tools/setup-wsl.sh (PC: SDL2's MinGW package in "
            "/opt/sdl2-mingw; Pi: libsdl2-dev:arm64), or configure with "
            "-DVP_WITH_SDL2=OFF for board output only.")
    endif()
endif()

# --- the library -----------------------------------------------------------

add_library(vectorpie STATIC
    src/core/vp_error.cpp
    src/core/vp_paths.cpp
    src/rc/vp_rc.cpp
    src/dvg/vp_dvg.cpp
    src/dvg/vp_serial_posix.cpp
    src/dvg/vp_serial_win.cpp
    src/text/vp_text.cpp
)
if(VP_WITH_SDL2)
    # The window output, the controls and the game loop need SDL2.
    target_sources(vectorpie PRIVATE
        src/dvg/vp_window_sdl.cpp
        src/input/vp_input.cpp
        src/audio/vp_audio.cpp
        src/app/vp_app.cpp)
    target_link_libraries(vectorpie PUBLIC ${VP_SDL2_TARGET})
else()
    target_sources(vectorpie PRIVATE src/dvg/vp_window_none.cpp)
endif()

target_include_directories(vectorpie PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_compile_options(vectorpie PRIVATE -Wall -Wextra)
find_package(Threads REQUIRED)          # the audio streaming thread
target_link_libraries(vectorpie PUBLIC Threads::Threads)
if(UNIX)
    target_link_libraries(vectorpie PUBLIC m)
endif()

# --- programs built on the library -------------------------------------------

# Everything a program needs to link the library and run where it is built.
function(vp_program target)
    target_link_libraries(${target} PRIVATE vectorpie)
    target_compile_options(${target} PRIVATE -Wall -Wextra)
    # The library is C++ inside: link with the C++ driver.
    set_target_properties(${target} PROPERTIES LINKER_LANGUAGE CXX)
    if(MINGW)
        # No libstdc++ / libgcc / winpthread DLLs next to the .exe: -static
        # links all three in (-static-libstdc++ alone still left
        # libwinpthread-1.dll dynamic). SDL2 stays a DLL: it is linked through
        # its import library file, which -static does not change. Only
        # SDL2.dll travels with the .exe, copied next to it here.
        target_link_options(${target} PRIVATE -static)
        if(TARGET SDL2::SDL2)
            get_target_property(sdl_type SDL2::SDL2 TYPE)
            if(sdl_type STREQUAL "SHARED_LIBRARY")
                add_custom_command(TARGET ${target} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                            $<TARGET_FILE:SDL2::SDL2> $<TARGET_FILE_DIR:${target}>)
            endif()
        endif()
    endif()
endfunction()

if(VP_BUILD_TOOLS)
    add_executable(vp-test tools/vp-test/vp_test.c)
    vp_program(vp-test)
endif()

if(VP_BUILD_EXAMPLES)
    add_executable(poc examples/poc/main.c)
    vp_program(poc)
    if(VP_WITH_SDL2)
        add_executable(hello examples/hello/main.c)
        vp_program(hello)
        # Optional local sounds (see examples/hello/README.md): copied next to
        # the executable, where vp_asset_path finds them.
        if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/examples/hello/assets)
            add_custom_command(TARGET hello POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                        ${CMAKE_CURRENT_SOURCE_DIR}/examples/hello/assets
                        $<TARGET_FILE_DIR:hello>/assets)
        endif()
    endif()
endif()

# --- install ---------------------------------------------------------------

include(GNUInstallDirs)
install(TARGETS vectorpie ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/vectorpie DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
if(VP_BUILD_TOOLS)
    install(TARGETS vp-test RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

if(VP_WITH_SDL2)
    set(VP_PC_REQUIRES "sdl2")
else()
    set(VP_PC_REQUIRES "")
endif()
if(UNIX)
    set(VP_PC_LIBS_EXTRA "-lstdc++ -lm")
else()
    set(VP_PC_LIBS_EXTRA "-lstdc++")
endif()
configure_file(cmake/vectorpie.pc.in ${CMAKE_CURRENT_BINARY_DIR}/vectorpie.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/vectorpie.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
