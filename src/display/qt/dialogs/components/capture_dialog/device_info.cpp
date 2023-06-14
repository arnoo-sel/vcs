#include "capture/capture.h"
#include "device_info.h"
#include "ui_device_info.h"

DeviceInfo::DeviceInfo(QWidget *parent) :
    VCSBaseDialog(parent),
    ui(new Ui::DeviceInfo)
{
    ui->setupUi(this);

    this->set_name("Device Info");

    // Fill the device feature matrix.
    {
        const resolution_s &minres = kc_get_device_minimum_resolution();
        const resolution_s &maxres = kc_get_device_maximum_resolution();

        ui->tableWidget_captureDeviceFeatures->modify_property("Name", QString::fromStdString(kc_get_device_name()));
        ui->tableWidget_captureDeviceFeatures->modify_property("Input range", QString("%1 \u00d7 %2 / %3 \u00d7 %4").arg(minres.w).arg(minres.h).arg(maxres.w).arg(maxres.h));
        ui->tableWidget_captureDeviceFeatures->modify_property("Input channels", QString::number(kc_get_device_maximum_input_count()));
        ui->tableWidget_captureDeviceFeatures->modify_property("Firmware", QString::fromStdString(kc_get_device_firmware_version()));
        ui->tableWidget_captureDeviceFeatures->modify_property("Driver", QString::fromStdString(kc_get_device_driver_version()));
    }
}

DeviceInfo::~DeviceInfo()
{
    delete ui;
}

