/*
 * Copyright (C) 2010 Daniel Nicoletti <dantti12@gmail.com>
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

    const KGuiItem yes = KStandardGuiItem::yes();
    radioButton->setText(yes.text());
    radioButton->setToolTip(yes.toolTip());
    radioButton->setWhatsThis(yes.whatsThis());

    const KGuiItem no = KStandardGuiItem::no();
    radioButton_2->setText(no.text());
    radioButton_2->setToolTip(no.toolTip());
    radioButton_2->setWhatsThis(no.whatsThis());
}

DebconfBoolean::~DebconfBoolean()
{
}

QString DebconfBoolean::value() const
{
    return radioButton->isChecked() ? QStringLiteral( "true" ) : QStringLiteral( "false" );
}

void DebconfBoolean::setBoolean(const QString &extended_description,
                                const QString &description,
                                bool default_boolean)
{
    extendedDescriptionL->setText(extended_description);
    descriptionL->setText(description);
    radioButton->setChecked(default_boolean);
}
