# vp_program(<target>): what a program needs to link the VectorPie SDK and
# run where it is built. The same in the SDK's own build and in a game using
# the installed SDK (find_package(VectorPie)).
function(vp_program target)
    target_link_libraries(${target} PRIVATE VectorPie::vectorpie)
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
