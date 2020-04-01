# Create highs binary using library without PIC.
add_executable(highs RunHighs.cpp
                     ${HIGHS_SOURCE_DIR}/src/lp_data/HighsRuntimeOptions.h)

target_link_libraries(highs PRIVATE libhighs)

set_target_properties(highs
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# install the binary
install(
  TARGETS highs
  EXPORT highs-targets
  RUNTIME DESTINATION bin)
