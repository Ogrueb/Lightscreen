# Replaces uglobalhotkey.pri and the standalone uglobalhotkey.pro (M11). Built as a static library, so the
# no-export ABI the .pri selected stays.
add_library(UGlobalHotkey STATIC
    hotkeymap.h
    uglobal.h
    uglobalhotkeys.cpp uglobalhotkeys.h
    ukeysequence.cpp ukeysequence.h
)
target_include_directories(UGlobalHotkey PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(UGlobalHotkey PUBLIC UGLOBALHOTKEY_NOEXPORT)
target_link_libraries(UGlobalHotkey PUBLIC Qt6::Core Qt6::Gui Qt6::Widgets)

if(WIN32)
    target_link_libraries(UGlobalHotkey PUBLIC user32)
else()
    target_link_libraries(UGlobalHotkey PUBLIC xcb xcb-keysyms)
endif()
