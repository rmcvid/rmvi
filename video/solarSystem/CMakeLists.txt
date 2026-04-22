# video/solarSystem/CMakeLists.txt
add_executable(solarSystem
  solarSystem.c
)
add_executable(diffEarth
  diffEarth.c
)

set(OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/output/")

foreach(tgt solarSystem diffEarth)
  target_include_directories(${tgt}
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_SOURCE_DIR}/include
      ${CMAKE_SOURCE_DIR}/video
  )

  target_compile_definitions(${tgt}
    PRIVATE
      FONT_PATH=\"${CMAKE_SOURCE_DIR}/extern/police/latinmodern-math-1959/otf/latinmodern-math.otf\"
      IMAGE_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/Image/\"
      SHADER_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/shader/\"
      OUTPUT_DIR=\"${OUTPUT_DIR}\"
      HERE_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/\"
  )

  target_link_libraries(${tgt} PRIVATE myvisual)
endforeach()


