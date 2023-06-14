/*
 * 2022 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 *
 * The GUI's keyboard shortcuts.
 *
 */

#include <QKeySequence>
#include <QShortcut>
#include <QWidget>
#include <unordered_map>
#include "display/qt/keyboard_shortcuts.h"
#include "common/assert.h"

static const std::unordered_map<std::string, QKeySequence> KEY_SEQUENCES = {
    {"output-window: exit-fullscreen-mode", QKeySequence("esc")},
    {"output-window: toggle-fullscreen-mode", QKeySequence("f11")},
    {"output-window: toggle-window-border", QKeySequence("f1")},
    {"output-window: set-input-channel-1", QKeySequence("shift+1")},
    {"output-window: set-input-channel-2", QKeySequence("shift+2")},
    {"output-window: open-control-panel-dialog", QKeySequence("ctrl+q")},

    {"video-preset-dialog: preset-activator-1", QKeySequence("ctrl+f1")},
    {"video-preset-dialog: preset-activator-2", QKeySequence("ctrl+f2")},
    {"video-preset-dialog: preset-activator-3", QKeySequence("ctrl+f3")},
    {"video-preset-dialog: preset-activator-4", QKeySequence("ctrl+f4")},
    {"video-preset-dialog: preset-activator-5", QKeySequence("ctrl+f5")},
    {"video-preset-dialog: preset-activator-6", QKeySequence("ctrl+f6")},
    {"video-preset-dialog: preset-activator-7", QKeySequence("ctrl+f7")},
    {"video-preset-dialog: preset-activator-8", QKeySequence("ctrl+f8")},
    {"video-preset-dialog: preset-activator-9", QKeySequence("ctrl+f9")},
    {"video-preset-dialog: preset-activator-10", QKeySequence("ctrl+f10")},
    {"video-preset-dialog: preset-activator-11", QKeySequence("ctrl+f11")},
    {"video-preset-dialog: preset-activator-12", QKeySequence("ctrl+f12")},

    {"input-resolution-dialog: resolution-activator-1", QKeySequence("ctrl+1")},
    {"input-resolution-dialog: resolution-activator-2", QKeySequence("ctrl+2")},
    {"input-resolution-dialog: resolution-activator-3", QKeySequence("ctrl+3")},
    {"input-resolution-dialog: resolution-activator-4", QKeySequence("ctrl+4")},
    {"input-resolution-dialog: resolution-activator-5", QKeySequence("ctrl+5")},
    {"input-resolution-dialog: resolution-activator-6", QKeySequence("ctrl+6")},
    {"input-resolution-dialog: resolution-activator-7", QKeySequence("ctrl+7")},
    {"input-resolution-dialog: resolution-activator-8", QKeySequence("ctrl+8")},
    {"input-resolution-dialog: resolution-activator-9", QKeySequence("ctrl+9")},

    {"filter-graph-dialog: delete-selected-nodes", QKeySequence("shift+del")},

    {"filter-graph-dialog: toggle-enabled", QKeySequence("ctrl+shift+f")},
    {"overlay-dialog: toggle-enabled", QKeySequence("ctrl+shift+o")},
};

#define ASSERT_LABEL_VALIDITY(label) \
    k_assert_optional(\
        (KEY_SEQUENCES.find(label) != KEY_SEQUENCES.end()),\
        std::string("Unrecognized GUI key sequence label: " + label).c_str()\
    );

const QKeySequence& kd_get_key_sequence(const std::string &label)
{
    ASSERT_LABEL_VALIDITY(label);
    return KEY_SEQUENCES.at(label);
}

QShortcut* kd_make_key_shortcut(const std::string &label, QWidget *const parent)
{
    ASSERT_LABEL_VALIDITY(label);
    return new QShortcut(KEY_SEQUENCES.at(label), parent);
}
