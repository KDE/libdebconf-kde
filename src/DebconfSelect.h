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

#ifndef DEBCONF_SELECT_H
#define DEBCONF_SELECT_H

#include "ui_DebconfSelect.h"

#include "DebconfElement.h"

namespace DebconfKde {

class DebconfSelect : public DebconfElement, Ui::DebconfSelect
{
    Q_OBJECT
public:
    explicit DebconfSelect(const QString &name, QWidget *parent = 0);
    ~DebconfSelect();

    void setSelect(const QString &extended_description,
                   const QString &description,
                   const QString &default_choice,
                   const QStringList &choices);
    QString value() const;

private slots:
    void on_helpPB_clicked();

private:
    QString m_extended_description;
};


}

#endif
