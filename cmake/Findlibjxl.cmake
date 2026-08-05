# cmake/Findlibjxl.cmake
find_package(PkgConfig REQUIRED)

pkg_check_modules(JXL REQUIRED IMPORTED_TARGET
    libjxl
    libjxl_threads
)

# Create the namespaced target that the rest of the project already expects
if(NOT TARGET libjxl::libjxl)
    add_library(libjxl::libjxl INTERFACE IMPORTED)
    set_target_properties(libjxl::libjxl PROPERTIES
        INTERFACE_LINK_LIBRARIES "PkgConfig::JXL"
    )
endif()

# For completeness (optional)
set(libjxl_FOUND TRUE)
set(libjxl_VERSION ${JXL_VERSION})