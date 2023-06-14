#ifndef VCS_DISPLAY_QT_DIALOGS_COMPONENTS_WINDOW_OPTIONS_DIALOG_WINDOW_RENDERER_H
#define VCS_DISPLAY_QT_DIALOGS_COMPONENTS_WINDOW_OPTIONS_DIALOG_WINDOW_RENDERER_H

#include "display/qt/subclasses/QDialog_vcs_base_dialog.h"

namespace Ui {
class WindowRenderer;
}

class OutputWindow;

class WindowRenderer : public VCSBaseDialog
{
    Q_OBJECT

public:
    explicit WindowRenderer(OutputWindow *parent = nullptr);
    ~WindowRenderer();

private:
    Ui::WindowRenderer *ui;
};

#endif
