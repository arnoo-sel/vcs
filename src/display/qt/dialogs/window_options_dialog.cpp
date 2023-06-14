#include <QVBoxLayout>
#include "display/qt/dialogs/components/window_options_dialog/window_renderer.h"
#include "display/qt/dialogs/components/window_options_dialog/window_scaler.h"
#include "display/qt/dialogs/components/window_options_dialog/window_size.h"
#include "display/qt/windows/output_window.h"
#include "window_options_dialog.h"
#include "ui_window_options_dialog.h"

WindowOptionsDialog::WindowOptionsDialog(OutputWindow *parent) :
    VCSBaseDialog(parent),
    ui(new Ui::WindowOptionsDialog)
{
    ui->setupUi(this);

    this->set_name("Window settings");

    auto *const layout = new QVBoxLayout(this);
    layout->setSpacing(6);
    layout->setContentsMargins(6, 6, 6, 6);

    this->windowSize = new WindowSize(parent);
    this->windowScaler = new WindowScaler(parent);
    this->windowRenderer = new WindowRenderer(parent);

    layout->addWidget(this->windowSize);
    layout->addWidget(this->windowScaler);
    layout->addWidget(this->windowRenderer);

    // Push out empty space from the dialogs.
    layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding));
}

WindowOptionsDialog::~WindowOptionsDialog()
{
    delete ui;
}
