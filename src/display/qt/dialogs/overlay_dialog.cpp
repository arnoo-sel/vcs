/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS overlay dialog
 *
 * The overlay allows the user to have custom text appear over the captured frame
 * stream. The overlay dialog, then, allows the user to edit that overlay's contents.
 *
 */

#include <QPlainTextEdit>
#include <QFileDialog>
#include <QDateTime>
#include <QPainter>
#include <QMenuBar>
#include <QDebug>
#include <QMenu>
#include "display/qt/dialogs/overlay_dialog.h"
#include "display/qt/persistent_settings.h"
#include "display/qt/utility.h"
#include "display/display.h"
#include "ui_overlay_dialog.h"

OverlayDialog::OverlayDialog(QWidget *parent) :
    VCSBaseDialog(parent),
    ui(new Ui::OverlayDialog)
{
    overlayDocument.setDefaultFont(QGuiApplication::font());
    overlayDocument.setDocumentMargin(0);

    ui->setupUi(this);

    this->set_name("Overlay editor");

    ui->plainTextEdit->setTabStopDistance(22);

    // Create the dialog's menu bar.
    {
        this->menuBar = new QMenuBar(this);

        const auto insert_text = [this](const QString &text)
        {
            this->ui->plainTextEdit->insertPlainText(text);
            this->ui->plainTextEdit->setFocus();
        };

        // Overlay...
        {
            QMenu *overlayMenu = new QMenu("Overlay", this->menuBar);

            {
                QAction *enable = new QAction("Enabled", this->menuBar);
                enable->setCheckable(true);
                enable->setChecked(this->is_enabled());

                connect(this, &VCSBaseDialog::enabled_state_set, this, [=](const bool isEnabled)
                {
                    enable->setChecked(isEnabled);
                    kd_update_output_window_title();
                });

                connect(enable, &QAction::triggered, this, [=]
                {
                    this->set_enabled(!this->is_enabled());
                });

                overlayMenu->addAction(enable);
            }

            this->menuBar->addMenu(overlayMenu);
        }

        // Insert...
        {
            QMenu *insertMenu = new QMenu("Insert", this->menuBar);

            // Variables
            {
                QMenu *variablesMenu = new QMenu("Variables", this->menuBar);

                const auto add_variable_action = [this, variablesMenu, insert_text](const QString &title, const QString &code)
                {
                    connect(variablesMenu->addAction(title), &QAction::triggered, this, [=]{insert_text(code);});
                };

                add_variable_action("Capture width", "$inWidth");
                add_variable_action("Capture height", "$inHeight");
                add_variable_action("Capture refresh rate", "$inRate");
                add_variable_action("Capture channel", "$inChannel");
                variablesMenu->addSeparator();
                add_variable_action("Output width", "$outWidth");
                add_variable_action("Output height", "$outHeight");
                add_variable_action("Output refresh rate", "$outRate");
                add_variable_action("Frame drop indicator", "$frameDropIndicator");
                variablesMenu->addSeparator();
                add_variable_action("Current time", "$time");
                add_variable_action("Current date", "$date");

                insertMenu->addMenu(variablesMenu);
            }

            // HTML
            {
                QMenu *htmlMenu = new QMenu("HTML", this->menuBar);

                connect(htmlMenu->addAction("Image..."), &QAction::triggered, this, [=]
                {
                    const QString filename = QFileDialog::getOpenFileName(
                        this,
                        "Select an image", "",
                        "Image files (*.png *.gif *.jpeg *.jpg *.bmp);; All files(*.*)"
                    );

                    if (filename.isEmpty())
                    {
                        return;
                    }

                    insert_text("<img src=\"" + filename + "\">\n");
                });

                htmlMenu->addSeparator();

                connect(htmlMenu->addAction("Table"), &QAction::triggered, this, [=]
                {
                    insert_text(
                        "<table style=\"color: white; background-color: black;\">\n"
                        "\t<tr>\n"
                        "\t\t<td>Cell 1</td>\n"
                        "\t\t<td>Cell 2</td>\n"
                        "\t</tr>\n"
                        "</table>\n"
                    );
                });

                connect(htmlMenu->addAction("Style block"), &QAction::triggered, this, [=]
                {
                    insert_text(
                        "<style>\n"
                        "\tdiv {\n"
                        "\t\tcolor: red;\n"
                        "\t}\n"
                        "</style>\n"
                    );
                });

                htmlMenu->addSeparator();

                connect(htmlMenu->addAction("Align right"), &QAction::triggered, this, [=]
                {
                    insert_text("<div style=\"text-align: right;\">Text</div>\n");
                });

                connect(htmlMenu->addAction("Align center"), &QAction::triggered, this, [=]
                {
                    insert_text("<div style=\"text-align: center;\">Text</div>\n");
                });

                insertMenu->addMenu(htmlMenu);
            }

            this->menuBar->addMenu(insertMenu);
        }

        this->layout()->setMenuBar(this->menuBar);
    }

    // Restore persistent settings.
    {
        ui->plainTextEdit->setPlainText(kpers_value_of(INI_GROUP_OVERLAY, "content", "").toString());
        this->set_enabled(kpers_value_of(INI_GROUP_OVERLAY, "enabled", false).toBool());
        this->resize(kpers_value_of(INI_GROUP_GEOMETRY, "overlay", this->size()).toSize());
    }

    return;
}

OverlayDialog::~OverlayDialog()
{
    // Save persistent settings.
    {
        kpers_set_value(INI_GROUP_OVERLAY, "enabled", this->is_enabled());
        kpers_set_value(INI_GROUP_OVERLAY, "content", ui->plainTextEdit->toPlainText());
        kpers_set_value(INI_GROUP_GEOMETRY, "overlay", this->size());
    }

    delete ui;
    ui = nullptr;

    return;
}

void OverlayDialog::set_overlay_max_width(const uint width)
{
    overlayDocument.setTextWidth(width);

    return;
}

// Renders the overlay into a QImage, and returns the image.
QImage OverlayDialog::rendered(void)
{
    const QString overlaySource = ([this]()->QString
    {
        QString source = this->ui->plainTextEdit->toPlainText();

        source.replace("$inWidth", QString::number(kc_current_capture_state().input.resolution.w));
        source.replace("$inHeight", QString::number(kc_current_capture_state().input.resolution.h));
        source.replace("$inRate", QString::number(kc_current_capture_state().input.refreshRate.value<double>(), 'f', 3));
        source.replace("$inChannel",
        #if __linux__
            QString("/dev/video%1").arg(kc_current_capture_state().hardwareChannelIdx)
        #else
            QString::number(kc_current_capture_state().hardwareChannelIdx + 1)
        #endif
        );
        source.replace("$outWidth", QString::number(kc_current_capture_state().output.resolution.w));
        source.replace("$outHeight", QString::number(kc_current_capture_state().output.resolution.h));
        source.replace("$outRate", QString::number(kc_current_capture_state().output.refreshRate.value<double>()));
        source.replace("$frameDropIndicator", (kc_current_capture_state().areFramesBeingDropped? "Dropping frames" : ""));
        source.replace("$time", QDateTime::currentDateTime().time().toString());
        source.replace("$date", QDateTime::currentDateTime().date().toString());

        return (
            "<span style=\"font-size: large; color: white;\">" +
            source +
            "</span>"
        );
    })();

    QImage image = QImage(kc_current_capture_state().output.resolution.w, kc_current_capture_state().output.resolution.h, QImage::Format_ARGB32_Premultiplied);
    image.fill("transparent");

    QPainter painter(&image);
    overlayDocument.setHtml(overlaySource);
    overlayDocument.drawContents(&painter, image.rect());

    return image;
}
