qt_add_translation(FIXTURE_QM_FILES
    fixtures/plural_uk.ts
    fixtures/plural_ru.ts
)

add_executable(tst_Translations
    tst_Translations.cpp
    ${FIXTURE_QM_FILES}
)

target_include_directories(tst_Translations PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_compile_definitions(tst_Translations PRIVATE
    FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
    FIXTURES_BUILD_DIR="${CMAKE_CURRENT_BINARY_DIR}"
    RESOURCES_TRANSLATIONS_DIR="${CMAKE_SOURCE_DIR}/resources/translations"
)

target_link_libraries(tst_Translations PRIVATE
    jobprep_core
    Qt6::Test
    Qt6::Widgets
)

jobprep_apply_warnings(tst_Translations)

add_test(NAME tst_Translations COMMAND tst_Translations)
set_tests_properties(tst_Translations PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    LABELS "i18n"
)
