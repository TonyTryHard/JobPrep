add_library(jobprep_test_helpers STATIC
    MessageCapture.cpp
    MessageCapture.h
    TestDatabase.cpp
    TestDatabase.h
)

target_include_directories(jobprep_test_helpers PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(jobprep_test_helpers PUBLIC
    jobprep_core
    Qt6::Test
)

jobprep_apply_warnings(jobprep_test_helpers)

# Every data-layer test links the shared helper library, so the sources are built once.
set(DATA_TESTS
    tst_Database
    tst_TrackRepository
    tst_TopicRepository
    tst_SessionRepository
    tst_ApplicationRepository
    tst_InterviewRepository
    tst_ReminderLogRepository
    tst_SeedService
    tst_TransactionBehavior
)

foreach(test_name IN LISTS DATA_TESTS)
    add_executable(${test_name} ${test_name}.cpp)
    target_link_libraries(${test_name} PRIVATE jobprep_test_helpers)
    jobprep_apply_warnings(${test_name})

    add_test(NAME ${test_name} COMMAND ${test_name})
    set_tests_properties(${test_name} PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endforeach()
