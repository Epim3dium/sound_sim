set(SOURCE_FILES
    main.cpp
    implot/implot.cpp
    implot/implot_items.cpp
)
set(HEADER_FILES
    implot/implot.h
    implot/implot_internal.h
    sound_grid.hpp
    sound_source.hpp
    utils.hpp
)
add_executable(EpiSoundSim
    ${SOURCE_FILES} ${HEADER_FILES}
)
include(Install.cmake)
