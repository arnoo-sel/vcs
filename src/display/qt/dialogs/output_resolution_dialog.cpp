#include "display/qt/dialogs/output_resolution_dialog.h"
#include "display/qt/persistent_settings.h"
#include "common/propagate/vcs_event.h"
#include "capture/capture.h"
#include "common/globals.h"
#include "scaler/scaler.h"
#include "ui_output_resolution_dialog.h"

OutputResolutionDialog::OutputResolutionDialog(QWidget *parent) :
    VCSBaseDialog(parent),
    ui(new Ui::OutputResolutionDialog)
{
    ui->setupUi(this);

    this->set_name("Output size");

    disable_output_size_controls(false);

    // Initialize GUI controls to their starting values.
    {
        ui->spinBox_outputResX->setMaximum(MAX_OUTPUT_WIDTH);
        ui->spinBox_outputResX->setMinimum(MIN_OUTPUT_WIDTH);

        ui->spinBox_outputResY->setMaximum(MAX_OUTPUT_HEIGHT);
        ui->spinBox_outputResY->setMinimum(MIN_OUTPUT_HEIGHT);
    }

    // Connect the GUI controls to consequences for changing their values.
    {
        // Returns true if the given checkbox is in a proper checked state. Returns
        // false if it's in a proper unchecked state; and assert fails if the checkbox
        // is neither fully checked nor fully unchecked.
        const auto is_checked = [](int qcheckboxCheckedState)->bool
        {
            k_assert(qcheckboxCheckedState != Qt::PartiallyChecked,
                     "Expected this QCheckBox to have a two-state toggle. It appears to have a third state.");

            return ((qcheckboxCheckedState == Qt::Checked)? true : false);
        };

        connect(ui->checkBox_forceOutputScale, &QCheckBox::stateChanged, this, [=](int state)
        {
            if (is_checked(state))
            {
                ks_set_scaling_multiplier(ui->spinBox_outputScale->value() / 100.0);
            }
            else
            {
                ui->spinBox_outputScale->setValue(100);
            }

            ui->spinBox_outputScale->setEnabled(is_checked(state));
            ks_set_scaling_multiplier_enabled(is_checked(state));
        });

        connect(ui->checkBox_forceOutputRes, &QCheckBox::stateChanged, this, [=](int state)
        {
            ui->spinBox_outputResX->setEnabled(is_checked(state));
            ui->spinBox_outputResY->setEnabled(is_checked(state));
            ui->label_resolutionX->setEnabled(is_checked(state));

            ks_set_base_resolution_enabled(is_checked(state));

            if (is_checked(state))
            {
                ks_set_base_resolution({(unsigned)ui->spinBox_outputResX->value(),
                                            (unsigned)ui->spinBox_outputResY->value()});
            }
            else
            {
                ui->spinBox_outputResX->setValue(ks_base_resolution().w);
                ui->spinBox_outputResY->setValue(ks_base_resolution().h);
            }
        });

        connect(ui->spinBox_outputResX, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this,
                [this]
        {
            if (!ui->checkBox_forceOutputRes->isChecked()) return;

            const resolution_s r = {(uint)ui->spinBox_outputResX->value(),
                                    (uint)ui->spinBox_outputResY->value()};

            ks_set_base_resolution(r);
        });

        connect(ui->spinBox_outputResY, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this,
                [this]
        {
            if (!ui->checkBox_forceOutputRes->isChecked()) return;

            const resolution_s r = {(uint)ui->spinBox_outputResX->value(),
                                    (uint)ui->spinBox_outputResY->value()};

            ks_set_base_resolution(r);
        });

        connect(ui->spinBox_outputScale, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this,
                [this]{ ks_set_scaling_multiplier(ui->spinBox_outputScale->value() / 100.0); });
    }

    // Restore persistent settings.
    ui->checkBox_forceOutputRes->setChecked(kpers_value_of(INI_GROUP_OUTPUT, "force_output_size", false).toBool());
    ui->spinBox_outputResX->setValue(kpers_value_of(INI_GROUP_OUTPUT, "output_size", QSize(640, 480)).toSize().width());
    ui->spinBox_outputResY->setValue(kpers_value_of(INI_GROUP_OUTPUT, "output_size", QSize(640, 480)).toSize().height());
    ui->checkBox_forceOutputScale->setChecked(kpers_value_of(INI_GROUP_OUTPUT, "force_relative_scale", false).toBool());
    ui->spinBox_outputScale->setValue(kpers_value_of(INI_GROUP_OUTPUT, "relative_scale", 100).toInt());
    this->resize(kpers_value_of(INI_GROUP_GEOMETRY, "output_resolution", this->size()).toSize());

    // Listen for app events.
    {
        kc_evNewVideoMode.listen([this](const video_mode_s &videoMode)
        {
            if (!ui->checkBox_forceOutputRes->isChecked())
            {
                ui->spinBox_outputResX->setValue(videoMode.resolution.w);
                ui->spinBox_outputResY->setValue(videoMode.resolution.h);
            }
        });

        ks_evCustomScalerEnabled.listen([this]
        {
            // The output scaling filter handles output sizing, so we want to bar the
            // user from manipulating it via this dialog.
            this->disable_output_size_controls(true);
        });

        ks_evCustomScalerDisabled.listen([this]
        {
            this->disable_output_size_controls(false);
        });
    }

    return;
}

OutputResolutionDialog::~OutputResolutionDialog()
{
    // Save persistent settings.
    {
        kpers_set_value(INI_GROUP_OUTPUT, "force_output_size", ui->checkBox_forceOutputRes->isChecked());
        kpers_set_value(INI_GROUP_OUTPUT, "output_size", QSize(ui->spinBox_outputResX->value(), ui->spinBox_outputResY->value()));
        kpers_set_value(INI_GROUP_OUTPUT, "force_relative_scale", ui->checkBox_forceOutputScale->isChecked());
        kpers_set_value(INI_GROUP_OUTPUT, "relative_scale", ui->spinBox_outputScale->value());
        kpers_set_value(INI_GROUP_GEOMETRY, "output_resolution", this->size());
    }

    delete ui;

    return;
}

// Adjusts the output scale value in the GUI by a pre-set step size in the given
// direction. Note that this will automatically trigger a change in the actual
// scaler output size as well.
void OutputResolutionDialog::adjust_output_scaling(const int dir)
{
    const int curValue = ui->spinBox_outputScale->value();
    const int stepSize = ui->spinBox_outputScale->singleStep();

    k_assert((dir == 1 || dir == -1),
             "Expected the parameter for AdjustOutputScaleValue to be either 1 or -1.");

    if (!ui->checkBox_forceOutputScale->isChecked())
    {
        ui->checkBox_forceOutputScale->setChecked(true);
    }

    ui->spinBox_outputScale->setValue(curValue + (stepSize * dir));

    return;
}

void OutputResolutionDialog::disable_output_size_controls(const bool areDisabled)
{
    ui->groupBox->setDisabled(areDisabled);

    return;
}

void OutputResolutionDialog::notify_of_new_capture_signal(void)
{
    if (!ui->checkBox_forceOutputRes->isChecked())
    {
        const resolution_s r = kc_get_capture_resolution();

        ui->spinBox_outputResX->setValue(r.w);
        ui->spinBox_outputResY->setValue(r.h);
    }

    return;
}
