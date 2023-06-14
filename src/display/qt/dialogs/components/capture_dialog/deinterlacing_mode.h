#ifndef VCS_DISPLAY_QT_DIALOGS_COMPONENTS_CAPTURE_DIALOG_DEINTERLACING_H
#define VCS_DISPLAY_QT_DIALOGS_COMPONENTS_CAPTURE_DIALOG_DEINTERLACING_H

#include "display/qt/subclasses/QDialog_vcs_base_dialog.h"

namespace Ui {
class DeinterlacingMode;
}

class DeinterlacingMode : public VCSBaseDialog
{
    Q_OBJECT

public:
    explicit DeinterlacingMode(QWidget *parent = nullptr);
    ~DeinterlacingMode();

private:
    Ui::DeinterlacingMode *ui;
};

#endif
