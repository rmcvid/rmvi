# video/markov/CMakeLists.txt
add_executable(markov
  markov.c
)

target_include_directories(markov
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}     # pour markov.h local
    ${CMAKE_SOURCE_DIR}/include     # headers projet (visual.h, ...)
    ${CMAKE_SOURCE_DIR}/video       # si tu as video/config.h
)
target_compile_definitions(markov
  PRIVATE
    FONT_PATH=\"${CMAKE_SOURCE_DIR}/extern/police/latinmodern-math-1959/otf/latinmodern-math.otf\"
)
# link vers la librairie commune (et transitivement vers raylib si myvisual le déclare PUBLIC)
target_link_libraries(markov
  PRIVATE
    myvisual
)
