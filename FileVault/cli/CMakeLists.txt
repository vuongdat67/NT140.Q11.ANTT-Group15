# FileVault CLI executable
add_executable(filevault_cli
    main.cpp
)

target_link_libraries(filevault_cli
    PRIVATE
        filevault
        CLI11::CLI11
        indicators::indicators
        fmt::fmt
)

# Statically link runtime on Windows for single executable
if(MSVC)
    set_property(TARGET filevault_cli PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Strip symbols in Release
if(CMAKE_BUILD_TYPE STREQUAL "Release" AND NOT MSVC)
    add_custom_command(TARGET filevault_cli POST_BUILD
        COMMAND ${CMAKE_STRIP} $<TARGET_FILE:filevault_cli>
        COMMENT "Stripping symbols..."
    )
endif()

set_target_properties(filevault_cli PROPERTIES
    OUTPUT_NAME filevault
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
