# Replaces singleapplication.pri (M11).
add_library(SingleApplication STATIC singleapplication.cpp singleapplication.h)
target_include_directories(SingleApplication PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(SingleApplication PUBLIC Qt6::Core Qt6::Network Qt6::Widgets)
