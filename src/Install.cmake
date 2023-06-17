target_compile_options(ImGui-SFML PRIVATE     -w)
target_compile_options(sfml-graphics PRIVATE -w)
target_compile_options(sfml-audio PRIVATE    -w)
target_compile_options(sfml-system PRIVATE   -w)

target_link_libraries(EpiSoundSim
    PUBLIC
    ImGui-SFML::ImGui-SFML
    sfml-system
    sfml-audio
    sfml-graphics
)

include(GNUInstallDirs)

install(TARGETS EpiSoundSim
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(NOT LINK_DEPS_STATIC)
    set_target_properties(EpiSoundSim PROPERTIES
        INSTALL_RPATH $ORIGIN/../${CMAKE_INSTALL_LIBDIR}
    )

    set_target_properties(
        ImGui-SFML
        sfml-graphics sfml-system sfml-window
        PROPERTIES
        INSTALL_RPATH $ORIGIN
    )

    install(TARGETS
        ImGui-SFML
        sfml-graphics sfml-system sfml-window
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        NAMELINK_SKIP 
    )
endif()
if (WIN32 AND BUILD_SHARED_LIBS)
    add_custom_command(TARGET EpiSoundSim POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:EpiSoundSim> $<TARGET_FILE_DIR:EpiSoundSim> COMMAND_EXPAND_LISTS)
endif()


include(CPack)
