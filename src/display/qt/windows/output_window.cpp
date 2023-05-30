/*
 * 2018-2022 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 *
 * VCS's main window. Displays captured frames and provides the user access to child dialogs
 * for adjusting various program settings.
 *
 */

#include <QRegularExpression>
#include <QElapsedTimer>
#include <QTextDocument>
#include <QElapsedTimer>
#include <QFontDatabase>
#include <QInputDialog>
#include <QActionGroup>
#include <QVBoxLayout>
#include <QTreeWidget>
#include <QMessageBox>
#include <QMouseEvent>
#include <QDateTime>
#include <QShortcut>
#include <QPainter>
#include <QScreen>
#include <QImage>
#include <QMenu>
#include <QLabel>
#include <QDir>
#include <cmath>
#include "display/qt/subclasses/QLabel_magnifying_glass.h"
#include "display/qt/subclasses/QOpenGLWidget_opengl_renderer.h"
#include "display/qt/subclasses/QDialog_vcs_base_dialog.h"
#include "display/qt/dialogs/linux_device_selector_dialog.h"
#include "display/qt/dialogs/output_resolution_dialog.h"
#include "display/qt/dialogs/video_presets_dialog.h"
#include "display/qt/dialogs/input_resolution_dialog.h"
#include "display/qt/dialogs/signal_dialog.h"
#include "display/qt/dialogs/filter_graph_dialog.h"
#include "display/qt/dialogs/resolution_dialog.h"
#include "display/qt/dialogs/anti_tear_dialog.h"
#include "display/qt/dialogs/overlay_dialog.h"
#include "display/qt/dialogs/record_dialog.h"
#include "display/qt/windows/output_window.h"
#include "display/qt/dialogs/alias_dialog.h"
#include "display/qt/dialogs/about_dialog.h"
#include "display/qt/persistent_settings.h"
#include "display/qt/keyboard_shortcuts.h"
#include "anti_tear/anti_tear.h"
#include "common/propagate/vcs_event.h"
#include "capture/video_presets.h"
#include "capture/capture.h"
#include "capture/alias.h"
#include "common/globals.h"
#include "record/record.h"
#include "scaler/scaler.h"
#include "main.h"
#include "ui_output_window.h"

// For an optional OpenGL render surface.
static OGLWidget *OGL_SURFACE = nullptr;

static unsigned CURRENT_OUTPUT_FPS = 0;

static bool IS_FRAME_DROP_INDICATOR_ENABLED = true;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->setWindowFlags(this->defaultWindowFlags);

    ui->centralwidget->setMouseTracking(true);
    ui->centralwidget->setFocusPolicy(Qt::StrongFocus);

    // Set up a layout for the central widget, so we can add the OpenGL
    // render surface to it when/if OpenGL is enabled.
    QVBoxLayout *const mainLayout = new QVBoxLayout(ui->centralwidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    this->magnifyingGlass = new MagnifyingGlass(this);

    // Restore persistent settings.
    {
        IS_FRAME_DROP_INDICATOR_ENABLED = kpers_value_of(INI_GROUP_OUTPUT, "frame_drop_indicator", IS_FRAME_DROP_INDICATOR_ENABLED).toBool();
        this->appwideFontSize = kpers_value_of(INI_GROUP_APP, "font_size", this->appwideFontSize).toUInt();
        k_set_eco_mode_enabled(kpers_value_of(INI_GROUP_APP, "eco_mode", k_is_eco_mode_enabled()).toBool());
    }

    // Set up the child dialogs.
    {
        outputResolutionDlg = new OutputResolutionDialog;
        inputResolutionDlg = new InputResolutionDialog;
        videoPresetsDlg = new VideoPresetsDialog;
        filterGraphDlg = new FilterGraphDialog;
        antitearDlg = new AntiTearDialog;
        overlayDlg = new OverlayDialog;
        signalDlg = new SignalDialog;
        aliasDlg = new AliasDialog;
        aboutDlg = new AboutDialog;
        recordDlg = new RecordDialog;

        this->dialogs << outputResolutionDlg
                      << inputResolutionDlg
                      << filterGraphDlg
                      << videoPresetsDlg
                      << antitearDlg
                      << overlayDlg
                      << signalDlg
                      << aliasDlg
                      << aboutDlg
                      << recordDlg;
    }

    // Create the window's context menu.
    {
        this->contextMenu = new QMenu(this);

        QMenu *captureMenu = new QMenu("Input", this);
        {
            QMenu *channel = new QMenu("Channel", this);
            {
                #if __linux__
                    QAction *select = new QAction("Select...", this);
                    channel->addAction(select);
                    connect(select, &QAction::triggered, this, [this]
                    {
                        unsigned newIdx = kc_get_device_input_channel_idx();

                        if (LinuxDeviceSelectorDialog(&newIdx).exec() != QDialog::Rejected)
                        {
                            kc_set_capture_input_channel(newIdx);
                        }
                    });

                    channel->addSeparator();
                #else
                    QActionGroup *group = new QActionGroup(this);

                    for (int i = 0; i < kc_get_device_maximum_input_count(); i++)
                    {
                        QAction *inputChannel = new QAction(QString::number(i+1), this);
                        inputChannel->setActionGroup(group);
                        inputChannel->setCheckable(true);
                        channel->addAction(inputChannel);

                        if (i == int(INPUT_CHANNEL_IDX))
                        {
                            inputChannel->setChecked(true);
                        }

                        connect(inputChannel, &QAction::triggered, this, [=]{kc_set_capture_input_channel(i);});
                    }
                #endif
            }

            QMenu *colorDepth = new QMenu("Color depth", this);
            {
                QActionGroup *group = new QActionGroup(this);

                QAction *c24 = new QAction("24-bit (RGB 888)", this);
                c24->setActionGroup(group);
                c24->setCheckable(true);
                c24->setChecked(true);
                colorDepth->addAction(c24);

                // Note: the Video4Linux capture API currently supports no other capture
                // color format besides RGB888.
                #ifndef CAPTURE_BACKEND_VISION_V4L
                    QAction *c16 = new QAction("16-bit (RGB 565)", this);
                    c16->setActionGroup(group);
                    c16->setCheckable(true);
                    colorDepth->addAction(c16);

                    QAction *c15 = new QAction("15-bit (RGB 555)", this);
                    c15->setActionGroup(group);
                    c15->setCheckable(true);
                    colorDepth->addAction(c15);

                    connect(c16, &QAction::triggered, this, [=]{kc_set_capture_pixel_format(capture_pixel_format_e::rgb_565);});
                    connect(c15, &QAction::triggered, this, [=]{kc_set_capture_pixel_format(capture_pixel_format_e::rgb_555);});
                #endif
                connect(c24, &QAction::triggered, this, [=]{kc_set_capture_pixel_format(capture_pixel_format_e::rgb_888);});
            }

            QMenu *deinterlacing = new QMenu("De-interlacing", this);
            {
                deinterlacing->setEnabled(kc_device_supports_deinterlacing());

                QActionGroup *group = new QActionGroup(this);

                QAction *weave = new QAction("Weave", this);
                weave->setActionGroup(group);
                weave->setCheckable(true);
                deinterlacing->addAction(weave);

                QAction *bob = new QAction("Bob", this);
                bob->setActionGroup(group);
                bob->setCheckable(true);
                deinterlacing->addAction(bob);

                QAction *field0 = new QAction("Field 1", this);
                field0->setActionGroup(group);
                field0->setCheckable(true);
                deinterlacing->addAction(field0);

                QAction *field1 = new QAction("Field 2", this);
                field1->setActionGroup(group);
                field1->setCheckable(true);
                deinterlacing->addAction(field1);

                const auto set_mode = [=](const capture_deinterlacing_mode_e mode)
                {
                    switch (mode)
                    {
                        case capture_deinterlacing_mode_e::bob:
                        {
                            kc_set_deinterlacing_mode(capture_deinterlacing_mode_e::bob);
                            kpers_set_value(INI_GROUP_OUTPUT, "interlacing_mode", "Bob");
                            break;
                        }
                        case capture_deinterlacing_mode_e::weave:
                        {
                            kc_set_deinterlacing_mode(capture_deinterlacing_mode_e::weave);
                            kpers_set_value(INI_GROUP_OUTPUT, "interlacing_mode", "Weave");
                            break;
                        }
                        case capture_deinterlacing_mode_e::field_0:
                        {
                            kc_set_deinterlacing_mode(capture_deinterlacing_mode_e::field_0);
                            kpers_set_value(INI_GROUP_OUTPUT, "interlacing_mode", "Field 1");
                            break;
                        }
                        case capture_deinterlacing_mode_e::field_1:
                        {
                            kc_set_deinterlacing_mode(capture_deinterlacing_mode_e::field_1);
                            kpers_set_value(INI_GROUP_OUTPUT, "interlacing_mode", "Field 2");
                            break;
                        }
                        default: k_assert(0, "Unknown deinterlacing mode."); break;
                    }
                };

                connect(bob, &QAction::triggered, this, [=]{set_mode(capture_deinterlacing_mode_e::bob);});
                connect(weave, &QAction::triggered, this, [=]{set_mode(capture_deinterlacing_mode_e::weave);});
                connect(field0, &QAction::triggered, this, [=]{set_mode(capture_deinterlacing_mode_e::field_0);});
                connect(field1, &QAction::triggered, this, [=]{set_mode(capture_deinterlacing_mode_e::field_1);});

                // Activate the default setting.
                {
                    const QString defaultMode = kpers_value_of(INI_GROUP_OUTPUT, "interlacing_mode", "Weave").toString();

                    QAction *action = bob;

                    if      (defaultMode.toLower() == "bob") action = bob;
                    else if (defaultMode.toLower() == "weave") action = weave;
                    else if (defaultMode.toLower() == "field 1") action = field0;
                    else if (defaultMode.toLower() == "field 2") action = field1;

                    action->trigger();
                }
            }

            captureMenu->addMenu(channel);
            captureMenu->addMenu(colorDepth);
            captureMenu->addMenu(deinterlacing);
            captureMenu->addSeparator();

            QAction *aliases = new QAction("Aliases...", this);
            captureMenu->addAction(aliases);

            QAction *resolution = new QAction("Resolution...", this);
            resolution->setShortcut(kd_get_key_sequence("output-window: open-input-resolution-dialog"));
            captureMenu->addAction(resolution);

            QAction *signal = new QAction("Signal info...", this);
            signal->setShortcut(kd_get_key_sequence("output-window: open-signal-info-dialog"));
            captureMenu->addAction(signal);

            QAction *videoPresets = new QAction("Video presets...", this);
            videoPresets->setShortcut(kd_get_key_sequence("output-window: open-video-presets-dialog"));
            captureMenu->addAction(videoPresets);

            #if CAPTURE_BACKEND_VISION_V4L
                aliases->setEnabled(false);
                INFO(("Aliases are not supported with Video4Linux."));
            #endif

            connect(signal, &QAction::triggered, this, [=]{this->signalDlg->open();});
            connect(videoPresets, &QAction::triggered, this, [=]{this->videoPresetsDlg->open();});
            connect(resolution, &QAction::triggered, this, [=]{this->inputResolutionDlg->open();});
            connect(aliases, &QAction::triggered, this, [=]{this->aliasDlg->open();});
        }

        QMenu *outputMenu = new QMenu("Output", this);
        {
            const std::vector<std::string> scalerNames = ks_scaler_names();
            k_assert(!scalerNames.empty(), "Expected to receive a list of scalers, but got an empty list.");

            QMenu *defaultScalerMenu = new QMenu("Scaler", this);
            {
                QActionGroup *group = new QActionGroup(this);

                const QString defaultUpscalerName = kpers_value_of(INI_GROUP_OUTPUT, "default_scaler", "Linear").toString();

                for (const auto &scalerName: scalerNames)
                {
                    QAction *scalerAction = new QAction(QString::fromStdString(scalerName), this);
                    scalerAction->setActionGroup(group);
                    scalerAction->setCheckable(true);
                    defaultScalerMenu->addAction(scalerAction);

                    connect(scalerAction, &QAction::toggled, this, [=](const bool checked)
                    {
                        if (checked)
                        {
                            ks_set_default_scaler(scalerName);
                        }
                    });

                    if (QString::fromStdString(scalerName) == defaultUpscalerName)
                    {
                        scalerAction->setChecked(true);
                    }
                }

                ks_evCustomScalerEnabled.listen([=]{defaultScalerMenu->setEnabled(false);});
                ks_evCustomScalerDisabled.listen([=]{defaultScalerMenu->setEnabled(true);});
            }

            outputMenu->addMenu(defaultScalerMenu);
            outputMenu->addSeparator();

            QAction *antiTear = new QAction("Anti-tear...", this);
            antiTear->setShortcut(kd_get_key_sequence("output-window: open-anti-tear-dialog"));
            outputMenu->addAction(antiTear);
            connect(antiTear, &QAction::triggered, this, [=]{this->antitearDlg->open();});

            QAction *filter = new QAction("Filter graph...", this);
            filter->setShortcut(kd_get_key_sequence("output-window: open-filter-graph-dialog"));
            outputMenu->addAction(filter);
            connect(filter, &QAction::triggered, this, [=]{this->filterGraphDlg->open();});

            QAction *overlay = new QAction("Overlay editor...", this);
            overlay->setShortcut(kd_get_key_sequence("output-window: open-overlay-dialog"));
            outputMenu->addAction(overlay);
            connect(overlay, &QAction::triggered, this, [=]{this->overlayDlg->open();});

            QAction *resolution = new QAction("Size...", this);
            resolution->setShortcut(kd_get_key_sequence("output-window: open-output-resolution-dialog"));
            outputMenu->addAction(resolution);
            connect(resolution, &QAction::triggered, this, [=]{this->outputResolutionDlg->open();});

            QAction *record = new QAction("Video recorder...", this);
            record->setShortcut(kd_get_key_sequence("output-window: open-record-dialog"));
            outputMenu->addAction(record);
            connect(record, &QAction::triggered, this, [=]{this->recordDlg->open();});
        }

        QMenu *windowMenu = new QMenu("Window", this);
        {
            {
                windowMenu->addSeparator();

                QAction *customTitle = new QAction("Set title...", this);

                connect(customTitle, &QAction::triggered, this, [=]
                {
                    const QString newTitle = QInputDialog::getMultiLineText(
                        this,
                        QString("Custom window title - %2").arg(PROGRAM_NAME),
                        "Enter a title for the output window, or leave empty to restore to default.",
                        this->windowTitleOverride
                    );

                    if (!newTitle.isNull())
                    {
                        this->windowTitleOverride = newTitle;
                        this->update_window_title();
                    }
                });

                connect(this, &MainWindow::fullscreen_mode_enabled, this, [=]
                {
                    customTitle->setEnabled(false);
                });

                connect(this, &MainWindow::fullscreen_mode_disabled, this, [=]
                {
                    customTitle->setEnabled(true);
                });

                windowMenu->addAction(customTitle);
            }

            {
                QAction *toggleDropIndicator = new QAction("Frame drop indicator", this);

                toggleDropIndicator->setCheckable(true);
                toggleDropIndicator->setChecked(IS_FRAME_DROP_INDICATOR_ENABLED);

                connect(toggleDropIndicator, &QAction::triggered, this, [=]
                {
                    IS_FRAME_DROP_INDICATOR_ENABLED = !IS_FRAME_DROP_INDICATOR_ENABLED;
                    this->update_window_title();
                });

                windowMenu->addAction(toggleDropIndicator);
            }

            {
                windowMenu->addSeparator();

                QMenu *rendererMenu = new QMenu("Renderer", this);
                QActionGroup *group = new QActionGroup(this);

                QAction *opengl = new QAction("OpenGL", this);
                opengl->setActionGroup(group);
                opengl->setCheckable(true);
                rendererMenu->addAction(opengl);

                QAction *software = new QAction("Software", this);
                software->setActionGroup(group);
                software->setCheckable(true);
                rendererMenu->addAction(software);

                connect(this, &MainWindow::fullscreen_mode_enabled, this, [=]
                {
                    rendererMenu->setEnabled(false);
                });

                connect(this, &MainWindow::fullscreen_mode_disabled, this, [=]
                {
                    rendererMenu->setEnabled(true);
                });

                connect(opengl, &QAction::triggered, this, [=]
                {
                    this->set_opengl_enabled(true);
                });

                connect(software, &QAction::triggered, this, [=]
                {
                    this->set_opengl_enabled(false);
                });

                if (kpers_value_of(INI_GROUP_OUTPUT, "renderer", "Software").toString() == "Software")
                {
                    software->setChecked(true);
                }
                else
                {
                    opengl->setChecked(true);
                }

                windowMenu->addMenu(rendererMenu);
            }

            {
                windowMenu->addSeparator();

                QMenu *const fontSizeMenu = new QMenu("Font size in dialogs", this);
                QActionGroup *const group = new QActionGroup(this);

                const auto add_size_action = [this, group, fontSizeMenu](const unsigned pxSize)
                {
                    QAction *const action  = new QAction(QString::number(pxSize), this);

                    action->setActionGroup(group);
                    action->setCheckable(true);
                    fontSizeMenu->addAction(action);

                    connect(action, &QAction::triggered, this, [this, pxSize]{this->set_global_font_size(pxSize);});

                    if (pxSize == this->appwideFontSize)
                    {
                        action->setChecked(true);
                    }
                };

                for (unsigned size = 15; size <= 19; size++)
                {
                    add_size_action(size);
                }

                windowMenu->addMenu(fontSizeMenu);
            }

            {
                windowMenu->addSeparator();

                QAction *showBorder = new QAction("Border", this);

                showBorder->setCheckable(true);
                showBorder->setChecked(this->window_has_border());
                showBorder->setShortcut(kd_get_key_sequence("output-window: toggle-window-border"));

                connect(this, &MainWindow::border_hidden, this, [=]
                {
                    showBorder->setChecked(false);
                });

                connect(this, &MainWindow::border_shown, this, [=]
                {
                    showBorder->setChecked(true);
                });

                connect(this, &MainWindow::fullscreen_mode_enabled, this, [=]
                {
                    showBorder->setEnabled(false);
                });

                connect(this, &MainWindow::fullscreen_mode_disabled, this, [=]
                {
                    showBorder->setEnabled(true);
                });

                connect(showBorder, &QAction::triggered, this, [this]
                {
                    this->toggle_window_border();
                });

                windowMenu->addAction(showBorder);
            }

            {
                QAction *fullscreen = new QAction("Fullscreen", this);

                fullscreen->setCheckable(true);
                fullscreen->setChecked(this->isFullScreen());
                fullscreen->setShortcut(kd_get_key_sequence("output-window: toggle-fullscreen-mode"));

                connect(this, &MainWindow::fullscreen_mode_enabled, this, [=]
                {
                    fullscreen->setChecked(true);
                });

                connect(this, &MainWindow::fullscreen_mode_disabled, this, [=]
                {
                    fullscreen->setChecked(false);
                });

                connect(fullscreen, &QAction::triggered, this, [this]
                {
                    if (this->isFullScreen())
                    {
                        this->showNormal();
                    }
                    else
                    {
                        this->showFullScreen();
                    }
                });

                windowMenu->addAction(fullscreen);
            }

            {
                QAction *center = new QAction("Center", this);

                QAction *topLeft = new QAction("Top left", this);
                topLeft->setShortcut(kd_get_key_sequence("output-window: snap-to-left"));

                connect(center, &QAction::triggered, this, [=]
                {
                    this->move(this->pos() + (QGuiApplication::primaryScreen()->geometry().center() - this->geometry().center()));
                });

                connect(topLeft, &QAction::triggered, this, [=]
                {
                    this->move(0, 0);
                });

                connect(this, &MainWindow::fullscreen_mode_enabled, this, [=]
                {
                    center->setEnabled(false);
                    topLeft->setEnabled(false);
                });

                connect(this, &MainWindow::fullscreen_mode_disabled, this, [=]
                {
                    center->setEnabled(true);
                    topLeft->setEnabled(true);
                });

                windowMenu->addSeparator();
                windowMenu->addAction(center);
                windowMenu->addAction(topLeft);
            }
        }

        QAction *ecoMode = new QAction("Eco mode", this);
        {
            ecoMode->setCheckable(true);
            ecoMode->setChecked(k_is_eco_mode_enabled());

            connect(ecoMode, &QAction::toggled, this, [](bool checked)
            {
                k_set_eco_mode_enabled(checked);
            });
        }

        connect(this->contextMenu->addAction("Save as image"), &QAction::triggered, this, [=]{this->save_screenshot();});
        this->contextMenu->addSeparator();
        this->contextMenu->addMenu(captureMenu);
        this->contextMenu->addMenu(outputMenu);
        this->contextMenu->addSeparator();
        this->contextMenu->addMenu(windowMenu);
        this->contextMenu->addSeparator();
        this->contextMenu->addAction(ecoMode);
        this->contextMenu->addSeparator();
        connect(this->contextMenu->addAction("About VCS..."), &QAction::triggered, this, [=]{this->aboutDlg->open();});

        // Ensure that action shortcuts can be used regardless of which dialog has focus.
        for (const auto *menu: {captureMenu, outputMenu, windowMenu})
        {
            this->addActions(menu->actions());

            for (auto dialog: this->dialogs)
            {
                dialog->addActions(menu->actions());
            }
        }
    }

    // We intend to repaint the entire window every time we update it, so ask for no automatic fill.
    this->setAttribute(Qt::WA_OpaquePaintEvent, true);

    this->update_window_size();
    this->update_window_title();

    // Set keyboard shortcuts.
    {
        auto makeAppwideShortcut = [this](const std::string label)->QShortcut*
        {
            QShortcut *shortcut = kd_make_key_shortcut(label, this);

            shortcut->setContext(Qt::ApplicationShortcut);
            shortcut->setAutoRepeat(false);

            return shortcut;
        };

        // Numkeys 1 through 9.
        k_assert(this->inputResolutionDlg != nullptr, "");
        for (uint i = 1; i <= 9; i++)
        {
            const std::string shortcutString = ("input-resolution-dialog: resolution-activator-" + std::to_string(i));

            connect(makeAppwideShortcut(shortcutString), &QShortcut::activated, [=]
            {
                this->inputResolutionDlg->activate_resolution_button(i);
            });
        }

        connect(makeAppwideShortcut("filter-graph-dialog: toggle-enabled"), &QShortcut::activated, [=]{this->filterGraphDlg->set_enabled(!this->filterGraphDlg->is_enabled());});
        connect(makeAppwideShortcut("overlay-dialog: toggle-enabled"), &QShortcut::activated, [=]{this->overlayDlg->set_enabled(!this->overlayDlg->is_enabled());});
        connect(makeAppwideShortcut("anti-tear-dialog: toggle-enabled"), &QShortcut::activated, [=]{this->antitearDlg->set_enabled(!this->antitearDlg->is_enabled());});
        connect(makeAppwideShortcut("record-dialog: toggle-enabled"), &QShortcut::activated, [=]{this->recordDlg->set_enabled(!this->recordDlg->is_enabled());});

        // F1 through F12.
        for (uint i = 1; i <= 12; i++)
        {
            const std::string shortcutString = ("video-preset-dialog: preset-activator-" + std::to_string(i));

            connect(makeAppwideShortcut(shortcutString), &QShortcut::activated, this, [=]
            {
                kvideopreset_activate_keyboard_shortcut(shortcutString);
            });
        }

        connect(makeAppwideShortcut("output-window: set-input-channel-1"), &QShortcut::activated, []{kc_set_capture_input_channel(0);});
        connect(makeAppwideShortcut("output-window: set-input-channel-2"), &QShortcut::activated, []{kc_set_capture_input_channel(1);});

        connect(kd_make_key_shortcut("output-window: exit-fullscreen-mode", this), &QShortcut::activated, this, [this]
        {
            if (this->isFullScreen())
            {
                this->showNormal();
            }
        });
    }

#ifdef _WIN32
    /// Temp hack. On my system, need to toggle this or there'll be a 2-pixel
    /// gap between the frame and the bottom corner of the window.
    toggle_window_border();
    toggle_window_border();
#endif

    this->activateWindow();
    this->raise();

    // Apply any custom app-wide styling styling.
    {
        qApp->setWindowIcon(QIcon(":/res/images/icons/appicon.ico"));
        this->apply_global_stylesheet(":/res/stylesheets/appstyle-newie.qss");
        this->set_global_font_size(this->appwideFontSize);
    }

    // Listen for app events.
    {
        kd_evDirty.listen([this]
        {
            this->redraw();
        });

        ks_evNewScaledImage.listen([this]
        {
            this->redraw();
        });

        ks_evFramesPerSecond.listen([this](const unsigned fps)
        {
            if (CURRENT_OUTPUT_FPS != fps)
            {
                CURRENT_OUTPUT_FPS = fps;
                this->update_window_title();
            }
        });

        k_evEcoModeEnabled.listen([this]
        {
            this->update_window_title();
        });

        k_evEcoModeDisabled.listen([this]
        {
            this->update_window_title();
        });

        ks_evNewOutputResolution.listen([this]
        {
            this->update_window_title();
            this->update_window_size();
        });

        kc_evSignalLost.listen([this]
        {
            this->update_window_title();
            this->update_window_size();
            this->redraw();
        });

        kc_evInvalidDevice.listen([this]
        {
            this->update_window_title();
            this->update_window_size();
            this->redraw();
        });

        kc_evInvalidSignal.listen([this]
        {
            this->update_window_title();
            this->update_window_size();
            this->redraw();
        });

        kc_evNewVideoMode.listen([this](video_mode_s)
        {
            this->update_window_title();
        });

        krecord_evRecordingStarted.listen([this]
        {
            if (!PROGRAM_EXIT_REQUESTED)
            {
                this->update_window_title();
            }
        });

        krecord_evRecordingEnded.listen([this]
        {
            if (!PROGRAM_EXIT_REQUESTED)
            {
                this->update_window_title();

                // The output resolution might have changed while we were recording, but
                // since we also prevent the size of the output window from changing while
                // recording, we should now - that recording has stopped - tell the window
                // to update its size to match the current proper output size.
                this->update_window_size();
            }
        });

        kc_evMissedFramesCount.listen([this](const unsigned numMissed)
        {
            const bool areDropped = (numMissed > 0);

            if (this->areFramesBeingDropped != areDropped)
            {
                this->areFramesBeingDropped = areDropped;
                this->update_window_title();
            }
        });
    }

    return;
}

bool MainWindow::apply_global_stylesheet(const QString &qssFilename)
{
    QFile styleFile(qssFilename);

    if (styleFile.open(QIODevice::ReadOnly))
    {
        const QString styleSheet = styleFile.readAll();
        this->appwideStyleSheet = styleSheet;
        qApp->setStyleSheet(styleSheet);
        return true;
    }

    return false;
}

bool MainWindow::set_global_font_size(const unsigned fontSize)
{
#if _WIN32
    QFile fontStyleFile(":/res/stylesheets/font-windows.qss");
#elif __linux__
    QFile fontStyleFile(":/res/stylesheets/font-linux.qss");
#else
    #error "Unknown platform."
#endif

    if (fontStyleFile.open(QIODevice::ReadOnly))
    {
        QString fontStyleSheet = fontStyleFile.readAll();
        fontStyleSheet.replace("%FONT_SIZE%", QString("%1px").arg(fontSize));

        qApp->setStyleSheet(this->appwideStyleSheet + "\n" + fontStyleSheet);
        this->appwideFontSize = fontSize;

        return true;
    }

    return false;
}

MainWindow::~MainWindow()
{
    // Save persistent settings.
    {
        kpers_set_value(INI_GROUP_OUTPUT, "frame_drop_indicator", IS_FRAME_DROP_INDICATOR_ENABLED);
        kpers_set_value(INI_GROUP_OUTPUT, "renderer", (OGL_SURFACE? "OpenGL" : "Software"));
        kpers_set_value(INI_GROUP_OUTPUT, "default_scaler", QString::fromStdString(ks_default_scaler()->name));
        kpers_set_value(INI_GROUP_APP, "font_size", QString::number(this->appwideFontSize));
        kpers_set_value(INI_GROUP_APP, "eco_mode", k_is_eco_mode_enabled());
    }

    delete ui;
    ui = nullptr;

    for (auto dialog: this->dialogs)
    {
        delete dialog;
        dialog = nullptr;
    }

    return;
}

bool MainWindow::load_font(const QString &filename)
{
    if (QFontDatabase::addApplicationFont(filename) == -1)
    {
        NBENE(("Failed to load the font '%s'.", filename.toStdString().c_str()));
        return false;
    }

    return true;
}

void MainWindow::redraw(void)
{
    if (OGL_SURFACE != nullptr)
    {
        OGL_SURFACE->repaint();
    }
    else this->repaint();

    return;
}

void MainWindow::set_opengl_enabled(const bool enabled)
{
    if (enabled)
    {
        INFO(("Renderer: OpenGL."));

        k_assert((OGL_SURFACE == nullptr), "Can't doubly enable OpenGL.");

        OGL_SURFACE = new OGLWidget(std::bind(&MainWindow::overlay_image, this), this);
        OGL_SURFACE->show();
        OGL_SURFACE->raise();

        QSurfaceFormat format;
        format.setDepthBufferSize(24);
        format.setStencilBufferSize(8);
        format.setVersion(1, 2);
        format.setSwapInterval(0); // Vsync.
        format.setSamples(0);
        format.setProfile(QSurfaceFormat::CompatibilityProfile);
        format.setRenderableType(QSurfaceFormat::OpenGL);
        QSurfaceFormat::setDefaultFormat(format);

        ui->centralwidget->layout()->addWidget(OGL_SURFACE);
    }
    else
    {
        INFO(("Renderer: Software."));

        ui->centralwidget->layout()->removeWidget(OGL_SURFACE);
        delete OGL_SURFACE;
        OGL_SURFACE = nullptr;
    }

    return;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // The main loop will close the window if it detects PROGRAM_EXIT_REQUESTED.
    event->ignore();

    // If there are unsaved changes, ask the user to confirm to exit.
    {
        QStringList dialogsWithUnsavedChanges;

        for (const VCSBaseDialog *dialog: this->dialogs)
        {
            if (dialog->has_unsaved_changes())
            {
                dialogsWithUnsavedChanges << QString("* %1").arg(dialog->name());
            }
        }

        if (dialogsWithUnsavedChanges.count() &&
            (QMessageBox::question(this, "Confirm exit",
                                   "The following dialogs have unsaved changes:"
                                   "<b><br><br>" + dialogsWithUnsavedChanges.join("<br>") +
                                   "</b><br><br>Exit and lose unsaved changes?",
                                   (QMessageBox::No | QMessageBox::Yes)) == QMessageBox::No))
        {
            return;
        }
    }

    PROGRAM_EXIT_REQUESTED = 1;

    return;
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        toggle_window_border();
    }

    return;
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange)
    {
        // Hide the window's cursor when in full-screen mode, and restore it when not.
        /// TODO: Ideally, this would only get set when we enter full-screen
        /// mode and back, not every time the window's state changes.
        if (this->windowState() & Qt::WindowFullScreen)
        {
            emit this->fullscreen_mode_enabled();
            this->setCursor(QCursor(Qt::BlankCursor));
        }
        else
        {
            emit this->fullscreen_mode_disabled();
            this->unsetCursor();
        }
    }

    QWidget::changeEvent(event);
}

/// Temp. Used to track mouse movement delta across frames.
static bool LEFT_MOUSE_BUTTON_DOWN = false;
static QPoint PREV_MOUSE_POS;

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    // If the cursor is over the capture window and the left mouse button is being
    // held down, drag the window. Note: We disable dragging in fullscreen mode,
    // since really you shouldn't be able to drag a fullscreen window.
    if (LEFT_MOUSE_BUTTON_DOWN &&
        !this->isFullScreen())
    {
        QPoint delta = (event->globalPos() - PREV_MOUSE_POS);
        this->move(pos() + delta);
    }

    PREV_MOUSE_POS = event->globalPos();

    return;
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    ui->centralwidget->setFocus();

    if (event->button() == Qt::LeftButton)
    {
        LEFT_MOUSE_BUTTON_DOWN = true;
        PREV_MOUSE_POS = event->globalPos();
    }

    return;
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        LEFT_MOUSE_BUTTON_DOWN = false;
    }

    return;
}


void MainWindow::wheelEvent(QWheelEvent *event)
{
    if ((this->outputResolutionDlg != nullptr) &&
        this->is_mouse_wheel_scaling_allowed())
    {
        // Adjust the size of the capture window with the mouse scroll wheel.
        const int dir = (event->angleDelta().y() < 0)? 1 : -1;
        this->outputResolutionDlg->adjust_output_scaling(dir);
    }

    return;
}

bool MainWindow::is_mouse_wheel_scaling_allowed(void)
{
    return (
        !kd_is_fullscreen() && // On my virtual machine, at least, wheel scaling while in full-screen messes up the full-screen mode.
        !krecord_is_recording() &&
        !ks_is_custom_scaler_active()
    );
}

QImage MainWindow::overlay_image(void)
{
    if (kc_is_receiving_signal() &&
        overlayDlg != nullptr &&
        overlayDlg->is_enabled())
    {
        return overlayDlg->rendered();
    }
    else return QImage();
}

void MainWindow::paintEvent(QPaintEvent *)
{
    // If OpenGL is enabled, its own paintGL() should be getting called instead of paintEvent().
    if (OGL_SURFACE != nullptr) return;

    // Convert the output buffer into a QImage frame.
    const QImage frameImage = ([]
    {
       const image_s image = ks_scaler_frame_buffer();
       k_assert((image.resolution.bpp == 32), "Expected output image data to be 32-bit.");

       if (!image.pixels)
       {
           DEBUG(("Requested the scaler output as a QImage while the scaler's output buffer was uninitialized."));
           return QImage();
       }
       else
       {
           return QImage(image.pixels, image.resolution.w, image.resolution.h, QImage::Format_RGB32);
       }
    })();

    QPainter painter(this);

    // Draw the frame.
    if (!frameImage.isNull())
    {
        painter.drawImage(0, 0, frameImage);
    }

    // Draw the overlay.
    const QImage overlayImg = overlay_image();
    if (!overlayImg.isNull())
    {
        painter.drawImage(0, 0, overlayImg);
    }

    // Show a magnifying glass effect which blows up part of the captured image.
    if (kc_is_receiving_signal() &&
        this->isActiveWindow() &&
        this->rect().contains(this->mapFromGlobal(QCursor::pos())) &&
        (QGuiApplication::mouseButtons() & Qt::MiddleButton))
    {
        this->magnifyingGlass->magnify(frameImage, this->mapFromGlobal(QCursor::pos()));
    }
    else
    {
        this->magnifyingGlass->hide();
    }

    return;
}

void MainWindow::update_window_title(void)
{
    QString programName = QString("%1%2").arg(PROGRAM_NAME).arg(k_is_eco_mode_enabled()? " Eco" : "");
    QString title = programName;

    if (PROGRAM_EXIT_REQUESTED)
    {
        title = QString("%1 - Closing...").arg(programName);
    }
    else if (!kc_has_valid_device())
    {
        title = QString("%1 - Invalid capture channel").arg(this->windowTitleOverride.isEmpty()? programName : this->windowTitleOverride);
    }
    else if (!kc_has_valid_signal())
    {
        title = QString("%1 - Signal out of range").arg(this->windowTitleOverride.isEmpty()? programName : this->windowTitleOverride);
    }
    else if (!kc_is_receiving_signal())
    {
        title = QString("%1 - No signal").arg(this->windowTitleOverride.isEmpty()? programName : this->windowTitleOverride);
    }
    else
    {
        // A symbol shown in the title if VCS is currently dropping frames.
        const QString missedFramesMarker = ((IS_FRAME_DROP_INDICATOR_ENABLED && this->areFramesBeingDropped)? "{!} " : "");

        const resolution_s inRes = kc_current_capture_state().input.resolution;
        const resolution_s outRes = ks_output_resolution();
        const refresh_rate_s refreshRate = kc_current_capture_state().input.refreshRate;

        QStringList programStatus;
        if (recordDlg->is_enabled())      programStatus << "R";
        if (filterGraphDlg->is_enabled()) programStatus << "F";
        if (overlayDlg->is_enabled())     programStatus << "O";
        if (antitearDlg->is_enabled())    programStatus << "A";

        if (!this->windowTitleOverride.isEmpty())
        {
            title = QString("%1%2").arg(missedFramesMarker).arg(this->windowTitleOverride);
        }
        else
        {
            title = QString("%1%2 - %3%4 \u00d7 %5 (%6 Hz) shown in %7 \u00d7 %8 (%9 FPS)")
                    .arg(missedFramesMarker)
                    .arg(programName)
                    .arg(programStatus.count()? QString("%1 - ").arg(programStatus.join("")) : "")
                    .arg(inRes.w)
                    .arg(inRes.h)
                    .arg(QString::number(refreshRate.value<double>(), 'f', 3))
                    .arg(outRes.w)
                    .arg(outRes.h)
                    .arg(CURRENT_OUTPUT_FPS);
        }
    }

    this->setWindowTitle(title);

    return;
}

void MainWindow::update_gui_state(void)
{
    // Manually spin the event loop.
    QCoreApplication::sendPostedEvents();
    QCoreApplication::processEvents();

    return;
}

void MainWindow::update_window_size(void)
{
    const resolution_s r = ks_output_resolution();

    this->setFixedSize(r.w, r.h);
    overlayDlg->set_overlay_max_width(r.w);

    update_window_title();

    return;
}

bool MainWindow::window_has_border(void)
{
    return !bool(windowFlags() & Qt::FramelessWindowHint);
}

void MainWindow::toggle_window_border(void)
{
    if (this->isFullScreen())
    {
        return;
    }

    const Qt::WindowFlags borderlessFlags = (Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

    if (!window_has_border())
    {
        this->setWindowFlags(this->defaultWindowFlags & ~borderlessFlags);
        emit this->border_shown();
    }
    else
    {
        this->setWindowFlags(Qt::FramelessWindowHint | borderlessFlags);
        emit this->border_hidden();
    }

    this->show();
    update_window_size();
}

void MainWindow::save_screenshot(void)
{
    const QString datestampedFilename = (
        QDir::currentPath() +
        "/" +
        ([]()->QString
        {
            QString filename = QString("vcs %1.png").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd 'at' hh.mm.ss"));

            if (QFile::exists(filename))
            {
                // Append the current time's milliseconds, for some protection against filename collisions.
                filename.replace(QRegularExpression(".png$"), QString(".%1.png").arg(QTime::currentTime().toString("z")));
            }

            return filename;
        })()
    );

    const QImage frameImage = ([]()->QImage
    {
        const image_s curOutputImage = ks_scaler_frame_buffer();

        if (!curOutputImage.pixels)
        {
            DEBUG(("Requested the scaler output as a QImage while the scaler's output buffer was uninitialized."));
            return QImage();
        }
        else
        {
            return QImage(curOutputImage.pixels, curOutputImage.resolution.w, curOutputImage.resolution.h, QImage::Format_RGB32);
        }
    })();

    if (frameImage.save(datestampedFilename))
    {
        INFO(("Output image saved to \"%s\".", QDir::toNativeSeparators(datestampedFilename).toStdString().c_str()));
    }
    else
    {
        NBENE(("Failed to save the output image to \"%s\".", QDir::toNativeSeparators(datestampedFilename).toStdString().c_str()));

        QMessageBox::critical(
            this,
            "Error saving image",
            QString(
                "The following location could not be written to:<br><br><b>%1</b>"
            ).arg(QDir::toNativeSeparators(QFileInfo(datestampedFilename).absolutePath()))
        );
    }

    return;
}

void MainWindow::contextMenuEvent(QContextMenuEvent *event)
{
    this->contextMenu->popup(event->globalPos());

    return;
}

void MainWindow::add_gui_log_entry(const log_entry_s e)
{
    /// GUI logging is currently not implemented.

    (void)e;

    return;
}
