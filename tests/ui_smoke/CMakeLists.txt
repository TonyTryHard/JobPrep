add_executable(ui_smoke
    tst_UiSmoke.cpp
)

target_link_libraries(ui_smoke PRIVATE
    jobprep_core
    Qt6::Test
)

jobprep_apply_warnings(ui_smoke)

add_test(NAME ui_smoke COMMAND ui_smoke)
set_tests_properties(ui_smoke PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;JOBPREP_REPORT_IMG_DIR=${CMAKE_SOURCE_DIR}/docs/reports/img"
)

add_executable(tst_MonthView tst_MonthView.cpp)
target_link_libraries(tst_MonthView PRIVATE
    jobprep_core
    Qt6::Test
)
jobprep_apply_warnings(tst_MonthView)
add_test(NAME tst_MonthView COMMAND tst_MonthView)
set_tests_properties(tst_MonthView PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
