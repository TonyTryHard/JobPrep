add_library(jobprep_ui_test_helpers STATIC
    UiTestHelpers.cpp
)

target_include_directories(jobprep_ui_test_helpers PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(jobprep_ui_test_helpers PUBLIC
    jobprep_core
    Qt6::Test
    Qt6::Widgets
)

jobprep_apply_warnings(jobprep_ui_test_helpers)
