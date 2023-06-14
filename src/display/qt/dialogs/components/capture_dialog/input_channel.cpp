/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "capture/capture.h"
#include "common/globals.h"
#include "common/assert.h"
#include "input_channel.h"
#include "ui_input_channel.h"

InputChannel::InputChannel(QWidget *parent) :
    VCSBaseDialog(parent),
    ui(new Ui::InputChannel)
{
    ui->setupUi(this);

    this->set_name("Input channel");

    ui->spinBox_deviceIdx->setValue(INPUT_CHANNEL_IDX);
    ui->spinBox_deviceIdx->setMinimum(0);
    ui->spinBox_deviceIdx->setMaximum(255);

    // Connect GUI controls to consequences for operating them.
    {
        connect(ui->spinBox_deviceIdx, &QSpinBox::editingFinished, this, [=]
        {
            kc_set_capture_input_channel(ui->spinBox_deviceIdx->value());
        });
    }

    return;
}

InputChannel::~InputChannel()
{
    delete ui;

    return;
}
