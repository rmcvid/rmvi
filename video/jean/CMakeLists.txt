# video/jean/CMakeLists.txt
add_executable(jean
  jean.c
)

target_include_directories(jean
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}     # pour fourier.h local
    ${CMAKE_SOURCE_DIR}/include     # headers projet (visual.h, ...)
    ${CMAKE_SOURCE_DIR}/video       # si tu as video/config.h
)

        #IMAGE_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/Image/\"
        #latinmodern-math-1959/otf/latinmodern-math.otf\"

target_compile_definitions(jean
  PRIVATE
    FONT_PATH=\"${CMAKE_SOURCE_DIR}/extern/police/latinmodern-math-1959/otf/latinmodern-math.otf\"
    HERE_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}\"
)
    
set(OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/output/")
add_compile_definitions(OUTPUT_DIR="${OUTPUT_DIR}")


# link vers la librairie commune (et transitivement vers raylib si myvisual le déclare PUBLIC)
target_link_libraries(jean
  PRIVATE
    myvisual
)
