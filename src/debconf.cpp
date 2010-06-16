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

#include "debconf.h"

#include <QtCore/QSocketNotifier>
#include <QtCore/QRegExp>
#include <QtCore/QFile>

#include <KDebug>

namespace DebconfKde {

const DebconfFrontend::Cmd DebconfFrontend::commands[] = {
    { "SET", &DebconfFrontend::cmd_set },
    { "GO", &DebconfFrontend::cmd_go },
    { "TITLE", &DebconfFrontend::cmd_title },
    { "SETTITLE", &DebconfFrontend::cmd_title },
    { "DATA", &DebconfFrontend::cmd_data },
    { "SUBST", &DebconfFrontend::cmd_subst },
    { "INPUT", &DebconfFrontend::cmd_input },
    { "GET", &DebconfFrontend::cmd_get },
    { "CAPB", &DebconfFrontend::cmd_capb },
    { "PROGRESS", &DebconfFrontend::cmd_progress },
    { 0, 0 } };

DebconfFrontend::DebconfFrontend(const QString &socketName, QObject *parent)
  : QObject(parent), m_socket(0)
{
    m_server = new QLocalServer(this);
    QFile::remove(socketName);
    m_server->listen(socketName);
    connect(m_server, SIGNAL(newConnection()), this, SLOT(newConnection()));
}

void DebconfFrontend::newConnection()
{
    kDebug();
    if (m_socket) {
        QLocalSocket *socket = m_server->nextPendingConnection();
        socket->disconnectFromServer();
        socket->deleteLater();
        return;
    }

    m_socket = m_server->nextPendingConnection();
    if (m_socket) {
        connect(m_socket, SIGNAL(readyRead()), this, SLOT(process()));
        connect(m_socket, SIGNAL(disconnected()), this, SLOT(disconnected()));
    }
}

DebconfFrontend::~DebconfFrontend()
{
}

void DebconfFrontend::disconnected()
{
    emit finished();
    reset();
}

QString DebconfFrontend::value(const QString &key) const
{
    return m_values[key];
}

void DebconfFrontend::setValue(const QString &key, const QString &value)
{
    m_values[key] = value;
}

template<class T> int DebconfFrontend::enumFromString(const QString &str, const char *enumName)
{
    QString realName(str);
    realName.replace(0, 1, str.at(0).toUpper());
    int pos;
    while ((pos = realName.indexOf('_')) != -1) {
        if (pos + 1 > realName.size()) {
            realName.chop(pos);
        } else{
            realName.replace(pos, 2, realName.at(pos + 1).toUpper());
        }
    }

    int id = T::staticMetaObject.indexOfEnumerator(enumName);
    QMetaEnum e = T::staticMetaObject.enumerator(id);
    int enumValue = e.keyToValue(realName.toAscii().data());

    if(enumValue == -1) {
        enumValue = e.keyToValue(QString("Unknown").append(enumName).toAscii().data());
        qDebug() << "enumFromString (" << enumName << ") : converted" << realName << "to" << QString("Unknown").append(enumName) << ", enum value" << enumValue;
    }
    return enumValue;
}

DebconfFrontend::PropertyKey DebconfFrontend::propertyKeyFromString(const QString &string)
{
    return static_cast<PropertyKey>(enumFromString<DebconfFrontend>(string, "PropertyKey"));
}

DebconfFrontend::TypeKey DebconfFrontend::type(const QString &string) const
{
    return static_cast<TypeKey>(enumFromString<DebconfFrontend>(property(string, Type), "TypeKey"));
}

void DebconfFrontend::reset()
{
    emit backup(false);
    m_socket->deleteLater();
    m_socket = 0;
    m_data.clear();
    m_subst.clear();
    m_values.clear();
}

void DebconfFrontend::say(const QString &string)
{
    kDebug() << "ADEPT ---> " << string;
    QTextStream out(m_socket);
    out << string << "\n";
}

QString DebconfFrontend::substitute(const QString &key, const QString &rest) const
{
    Substitutions sub = m_subst[key];
    QString result, var, escape;
    QRegExp rx("^(.*?)(\\\\)?\\$\\{([^{}]+)\\}(.*)$");
    QString last(rest);
    while (rx.indexIn(rest) != -1) {
        result += rx.cap(1);
        escape = rx.cap(2);
        var = rx.cap(3);
        last = rx.cap(4);
        if (!escape.isEmpty()) {
            result += "${" + var + "}";
        } else {
            result += sub[var];
        }
    }
    return result + last;
}

QString DebconfFrontend::property(const QString &key, PropertyKey p) const
{
    QString r = m_data[key][p];
    if (p == Description || p == Choices) {
        return substitute(key, r);
    }
    return r;
}

void DebconfFrontend::cmd_capb(const QString &caps)
{
    emit backup(caps.split(", ").contains("backup"));
    say("0 backup");
}

void DebconfFrontend::cmd_set(const QString &param)
{
    QString item = param.section(' ', 0, 0);
    QString value = param.section(' ', 1);
    m_values[item] = value;
    kDebug() << "# SET: [" << item << "] " << value;
    say("0 ok");
}

void DebconfFrontend::cmd_get(const QString &param)
{
    say("0 " + m_values[param]);
}

void DebconfFrontend::cmd_input(const QString &param)
{
    m_input.append(param.section(' ', 1));
    say("0 will ask");
}

void DebconfFrontend::cmd_go(const QString &)
{
    kDebug() << "# GO";
    m_input.removeDuplicates();
    emit go(m_title, m_input);
    m_input.clear();
}

void DebconfFrontend::cmd_progress(const QString &param)
{
    kDebug() << "ADEPT: PROGRESS " << param;
    emit progress(param);
}

void DebconfFrontend::next()
{
    m_input.clear();
    say("0 ok, got the answers");
}

void DebconfFrontend::back()
{
    m_input.clear();
    say("30 go back");
}

void DebconfFrontend::cancel()
{
    m_socket->disconnectFromServer();
    reset();
}

void DebconfFrontend::cmd_title(const QString &param)
{
    if (!property(param, Description).isEmpty()) {
        m_title = property(param, Description);
    } else {
        m_title = param;
    }
    kDebug() << "ADEPT: TITLE " << m_title;
    say("0 ok");
}

void DebconfFrontend::cmd_data(const QString &param)
{
    // We get scrings like
    // aiccu/brokername description Tunnel broker:
    // item = "aiccu/brokername"
    // type = "description"
    // rest = "Tunnel broker:"
    QString item = param.section(' ', 0, 0);
    QString type = param.section(' ', 1, 1);
    QString value = param.section(' ', 2);

    m_data[item][propertyKeyFromString(type)] = value;
    kDebug() << "# NOTED: [" << item << "] [" << type << "] " << value;
    say("0 ok");
}

void DebconfFrontend::cmd_subst(const QString &param)
{
    // We get scrings like
    // aiccu/brokername brokers AARNet, Hexago / Freenet6, SixXS, Wanadoo France
    // item = "aiccu/brokername"
    // type = "brokers"
    // value = "AARNet, Hexago / Freenet6, SixXS, Wanadoo France"
    QString item = param.section(' ', 0, 0);
    QString type = param.section(' ', 1, 1);
    QString value = param.section(' ', 2);

    m_subst[item][type] = value;
    kDebug() << "# SUBST: [" << item << "] [" << type << "] " << value;
    say("0 ok");
}

bool DebconfFrontend::process()
{
    QTextStream in(m_socket);
    QString line = in.readLine();

    if (line.isEmpty()) {
        return false;
    }

    QString command = line.section(' ', 0, 0);
    QString value = line.section(' ', 1);

    kDebug() << "ADEPT <--- [" << command << "] " << value;
    const Cmd *c = commands;
    while (c->cmd != 0) {
        if (command == c->cmd) {
            (this->*(c->run))(value);
            return true;
        }
        ++ c;
    }
    return false;
}

}
#include "debconf.moc"
