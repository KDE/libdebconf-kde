/*
 * Copyright (C) 2010 Daniel Nicoletti <dantti85-pk@yahoo.com.br>
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

#include "DebconfSelect.h"

using namespace DebconfKde;

DebconfSelect::DebconfSelect(const QString &name, QWidget *parent)
 : DebconfElement(name, parent)
{
    setupUi(this);
}

DebconfSelect::~DebconfSelect()
{
}

QString DebconfSelect::value() const
{
    return selectCB->currentText();
}

void DebconfSelect::setSelect(const QString &extended_description,
                              const QString &description,
                              const QString &default_choice,
                              const QStringList &choices)
{
    extendedDescriptionL->setText(extended_description);
    descriptionL->setText(description);

    selectCB->clear();
    selectCB->addItems(choices);
    int index = selectCB->findText(default_choice);
    selectCB->setCurrentIndex(index != -1 ? index : 0);
}

#include "DebconfSelect.moc"
