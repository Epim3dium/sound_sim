set(SOURCE_FILES
    main.cpp
    implot/implot.cpp
    implot/implot_demo.cpp
    implot/implot_items.cpp
)
set(HEADER_FILES
    sound_source.hpp
    sound_grid.hpp
    utils.hpp
    implot/implot.h
    implot/implot_internal.h
)
add_executable(EpiSoundSim
    ${SOURCE_FILES} ${HEADER_FILES}
)
target_compile_features(EpiSoundSim PRIVATE cxx_std_17)
include(Install.cmake)
