# video/fourier/CMakeLists.txt
add_executable(fourier
  fourier.c
)

target_include_directories(fourier
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}     # pour fourier.h local
    ${CMAKE_SOURCE_DIR}/include     # headers projet (visual.h, ...)
    ${CMAKE_SOURCE_DIR}/video       # si tu as video/config.h
)
target_compile_definitions(fourier
  PRIVATE
    FONT_PATH=\"${CMAKE_SOURCE_DIR}/extern/police/latinmodern-math-1959/otf/latinmodern-math.otf\"
)

set(OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/output")
add_compile_definitions(OUTPUT_DIR="${OUTPUT_DIR}")


# link vers la librairie commune (et transitivement vers raylib si myvisual le déclare PUBLIC)
target_link_libraries(fourier
  PRIVATE
    myvisual
)
