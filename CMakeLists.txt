cmake_minimum_required(VERSION 3.21)

project(JobPrep
    VERSION 1.0.0
    LANGUAGES CXX
    DESCRIPTION "Desktop application to plan interview preparation and track job applications"
)

# C++20 requirement per AGENTS.md §2
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Handle QT_PREFIX_PATH if set in environment
if(DEFINED ENV{QT_PREFIX_PATH} AND NOT "$ENV{QT_PREFIX_PATH}" STREQUAL "")
    list(PREPEND CMAKE_PREFIX_PATH "$ENV{QT_PREFIX_PATH}")
endif()

# Qt standard code generators
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt 6 packages
find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Sql
    Svg
    Test
)

# Quality gate: high warning level and zero warnings option
option(JOBPREP_WERROR "Treat compiler warnings as errors" ON)

function(jobprep_apply_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
        if(JOBPREP_WERROR)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
        if(JOBPREP_WERROR)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()

# --- Static core library target ---
add_library(jobprep_core STATIC
    # App composition root
    src/app/AppContext.h
    src/app/AppContext.cpp

    # Domain models
    src/domain/Enums.h
    src/domain/Structs.h
    src/domain/EnumStrings.h
    src/domain/EnumStrings.cpp

    # Data layer
    src/data/DataLogging.h
    src/data/DbFormat.h
    src/data/DbFormat.cpp
    src/data/Database.h
    src/data/Database.cpp
    src/data/Migrations.h
    src/data/Migrations.cpp
    src/data/SqlStatement.h
    src/data/SqlStatement.cpp
    src/data/TrackRepository.h
    src/data/TrackRepository.cpp
    src/data/TopicRepository.h
    src/data/TopicRepository.cpp
    src/data/SessionRepository.h
    src/data/SessionRepository.cpp
    src/data/ApplicationRepository.h
    src/data/ApplicationRepository.cpp
    src/data/InterviewRepository.h
    src/data/InterviewRepository.cpp
    src/data/ReminderLogRepository.h
    src/data/ReminderLogRepository.cpp

    # Services
    src/services/SettingsService.h
    src/services/SettingsService.cpp
    src/services/SeedService.h
    src/services/SeedService.cpp

    # Theme & Tokens
    src/ui/theme/Tokens.h
    src/ui/theme/Tokens.cpp
    src/ui/theme/StatusStyle.h
    src/ui/theme/StatusStyle.cpp
    src/ui/theme/IconProvider.h
    src/ui/theme/IconProvider.cpp
    src/ui/theme/ThemeManager.h
    src/ui/theme/ThemeManager.cpp

    # Widgets & Base Pages
    src/ui/widgets/EmptyState.h
    src/ui/widgets/EmptyState.cpp
    src/ui/PageBase.h
    src/ui/PageBase.cpp

    # Page implementations
    src/ui/pages/HomePage.h
    src/ui/pages/HomePage.cpp
    src/ui/pages/StudyPage.h
    src/ui/pages/StudyPage.cpp
    src/ui/pages/JobsPage.h
    src/ui/pages/JobsPage.cpp
    src/ui/pages/CalendarPage.h
    src/ui/pages/CalendarPage.cpp
    src/ui/pages/SettingsPage.h
    src/ui/pages/SettingsPage.cpp

    # Shell UI
    src/ui/Sidebar.h
    src/ui/Sidebar.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp

    # Resources
    resources/resources.qrc
)

target_include_directories(jobprep_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(jobprep_core PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Sql
    Qt6::Svg
)

jobprep_apply_warnings(jobprep_core)

# --- Main executable target ---
add_executable(jobprep
    src/main.cpp
)

target_link_libraries(jobprep PRIVATE
    jobprep_core
)

jobprep_apply_warnings(jobprep)

# --- Testing ---
enable_testing()
add_subdirectory(tests)
