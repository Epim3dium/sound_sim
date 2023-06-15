set(SOURCE_FILES
    main.cpp
    implot/implot.cpp
    implot/implot_demo.cpp
    implot/implot_items.cpp
)
set(HEADER_FILES
    implot/implot.h
    implot/implot_internal.h
    sound_grid.hpp
    sound_source.hpp
    utils.hpp
)
add_compile_options(
    #-Wall
    #-Wextra
    #-Wconversion
    #-Wsign-conversion
    -glldb
    $<$<CONFIG:DEBUG>:-glldb>
    $<$<CONFIG:DEBUG>:-Og>
    $<$<CONFIG:RELEASE>:-O3>
)
add_compile_definitions(
$<$<CONFIG:DEBUG>:EPI_DEBUG>
)

find_package(OpenMP REQUIRED)
find_package(SFML 2.5 COMPONENTS graphics audio REQUIRED)

link_directories(/opt/homebrew/opt/sfml/lib)

add_executable(EpiSim ${SOURCE_FILES} ${HEADER_FILES} )
target_include_directories(ImGui-SFML PUBLIC /opt/homebrew/opt/sfml/include)
target_include_directories(EpiSim PUBLIC ./physics . ./../vendor)

# Yep, that's it!
target_link_libraries(EpiSim sfml-graphics sfml-audio ImGui-SFML::ImGui-SFML OpenMP::OpenMP_CXX)

include(GNUInstallDirs)

install(TARGETS EpiSim
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Installing is not easy, when we're dealing with shared libs
if(NOT LINK_DEPS_STATIC)
  set_target_properties(EpiSim PROPERTIES
    INSTALL_RPATH $ORIGIN/../${CMAKE_INSTALL_LIBDIR}
  )

  set_target_properties(
    ImGui-SFML
    #sfml-graphics sfml-system sfml-window sfml-audio
    PROPERTIES
    INSTALL_RPATH $ORIGIN
  )

  install(TARGETS
    ImGui-SFML
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    NAMELINK_SKIP # don't need versionless .so's
  )
endif()
if (WIN32 AND BUILD_SHARED_LIBS)
    add_custom_command(TARGET CMakeSFMLProject POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:CMakeSFMLProject> $<TARGET_FILE_DIR:CMakeSFMLProject> COMMAND_EXPAND_LISTS)
endif()

#install(SCRIPT PostInstall.cmake)

include(CPack)
