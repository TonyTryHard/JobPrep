add_executable(tst_Notifiers tst_Notifiers.cpp)
target_link_libraries(tst_Notifiers PRIVATE jobprep_core jobprep_test_helpers jobprep_ui_test_helpers Qt6::Test Qt6::Widgets)
target_include_directories(tst_Notifiers PRIVATE ${CMAKE_SOURCE_DIR}/tests/data)
jobprep_apply_warnings(tst_Notifiers)

add_test(NAME tst_Notifiers COMMAND tst_Notifiers)
set_tests_properties(tst_Notifiers PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    LABELS "ui"
)

add_executable(tst_StudyRegression tst_StudyRegression.cpp)
target_link_libraries(tst_StudyRegression PRIVATE jobprep_core jobprep_test_helpers jobprep_ui_test_helpers Qt6::Test Qt6::Widgets)
target_include_directories(tst_StudyRegression PRIVATE ${CMAKE_SOURCE_DIR}/tests/data)
jobprep_apply_warnings(tst_StudyRegression)

add_test(NAME tst_StudyRegression COMMAND tst_StudyRegression)
set_tests_properties(tst_StudyRegression PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    LABELS "ui"
)

set(PAGE_TESTS
    tst_PageJobs
    tst_PageCalendar
    tst_PageStudy
    tst_PageHome
    tst_PageSettings
)

foreach(test_name IN LISTS PAGE_TESTS)
    add_executable(${test_name} ${test_name}.cpp)
    target_link_libraries(${test_name} PRIVATE jobprep_core jobprep_ui_test_helpers Qt6::Test Qt6::Widgets)
    jobprep_apply_warnings(${test_name})
    add_test(NAME ${test_name} COMMAND ${test_name})
    set_tests_properties(${test_name} PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;JOBPREP_REPORT_IMG_DIR=${CMAKE_SOURCE_DIR}/docs/reports/img"
        LABELS "ui"
    )
endforeach()


