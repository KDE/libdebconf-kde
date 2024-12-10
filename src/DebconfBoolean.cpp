/*
 * Copyright (C) 2010-2018 Daniel Nicoletti <dantti12@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB. If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "DebconfBoolean.h"

#include <KGuiItem>
#include <KStandardGuiItem>

using namespace DebconfKde;

DebconfBoolean::DebconfBoolean(const QString &name, QWidget *parent)
    : DebconfElement(name, parent)
{
    setupUi(this);

    const KGuiItem yes = KStandardGuiItem::ok();
    radioButtonYes->setText(yes.text());
    radioButtonYes->setToolTip(yes.toolTip());
    radioButtonYes->setWhatsThis(yes.whatsThis());

    const KGuiItem no = KStandardGuiItem::cancel();
    radioButtonNo->setText(no.text());
    radioButtonNo->setToolTip(no.toolTip());
    radioButtonNo->setWhatsThis(no.whatsThis());
}

DebconfBoolean::~DebconfBoolean()
{
}

QString DebconfBoolean::value() const
{
    return radioButtonYes->isChecked() ? QLatin1String("true") : QLatin1String("false");
}

void DebconfBoolean::setBoolean(const QString &extended_description, const QString &description, bool default_boolean)
{
    extendedDescriptionL->setText(extended_description);
    descriptionL->setText(description);
    if (default_boolean)
        radioButtonYes->setChecked(true);
    else
        radioButtonNo->setChecked(true);
}

#include "moc_DebconfBoolean.cpp"
