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
        # Visible to a game that builds the SDK as a subdirectory too (it
        # needs SDL2.dll copied next to its .exe: vp_program).
        get_target_property(vp_sdl2_alias SDL2::SDL2 ALIASED_TARGET)
        get_target_property(vp_sdl2_global SDL2::SDL2 IMPORTED_GLOBAL)
        if(NOT vp_sdl2_alias AND NOT vp_sdl2_global)
            set_target_properties(SDL2::SDL2 PROPERTIES IMPORTED_GLOBAL TRUE)
        endif()
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
    src/core/vp_hiscore.cpp
    src/core/vp_lamps.cpp
    src/rc/vp_rc.cpp
    src/dvg/vp_dvg.cpp
    src/dvg/vp_serial_posix.cpp
    src/dvg/vp_serial_win.cpp
    src/text/vp_text.cpp
    src/3d/vp_3d.cpp
    src/3d/vp_obj.cpp
    src/3d/vp_scene.cpp
)
if(VP_WITH_SDL2)
    # The window output, the controls and the game loop need SDL2.
    target_sources(vectorpie PRIVATE
        src/dvg/vp_window_sdl.cpp
        src/input/vp_input.cpp
        src/audio/vp_audio.cpp
        src/app/vp_app.cpp
        src/app/vp_initials.cpp
        src/app/vp_credits.cpp
        src/app/vp_marquee.cpp
        src/third_party/lodepng/lodepng.cpp
        src/third_party/stb/stb_vorbis.c)
    # lodepng (zlib license, see its header): third-party code, kept as is.
    # stb_vorbis (public domain or MIT, see its end): Ogg Vorbis, kept as is.
    set_source_files_properties(src/third_party/lodepng/lodepng.cpp
                                src/third_party/stb/stb_vorbis.c
        PROPERTIES COMPILE_OPTIONS "-w")
    target_link_libraries(vectorpie PUBLIC ${VP_SDL2_TARGET})
else()
    target_sources(vectorpie PRIVATE src/dvg/vp_window_none.cpp)
endif()

# The name games link, whether they build the SDK or use it installed.
add_library(VectorPie::vectorpie ALIAS vectorpie)

target_include_directories(vectorpie PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_compile_options(vectorpie PRIVATE -Wall -Wextra
    # GCC on arm64 notes that passing std::pair<float, float> changed ABI in
    # GCC 10.1: nothing to do here (no such pair crosses a library boundary).
    $<$<CXX_COMPILER_ID:GNU>:-Wno-psabi>)
find_package(Threads REQUIRED)          # the audio streaming thread
target_link_libraries(vectorpie PUBLIC Threads::Threads)
if(UNIX)
    target_link_libraries(vectorpie PUBLIC m)
endif()

# --- programs built on the library -------------------------------------------

# vp_program(<target>): link a program with the library (installed with it,
# for games using the installed SDK).
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/VectorPieProgram.cmake)
# vp-pack, for a game that builds the SDK as a subdirectory (its vpk target).
set(VECTORPIE_PACK ${CMAKE_CURRENT_SOURCE_DIR}/tools/vp-pack/vp-pack.sh CACHE INTERNAL "vp-pack")

if(VP_BUILD_TOOLS)
    add_executable(vp-test tools/vp-test/vp_test.c)
    vp_program(vp-test)
endif()

if(VP_BUILD_EXAMPLES)
    add_executable(poc examples/poc/main.c)
    vp_program(poc)
    if(VP_WITH_SDL2)
        add_executable(models examples/models/main.c)
        vp_program(models)
        add_executable(hello examples/hello/main.c)
        vp_program(hello)
        add_executable(lamps examples/lamps/main.c)
        vp_program(lamps)
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

# `cmake --install build-pc` / `build-pi`: the library, its headers, a CMake
# package (find_package(VectorPie)) and pkg-config file, vp-pack and the
# cross-compiling toolchain files, in the preset's install prefix
# (~/VectorPie/dist/pc or /pi: one per platform). See README.md.
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
set(VP_CMAKE_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/VectorPie)
install(TARGETS vectorpie EXPORT VectorPieTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY include/vectorpie DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT VectorPieTargets NAMESPACE VectorPie:: DESTINATION ${VP_CMAKE_DIR})
configure_package_config_file(cmake/VectorPieConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/VectorPieConfig.cmake
    INSTALL_DESTINATION ${VP_CMAKE_DIR}
    PATH_VARS CMAKE_INSTALL_BINDIR)
# 0.x: a game asks for the minor version it was written for.
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/VectorPieConfigVersion.cmake
    VERSION ${PROJECT_VERSION} COMPATIBILITY SameMinorVersion)
install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/VectorPieConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/VectorPieConfigVersion.cmake
        cmake/VectorPieProgram.cmake
        cmake/toolchain-mingw-w64.cmake
        cmake/toolchain-pi-arm64.cmake
        DESTINATION ${VP_CMAKE_DIR})
install(PROGRAMS tools/vp-pack/vp-pack.sh DESTINATION ${CMAKE_INSTALL_BINDIR} RENAME vp-pack)
install(FILES LICENSE README.md DESTINATION ${CMAKE_INSTALL_DOCDIR})
install(DIRECTORY docs/ DESTINATION ${CMAKE_INSTALL_DOCDIR}/docs)
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
