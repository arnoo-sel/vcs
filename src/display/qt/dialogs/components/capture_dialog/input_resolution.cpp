/*
 * 2019 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#include "display/qt/dialogs/resolution_dialog.h"
#include "display/qt/persistent_settings.h"
#include "display/qt/utility.h"
#include "common/propagate/vcs_event.h"
#include "capture/capture.h"
#include "input_resolution.h"
#include "ui_input_resolution.h"

InputResolution::InputResolution(QWidget *parent) :
    VCSBaseDialog(parent),
    ui(new Ui::InputResolution)
{
    ui->setupUi(this);

    this->set_name("Input resolution");

    // Connect GUI controls to consequences for operating them.
    {
        // Wire up buttons for forcing the capture input resolution.
        for (int i = 0; i < ui->frame_inputForceButtons->layout()->count(); i++)
        {
            QPushButton *const button = qobject_cast<QPushButton*>(ui->frame_inputForceButtons->layout()->itemAt(i)->widget());

            if (!button)
            {
                continue;
            }

            // Sanity check. Buttons that force the resolution have no label text but
            // include the "resolution" dynamic property. Auxiliary buttons don't define
            // the property but have a label text. Other buttons are considered malformed.
            k_assert((!button->text().isEmpty() ||
                      button->property("resolution").isValid()),
                     "Detected a malformed input resolution button.");

            // Ignore non-resolution-setting buttons.
            if (!button->property("resolution").isValid())
            {
                continue;
            }

            this->update_button_text(button);

            connect(button, &QPushButton::clicked, this, [this, button]
            {
                const QSize buttonResolution = button->property("resolution").toSize();
                k_assert(buttonResolution.isValid(), "Detected a malformed input resolution button.");

                kc_force_capture_resolution({
                    unsigned(buttonResolution.width()),
                    unsigned(buttonResolution.height())
                });
            });
        }

        connect(ui->pushButton_setCustomResolution, &QPushButton::clicked, this, [this]
        {
            resolution_s customResolution = {1920, 1080};

            if (ResolutionDialog("Force an input resolution", &customResolution, parentWidget()).exec() == QDialog::Accepted)
            {
                kc_force_capture_resolution(customResolution);
            }
        });
    }

    return;
}

InputResolution::~InputResolution(void)
{
    delete ui;

    return;
}

void InputResolution::update_button_text(QPushButton *const button)
{
    const QSize buttonResolution = button->property("resolution").toSize();

    if (buttonResolution.isValid())
    {
        button->setText(QString("%1 \u00d7 %2").arg(buttonResolution.width()). arg(buttonResolution.height()));
    }

    return;
}

void InputResolution::activate_resolution_button(const uint buttonIdx)
{
    for (int i = 0; i < ui->frame_inputForceButtons->layout()->count(); i++)
    {
        QPushButton *const button = qobject_cast<QPushButton*>(ui->frame_inputForceButtons->layout()->itemAt(i)->widget());

        if (!button)
        {
            continue;
        }

        if (button->property("button_idx").toUInt() == buttonIdx)
        {
            button->click();
            return;
        }
    }

    NBENE(("Failed to find an input resolution button at index #%u.", buttonIdx));

    return;
}
