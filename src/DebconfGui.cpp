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

#include <QtCore/QProcess>
#include <QtCore/QFile>
#include <QtGui/QLabel>

#include <KLocale>

#include <debconf.h>

#include <KDebug>

using namespace DebconfKde;

class DebconfKde::DebconfGuiPrivate : public Ui::DebconfGui
{
public:
    virtual ~DebconfGuiPrivate() { }
    DebconfFrontend *frontend;
    DebconfProgress *elementProgress;
    QWidget *parentWidget;
    QList<DebconfElement*> elements;

    DebconfElement* createElement(const QString &k);
    void cleanup();
};

DebconfGui::DebconfGui(const QString &socketName, QWidget *parent)
 : QWidget(parent),
   d_ptr(new DebconfGuiPrivate)
{
    Q_D(DebconfGui);
    d->frontend = new DebconfFrontend(socketName, this);
    d->elementProgress = 0;
    d->parentWidget = 0;
    d->setupUi(this);
    setMinimumSize(500, 400);
    d->nextPB->setIcon(KIcon("go-next"));
    d->backPB->setIcon(KIcon("go-previous"));
    d->cancelPB->setIcon(KIcon("dialog-cancel"));

    connect(d->frontend, SIGNAL(go(const QString &, const QStringList &)),
            this, SLOT(cmd_go(const QString &, const QStringList &)));
    connect(d->frontend, SIGNAL(finished()),
            this, SIGNAL(deactivated()));
    connect(d->frontend, SIGNAL(progress(const QString &)),
            this, SLOT(cmd_progress(const QString &)));
    connect(d->frontend, SIGNAL(backup(bool)),
            d->backPB, SLOT(setEnabled(bool)));

    // find out the distribution logo
    QString distro_logo("/usr/share/pixmaps/debian-logo.png");
    QProcess *myProcess = new QProcess(this);

    myProcess->start("hostname");
    myProcess->waitForFinished();
    QString hostname = myProcess->readAllStandardOutput();
    setWindowTitle(i18n("Debconf on %1", hostname.trimmed()));

    myProcess->start("lsb_release", QStringList() << "-is");
    if (myProcess->waitForFinished()) {
        if (myProcess->exitCode() == 0){
            QString data = myProcess->readAllStandardOutput();
            data = QString("/usr/share/pixmaps/%1-logo.png").arg(data.trimmed().toLower());
            if (QFile::exists(data)) {
                distro_logo = data;
            }
        }
    }

    QPixmap icon = KIconLoader::global()->loadIcon(distro_logo,
                                                   KIconLoader::NoGroup,
                                                   KIconLoader::SizeLarge,
                                                   KIconLoader::DefaultState);
    if (!icon.isNull()) {
        d->iconL->setPixmap(icon);
        setWindowIcon(icon);
    }

    d->scrollArea->viewport()->setAutoFillBackground(false);
}

DebconfElement* DebconfGuiPrivate::createElement(const QString &k)
{
    kDebug() << "creating widget for " << k;

    QString extendedDescription = frontend->property(k, DebconfFrontend::ExtendedDescription);
    extendedDescription.replace("\\n", "\n");

    switch (frontend->type(k)) {
    case DebconfFrontend::Boolean:
    {
        DebconfBoolean *element = new DebconfBoolean(k, parentWidget);
        element->setBoolean(extendedDescription,
                            frontend->property(k, DebconfFrontend::Description),
                            frontend->value(k) == "true");
        return element;
    }
    case DebconfFrontend::Error:
    {
        DebconfError *element = new DebconfError(k, parentWidget);
        element->setError(extendedDescription,
                          frontend->property(k, DebconfFrontend::Description));
        return element;
    }
    case DebconfFrontend::Multiselect:
    {
        DebconfMultiselect *element = new DebconfMultiselect(k, parentWidget);
        element->setMultiselect(extendedDescription,
                                frontend->property(k, DebconfFrontend::Description),
                                frontend->value(k).split(", "),
                                frontend->property(k, DebconfFrontend::Choices).split(", "));
        return element;
    }
    case DebconfFrontend::Note:
    {
        DebconfNote *element = new DebconfNote(k, parentWidget);
        element->setNote(extendedDescription,
                         frontend->property(k, DebconfFrontend::Description));
        return element;
    }
    case DebconfFrontend::Password:
    {
        DebconfPassword *element = new DebconfPassword(k, parentWidget);
        element->setPassword(extendedDescription,
                             frontend->property(k, DebconfFrontend::Description));
        return element;
    }
    case DebconfFrontend::Select:
    {
        DebconfSelect *element = new DebconfSelect(k, parentWidget);
        element->setSelect(extendedDescription,
                           frontend->property(k, DebconfFrontend::Description),
                           frontend->value(k),
                           frontend->property(k, DebconfFrontend::Choices).split(", "));
        return element;
    }
    case DebconfFrontend::String:
    {
        DebconfString *element = new DebconfString(k, parentWidget);
        element->setString(extendedDescription,
                           frontend->property(k, DebconfFrontend::Description),
                           frontend->value(k));
        return element;
    }
    case DebconfFrontend::Text:
    {
        DebconfText *element = new DebconfText(k, parentWidget);
        element->setText(frontend->property(k, DebconfFrontend::Description));
        return element;
    }
    default:
        kWarning() << "Default REACHED!!!";
        DebconfElement *element = new DebconfElement(k, parentWidget);
        QLabel *label = new QLabel(element);
        label->setText(i18n("<b>Not implemented</b>: The input widget for data"
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
    kDebug() << "# GO GUI";
    d->cleanup();
    QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(d->parentWidget->layout());
    // if we have just one element and we are showing
    // elements that can make use of extra space
    // we don't add stretches
    bool needStretch = true;
    if (input.size() == 1) {
        QString key = input.first();
        if (d->frontend->type(key) == DebconfFrontend::Text &&
            d->frontend->type(key) == DebconfFrontend::Note &&
            d->frontend->type(key) == DebconfFrontend::Error &&
            d->frontend->type(key) == DebconfFrontend::Multiselect) {
            needStretch = false;
        }
    }

    if (needStretch) {
        layout->addStretch();
    }

    foreach (const QString &elementName, input) {
        DebconfElement *element =  d->createElement(elementName);
        d->elements.append(element);
        layout->addWidget(element);
    }

    if (needStretch) {
        layout->addStretch();
    }

    d->parentWidget->setAutoFillBackground(false);

    d->titleL->setText(title);
    d->nextPB->setEnabled(true);
    emit activated();
}

void DebconfGui::cmd_progress(const QString &cmd)
{
    Q_D(DebconfGui);
    if (!d->elementProgress) {
        d->cleanup();
        d->elementProgress = new DebconfProgress(QString(), d->parentWidget);
        QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(d->parentWidget->layout());
        layout->addStretch();
        layout->addWidget(d->elementProgress);
        layout->addStretch();
        d->parentWidget->setAutoFillBackground(false);
        d->nextPB->setEnabled(false);
        d->backPB->setEnabled(false);
    }
    DebconfProgress *element = d->elementProgress;

    QStringList commands = cmd.split(' ');
    kDebug() << "KPROGRESS" << commands;
    if (commands.first() == "START") {
        d->titleL->setText(d->frontend->property(commands.at(3), DebconfFrontend::Description));
        int progress_min = commands.at(1).toInt();
        int progress_max = commands.at(2).toInt();
        element->startProgress(d->frontend->property(commands.at(3), DebconfFrontend::ExtendedDescription),
                               progress_min,
                               progress_max);
    } else if (commands.first() == "SET") {
        element->setProgress(commands.at(1).toInt());
    } else if (commands.first() == "STEP") {
        element->stepProgress(commands.at(1).toInt());
    } else if (commands.first() == "INFO") {
        element->setProgressInfo(d->frontend->property(commands.at(1), DebconfFrontend::Description));
    } else if (commands.first() == "STOP") {
        element->stopProgress();
    }
    emit activated();
    d->frontend->say("0 ok");
}

void DebconfGuiPrivate::cleanup()
{
    delete parentWidget;
    elementProgress = 0;
    elements.clear();

    parentWidget = new QWidget(scrollArea);
    scrollArea->setWidget(parentWidget);
    QVBoxLayout *layout = new QVBoxLayout(parentWidget);
    parentWidget->setLayout(layout);
}

void DebconfGui::on_nextPB_clicked()
{
    Q_D(DebconfGui);
    // extract all the elements
    foreach (const DebconfElement *element, d->elements) {
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

#include "DebconfGui.moc"
