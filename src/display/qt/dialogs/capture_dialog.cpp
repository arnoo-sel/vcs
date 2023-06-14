#include <QVBoxLayout>
#include "display/qt/dialogs/components/capture_dialog/input_resolution.h"
#include "display/qt/dialogs/components/capture_dialog/input_channel.h"
#include "display/qt/dialogs/components/capture_dialog/device_info.h"
#include "display/qt/dialogs/components/capture_dialog/signal_status.h"
#include "display/qt/dialogs/components/capture_dialog/deinterlacing_mode.h"
#include "capture_dialog.h"
#include "ui_capture_dialog.h"

CaptureDialog::CaptureDialog(QWidget *parent) :
    VCSBaseDialog(parent),
    ui(new Ui::CaptureDialog)
{
    ui->setupUi(this);

    auto *const layout = new QVBoxLayout(this);
    layout->setSpacing(6);
    layout->setContentsMargins(6, 6, 6, 6);

    this->inputChannel = new InputChannel(parent);
    this->signalStatus = new SignalStatus(parent);
    this->inputResolution = new InputResolution(parent);
    this->deviceInfo = new DeviceInfo(parent);
    this->deinterlacingMode = new DeinterlacingMode(parent);

    layout->addWidget(this->inputChannel);
    layout->addWidget(this->signalStatus);
    layout->addWidget(this->inputResolution);
    layout->addWidget(this->deviceInfo);
    layout->addWidget(this->deinterlacingMode);

    // Push out empty space from the dialogs.
    layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding));
}

CaptureDialog::~CaptureDialog()
{
    delete ui;
}
