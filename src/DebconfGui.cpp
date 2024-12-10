// This license reflects the original Adept code:
// -*- C++ -*- (c) 2008 Petr Rockai <me@mornfall.net>
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//
//     * Neither the name of [original copyright holder] nor the names of
//       its contributors may be used to endorse or promote products
//       derived from this software without specific prior written
//       permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
/*
 * All the modifications below are licensed under this license
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

#include "DebconfGui.h"

#include "ui_DebconfGui.h"

#include "DebconfBoolean.h"
#include "DebconfError.h"
#include "DebconfMultiselect.h"
#include "DebconfNote.h"
#include "DebconfPassword.h"
#include "DebconfProgress.h"
#include "DebconfSelect.h"
#include "DebconfString.h"
#include "DebconfText.h"
#include "Debug_p.h"

#include <QFrame>
#include <QHostInfo>
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QProcess>
#include <QtWidgets/QLabel>

#include <KGuiItem>
#include <KIconLoader>
#include <KLocalizedString>
#include <KOSRelease>
#include <KStandardGuiItem>

#include <debconf.h>

using namespace DebconfKde;

class DebconfKde::DebconfGuiPrivate : public Ui::DebconfGui
{
public:
    virtual ~DebconfGuiPrivate()
    {
    }
    DebconfFrontend *frontend;
    DebconfProgress *elementProgress = nullptr;
    QWidget *parentWidget = nullptr;
    QVector<DebconfElement *> elements;

    DebconfElement *createElement(const QString &k);
    void cleanup();
};

DebconfGui::DebconfGui(const QString &socketName, QWidget *parent)
    : QWidget(parent)
    , d_ptr(new DebconfGuiPrivate)
{
    Q_D(DebconfGui);
    d->frontend = new DebconfFrontendSocket(socketName, this);
    init();
}

DebconfGui::DebconfGui(int readfd, int writefd, QWidget *parent)
    : QWidget(parent)
    , d_ptr(new DebconfGuiPrivate)
{
    Q_D(DebconfGui);
    d->frontend = new DebconfFrontendFifo(readfd, writefd, this);
    init();
}

DebconfGui::~DebconfGui()
{
    delete d_ptr;
}

void DebconfGui::init()
{
    Q_D(DebconfGui);
    d->setupUi(this);

    // Setup buttons. They are marked non-translatable in the UI file.
    KGuiItem::assign(d->cancelPB, KStandardGuiItem::cancel());
    KGuiItem::assign(d->backPB, KStandardGuiItem::back());
    KGuiItem::assign(d->nextPB, KStandardGuiItem::cont());

    setMinimumSize(500, 400);
    d->cancelPB->setVisible(false);

    connect(d->frontend, &DebconfFrontend::go, this, &DebconfGui::cmd_go);
    connect(d->frontend, &DebconfFrontend::finished, this, &DebconfGui::deactivated);
    connect(d->frontend, &DebconfFrontend::progress, this, &DebconfGui::cmd_progress);
    connect(d->frontend, &DebconfFrontend::backup, d->backPB, &QPushButton::setEnabled);

    setWindowTitle(i18n("Debconf on %1", QHostInfo::localHostName()));

    // find out the distribution logo
    QString distroLogo(QLatin1String("/usr/share/pixmaps/debian-logo.png"));
    KOSRelease osInfo;
    if (!osInfo.logo().isEmpty())
        distroLogo = osInfo.logo();

    const QPixmap icon = KIconLoader::global()->loadIcon(distroLogo, KIconLoader::NoGroup, KIconLoader::SizeLarge, KIconLoader::DefaultState);
    if (!icon.isNull()) {
        d->iconL->setPixmap(icon);
        setWindowIcon(icon);
    }

    d->scrollArea->viewport()->setAutoFillBackground(false);
}

DebconfElement *DebconfGuiPrivate::createElement(const QString &k)
{
    qCDebug(DEBCONF) << "creating widget for " << k;

    QString extendedDescription = frontend->property(k, DebconfFrontend::ExtendedDescription);
    extendedDescription.replace(QLatin1String("\\n"), QLatin1String("\n"));

    switch (frontend->type(k)) {
    case DebconfFrontend::Boolean: {
        auto element = new DebconfBoolean(k, parentWidget);
        element->setBoolean(extendedDescription, frontend->property(k, DebconfFrontend::Description), frontend->value(k) == QLatin1String("true"));
        return element;
    }
    case DebconfFrontend::Error: {
        auto element = new DebconfError(k, parentWidget);
        element->setError(extendedDescription, frontend->property(k, DebconfFrontend::Description));
        return element;
    }
    case DebconfFrontend::Multiselect: {
        auto element = new DebconfMultiselect(k, parentWidget);
        element->setMultiselect(extendedDescription,
                                frontend->property(k, DebconfFrontend::Description),
                                frontend->value(k).split(QLatin1String(", ")),
                                frontend->property(k, DebconfFrontend::Choices).split(QLatin1String(", ")));
        return element;
    }
    case DebconfFrontend::Note: {
        auto element = new DebconfNote(k, parentWidget);
        element->setNote(extendedDescription, frontend->property(k, DebconfFrontend::Description));
        return element;
    }
    case DebconfFrontend::Password: {
        auto element = new DebconfPassword(k, parentWidget);
        element->setPassword(extendedDescription, frontend->property(k, DebconfFrontend::Description));
        return element;
    }
    case DebconfFrontend::Select: {
        auto element = new DebconfSelect(k, parentWidget);
        element->setSelect(extendedDescription,
                           frontend->property(k, DebconfFrontend::Description),
                           frontend->value(k),
                           frontend->property(k, DebconfFrontend::Choices).split(QLatin1String(", ")));
        return element;
    }
    case DebconfFrontend::String: {
        auto element = new DebconfString(k, parentWidget);
        element->setString(extendedDescription, frontend->property(k, DebconfFrontend::Description), frontend->value(k));
        return element;
    }
    case DebconfFrontend::Text: {
        auto element = new DebconfText(k, parentWidget);
        element->setText(frontend->property(k, DebconfFrontend::Description));
        return element;
    }
    default:
        qWarning() << "Default REACHED!!!";
        auto element = new DebconfElement(k, parentWidget);
        auto label = new QLabel(element);
        label->setText(
            i18n("<b>Not implemented</b>: The input widget for data"
                 " type '%1' is not implemented. Will use default of '%2'.",
                 frontend->property(k, DebconfFrontend::Type),
                 frontend->value(k)));
        label->setWordWrap(true);
        return element;
    }
}

void DebconfGui::cmd_go(const QString &title, const QStringList &input)
{
    Q_D(DebconfGui);
    qCDebug(DEBCONF) << "# GO GUI";
    d->cleanup();
    auto layout = qobject_cast<QVBoxLayout *>(d->parentWidget->layout());
    // if we have just one element and we are showing
    // elements that can make use of extra space
    // we don't add stretches
    bool needStretch = true;
    if (input.size() == 1) {
        const QString key = input.first();
        switch (d->frontend->type(key)) {
        case DebconfFrontend::Text:
        case DebconfFrontend::Note:
        case DebconfFrontend::Error:
        case DebconfFrontend::Multiselect:
            needStretch = false;
            break;
        default:
            break;
        }
    }

    if (needStretch) {
        layout->addStretch();
    }

    const QString side_info = d->frontend->getSideInfo();
    if (!side_info.isEmpty()) {
        QLabel *label = new QLabel(side_info);
        layout->addWidget(label);
    }

    for (const QString &elementName : input) {
        if (elementName == QLatin1String("div")) {
            QFrame *divider = new QFrame(d->parentWidget);
            divider->setFrameShape(QFrame::HLine);
            divider->setFrameShadow(QFrame::Sunken);
            layout->addWidget(divider);
            continue;
        }
        DebconfElement *element = d->createElement(elementName);
        d->elements.append(element);
        layout->addWidget(element);
    }

    if (needStretch) {
        layout->addStretch();
    }

    d->parentWidget->setAutoFillBackground(false);

    d->titleL->setText(title);
    d->nextPB->setEnabled(true);
    Q_EMIT activated();
}

void DebconfGui::cmd_progress(const QString &cmd)
{
    Q_D(DebconfGui);
    if (!d->elementProgress) {
        d->cleanup();
        d->elementProgress = new DebconfProgress(QString(), d->parentWidget);
        auto layout = qobject_cast<QVBoxLayout *>(d->parentWidget->layout());
        layout->addStretch();
        layout->addWidget(d->elementProgress);
        layout->addStretch();
        d->parentWidget->setAutoFillBackground(false);
        d->nextPB->setEnabled(false);
        d->backPB->setEnabled(false);
    }
    DebconfProgress *element = d->elementProgress;

    QStringList commands = cmd.split(QLatin1Char(' '));
    qCDebug(DEBCONF) << "KPROGRESS" << commands;
    if (commands.first() == QLatin1String("START")) {
        d->titleL->setText(d->frontend->property(commands.at(3), DebconfFrontend::Description));
        int progress_min = commands.at(1).toInt();
        int progress_max = commands.at(2).toInt();
        element->startProgress(d->frontend->property(commands.at(3), DebconfFrontend::ExtendedDescription), progress_min, progress_max);
    } else if (commands.first() == QLatin1String("SET")) {
        element->setProgress(commands.at(1).toInt());
    } else if (commands.first() == QLatin1String("STEP")) {
        element->stepProgress(commands.at(1).toInt());
    } else if (commands.first() == QLatin1String("INFO")) {
        element->setProgressInfo(d->frontend->property(commands.at(1), DebconfFrontend::Description));
    } else if (commands.first() == QLatin1String("STOP")) {
        element->stopProgress();
    }
    Q_EMIT activated();
    d->frontend->say(QLatin1String("0 ok"));
}

void DebconfGuiPrivate::cleanup()
{
    delete parentWidget;
    elementProgress = nullptr;
    elements.clear();

    parentWidget = new QWidget(scrollArea);
    scrollArea->setWidget(parentWidget);
    auto layout = new QVBoxLayout(parentWidget);
    parentWidget->setLayout(layout);
}

void DebconfGui::on_nextPB_clicked()
{
    Q_D(DebconfGui);
    // extract all the elements
    const auto elements = d->elements;
    for (const DebconfElement *element : elements) {
        d->frontend->setValue(element->name(), element->value());
    }

    d->frontend->next();
}

void DebconfGui::on_backPB_clicked()
{
    Q_D(DebconfGui);
    d->frontend->back();
}

void DebconfGui::on_cancelPB_clicked()
{
    Q_D(DebconfGui);
    d->frontend->cancel();
}

void DebconfGui::closeEvent(QCloseEvent *event)
{
    Q_D(DebconfGui);
    // It would be better to hid the close button on
    // on the window decoration:
    d->frontend->cancel();
    QWidget::closeEvent(event);
}

#include "moc_DebconfGui.cpp"
