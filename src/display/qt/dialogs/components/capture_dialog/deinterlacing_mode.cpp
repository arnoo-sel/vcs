#include "capture/capture.h"
#include "display/qt/persistent_settings.h"
#include "deinterlacing_mode.h"
#include "ui_deinterlacing_mode.h"

DeinterlacingMode::DeinterlacingMode(QWidget *parent) :
    VCSBaseDialog(parent),
    ui(new Ui::DeinterlacingMode)
{
    ui->setupUi(this);

    connect(ui->comboBox_deinterlacingMethod, &QComboBox::currentTextChanged, this, [](const QString &deinterlacingMethod)
    {
        if (deinterlacingMethod == "Weave")
        {
            kc_set_deinterlacing_mode(capture_deinterlacing_mode_e::weave);
        }
        else if (deinterlacingMethod == "Bob")
        {
            kc_set_deinterlacing_mode(capture_deinterlacing_mode_e::bob);
        }
        else if (deinterlacingMethod == "Field 1")
        {
            kc_set_deinterlacing_mode(capture_deinterlacing_mode_e::field_0);
        }
        else if (deinterlacingMethod == "Field 2")
        {
            kc_set_deinterlacing_mode(capture_deinterlacing_mode_e::field_1);
        }
        else
        {
            k_assert(0, "Unrecognized interlacing mode");
        }

        kpers_set_value(INI_GROUP_OUTPUT_WINDOW, "DeinterlacingMode", deinterlacingMethod);
    });

    // Restore the previous persistent setting.
    ui->comboBox_deinterlacingMethod->setCurrentText(kpers_value_of(INI_GROUP_OUTPUT_WINDOW, "DeinterlacingMode", ui->comboBox_deinterlacingMethod->itemText(0)).toString());
}

DeinterlacingMode::~DeinterlacingMode()
{
    delete ui;
}
