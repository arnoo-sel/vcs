# Whether this build of VCS is intended for release. Release builds may perform better
# but have fewer run-time error checks and contain less debug information.
#DEFINES += VCS_RELEASE_BUILD

linux {
    DEFINES += CAPTURE_BACKEND_VISION_V4L

    INCLUDEPATH += \
        # The base path for Datapath's Linux Vision driver header files. These are
        # bundled with the driver downloadable from Datapath's website. The header
        # files are expected to be in a "visionrgb/" subdirectory of this path, eg.
        # ".../visionrgb/include/rgb133control.h".
        $$(HOME)/sdk/

        LIBS += \
            -L/usr/local/lib/ \
            -lopencv_imgproc \
            -lopencv_highgui \
            -lopencv_core \
            -lopencv_photo
}

win32 {
    DEFINES += CAPTURE_BACKEND_RGBEASY

    # The paths below assume:
    # - the RGBEASY library, as found in VisionCaptureSDKs_v7.19.4, is found in the parent folder
    # - a MingGW-x64 build of OpenCV 3.4.14, is found in the parent folder

    INCLUDEPATH += \
        "../RGBEASY/C/INCLUDE" \
        "../opencv-3.4.14/mingw/install/include"

    LIBS += \
        "../RGBEASY/C/LIB/x64/Release/RGBEASY.lib" \
        -L"../opencv-3.4.14/mingw/install/x64/mingw/lib" \
        -lopencv_world3414

    RC_ICONS = "src/display/qt/images/icons/appicon.ico"
}

INCLUDEPATH += \
    $$PWD/src/ \
    $$PWD/src/display/qt/subclasses/

RESOURCES += \
    src/display/qt/res.qrc

SOURCES += \
    src/display/qt/dialogs/filter_graph/base_filter_graph_node.cpp \
    src/display/qt/dialogs/filter_graph/output_scaler_node.cpp \
    src/display/qt/dialogs/video_presets_dialog.cpp \
    src/display/qt/keyboard_shortcuts.cpp \
    src/filter/filters/output_scaler/filter_output_scaler.cpp \
    src/filter/filters/output_scaler/gui/filtergui_output_scaler.cpp \
    src/filter/filters/render_text/filter_render_text.cpp \
    src/filter/filters/render_text/gui/filtergui_render_text.cpp \
    src/filter/filters/source_fps_estimate/filter_source_fps_estimate.cpp \
    src/filter/filters/source_fps_estimate/gui/filtergui_source_fps_estimate.cpp \
    src/main.cpp \
    src/display/display.cpp \
    src/display/qt/subclasses/QLabel_magnifying_glass.cpp \
    src/display/qt/subclasses/QComboBox_video_preset_list.cpp \
    src/display/qt/subclasses/QDialog_vcs_base_dialog.cpp \
    src/display/qt/subclasses/QFrame_filtergui_for_qt.cpp \
    src/display/qt/subclasses/QGroupBox_parameter_grid.cpp \
    src/display/qt/subclasses/QMenu_dialog_file_menu.cpp \
    src/display/qt/windows/output_window.cpp \
    src/display/qt/dialogs/resolution_dialog.cpp \
    src/display/qt/d_main.cpp \
    src/display/qt/dialogs/overlay_dialog.cpp \
    src/display/qt/dialogs/alias_dialog.cpp \
    src/display/qt/dialogs/anti_tear_dialog.cpp \
    src/anti_tear/anti_tear_multiple_per_frame.cpp \
    src/anti_tear/anti_tear_one_per_frame.cpp \
    src/anti_tear/anti_tearer.cpp \
    src/filter/abstract_filter.cpp \
    src/filter/filters/anti_tear/filter_anti_tear.cpp \
    src/filter/filters/anti_tear/gui/filtergui_anti_tear.cpp \
    src/filter/filters/blur/filter_blur.cpp \
    src/filter/filters/blur/gui/filtergui_blur.cpp \
    src/filter/filters/color_depth/filter_color_depth.cpp \
    src/filter/filters/color_depth/gui/filtergui_color_depth.cpp \
    src/filter/filters/crop/filter_crop.cpp \
    src/filter/filters/crop/gui/filtergui_crop.cpp \
    src/filter/filters/decimate/filter_decimate.cpp \
    src/filter/filters/decimate/gui/filtergui_decimate.cpp \
    src/filter/filters/delta_histogram/filter_delta_histogram.cpp \
    src/filter/filters/delta_histogram/gui/filtergui_delta_histogram.cpp \
    src/filter/filters/denoise_nonlocal_means/filter_denoise_nonlocal_means.cpp \
    src/filter/filters/denoise_nonlocal_means/gui/filtergui_denoise_nonlocal_means.cpp \
    src/filter/filters/denoise_pixel_gate/filter_denoise_pixel_gate.cpp \
    src/filter/filters/denoise_pixel_gate/gui/filtergui_denoise_pixel_gate.cpp \
    src/filter/filtergui.cpp \
    src/filter/filters/flip/filter_flip.cpp \
    src/filter/filters/flip/gui/filtergui_flip.cpp \
    src/filter/filters/input_gate/filter_input_gate.cpp \
    src/filter/filters/input_gate/gui/filtergui_input_gate.cpp \
    src/filter/filters/kernel_3x3/filter_kernel_3x3.cpp \
    src/filter/filters/kernel_3x3/gui/filtergui_kernel_3x3.cpp \
    src/filter/filters/median/filter_median.cpp \
    src/filter/filters/median/gui/filtergui_median.cpp \
    src/filter/filters/output_gate/filter_output_gate.cpp \
    src/filter/filters/output_gate/gui/filtergui_output_gate.cpp \
    src/filter/filters/rotate/filter_rotate.cpp \
    src/filter/filters/rotate/gui/filtergui_rotate.cpp \
    src/filter/filters/sharpen/filter_sharpen.cpp \
    src/filter/filters/sharpen/gui/filtergui_sharpen.cpp \
    src/filter/filters/unsharp_mask/filter_unsharp_mask.cpp \
    src/filter/filters/unsharp_mask/gui/filtergui_unsharp_mask.cpp \
    src/scaler/scaler.cpp \
    src/common/log/log.cpp \
    src/filter/filter.cpp \
    src/common/command_line/command_line.cpp \
    src/capture/capture.cpp \
    src/anti_tear/anti_tear.cpp \
    src/display/qt/persistent_settings.cpp \
    src/common/disk/disk.cpp \
    src/capture/alias.cpp \
    src/display/qt/subclasses/QOpenGLWidget_opengl_renderer.cpp \
    src/display/qt/subclasses/QGraphicsItem_interactible_node_graph_node.cpp \
    src/display/qt/subclasses/QGraphicsScene_interactible_node_graph.cpp \
    src/display/qt/subclasses/QGraphicsView_interactible_node_graph_view.cpp \
    src/display/qt/dialogs/filter_graph_dialog.cpp \
    src/display/qt/dialogs/about_dialog.cpp \
    src/display/qt/dialogs/output_resolution_dialog.cpp \
    src/display/qt/dialogs/input_resolution_dialog.cpp \
    src/display/qt/subclasses/QTableWidget_property_table.cpp \
    src/display/qt/dialogs/signal_dialog.cpp \
    src/common/disk/file_writers/file_writer_filter_graph_version_b.cpp \
    src/common/disk/file_reader.cpp \
    src/common/disk/file_readers/file_reader_filter_graph_version_b.cpp \
    src/common/disk/file_readers/file_reader_filter_graph_version_a.cpp \
    src/common/disk/file_writers/file_writer_aliases_version_a.cpp \
    src/common/disk/file_readers/file_reader_aliases_version_a.cpp \
    src/common/disk/file_readers/file_reader_video_params_legacy_1_6_5.cpp \
    src/common/disk/file_writers/file_writer_video_params_legacy_1_6_5.cpp \
    src/common/disk/file_writer.cpp \
    src/common/disk/file_writers/file_writer_video_params_version_a.cpp \
    src/common/disk/file_readers/file_reader_video_params_version_a.cpp \
    src/common/disk/file_readers/file_reader_aliases_legacy_1_6_5.cpp \
    src/display/qt/dialogs/filter_graph/output_gate_node.cpp \
    src/display/qt/dialogs/filter_graph/input_gate_node.cpp \
    src/display/qt/dialogs/filter_graph/filter_node.cpp \
    src/capture/video_presets.cpp \
    src/common/disk/file_writers/file_writer_video_presets_version_a.cpp \
    src/common/disk/file_readers/file_reader_video_presets_version_a.cpp \
    src/common/timer/timer.cpp \
    src/display/qt/dialogs/linux_device_selector_dialog.cpp

HEADERS += \
    src/common/assert.h \
    src/common/globals.h \
    src/common/types.h \
    src/display/qt/dialogs/filter_graph/base_filter_graph_node.h \
    src/display/qt/dialogs/filter_graph/output_scaler_node.h \
    src/display/qt/dialogs/video_presets_dialog.h \
    src/display/qt/keyboard_shortcuts.h \
    src/display/qt/subclasses/QComboBox_video_preset_list.h \
    src/display/qt/subclasses/QDialog_vcs_base_dialog.h \
    src/display/qt/subclasses/QFrame_filtergui_for_qt.h \
    src/display/qt/subclasses/QGroupBox_parameter_grid.h \
    src/display/qt/subclasses/QLabel_magnifying_glass.h \
    src/display/qt/subclasses/QMenu_dialog_file_menu.h \
    src/display/qt/windows/output_window.h \
    src/display/qt/dialogs/resolution_dialog.h \
    src/anti_tear/anti_tear_multiple_per_frame.h \
    src/anti_tear/anti_tear_one_per_frame.h \
    src/anti_tear/anti_tearer.h \
    src/filter/filters/anti_tear/filter_anti_tear.h \
    src/filter/filters/anti_tear/gui/filtergui_anti_tear.h \
    src/filter/filters/blur/filter_blur.h \
    src/filter/filters/blur/gui/filtergui_blur.h \
    src/filter/filters/color_depth/filter_color_depth.h \
    src/filter/filters/color_depth/gui/filtergui_color_depth.h \
    src/filter/filters/crop/filter_crop.h \
    src/filter/filters/crop/gui/filtergui_crop.h \
    src/filter/filters/decimate/filter_decimate.h \
    src/filter/filters/decimate/gui/filtergui_decimate.h \
    src/filter/filters/delta_histogram/filter_delta_histogram.h \
    src/filter/filters/delta_histogram/gui/filtergui_delta_histogram.h \
    src/filter/filters/denoise_nonlocal_means/filter_denoise_nonlocal_means.h \
    src/filter/filters/denoise_nonlocal_means/gui/filtergui_denoise_nonlocal_means.h \
    src/filter/filters/denoise_pixel_gate/filter_denoise_pixel_gate.h \
    src/filter/filters/denoise_pixel_gate/gui/filtergui_denoise_pixel_gate.h \
    src/filter/filtergui.h \
    src/filter/filters/filters.h \
    src/filter/filters/flip/filter_flip.h \
    src/filter/filters/flip/gui/filtergui_flip.h \
    src/filter/filters/output_scaler/filter_output_scaler.h \
    src/filter/filters/output_scaler/gui/filtergui_output_scaler.h \
    src/filter/filters/render_text/font.h \
    src/filter/filters/render_text/font_10x6_sans_serif.h \
    src/filter/filters/render_text/font_10x6_serif.h \
    src/filter/filters/render_text/font_5x3.h \
    src/filter/filters/render_text/font_fraps.h \
    src/filter/filters/source_fps_estimate/filter_source_fps_estimate.h \
    src/filter/filters/source_fps_estimate/gui/filtergui_source_fps_estimate.h \
    src/filter/filters/input_gate/filter_input_gate.h \
    src/filter/filters/input_gate/gui/filtergui_input_gate.h \
    src/filter/filters/kernel_3x3/filter_kernel_3x3.h \
    src/filter/filters/kernel_3x3/gui/filtergui_kernel_3x3.h \
    src/filter/filters/median/filter_median.h \
    src/filter/filters/median/gui/filtergui_median.h \
    src/filter/filters/output_gate/filter_output_gate.h \
    src/filter/filters/output_gate/gui/filtergui_output_gate.h \
    src/filter/filters/rotate/filter_rotate.h \
    src/filter/filters/rotate/gui/filtergui_rotate.h \
    src/filter/filters/sharpen/filter_sharpen.h \
    src/filter/filters/sharpen/gui/filtergui_sharpen.h \
    src/filter/filters/render_text/filter_render_text.h \
    src/filter/filters/render_text/gui/filtergui_render_text.h \
    src/filter/filters/unsharp_mask/filter_unsharp_mask.h \
    src/filter/filters/unsharp_mask/gui/filtergui_unsharp_mask.h \
    src/main.h \
    src/scaler/scaler.h \
    src/capture/capture.h \
    src/display/display.h \
    src/common/log/log.h \
    src/display/qt/dialogs/overlay_dialog.h \
    src/display/qt/dialogs/alias_dialog.h \
    src/anti_tear/anti_tear.h \
    src/display/qt/dialogs/anti_tear_dialog.h \
    src/filter/filter.h \
    src/common/command_line/command_line.h \
    src/display/qt/persistent_settings.h \
    src/display/qt/utility.h \
    src/common/disk/csv.h \
    src/common/disk/disk.h \
    src/capture/alias.h \
    src/display/qt/subclasses/QOpenGLWidget_opengl_renderer.h \
    src/display/qt/subclasses/QGraphicsItem_interactible_node_graph_node.h \
    src/display/qt/subclasses/QGraphicsScene_interactible_node_graph.h \
    src/display/qt/subclasses/QGraphicsView_interactible_node_graph_view.h \
    src/display/qt/dialogs/filter_graph_dialog.h \
    src/display/qt/dialogs/about_dialog.h \
    src/display/qt/dialogs/output_resolution_dialog.h \
    src/display/qt/dialogs/input_resolution_dialog.h \
    src/display/display.h \
    src/display/qt/subclasses/QTableWidget_property_table.h \
    src/display/qt/dialogs/signal_dialog.h \
    src/common/disk/file_writers/file_writer_filter_graph.h \
    src/common/disk/file_readers/file_reader_filter_graph.h \
    src/common/disk/file_reader.h \
    src/common/disk/file_streamer.h \
    src/common/disk/file_writers/file_writer_aliases.h \
    src/common/disk/file_readers/file_reader_video_params.h \
    src/common/disk/file_writers/file_writer_video_params.h \
    src/common/disk/file_writer.h \
    src/display/qt/dialogs/filter_graph/output_gate_node.h \
    src/display/qt/dialogs/filter_graph/input_gate_node.h \
    src/display/qt/dialogs/filter_graph/filter_node.h \
    src/common/refresh_rate.h \
    src/capture/video_presets.h \
    src/common/disk/file_writers/file_writer_video_presets.h \
    src/common/disk/file_readers/file_reader_video_presets.h \
    src/common/propagate/vcs_event.h \
    src/common/timer/timer.h \
    src/display/qt/dialogs/linux_device_selector_dialog.h

FORMS += \
    src/display/qt/dialogs/ui/video_presets_dialog.ui \
    src/display/qt/windows/ui/output_window.ui \
    src/display/qt/dialogs/ui/overlay_dialog.ui \
    src/display/qt/dialogs/ui/resolution_dialog.ui \
    src/display/qt/dialogs/ui/alias_dialog.ui \
    src/display/qt/dialogs/ui/anti_tear_dialog.ui \
    src/display/qt/dialogs/ui/filter_graph_dialog.ui \
    src/display/qt/dialogs/ui/about_dialog.ui \
    src/display/qt/dialogs/ui/output_resolution_dialog.ui \
    src/display/qt/dialogs/ui/input_resolution_dialog.ui \
    src/display/qt/dialogs/ui/signal_dialog.ui \
    src/display/qt/dialogs/ui/linux_device_selector_dialog.ui

contains(DEFINES, CAPTURE_BACKEND_VIRTUAL) {
    SOURCES += src/capture/virtual/capture_virtual.cpp
}

contains(DEFINES, CAPTURE_BACKEND_DOSBOX_MMAP) {
    SOURCES += src/capture/dosbox_mmap/capture_dosbox_mmap.cpp
    LIBS += -lrt
}

contains(DEFINES, CAPTURE_BACKEND_VISION_V4L) {
    SOURCES += \
        src/capture/vision_v4l/capture_vision_v4l.cpp \
        src/capture/vision_v4l/input_channel_v4l.cpp \
        src/capture/vision_v4l/ic_v4l_video_parameters.cpp

    HEADERS += \
        src/capture/vision_v4l/input_channel_v4l.h \
        src/capture/vision_v4l/ic_v4l_video_parameters.h
}

contains(DEFINES, CAPTURE_BACKEND_RGBEASY) {
    SOURCES += src/capture/rgbeasy/capture_rgbeasy.cpp

    linux {
        SOURCES += src/capture/rgbeasy/null_rgbeasy.h
    }
}

QT += core gui widgets opengl openglwidgets
TARGET = vcs
TEMPLATE = app
CONFIG += console release
QMAKE_CXXFLAGS += -pedantic -std=c++17 -Wno-missing-field-initializers
OBJECTS_DIR = generated_files
RCC_DIR = generated_files
MOC_DIR = generated_files
UI_DIR = generated_files

!contains(DEFINES, VCS_RELEASE_BUILD) {
    CONFIG = $$replace(CONFIG, "release", "debug")
    linux {
        CONFIG += sanitizer sanitize_address sanitize_undefined
    }
}
