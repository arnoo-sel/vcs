#ifndef VCS_DISPLAY_QT_DIALOGS_COMPONENTS_WINDOW_OPTIONS_DIALOG_WINDOW_SCALER_H
#define VCS_DISPLAY_QT_DIALOGS_COMPONENTS_WINDOW_OPTIONS_DIALOG_WINDOW_SCALER_H

#include "display/qt/subclasses/QDialog_vcs_base_dialog.h"

namespace Ui {
class WindowScaler;
}

class WindowScaler : public VCSBaseDialog
{
    Q_OBJECT

public:
    explicit WindowScaler(QWidget *parent = nullptr);
    ~WindowScaler();

private:
    Ui::WindowScaler *ui;
};

#endif
