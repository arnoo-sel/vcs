/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef VCS_DISPLAY_QT_DIALOGS_COMPONENTS_CAPTURE_DIALOG_SIGNAL_STATUS_H
#define VCS_DISPLAY_QT_DIALOGS_COMPONENTS_CAPTURE_DIALOG_SIGNAL_STATUS_H

#include "display/qt/subclasses/QDialog_vcs_base_dialog.h"

class QMenuBar;

namespace Ui {
class SignalStatus;
}

struct capture_video_settings_s;
struct capture_color_settings_s;
struct resolution_s;

class SignalStatus : public VCSBaseDialog
{
    Q_OBJECT

public:
    explicit SignalStatus(QWidget *parent = 0);
    ~SignalStatus(void);

    void set_controls_enabled(const bool state);

    void update_controls(void);

private:
    void update_information_table(const bool isReceivingSignal);

    Ui::SignalStatus *ui;
};

#endif
