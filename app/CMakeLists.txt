# Create highs binary using library without PIC.
add_executable(highs RunHighs.cpp
                     ${HIGHS_SOURCE_DIR}/src/lp_data/HighsRuntimeOptions.h)

target_link_libraries(highs PRIVATE libhighs)

# install the binary
install(
  TARGETS highs
  EXPORT highs-targets
  RUNTIME DESTINATION bin)
