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

#include "DebconfError.h"

#include <QIcon>

#include <KIconLoader>

using namespace DebconfKde;

DebconfError::DebconfError(const QString &name, QWidget *parent)
    : DebconfElement(name, parent)
{
    setupUi(this);
    iconL->setPixmap(QIcon::fromTheme(QLatin1String("dialog-error")).pixmap(KIconLoader::SizeHuge, KIconLoader::SizeHuge));
}

DebconfError::~DebconfError()
{
}

int DebconfError::nextId() const
{
    return 0;
}

void DebconfError::setError(const QString &description, const QString &title)
{
    descriptionL->setText(title);
    textTE->setText(description);
}

#include "moc_DebconfError.cpp"
