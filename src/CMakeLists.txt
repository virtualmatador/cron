cmake_minimum_required(VERSION 3.16)
enable_testing()

add_library("cron" INTERFACE)
target_include_directories("cron" INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
