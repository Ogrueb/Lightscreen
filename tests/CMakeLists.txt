# One executable per test, each linking the app's sources (lightscreen_core), so a file added to the app
# goes into the root CMakeLists.txt only. Each test is built into its own directory, as with qmake:
# tst_history and tst_optimize put files next to their executable.
find_package(Qt6 REQUIRED COMPONENTS Test)

function(lightscreen_add_test name)
    add_executable(${name} ${name}/${name}.cpp)
    target_link_libraries(${name} PRIVATE lightscreen_core Qt6::Test)
    # $<1:...> stops multi-config generators from adding a per-configuration subdirectory.
    set_target_properties(${name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "$<1:${CMAKE_CURRENT_BINARY_DIR}/${name}>")
    add_test(NAME ${name} COMMAND ${name} WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${name}")
endfunction()

lightscreen_add_test(tst_smoke)
lightscreen_add_test(tst_history)
lightscreen_add_test(tst_screenshot)
lightscreen_add_test(tst_settings)
lightscreen_add_test(tst_uploader)
lightscreen_add_test(tst_areadialog)
lightscreen_add_test(tst_os)
lightscreen_add_test(tst_hotkeywidget)
lightscreen_add_test(tst_mainwindow)
lightscreen_add_test(tst_updater)
lightscreen_add_test(tst_optimize)

# A stand-in for optipng, used by tst_optimize. Built as "optipng" into tst_optimize's stub/ directory.
set(optipng_stub_dir "${CMAKE_CURRENT_BINARY_DIR}/tst_optimize/stub")
add_executable(optipng_stub optipng_stub/main.cpp)
set_target_properties(optipng_stub PROPERTIES
    OUTPUT_NAME optipng
    RUNTIME_OUTPUT_DIRECTORY "$<1:${optipng_stub_dir}>"
    AUTOMOC OFF AUTOUIC OFF AUTORCC OFF
)
target_compile_definitions(tst_optimize PRIVATE OPTIPNG_STUB_DIR="${optipng_stub_dir}")
add_dependencies(tst_optimize optipng_stub)
