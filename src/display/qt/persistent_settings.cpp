/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 * A basic convenience wrapper for QSettings.
 *
 */

#include <QSettings>
#include "display/qt/persistent_settings.h"

static QSettings SETTINGS_FILE("vcs.ini", QSettings::IniFormat);

void kpers_initialize(void)
{
    /// Nothing at the moment.

    return;
}

// Concatenates the given name and group into a key usable with QSettings's value()
// and setValue().
//
static QString as_key(const QString &group, const QString &name)
{
    return QString("%1/%2").arg(group).arg(name);
}

QVariant kpers_value_of(const QString &group, const QString &name, const QVariant &defaultValue)
{
    const QVariant v = SETTINGS_FILE.value(as_key(group, name), defaultValue);

    return v;
}

bool kpers_contains(const QString &group, const QString &name)
{
    return SETTINGS_FILE.contains(as_key(group, name));
}

void kpers_set_value(const QString &group, const QString &name, const QVariant &value)
{
    SETTINGS_FILE.setValue(as_key(group, name), value);

    return;
}
