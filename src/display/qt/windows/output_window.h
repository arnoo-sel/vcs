/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef VCS_DISPLAY_QT_WINDOWS_OUTPUT_WINDOW_H
#define VCS_DISPLAY_QT_WINDOWS_OUTPUT_WINDOW_H

#include <QMainWindow>
#include "display/display.h"
#include "common/globals.h"

class OutputResolutionDialog;
class InputResolutionDialog;
class VideoPresetsDialog;
class FilterGraphDialog;
class AntiTearDialog;
class OverlayDialog;
class SignalDialog;
class AboutDialog;
class VCSBaseDialog;
class QAction;
class MagnifyingGlass;

struct resolution_alias_s;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    // Gets user input in the GUI, etc.
    void update_gui_state(void);

    // Returns true if the window has a border.
    bool window_has_border(void);

    void add_gui_log_entry(const log_entry_s e);

    void update_window_size(void);

    // Updates the window title with the current status of capture and that of the program
    // in general.
    void update_window_title(void);

    void set_opengl_enabled(const bool enabled);

    void redraw(void);

    // Returns the current overlay as a QImage, or a null QImage if the overlay
    // should not be shown at this time.
    QImage overlay_image(void);

    // Returns true if the user is allowed to scale the output resolution by using the
    // mouse wheel over the capture window.
    bool is_mouse_wheel_scaling_allowed(void);

    bool apply_global_stylesheet(const QString &qssFilename);

    bool set_global_font_size(const unsigned fontSize);

    OutputResolutionDialog* output_resolution_dialog(void) const { return this->outputResolutionDlg; }
    InputResolutionDialog* input_resolution_dialog(void)   const { return this->inputResolutionDlg; }
    FilterGraphDialog* filter_graph_dialog(void)           const { return this->filterGraphDlg; }
    VideoPresetsDialog* video_presets_dialog(void)         const { return this->videoPresetsDlg; }
    AntiTearDialog* anti_tear_dialog(void)                 const { return this->antitearDlg; }
    OverlayDialog* overlay_dialog(void)                    const { return this->overlayDlg; }
    SignalDialog* signal_info_dialog(void)                 const { return this->signalDlg; }
    AboutDialog* about_dialog(void)                        const { return this->aboutDlg; }

signals:
    void fullscreen_mode_enabled(void);
    void fullscreen_mode_disabled(void);
    void border_hidden(void);
    void border_shown(void);

private:
    void contextMenuEvent(QContextMenuEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void changeEvent(QEvent *event);
    void paintEvent(QPaintEvent *);
    void closeEvent(QCloseEvent *event);
    void wheelEvent(QWheelEvent *event);

    // Load the font pointed to by the given filename, and make it available to the program.
    bool load_font(const QString &filename);

    void toggle_window_border(void);

    void save_screenshot(void);

    Ui::MainWindow *ui = nullptr;

    // The menu items shown when the user right-clicks this window.
    std::vector<QMenu*> contextMenuSubmenus;
    QMenu *contextMenu;

    OutputResolutionDialog *outputResolutionDlg = nullptr;
    InputResolutionDialog *inputResolutionDlg = nullptr;
    VideoPresetsDialog *videoPresetsDlg = nullptr;
    FilterGraphDialog *filterGraphDlg = nullptr;
    AntiTearDialog *antitearDlg = nullptr;
    OverlayDialog *overlayDlg = nullptr;
    SignalDialog *signalDlg = nullptr;
    AboutDialog *aboutDlg = nullptr;

    // A master list of all the dialogs that this window spawns. Will be filled in
    // when we create the dialog instances.
    QVector<VCSBaseDialog*> dialogs;

    MagnifyingGlass *magnifyingGlass = nullptr;

    // Set to true when the user has selected to close the
    // main window.
    bool userExiting = false;

    // Set to true if the program failed to initialize the capture card.
    bool captureInitFail = false;

    // A user-definable custom title for the output window.
    QString windowTitleOverride = "";

    // The currently-active app-wide QSS style sheet.
    QString appwideStyleSheet = "";

    unsigned appwideFontSize = 15;

    // Set to true if the capture subsystem is currently reporting that captured frames
    // are being dropped; false otherwise. The value gets updated at some interval,
    // e.g. 1 second.
    bool areFramesBeingDropped = false;

    const Qt::WindowFlags defaultWindowFlags = (Qt::Window | Qt::WindowCloseButtonHint | Qt::WindowMinimizeButtonHint);
};

#endif
