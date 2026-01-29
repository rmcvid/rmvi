# video/element/CMakeLists.txt
add_executable(element
  element.c
)

target_include_directories(element
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}     # pour fourier.h local
    ${CMAKE_SOURCE_DIR}/include     # headers projet (visual.h, ...)
    ${CMAKE_SOURCE_DIR}/video       # si tu as video/config.h
)
target_compile_definitions(element
  PRIVATE
    FONT_PATH=\"${CMAKE_SOURCE_DIR}/extern/police/jsMath-cmsy10.ttf\" #latinmodern-math-1959/otf/latinmodern-math.otf\"
    IMAGE_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/Image/\"
)

set(OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/output")
add_compile_definitions(OUTPUT_DIR="${OUTPUT_DIR}")


# link vers la librairie commune (et transitivement vers raylib si myvisual le déclare PUBLIC)
target_link_libraries(element
  PRIVATE
    myvisual
)
