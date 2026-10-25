add_executable(tst_Notifiers tst_Notifiers.cpp)
target_link_libraries(tst_Notifiers PRIVATE jobprep_core jobprep_test_helpers Qt6::Test Qt6::Widgets)
target_include_directories(tst_Notifiers PRIVATE ${CMAKE_SOURCE_DIR}/tests/data)
jobprep_apply_warnings(tst_Notifiers)

add_test(NAME tst_Notifiers COMMAND tst_Notifiers)
set_tests_properties(tst_Notifiers PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
