#ifndef VCS_DISPLAY_QT_DIALOGS_CONTROL_PANEL_DIALOG_H
#define VCS_DISPLAY_QT_DIALOGS_CONTROL_PANEL_DIALOG_H

#include "display/qt/subclasses/QDialog_vcs_base_dialog.h"

namespace Ui {
class ControlPanelDialog;
}

class OutputWindow;
class CaptureDialog;
class WindowOptionsDialog;
class FilterGraphDialog;
class VideoPresetsDialog;
class OverlayDialog;
class AboutDialog;

class ControlPanelDialog : public VCSBaseDialog
{
    Q_OBJECT

public:
    explicit ControlPanelDialog(OutputWindow *parent = nullptr);
    ~ControlPanelDialog();

    CaptureDialog* capture(void)              const { return this->captureDialog; }
    FilterGraphDialog* filter_graph(void)     const { return this->filterGraphDialog; }
    VideoPresetsDialog* video_presets(void)   const { return this->videoPresetsDialog; }
    WindowOptionsDialog* window_options(void) const { return this->windowOptionsDialog; }
    OverlayDialog* overlay(void)              const { return this->overlayDialog; }

private:
    Ui::ControlPanelDialog *ui;

    CaptureDialog *captureDialog = nullptr;
    WindowOptionsDialog *windowOptionsDialog = nullptr;
    FilterGraphDialog *filterGraphDialog = nullptr;
    VideoPresetsDialog *videoPresetsDialog = nullptr;
    OverlayDialog *overlayDialog = nullptr;
    AboutDialog *aboutDialog = nullptr;
};

#endif
