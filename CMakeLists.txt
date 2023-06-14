cmake_minimum_required(VERSION 3.21)

# Project name and version
project(vcs VERSION 2.7.0 LANGUAGES CXX)

# Require C++17 or later
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# VCS code insists on having NDEBUG not defined.
# The following hacks remove NDEBUG definitions from release builds.
if (MSVC)
    string(REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE        "${CMAKE_CXX_FLAGS_RELEASE}")
    string(REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELEASE}")
    string(REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_MINSIZEREL     "${CMAKE_CXX_FLAGS_RELEASE}")
else()
    string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE        "${CMAKE_CXX_FLAGS_RELEASE}")
    string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELEASE}")
    string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_MINSIZEREL     "${CMAKE_CXX_FLAGS_RELEASE}")
endif()

# Find and setup Qt - User can set Qt6_DIR to help cmake find Qt
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui OpenGL OpenGLWidgets)
qt_standard_project_setup()
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/display/qt/windows/ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/display/qt/dialogs/ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/display/qt/dialogs/components/capture_dialog/ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/display/qt/dialogs/components/window_options_dialog/ui"
)

# Find OpenCV - User can set OpenCV_DIR to help cmake find OpenCV
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
find_package(OpenCV REQUIRED imgproc highgui core photo)

# User-settable option: VCS_DISPLAY_SYSTEM
#   Used to set the display system on Linux (X11 or Wayland).
#   - default    Only available choice on Windows. Chooses x11 on Linux.
#   - x11        Linux-only
#   - wayland    Linux-only
set(VCS_DISPLAY_SYSTEM default CACHE STRING "default, x11, wayland")
set_property(CACHE VCS_DISPLAY_SYSTEM PROPERTY STRINGS "default;x11;wayland")

# User-settable option: VCS_CAPTURE_BACKEND
#   Used to set the capture backend.
#   - default        Chooses rgbeasy on Windows. Chooses vision-v4l on Linux.
#   - rgbeasy        Windows-only
#   - vision-v4l     Linux-only
#   - virtual
#   - dosbox-mmap    Linux-only
set(VCS_CAPTURE_BACKEND default CACHE STRING "default, rgbeasy, vision-v4l, virtual, dosbox-mmap")
set_property(CACHE VCS_CAPTURE_BACKEND PROPERTY STRINGS "default;rgbeasy;vision-v4l;virtual;dosbox-mmap")

# Process VCS_CAPTURE_BACKEND
if((WIN32 AND ${VCS_CAPTURE_BACKEND} STREQUAL default) OR ${VCS_CAPTURE_BACKEND} STREQUAL rgbeasy)
    set(VCS_CHOSEN_CAPTURE_BACKEND rgbeasy)
elseif((LINUX AND ${VCS_CAPTURE_BACKEND} STREQUAL default) OR ${VCS_CAPTURE_BACKEND} STREQUAL vision-v4l)
    set(VCS_CHOSEN_CAPTURE_BACKEND vision-v4l)
elseif(${VCS_CAPTURE_BACKEND} STREQUAL virtual)
    set(VCS_CHOSEN_CAPTURE_BACKEND virtual)
elseif(${VCS_CAPTURE_BACKEND} STREQUAL dosbox-mmap)
    set(VCS_CHOSEN_CAPTURE_BACKEND dosbox-mmap)
endif()
mark_as_advanced(VCS_CHOSEN_CAPTURE_BACKEND)

# Process VCS_DISPLAY_SYSTEM
if (WIN32 AND ${VCS_DISPLAY_SYSTEM} STREQUAL default)
    set(VCS_CHOSEN_DISPLAY_SYSTEM windows)
elseif ((LINUX AND ${VCS_DISPLAY_SYSTEM} STREQUAL default) OR ${VCS_DISPLAY_SYSTEM} STREQUAL x11)
    set(VCS_CHOSEN_DISPLAY_SYSTEM x11)
elseif (${VCS_DISPLAY_SYSTEM} STREQUAL wayland)
    set(VCS_CHOSEN_DISPLAY_SYSTEM wayland)
endif()
mark_as_advanced(VCS_CHOSEN_DISPLAY_SYSTEM)

# Set the common source files for all configurations
set(vcs_sources
    src/anti_tear/anti_tear.cpp
    src/anti_tear/anti_tear.h
    src/anti_tear/anti_tear_multiple_per_frame.cpp
    src/anti_tear/anti_tear_multiple_per_frame.h
    src/anti_tear/anti_tear_one_per_frame.cpp
    src/anti_tear/anti_tear_one_per_frame.h
    src/anti_tear/anti_tearer.cpp
    src/anti_tear/anti_tearer.h
    src/capture/capture.cpp
    src/capture/capture.h
    src/capture/video_presets.cpp
    src/capture/video_presets.h
    src/common/assert.h
    src/common/command_line/command_line.cpp
    src/common/command_line/command_line.h
    src/common/disk/csv.h
    src/common/disk/disk.cpp
    src/common/disk/disk.h
    src/common/disk/file_reader.cpp
    src/common/disk/file_reader.h
    src/common/disk/file_readers/file_reader_filter_graph.h
    src/common/disk/file_readers/file_reader_filter_graph_version_a.cpp
    src/common/disk/file_readers/file_reader_filter_graph_version_b.cpp
    src/common/disk/file_readers/file_reader_video_params.h
    src/common/disk/file_readers/file_reader_video_params_legacy_1_6_5.cpp
    src/common/disk/file_readers/file_reader_video_params_version_a.cpp
    src/common/disk/file_readers/file_reader_video_presets.h
    src/common/disk/file_readers/file_reader_video_presets_version_a.cpp
    src/common/disk/file_streamer.h
    src/common/disk/file_writer.cpp
    src/common/disk/file_writer.h
    src/common/disk/file_writers/file_writer_filter_graph.h
    src/common/disk/file_writers/file_writer_filter_graph_version_b.cpp
    src/common/disk/file_writers/file_writer_video_params.h
    src/common/disk/file_writers/file_writer_video_params_legacy_1_6_5.cpp
    src/common/disk/file_writers/file_writer_video_params_version_a.cpp
    src/common/disk/file_writers/file_writer_video_presets.h
    src/common/disk/file_writers/file_writer_video_presets_version_a.cpp
    src/common/globals.h
    src/common/log/log.cpp
    src/common/log/log.h
    src/common/propagate/vcs_event.h
    src/common/refresh_rate.h
    src/common/timer/timer.cpp
    src/common/timer/timer.h
    src/common/types.h
    src/display/display.cpp
    src/display/display.h
    src/display/display.h
    src/display/qt/d_main.cpp
    src/display/qt/dialogs/about_dialog.cpp
    src/display/qt/dialogs/about_dialog.h
    src/display/qt/dialogs/capture_dialog.cpp
    src/display/qt/dialogs/capture_dialog.h
    src/display/qt/dialogs/components/capture_dialog/deinterlacing_mode.cpp
    src/display/qt/dialogs/components/capture_dialog/deinterlacing_mode.h
    src/display/qt/dialogs/components/capture_dialog/device_info.cpp
    src/display/qt/dialogs/components/capture_dialog/device_info.h
    src/display/qt/dialogs/components/capture_dialog/input_channel.cpp
    src/display/qt/dialogs/components/capture_dialog/input_channel.h
    src/display/qt/dialogs/components/capture_dialog/input_resolution.cpp
    src/display/qt/dialogs/components/capture_dialog/input_resolution.h
    src/display/qt/dialogs/components/capture_dialog/signal_status.cpp
    src/display/qt/dialogs/components/capture_dialog/signal_status.h
    src/display/qt/dialogs/components/filter_graph_dialog/base_filter_graph_node.cpp
    src/display/qt/dialogs/components/filter_graph_dialog/base_filter_graph_node.h
    src/display/qt/dialogs/components/filter_graph_dialog/filter_node.cpp
    src/display/qt/dialogs/components/filter_graph_dialog/filter_node.h
    src/display/qt/dialogs/components/filter_graph_dialog/input_gate_node.cpp
    src/display/qt/dialogs/components/filter_graph_dialog/input_gate_node.h
    src/display/qt/dialogs/components/filter_graph_dialog/output_gate_node.cpp
    src/display/qt/dialogs/components/filter_graph_dialog/output_gate_node.h
    src/display/qt/dialogs/components/filter_graph_dialog/output_scaler_node.cpp
    src/display/qt/dialogs/components/filter_graph_dialog/output_scaler_node.h
    src/display/qt/dialogs/components/window_options_dialog/window_renderer.cpp
    src/display/qt/dialogs/components/window_options_dialog/window_renderer.h
    src/display/qt/dialogs/components/window_options_dialog/window_scaler.cpp
    src/display/qt/dialogs/components/window_options_dialog/window_scaler.h
    src/display/qt/dialogs/components/window_options_dialog/window_size.cpp
    src/display/qt/dialogs/components/window_options_dialog/window_size.h
    src/display/qt/dialogs/control_panel_dialog.cpp
    src/display/qt/dialogs/control_panel_dialog.h
    src/display/qt/dialogs/filter_graph_dialog.cpp
    src/display/qt/dialogs/filter_graph_dialog.h
    src/display/qt/dialogs/overlay_dialog.cpp
    src/display/qt/dialogs/overlay_dialog.h
    src/display/qt/dialogs/resolution_dialog.cpp
    src/display/qt/dialogs/resolution_dialog.h
    src/display/qt/dialogs/video_presets_dialog.cpp
    src/display/qt/dialogs/video_presets_dialog.h
    src/display/qt/dialogs/window_options_dialog.cpp
    src/display/qt/dialogs/window_options_dialog.h
    src/display/qt/keyboard_shortcuts.cpp
    src/display/qt/keyboard_shortcuts.h
    src/display/qt/persistent_settings.cpp
    src/display/qt/persistent_settings.h
    src/display/qt/subclasses/QComboBox_video_preset_list.cpp
    src/display/qt/subclasses/QComboBox_video_preset_list.h
    src/display/qt/subclasses/QDialog_vcs_base_dialog.cpp
    src/display/qt/subclasses/QDialog_vcs_base_dialog.h
    src/display/qt/subclasses/QFrame_filtergui_for_qt.cpp
    src/display/qt/subclasses/QFrame_filtergui_for_qt.h
    src/display/qt/subclasses/QGraphicsItem_interactible_node_graph_node.cpp
    src/display/qt/subclasses/QGraphicsItem_interactible_node_graph_node.h
    src/display/qt/subclasses/QGraphicsScene_interactible_node_graph.cpp
    src/display/qt/subclasses/QGraphicsScene_interactible_node_graph.h
    src/display/qt/subclasses/QGraphicsView_interactible_node_graph_view.cpp
    src/display/qt/subclasses/QGraphicsView_interactible_node_graph_view.h
    src/display/qt/subclasses/QGroupBox_parameter_grid.cpp
    src/display/qt/subclasses/QGroupBox_parameter_grid.h
    src/display/qt/subclasses/QLabel_magnifying_glass.cpp
    src/display/qt/subclasses/QLabel_magnifying_glass.h
    src/display/qt/subclasses/QMenu_dialog_file_menu.cpp
    src/display/qt/subclasses/QMenu_dialog_file_menu.h
    src/display/qt/subclasses/QOpenGLWidget_opengl_renderer.cpp
    src/display/qt/subclasses/QOpenGLWidget_opengl_renderer.h
    src/display/qt/subclasses/QTableWidget_property_table.cpp
    src/display/qt/subclasses/QTableWidget_property_table.h
    src/display/qt/utility.h
    src/display/qt/windows/output_window.cpp
    src/display/qt/windows/output_window.h
    src/filter/abstract_filter.cpp
    src/filter/filter.cpp
    src/filter/filter.h
    src/filter/filtergui.cpp
    src/filter/filtergui.h
    src/filter/filters/anti_tear/filter_anti_tear.cpp
    src/filter/filters/anti_tear/filter_anti_tear.h
    src/filter/filters/anti_tear/gui/filtergui_anti_tear.cpp
    src/filter/filters/anti_tear/gui/filtergui_anti_tear.h
    src/filter/filters/blur/filter_blur.cpp
    src/filter/filters/blur/filter_blur.h
    src/filter/filters/blur/gui/filtergui_blur.cpp
    src/filter/filters/blur/gui/filtergui_blur.h
    src/filter/filters/color_depth/filter_color_depth.cpp
    src/filter/filters/color_depth/filter_color_depth.h
    src/filter/filters/color_depth/gui/filtergui_color_depth.cpp
    src/filter/filters/color_depth/gui/filtergui_color_depth.h
    src/filter/filters/crop/filter_crop.cpp
    src/filter/filters/crop/filter_crop.h
    src/filter/filters/crop/gui/filtergui_crop.cpp
    src/filter/filters/crop/gui/filtergui_crop.h
    src/filter/filters/decimate/filter_decimate.cpp
    src/filter/filters/decimate/filter_decimate.h
    src/filter/filters/decimate/gui/filtergui_decimate.cpp
    src/filter/filters/decimate/gui/filtergui_decimate.h
    src/filter/filters/delta_histogram/filter_delta_histogram.cpp
    src/filter/filters/delta_histogram/filter_delta_histogram.h
    src/filter/filters/delta_histogram/gui/filtergui_delta_histogram.cpp
    src/filter/filters/delta_histogram/gui/filtergui_delta_histogram.h
    src/filter/filters/denoise_nonlocal_means/filter_denoise_nonlocal_means.cpp
    src/filter/filters/denoise_nonlocal_means/filter_denoise_nonlocal_means.h
    src/filter/filters/denoise_nonlocal_means/gui/filtergui_denoise_nonlocal_means.cpp
    src/filter/filters/denoise_nonlocal_means/gui/filtergui_denoise_nonlocal_means.h
    src/filter/filters/denoise_pixel_gate/filter_denoise_pixel_gate.cpp
    src/filter/filters/denoise_pixel_gate/filter_denoise_pixel_gate.h
    src/filter/filters/denoise_pixel_gate/gui/filtergui_denoise_pixel_gate.cpp
    src/filter/filters/denoise_pixel_gate/gui/filtergui_denoise_pixel_gate.h
    src/filter/filters/filters.h
    src/filter/filters/flip/filter_flip.cpp
    src/filter/filters/flip/filter_flip.h
    src/filter/filters/flip/gui/filtergui_flip.cpp
    src/filter/filters/flip/gui/filtergui_flip.h
    src/filter/filters/input_gate/filter_input_gate.cpp
    src/filter/filters/input_gate/filter_input_gate.h
    src/filter/filters/input_gate/gui/filtergui_input_gate.cpp
    src/filter/filters/input_gate/gui/filtergui_input_gate.h
    src/filter/filters/kernel_3x3/filter_kernel_3x3.cpp
    src/filter/filters/kernel_3x3/filter_kernel_3x3.h
    src/filter/filters/kernel_3x3/gui/filtergui_kernel_3x3.cpp
    src/filter/filters/kernel_3x3/gui/filtergui_kernel_3x3.h
    src/filter/filters/median/filter_median.cpp
    src/filter/filters/median/filter_median.h
    src/filter/filters/median/gui/filtergui_median.cpp
    src/filter/filters/median/gui/filtergui_median.h
    src/filter/filters/output_gate/filter_output_gate.cpp
    src/filter/filters/output_gate/filter_output_gate.h
    src/filter/filters/output_gate/gui/filtergui_output_gate.cpp
    src/filter/filters/output_gate/gui/filtergui_output_gate.h
    src/filter/filters/output_scaler/filter_output_scaler.cpp
    src/filter/filters/output_scaler/filter_output_scaler.h
    src/filter/filters/output_scaler/gui/filtergui_output_scaler.cpp
    src/filter/filters/output_scaler/gui/filtergui_output_scaler.h
    src/filter/filters/render_text/filter_render_text.cpp
    src/filter/filters/render_text/filter_render_text.h
    src/filter/filters/render_text/font.h
    src/filter/filters/render_text/font_10x6_sans_serif.h
    src/filter/filters/render_text/font_10x6_serif.h
    src/filter/filters/render_text/font_5x3.h
    src/filter/filters/render_text/font_fraps.h
    src/filter/filters/render_text/gui/filtergui_render_text.cpp
    src/filter/filters/render_text/gui/filtergui_render_text.h
    src/filter/filters/rotate/filter_rotate.cpp
    src/filter/filters/rotate/filter_rotate.h
    src/filter/filters/rotate/gui/filtergui_rotate.cpp
    src/filter/filters/rotate/gui/filtergui_rotate.h
    src/filter/filters/sharpen/filter_sharpen.cpp
    src/filter/filters/sharpen/filter_sharpen.h
    src/filter/filters/sharpen/gui/filtergui_sharpen.cpp
    src/filter/filters/sharpen/gui/filtergui_sharpen.h
    src/filter/filters/source_fps_estimate/filter_source_fps_estimate.cpp
    src/filter/filters/source_fps_estimate/filter_source_fps_estimate.h
    src/filter/filters/source_fps_estimate/gui/filtergui_source_fps_estimate.cpp
    src/filter/filters/source_fps_estimate/gui/filtergui_source_fps_estimate.h
    src/filter/filters/unsharp_mask/filter_unsharp_mask.cpp
    src/filter/filters/unsharp_mask/filter_unsharp_mask.h
    src/filter/filters/unsharp_mask/gui/filtergui_unsharp_mask.cpp
    src/filter/filters/unsharp_mask/gui/filtergui_unsharp_mask.h
    src/main.cpp
    src/main.h
    src/scaler/scaler.cpp
    src/scaler/scaler.h

    src/display/qt/dialogs/components/capture_dialog/ui/deinterlacing_mode.ui
    src/display/qt/dialogs/components/capture_dialog/ui/device_info.ui
    src/display/qt/dialogs/components/capture_dialog/ui/input_channel.ui
    src/display/qt/dialogs/components/capture_dialog/ui/input_resolution.ui
    src/display/qt/dialogs/components/capture_dialog/ui/signal_status.ui
    src/display/qt/dialogs/components/window_options_dialog/ui/window_renderer.ui
    src/display/qt/dialogs/components/window_options_dialog/ui/window_scaler.ui
    src/display/qt/dialogs/components/window_options_dialog/ui/window_size.ui
    src/display/qt/dialogs/ui/about_dialog.ui
    src/display/qt/dialogs/ui/capture_dialog.ui
    src/display/qt/dialogs/ui/control_panel_dialog.ui
    src/display/qt/dialogs/ui/filter_graph_dialog.ui
    src/display/qt/dialogs/ui/overlay_dialog.ui
    src/display/qt/dialogs/ui/resolution_dialog.ui
    src/display/qt/dialogs/ui/video_presets_dialog.ui
    src/display/qt/dialogs/ui/window_options_dialog.ui
    src/display/qt/windows/ui/output_window.ui

    src/display/qt/res.qrc
)

# Set the source files specific to some configurations
if (WIN32)
    list(APPEND vcs_sources src/display/prevent_screensaver_windows.cpp)
    set(app_icon_resource_windows "${CMAKE_CURRENT_SOURCE_DIR}/vcs.rc")
elseif (LINUX)
    if (${VCS_CHOSEN_DISPLAY_SYSTEM} STREQUAL x11)
        list(APPEND vcs_sources src/display/prevent_screensaver_x11.cpp)
    else()
        list(APPEND vcs_sources src/display/prevent_screensaver_null.cpp)
    endif()
endif()
if (${VCS_CHOSEN_CAPTURE_BACKEND} STREQUAL rgbeasy)
    list(APPEND vcs_sources src/capture/rgbeasy/capture_rgbeasy.cpp)
elseif (${VCS_CHOSEN_CAPTURE_BACKEND} STREQUAL vision-v4l)
    list(
        APPEND vcs_sources
        src/capture/vision_v4l/capture_vision_v4l.cpp
        src/capture/vision_v4l/ic_v4l_video_parameters.cpp
        src/capture/vision_v4l/ic_v4l_video_parameters.h
        src/capture/vision_v4l/input_channel_v4l.cpp
        src/capture/vision_v4l/input_channel_v4l.h)
elseif (${VCS_CHOSEN_CAPTURE_BACKEND} STREQUAL virtual)
    list(APPEND vcs_sources src/capture/virtual/capture_virtual.cpp)
endif()

# Create the vcs executable target
qt_add_executable(vcs
    ${vcs_sources}
    ${app_icon_resource_windows}
)
# set_target_properties(vcs PROPERTIES
#     WIN32_EXECUTABLE ON
# )

target_compile_definitions(vcs PRIVATE $<$<NOT:$<CONFIG:Debug>>:VCS_RELEASE_BUILD>)
if (${VCS_CHOSEN_CAPTURE_BACKEND} STREQUAL rgbeasy)
    target_compile_definitions(vcs PRIVATE CAPTURE_BACKEND_RGBEASY)
elseif (${VCS_CHOSEN_CAPTURE_BACKEND} STREQUAL vision-v4l)
    target_compile_definitions(vcs PRIVATE CAPTURE_BACKEND_VISION_V4L)
elseif (${VCS_CHOSEN_CAPTURE_BACKEND} STREQUAL virtual)
    target_compile_definitions(vcs PRIVATE CAPTURE_BACKEND_VIRTUAL)
elseif (${VCS_CHOSEN_CAPTURE_BACKEND} STREQUAL dosbox-mmap)
    target_compile_definitions(vcs PRIVATE CAPTURE_BACKEND_DOSBOX_MMAP)
endif()

# Set the include directories
target_include_directories(vcs PRIVATE ${CMAKE_SOURCE_DIR}/src/display/qt/subclasses)
target_include_directories(vcs PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Set the dependencies
target_link_libraries(
    vcs PRIVATE
    Qt6::Core Qt6::Widgets Qt6::Gui Qt6::OpenGL Qt6::OpenGLWidgets
    ${OpenCV_LIBS})
if (${VCS_CHOSEN_CAPTURE_BACKEND} STREQUAL rgbeasy)
    # Find Vision SDK - User can set Vision_DIR to help cmake find the Vision SDK
    find_package(Vision REQUIRED)
    target_link_libraries(vcs PRIVATE
        debug vision::vision_debug
        optimized vision::vision_release)
elseif (${VCS_CHOSEN_CAPTURE_BACKEND} STREQUAL dosbox-mmap)
    target_link_libraries(vcs PRIVATE rt)
endif()
if (${VCS_CHOSEN_DISPLAY_SYSTEM} STREQUAL x11)
    find_package(X11 REQUIRED COMPONENTS X11 Xext)
    target_link_libraries(vcs PRIVATE X11::X11 X11::Xext)
endif()

# Setup the installation/deployment
include(GNUInstallDirs)
install(TARGETS vcs)
qt_generate_deploy_app_script(
    TARGET vcs
    OUTPUT_SCRIPT deployVcs
)
install(SCRIPT "${deployVcs}")
install(FILES $<TARGET_RUNTIME_DLLS:vcs> TYPE BIN)
