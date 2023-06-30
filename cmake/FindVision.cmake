# CMake Finder for the Vision SDK on Windows.
#
# When installed at the default location using the Vision Drivers installer,
# the SDK should be found automatically.
#
# The path to the Vision SDK can also be specified with the Vision_DIR variable.
#
# If the Vision SDK is found:
# - the Vision_FOUND variable will be set
# - the vision::vision_release and vision::vision_debug targets will be defined

include(GNUInstallDirs)

# If Vision_DIR is set, look only in this directory
if (Vision_DIR)
    find_library(
        VISION_LIBRARY_RELEASE
        NAMES RGBEASY.lib
        NO_DEFAULT_PATH
        PATHS ${Vision_DIR}
        PATH_SUFFIXES
            Vision/SDK/RGBEASY/C/LIB/x64/Release
            SDK/RGBEASY/C/LIB/x64/Release
            RGBEASY/C/LIB/x64/Release
            C/LIB/x64/Release
            LIB/x64/Release)

    find_library(
        VISION_LIBRARY_DEBUG
        NAMES RGBEASY.lib
        NO_DEFAULT_PATH
        PATHS ${Vision_DIR}
        PATH_SUFFIXES
            Vision/SDK/RGBEASY/C/LIB/x64/Debug
            SDK/RGBEASY/C/LIB/x64/Debug
            RGBEASY/C/LIB/x64/Debug
            C/LIB/x64/Debug
            LIB/x64/Debug)

    find_path(
        VISION_INCLUDE_DIR
        NAMES RGB.H
        NO_DEFAULT_PATH
        PATHS ${Vision_DIR}
        PATH_SUFFIXES
            Vision/SDK/RGBEASY/C/INCLUDE
            SDK/RGBEASY/C/INCLUDE
            RGBEASY/C/INCLUDE
            C/INCLUDE
            "INCLUDE")
# Otherwise, look into the default locations
else()
    find_library(
        VISION_LIBRARY_RELEASE
        NAMES RGBEASY.lib
        PATH_SUFFIXES Vision/SDK/RGBEASY/C/LIB/x64/Release)

    find_library(
        VISION_LIBRARY_DEBUG
        NAMES RGBEASY.lib
        PATH_SUFFIXES Vision/SDK/RGBEASY/C/LIB/x64/Debug)

    find_path(
        VISION_INCLUDE_DIR
        NAMES RGB.H
        PATH_SUFFIXES Vision/SDK/RGBEASY/C/INCLUDE)
endif()

include(FindPackageHandleStandardArgs)

cmake_path(GET VISION_INCLUDE_DIR PARENT_PATH VISION_DIR)
cmake_path(GET VISION_DIR PARENT_PATH VISION_DIR)

find_package_handle_standard_args(
    Vision DEFAULT_MSG
    VISION_DIR
    VISION_LIBRARY_RELEASE
    VISION_LIBRARY_DEBUG
    VISION_INCLUDE_DIR)

mark_as_advanced(
    VISION_DIR
    VISION_LIBRARY_DEBUG
    VISION_LIBRARY_RELEASE
    VISION_INCLUDE_DIR)

if(VISION_FOUND AND NOT TARGET vision::vision_debug AND NOT TARGET vision::vision_release)
    add_library(vision::vision_debug STATIC IMPORTED)
    add_library(vision::vision_release STATIC IMPORTED)

    set_target_properties(
        vision::vision_debug
        PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${VISION_INCLUDE_DIR}"
        IMPORTED_LOCATION "${VISION_LIBRARY_DEBUG}")

    set_target_properties(
        vision::vision_release
        PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${VISION_INCLUDE_DIR}"
        IMPORTED_LOCATION "${VISION_LIBRARY_RELEASE}")
endif()
