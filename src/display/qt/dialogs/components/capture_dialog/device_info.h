#ifndef VCS_DISPLAY_QT_DIALOGS_COMPONENTS_CAPTURE_DIALOG_DEVICE_INFO_H
#define VCS_DISPLAY_QT_DIALOGS_COMPONENTS_CAPTURE_DIALOG_DEVICE_INFO_H

#include "display/qt/subclasses/QDialog_vcs_base_dialog.h"

namespace Ui {
class DeviceInfo;
}

class DeviceInfo : public VCSBaseDialog
{
    Q_OBJECT

public:
    explicit DeviceInfo(QWidget *parent = nullptr);
    ~DeviceInfo();

private:
    Ui::DeviceInfo *ui;
};

#endif
