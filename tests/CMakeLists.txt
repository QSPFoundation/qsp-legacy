add_executable(qsp-legacy-tests runner/qsp_test_runner.c)
set_target_properties(qsp-legacy-tests PROPERTIES C_STANDARD 17 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
target_link_libraries(qsp-legacy-tests PRIVATE qsp-legacy)

# One CTest test per scenario file
file(GLOB QSP_LEGACY_TEST_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/engine/*.qspt")
foreach (testFile ${QSP_LEGACY_TEST_FILES})
    get_filename_component(testName "${testFile}" NAME_WE)
    add_test(NAME "engine.${testName}" COMMAND qsp-legacy-tests "${testFile}")
endforeach()