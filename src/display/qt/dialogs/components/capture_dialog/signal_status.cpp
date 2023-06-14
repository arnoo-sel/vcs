/*
 * 2020 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#include <QElapsedTimer>
#include <QFileDialog>
#include <QPushButton>
#include <QFileInfo>
#include <QToolBar>
#include <QMenuBar>
#include <QDebug>
#include <QTimer>
#include <QTime>
#include "display/qt/subclasses/QTableWidget_property_table.h"
#include "common/propagate/vcs_event.h"
#include "display/qt/utility.h"
#include "display/display.h"
#include "capture/capture.h"
#include "common/disk/disk.h"
#include "signal_status.h"
#include "ui_signal_status.h"

// Used to keep track of how long we've had a particular video mode set.
static QElapsedTimer VIDEO_MODE_UPTIME;
static QTimer INFO_UPDATE_TIMER;

// How many captured frames we've had to drop. This count is shown to the user
// in the signal dialog.
static unsigned NUM_DROPPED_FRAMES = 0;

// How many captured frames the capture subsystem has had to drop, in total. We'll
// use this value to derive NUM_DROPPED_FRAMES.
static unsigned GLOBAL_NUM_DROPPED_FRAMES = 0;

SignalStatus::SignalStatus(QWidget *parent) :
    VCSBaseDialog(parent),
    ui(new Ui::SignalStatus)
{
    ui->setupUi(this);

    this->set_name("Signal info");

    // Set the GUI controls to their proper initial values.
    {
        // Initialize the table of information. Note that this also sets
        // the vertical order in which the table's parameters are shown.
        {
            ui->tableWidget_propertyTable->modify_property("Analog", "-");
            ui->tableWidget_propertyTable->modify_property("Resolution", "-");
            ui->tableWidget_propertyTable->modify_property("Input rate", "-");
            ui->tableWidget_propertyTable->modify_property("Output rate", "-");
            ui->tableWidget_propertyTable->modify_property("Mode uptime", "-");
            ui->tableWidget_propertyTable->modify_property("Frames dropped", "-");
        }

        // Start timers to keep track of the video mode's uptime and dropped
        // frames count.
        {
            VIDEO_MODE_UPTIME.start();
            INFO_UPDATE_TIMER.start(1000);

            GLOBAL_NUM_DROPPED_FRAMES = kc_get_missed_frames_count();

            connect(&INFO_UPDATE_TIMER, &QTimer::timeout, [this]
            {
                // Update missed frames count.
                {
                    NUM_DROPPED_FRAMES += (kc_get_missed_frames_count() - GLOBAL_NUM_DROPPED_FRAMES);
                    GLOBAL_NUM_DROPPED_FRAMES = kc_get_missed_frames_count();

                    ui->tableWidget_propertyTable->modify_property("Frames dropped", QString::number(NUM_DROPPED_FRAMES));
                }

                // Update uptime.
                {
                    const unsigned seconds = unsigned(VIDEO_MODE_UPTIME.elapsed() / 1000);
                    const unsigned minutes = (seconds / 60);
                    const unsigned hours   = (minutes / 60);

                    if (!kc_is_receiving_signal())
                    {
                        ui->tableWidget_propertyTable->modify_property("Mode uptime", "-");
                    }
                    else
                    {
                        ui->tableWidget_propertyTable->modify_property(
                            "Mode uptime",
                            QString("%1:%2:%3")
                                .arg(QString::number(hours).rightJustified(2, '0'))
                                .arg(QString::number(minutes % 60).rightJustified(2, '0'))
                                .arg(QString::number(seconds % 60).rightJustified(2, '0'))
                        );
                    }
                }
            });
        }
    }

    // Listen for app events.
    {
        const auto update_info = [this]
        {
            VIDEO_MODE_UPTIME.restart();
            update_information_table(kc_is_receiving_signal());
        };

        kc_evSignalLost.listen([this]
        {
            this->set_controls_enabled(false);
        });

        kc_evSignalGained.listen([this]
        {
            this->set_controls_enabled(true);
        });

        ks_evFramesPerSecond.listen([this](const unsigned fps)
        {
            ui->tableWidget_propertyTable->modify_property("Output rate", QString("%1 FPS").arg(fps));
        });

        kc_evNewVideoMode.listen([update_info](const video_mode_s&)
        {
            update_info();
        });

        ks_evInputChannelChanged.listen(update_info);
    }

    return;
}

SignalStatus::~SignalStatus(void)
{
    delete ui;
    ui = nullptr;

    return;
}

void SignalStatus::set_controls_enabled(const bool state)
{
    update_information_table(state);

    return;
}

void SignalStatus::update_information_table(const bool isReceivingSignal)
{
    const auto inputChannelIdx = kc_get_device_input_channel_idx();

    if (isReceivingSignal)
    {
        const resolution_s resolution = kc_current_capture_state().input.resolution;
        const auto refreshRate = kc_current_capture_state().input.refreshRate.value<double>();

        ui->tableWidget_propertyTable->modify_property("Input rate", QString("%1 Hz").arg(QString::number(refreshRate, 'f', 3)));
        ui->tableWidget_propertyTable->modify_property("Resolution", QString("%1 \u00d7 %2").arg(resolution.w).arg(resolution.h));
        ui->tableWidget_propertyTable->modify_property("Analog", (kc_current_capture_state().signalFormat == signal_format_e::digital)? "No" : "Yes");
    }
    else
    {
        ui->tableWidget_propertyTable->modify_property("Resolution", "-");
        ui->tableWidget_propertyTable->modify_property("Input rate", "-");
        ui->tableWidget_propertyTable->modify_property("Analog", "-");
    }

    return;
}
