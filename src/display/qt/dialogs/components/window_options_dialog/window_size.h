#ifndef VCS_DISPLAY_QT_DIALOGS_COMPONENTS_WINDOW_OPTIONS_DIALOG_WINDOW_SIZE_H
#define VCS_DISPLAY_QT_DIALOGS_COMPONENTS_WINDOW_OPTIONS_DIALOG_WINDOW_SIZE_H

#include "display/qt/subclasses/QDialog_vcs_base_dialog.h"

namespace Ui {
class WindowSize;
}

class WindowSize : public VCSBaseDialog
{
    Q_OBJECT

public:
    explicit WindowSize(QWidget *parent = 0);
    ~WindowSize();

    void adjust_output_scaling(const int dir);

    void disable_output_size_controls(const bool areDisabled);

    void notify_of_new_capture_signal(void);

private:
    Ui::WindowSize *ui;
};

#endif
