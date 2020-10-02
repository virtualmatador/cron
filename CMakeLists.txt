cmake_minimum_required(VERSION 3.16)
enable_testing()

add_library("cron")
set_property(TARGET "cron" PROPERTY CXX_STANDARD 17)
target_sources("cron" PRIVATE "cron.cpp" "field.cpp")
target_include_directories("cron" INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

add_executable("cron-test")
set_property(TARGET "cron-test" PROPERTY CXX_STANDARD 17)
target_sources("cron-test" PRIVATE "cron-test.cpp")
target_link_libraries("cron-test" "cron")
add_test("cron" "cron-test")
