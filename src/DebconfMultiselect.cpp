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

#include "DebconfMultiselect.h"

#include <QStandardItem>
#include <QStandardItemModel>

using namespace DebconfKde;

DebconfMultiselect::DebconfMultiselect(const QString &name, QWidget *parent)
    : DebconfElement(name, parent)
{
    setupUi(this);

    m_model = new QStandardItemModel(this);
    multiselectLV->setModel(m_model);
}

DebconfMultiselect::~DebconfMultiselect()
{
}

QString DebconfMultiselect::value() const
{
    QStringList checked;
    for (int i = 0; i < m_model->rowCount(); i++) {
        int state;
        state = m_model->data(m_model->index(i, 0), Qt::CheckStateRole).toInt();
        if (state == Qt::Checked) {
            checked << m_model->data(m_model->index(i, 0), Qt::DisplayRole).toString();
        }
    }
    return checked.join(QLatin1String(", "));
}

void DebconfMultiselect::setMultiselect(const QString &extended_description,
                                        const QString &description,
                                        const QStringList &default_choices,
                                        const QStringList &choices)
{
    extendedDescriptionL->setText(extended_description);
    descriptionL->setText(description);

    m_model->clear();
    for (const QString &choice : choices) {
        auto item = new QStandardItem(choice);
        item->setSelectable(false);
        item->setCheckable(true);
        if (default_choices.contains(choice)) {
            item->setCheckState(Qt::Checked);
        } else {
            item->setCheckState(Qt::Unchecked);
        }
        m_model->appendRow(item);
    }
}

#include "moc_DebconfMultiselect.cpp"
