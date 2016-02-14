/*
 * ty, a collection of GUI and command-line tools to manage Teensy devices
 *
 * Distributed under the MIT license (see LICENSE.txt or http://opensource.org/licenses/MIT)
 * Copyright (c) 2015 Niels Martignène <niels.martignene@gmail.com>
 */

#include <QSettings>

#include "database.hh"

using namespace std;

void SettingsDatabase::put(const QString &key, const QVariant &value)
{
    settings_->setValue(key, value);
}

void SettingsDatabase::remove(const QString &key)
{
    settings_->remove(key);
}

QVariant SettingsDatabase::get(const QString &key, const QVariant &default_value) const
{
    return settings_->value(key, default_value);
}

void SettingsDatabase::clear()
{
    settings_->clear();
}
