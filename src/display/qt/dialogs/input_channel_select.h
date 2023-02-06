/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_DISPLAY_QT_DIALOGS_INPUT_CHANNEL_SELECT_H
#define VCS_DISPLAY_QT_DIALOGS_INPUT_CHANNEL_SELECT_H

#include <QDialog>

namespace Ui {
class LinuxDeviceSelectorDialog;
}

class LinuxDeviceSelectorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LinuxDeviceSelectorDialog(unsigned *const deviceIdx, QWidget *parent = 0);
    ~LinuxDeviceSelectorDialog();

private:
    Ui::LinuxDeviceSelectorDialog *ui;

    // The device index value (for /dev/videoX) that we'll modify.
    unsigned *const deviceIdx;
};

#endif
