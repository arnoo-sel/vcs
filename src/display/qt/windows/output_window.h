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

class ControlPanelDialog;
class VCSBaseDialog;
class QAction;
class MagnifyingGlass;

struct resolution_alias_s;

namespace Ui {
class OutputWindow;
}

class OutputWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit OutputWindow(QWidget *parent = 0);
    ~OutputWindow();

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

    bool apply_global_stylesheet(const QString &qssFilename);

    bool set_global_font_size(const unsigned fontSize);

    void override_window_title(const std::string &newTitle);

    ControlPanelDialog* control_panel(void) const { return this->controlPanelDialog; }

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

    Ui::OutputWindow *ui = nullptr;

    // The menu items shown when the user right-clicks this window.
    std::vector<QMenu*> contextMenuSubmenus;
    QMenu *contextMenu;

    ControlPanelDialog *controlPanelDialog = nullptr;

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
