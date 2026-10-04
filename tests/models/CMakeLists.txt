add_library(jobprep_model_test_helpers INTERFACE)

# Models tests
set(MODELS_TESTS
    tst_ApplicationTableModel
    tst_ApplicationFilterProxy
)

foreach(test_name IN LISTS MODELS_TESTS)
    add_executable(${test_name} ${test_name}.cpp)
    target_link_libraries(${test_name} PRIVATE jobprep_core jobprep_test_helpers Qt6::Test)
    jobprep_apply_warnings(${test_name})
    add_test(NAME ${test_name} COMMAND ${test_name})
    set_tests_properties(${test_name} PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endforeach()
