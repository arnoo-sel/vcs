/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS display wrapper
 *
 * Implements VCS's display interface using Qt, i.e. wraps the interface's
 * functions around the Qt-based GUI.
 *
 */

#include <QApplication>
#include <QMessageBox>
#include <cassert>
#include <thread>
#include "display/qt/windows/output_window.h"
#include "display/qt/dialogs/video_presets_dialog.h"
#include "display/qt/dialogs/filter_graph_dialog.h"
#include "display/qt/dialogs/alias_dialog.h"
#include "display/qt/dialogs/output_resolution_dialog.h"
#include "display/qt/dialogs/signal_dialog.h"
#include "capture/capture.h"
#include "common/globals.h"
#include "filter/filter.h"
#include "capture/alias.h"
#include "common/log/log.h"

// We'll want to avoid accessing the GUI via non-GUI threads, so let's assume
// the thread that creates this unit is the GUI thread. We'll later compare
// against this id to detect out-of-GUI-thread access.
static const std::thread::id NATIVE_THREAD_ID = std::this_thread::get_id();

// Qt wants a QApplication object around for the GUI to function.
namespace app_n
{
    static int ARGC = 1;
    static char NAME[] = "VCS";
    static char *ARGV = NAME;
    static QApplication *APP = nullptr;
}

// The window we'll display the program in. Also owns the various sub-dialogs, etc.
static MainWindow *WINDOW = nullptr;

#define ASSERT_WINDOW_IS_NOT_NULL k_assert(WINDOW, "Tried to query the display before it had been initialized.");

void kd_recalculate_filter_graph_chains(void)
{
    ASSERT_WINDOW_IS_NOT_NULL;
    WINDOW->filter_graph_dialog()->recalculate_filter_chains();

    return;
}

subsystem_releaser_t kd_acquire_output_window(void)
{
    DEBUG(("Acquiring the display."));

    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::RoundPreferFloor);
    app_n::APP = new QApplication(app_n::ARGC, &app_n::ARGV);

    WINDOW = new MainWindow;
    WINDOW->show();

    return []{
        DEBUG(("Releasing the display."));
        if (WINDOW)
        {
            delete WINDOW;
        }
    };
}

void kd_spin_event_loop(void)
{
    ASSERT_WINDOW_IS_NOT_NULL;
    WINDOW->update_gui_state();

    return;
}

void kd_update_output_window_title(void)
{
    if (WINDOW)
    {
        WINDOW->update_window_title();
    }

    return;
}

void kd_update_output_window_size(void)
{
    if (WINDOW)
    {
        WINDOW->update_window_size();
    }

    return;
}

void kd_load_video_presets(const std::string &filename)
{
    ASSERT_WINDOW_IS_NOT_NULL;
    WINDOW->video_presets_dialog()->load_presets_from_file(QString::fromStdString(filename));
}

void kd_load_filter_graph(const std::string &filename)
{
    ASSERT_WINDOW_IS_NOT_NULL;
    WINDOW->filter_graph_dialog()->load_graph_from_file(QString::fromStdString(filename));
}

void kd_load_aliases(const std::string &filename)
{
    ASSERT_WINDOW_IS_NOT_NULL;
    WINDOW->alias_resolutions_dialog()->load_aliases_from_file(QString::fromStdString(filename));
}

bool kd_is_fullscreen(void)
{
    ASSERT_WINDOW_IS_NOT_NULL;
    return WINDOW->isFullScreen();
}

void kd_show_headless_info_message(const char *const title,
                                   const char *const msg)
{
    if (std::this_thread::get_id() == NATIVE_THREAD_ID)
    {
        QMessageBox mb;
        mb.setWindowTitle(strlen(title) == 0? "VCS has this to say" : title);
        mb.setText(msg);
        mb.setStandardButtons(QMessageBox::Ok);
        mb.setIcon(QMessageBox::Information);
        mb.setDefaultButton(QMessageBox::Ok);

        mb.exec();
    }

    INFO(("%s", msg));

    return;
}

// Displays the given error in a message box that isn't tied to a particular window
// of the program. Useful for giving out e.g. startup error messages for things that
// occur before the GUI has been initialized.
//
void kd_show_headless_error_message(const char *const title,
                                    const char *const msg)
{
    if (std::this_thread::get_id() == NATIVE_THREAD_ID)
    {
        QMessageBox mb;
        mb.setWindowTitle(strlen(title) == 0? "VCS has this to say" : title);
        mb.setText(msg);
        mb.setStandardButtons(QMessageBox::Ok);
        mb.setIcon(QMessageBox::Critical);
        mb.setDefaultButton(QMessageBox::Ok);

        mb.exec();
    }

    NBENE(("%s", msg));

    return;
}
void kd_show_headless_assert_error_message(const char *const msg,
                                           const char *const filename,
                                           const uint lineNum)
{
    if (std::this_thread::get_id() == NATIVE_THREAD_ID)
    {
        QMessageBox mb;
        mb.setWindowTitle("VCS Assertion Error");
        mb.setText("<html>"
                "VCS has come across an unexpected condition in its code. As a precaution, "
                "the program will shut itself down now.<br><br>"
                "<b>Error:</b> " + QString(msg) + "<br>"
                "<b>In file:</b> " + QString(filename) + "<br>"
                "<b>On line:</b> " + QString::number(lineNum) +
                "</html>");
        mb.setStandardButtons(QMessageBox::Ok);
        mb.setIcon(QMessageBox::Critical);
        mb.setDefaultButton(QMessageBox::Ok);

        mb.exec();
    }

    return;
}
