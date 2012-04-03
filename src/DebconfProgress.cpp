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

#include "DebconfProgress.h"

using namespace DebconfKde;

DebconfProgress::DebconfProgress(const QString &name, QWidget *parent)
 : DebconfElement(name, parent)
{
    setupUi(this);
}

DebconfProgress::~DebconfProgress()
{
}

void DebconfProgress::startProgress(const QString &extended_description,
                                    uint progress_min,
                                    uint progress_max)
{
    label->setText(extended_description);
    progressBar->setMaximum(progress_max);
    progressBar->setMinimum(progress_min);
    progressBar->setValue(progress_min);
}

void DebconfProgress::stopProgress()
{
    progressBar->setValue(progressBar->maximum());
}

void DebconfProgress::setProgress(uint progress_cur)
{
    progressBar->setValue(progress_cur);
}

void DebconfProgress::stepProgress(uint progress_step)
{
    progressBar->setValue(progressBar->value() + progress_step);
}

void DebconfProgress::setProgressInfo(const QString &description)
{
    label->setText(description);
}

#include "DebconfProgress.moc"
