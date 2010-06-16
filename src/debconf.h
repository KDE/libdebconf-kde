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

#ifndef DEBCONF_H
#define DEBCONF_H

#include <QtCore/QHash>
#include <QtCore/QStringList>
#include <QtCore/QMetaEnum>
#include <QtCore/QMetaObject>
#include <QtNetwork/QLocalSocket>
#include <QtNetwork/QLocalServer>

namespace DebconfKde {

class DebconfFrontend : public QObject {
    Q_OBJECT
    Q_ENUMS(PropertyKey)
    Q_ENUMS(TypeKey)
public:
    typedef enum {
        Choices,
        Description,
        ExtendedDescription,
        Type,
        UnknownPropertyKey = -1
    } PropertyKey;

    typedef enum {
        String,
        Password,
        Progress,
        Boolean,
        Select,
        Multiselect,
        Note,
        Error,
        Title,
        Text,
        UnknownTypeKey = -1
    } TypeKey;

    DebconfFrontend(const QString &socketName, QObject *parent = 0);
    ~DebconfFrontend();

    QString value(const QString &key) const;
    void setValue(const QString &key, const QString &value);

    QString property(const QString &key, PropertyKey p) const;

    TypeKey type(const QString &string) const;

    void say(const QString &string);

    /**
     * Goes to the next question
     */
    void next();
    /**
     * Goes back one question (if backup(true) was emitted)
     */
    void back();
    /**
     * Closes the current connection, canceling all questions
     */
    void cancel();

signals:
    void go(const QString &title, const QStringList &input);
    void progress(const QString &param);
    void finished();
    void backup(bool capable);

private slots:
    /**
     * This slot is called when the socket has new data
     */
    bool process();
    /**
     * Called when a new connection is received on the socket
     * If we are already handlyng a connection we refuse the others
     */
    void newConnection();
    /**
     * When the client disconnects we need to hide the GUI and clean
     * our internal data
     */
    void disconnected();

private:
    void cmd_capb(const QString &caps);
    void cmd_set(const QString &param);
    void cmd_get(const QString &param);
    void cmd_input(const QString &param);
    void cmd_go(const QString &);
    void cmd_title(const QString &param);
    void cmd_data(const QString &param);
    void cmd_subst(const QString &param);
    void cmd_progress(const QString &param);
    struct Cmd {
        const char *cmd;
        void (DebconfFrontend::*run)(const QString &);
    };
    static const Cmd commands[];

    void reset();

    // TODO this is apparently very much untested
    QString substitute(const QString &key, const QString &rest) const;
    /**
     * Transforms the string camel cased and return it's enum given the enum name
     */
    template<class T> static int enumFromString(const QString &str, const char *enumName);
    PropertyKey propertyKeyFromString(const QString &string);

    typedef QHash<PropertyKey, QString> Properties;
    typedef QHash<QString, QString> Substitutions;

    QLocalSocket *m_socket;
    QLocalServer *m_server;
    QHash<QString, Properties>    m_data;
    QHash<QString, Substitutions> m_subst;
    QHash<QString, QString>       m_values;
    QString m_title;
    QStringList m_input;
};

}

#endif
