# Unit Test 1: Foundation (Core & Camera Simulator)
add_executable(sabit_unit_tests
    unit/test_foundation.cpp
)

target_include_directories(sabit_unit_tests PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(sabit_unit_tests PRIVATE
    sabit_camera
    sabit_core
)

target_compile_features(sabit_unit_tests PRIVATE cxx_std_20)

add_test(NAME FoundationTests COMMAND sabit_unit_tests)
set_tests_properties(FoundationTests PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

# Unit Test 2: Batch 2 (Imaging & Astronomy)
add_executable(sabit_imaging_astro_tests
    unit/test_imaging_astro.cpp
)

target_include_directories(sabit_imaging_astro_tests PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(sabit_imaging_astro_tests PRIVATE
    sabit_imaging
    sabit_astro
    sabit_core
)

target_compile_features(sabit_imaging_astro_tests PRIVATE cxx_std_20)

add_test(NAME ImagingAstroTests COMMAND sabit_imaging_astro_tests)
set_tests_properties(ImagingAstroTests PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

# Integration Test 3: Batch 3 (Hardware & AI Detection)
add_executable(sabit_hardware_ai_tests
    integration/test_hardware_ai.cpp
)

target_include_directories(sabit_hardware_ai_tests PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(sabit_hardware_ai_tests PRIVATE
    sabit_detect
    sabit_camera
    sabit_imaging
    sabit_core
)

target_compile_features(sabit_hardware_ai_tests PRIVATE cxx_std_20)

add_test(NAME HardwareAiTests COMMAND sabit_hardware_ai_tests)
set_tests_properties(HardwareAiTests PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

# Unit Test 4: Batch 4 (App Controller & UI)
add_executable(sabit_app_ui_tests
    unit/test_app_ui.cpp
)

target_include_directories(sabit_app_ui_tests PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(sabit_app_ui_tests PRIVATE
    sabit_ui
    sabit_app
    sabit_detect
    sabit_camera
    sabit_imaging
    sabit_core
    Qt6::Widgets
    Qt6::Core
    Qt6::Gui
)

target_compile_features(sabit_app_ui_tests PRIVATE cxx_std_20)

add_test(NAME AppUiTests COMMAND sabit_app_ui_tests)
set_tests_properties(AppUiTests PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
