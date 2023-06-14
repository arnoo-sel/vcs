/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef VCS_DISPLAY_QT_PERSISTENT_SETTINGS_H
#define VCS_DISPLAY_QT_PERSISTENT_SETTINGS_H

#include <QVariant>

// Group names for the settings INI file.
#define INI_GROUP_APP "App"
#define INI_GROUP_INPUT "Input"
#define INI_GROUP_CAPTURE "Capture"
#define INI_GROUP_FILTER_GRAPH "FilterGraph"
#define INI_GROUP_OVERLAY "Overlay"
#define INI_GROUP_VIDEO_PRESETS "VideoPresets"
#define INI_GROUP_OUTPUT_WINDOW "OutputWindow"

QVariant kpers_value_of(const QString &group, const QString &name, const QVariant &defaultValue = QVariant());

void kpers_set_value(const QString &group, const QString &name, const QVariant &value);

bool kpers_contains(const QString &group, const QString &name);

#endif
