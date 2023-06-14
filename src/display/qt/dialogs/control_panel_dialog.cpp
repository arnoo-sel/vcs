#include <QPushButton>
#include <QVBoxLayout>
#include <QStyle>
#include "display/qt/dialogs/capture_dialog.h"
#include "display/qt/dialogs/window_options_dialog.h"
#include "display/qt/dialogs/video_presets_dialog.h"
#include "display/qt/dialogs/filter_graph_dialog.h"
#include "display/qt/dialogs/about_dialog.h"
#include "display/qt/dialogs/overlay_dialog.h"
#include "display/qt/windows/output_window.h"
#include "control_panel_dialog.h"
#include "ui_control_panel_dialog.h"

ControlPanelDialog::ControlPanelDialog(OutputWindow *parent) :
    VCSBaseDialog(parent),
    ui(new Ui::ControlPanelDialog)
{
    ui->setupUi(this);

    this->set_name("Control panel");
    this->setWindowFlags(Qt::Window);

    this->captureDialog = new CaptureDialog(parent);
    this->windowOptionsDialog = new WindowOptionsDialog(parent);
    this->filterGraphDialog = new FilterGraphDialog(parent);
    this->videoPresetsDialog = new VideoPresetsDialog(parent);
    this->overlayDialog = new OverlayDialog(parent);
    this->aboutDialog = new AboutDialog(parent);

    // Populate the side navi.
    {
        QVBoxLayout *const layout = dynamic_cast<QVBoxLayout*>(ui->buttonsContainer->layout());

        const auto add_navi_button = [this, layout](const QString &label, VCSBaseDialog *const dialog)->QPushButton*
        {
            auto *const naviButton = new QPushButton(label);
            naviButton->setFlat(true);
            naviButton->setFocusPolicy(Qt::NoFocus);
            layout->addWidget(naviButton);

            connect(naviButton, &QPushButton::pressed, this, [this, naviButton, dialog]
            {
                ui->contentsScroller->takeWidget();
                ui->contentsScroller->setWidget(dialog);
                dialog->show();

                foreach (auto *const childButton, this->ui->buttonsContainer->children())
                {
                    childButton->setProperty("isSelected", ((childButton == naviButton)? "true" : "false"));
                    this->style()->polish(dynamic_cast<QWidget*>(childButton));
                };
            });

            return naviButton;
        };

        auto *const defaultButton = add_navi_button("Capture", this->captureDialog);
        add_navi_button("Output", this->windowOptionsDialog);
        add_navi_button("Filter graph", this->filterGraphDialog);
        add_navi_button("Video presets", this->videoPresetsDialog);
        add_navi_button("Overlay", this->overlayDialog);
        add_navi_button("About VCS", this->aboutDialog);

        // Push the buttons against the top edge of the container.
        layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding));

        defaultButton->click();
    }
}

ControlPanelDialog::~ControlPanelDialog()
{
    delete ui;
}
