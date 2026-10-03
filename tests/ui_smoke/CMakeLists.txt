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
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
