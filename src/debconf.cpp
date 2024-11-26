// This license reflects the original Adept code:
// -*- C++ -*- (c) 2008 Petr Rockai <me@mornfall.net>
//             (c) 2011 Modestas Vainius <modax@debian.org>
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

#include "debconf.h"

#include <QtCore/QSocketNotifier>
#include <QtCore/QRegularExpression>
#include <QtCore/QFile>
#include <cstdio>
#include <unistd.h>

#include "Debug_p.h"

namespace DebconfKde {

const DebconfFrontend::Cmd DebconfFrontend::commands[] = {
    { "SET", &DebconfFrontend::cmd_set, 2 },
    { "GO", &DebconfFrontend::cmd_go, 0 },
    { "TITLE", &DebconfFrontend::cmd_title, 1 },
    { "SETTITLE", &DebconfFrontend::cmd_title, 1 },
    { "DATA", &DebconfFrontend::cmd_data, 3 },
    { "SUBST", &DebconfFrontend::cmd_subst, 3 },
    { "INPUT", &DebconfFrontend::cmd_input, 1 },
    { "GET", &DebconfFrontend::cmd_get, 1 },
    { "CAPB", &DebconfFrontend::cmd_capb, 1 },
    { "PROGRESS", &DebconfFrontend::cmd_progress, 1 },
    { "X_PING", &DebconfFrontend::cmd_x_ping, 0 },
    { "VERSION", &DebconfFrontend::cmd_version, 0 },
    { "X_LOADTEMPLATEFILE", &DebconfFrontend::cmd_x_loadtemplatefile, 1 },
    { "INFO", &DebconfFrontend::cmd_info, 0 },
    { "FGET", &DebconfFrontend::cmd_fget, 2 },
    { "FSET", &DebconfFrontend::cmd_fset, 3 },
    { "BEGINBLOCK", &DebconfFrontend::cmd_beginblock, 0 },
    { "ENDBLOCK", &DebconfFrontend::cmd_endblock, 0 },
    { "STOP", &DebconfFrontend::cmd_stop, 0 },
    { "REGISTER", &DebconfFrontend::cmd_register, 2 },
    { "UNREGISTER", &DebconfFrontend::cmd_unregister, 1 },
    { "EXIST", &DebconfFrontend::cmd_exist, 1 },
    { nullptr, nullptr, 0 } };

DebconfFrontend::DebconfFrontend(QObject *parent)
  : QObject(parent)
{
}

DebconfFrontend::~DebconfFrontend()
{
}

void DebconfFrontend::disconnected()
{
    reset();
    Q_EMIT finished();
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
    while ((pos = realName.indexOf(QLatin1Char( '_' ))) != -1) {
        if (pos + 1 >= realName.size()) { // pos is from 0, size from 1, mustn't go off-by-one
            realName.chop(1);
        } else{
            realName.replace(pos, 2, realName.at(pos + 1).toUpper());
        }
    }

    int id = T::staticMetaObject.indexOfEnumerator(enumName);
    QMetaEnum e = T::staticMetaObject.enumerator(id);
    int enumValue = e.keyToValue(realName.toLatin1().data());

    if(enumValue == -1) {
        enumValue = e.keyToValue(QString(QLatin1String("Unknown") + QLatin1String(enumName)).toLatin1().data());
        qCDebug(DEBCONF) << "enumFromString (" <<QLatin1String(enumName) << ") : converted" << realName << "to" << QString(QLatin1String("Unknown") + QLatin1String(enumName)) << ", enum value" << enumValue;
    }
    return enumValue;
}

DebconfFrontend::PropertyKey DebconfFrontend::propertyKeyFromString(const QString &string)
{
    return static_cast<PropertyKey>(enumFromString<DebconfFrontend>(string, "PropertyKey" ));
}

DebconfFrontend::TypeKey DebconfFrontend::type(const QString &string) const
{
    return static_cast<TypeKey>(enumFromString<DebconfFrontend>(property(string, Type), "TypeKey" ));
}

void DebconfFrontend::reset()
{
    Q_EMIT backup(false);
    m_data.clear();
    m_subst.clear();
    m_values.clear();
    m_flags.clear();
}

void DebconfFrontend::say(const QString &string)
{
    qCDebug(DEBCONF) << "DEBCONF ---> " << string;
    QTextStream out(getWriteDevice());
    out << string << "\n";
    out.flush();
}

QString DebconfFrontend::substitute(const QString &key, const QString &rest) const
{
    Substitutions sub = m_subst[key];
    QString result, var, escape;
    QRegularExpression rx(QLatin1String("^(.*)(\\\\)?\\$\\{([^\\{\\}]+)\\}(.*)$"), QRegularExpression::DotMatchesEverythingOption);
    QString last(rest);
    QRegularExpressionMatch rxMatch = rx.match(rest);
    while (rxMatch.hasMatch()) {
        result += rxMatch.captured(1);
        escape = rxMatch.captured(2);
        var = rxMatch.captured(3);
        last = rxMatch.captured(4);
        if (!escape.isEmpty()) {
            result += QLatin1String("${") + var + QLatin1Char('}');
        } else {
            result += sub.value(var);
        }
        rxMatch = rx.match(last);
    }
    return result + last;
}

QString DebconfFrontend::property(const QString &key, PropertyKey p) const
{
    const QString r = m_data.value(key).value(p);
    if (p == Description || p == Choices) {
        return substitute(key, r);
    }
    return r;
}

void DebconfFrontend::cmd_capb(const QStringList &args)
{
    const QString caps = args[0];
    Q_EMIT backup(caps.split(QLatin1String(", ")).contains(QLatin1String("backup")));
    say(QLatin1String("0 backup"));
}

void DebconfFrontend::cmd_set(const QStringList &args)
{
    const QString item = args[0];
    const QString value = args[1];
    m_values[item] = value;
    qCDebug(DEBCONF) << "# SET: [" << item << "] " << value;
    say(QLatin1String("0 ok"));
}

void DebconfFrontend::cmd_get(const QStringList &args)
{
    say(QLatin1String("0 ") + m_values.value(args[0]));
}

void DebconfFrontend::cmd_input(const QStringList &args)
{
    m_input.append(args[0]);
    say(QLatin1String("0 will ask" ));
}

void DebconfFrontend::cmd_go(const QStringList &)
{
    qCDebug(DEBCONF) << "# GO";
    m_input.removeDuplicates();
    Q_EMIT go(m_title, m_input);
    m_input.clear();
}

void DebconfFrontend::cmd_progress(const QStringList &args)
{
    const QString param = args[0];
    qCDebug(DEBCONF) << "DEBCONF: PROGRESS " << param;
    Q_EMIT progress(param);
}

void DebconfFrontend::cmd_register(const QStringList &args)
{
    const QString templateName = args[0];
    const QString question = args[1];

    if (!m_data.contains(templateName)) {
        say(QLatin1String("20 No such template, \"%1\"").arg(templateName));
        return;
    }
    m_data[question] = m_data[templateName];
    say(QLatin1String("0 ok"));
}

void DebconfFrontend::cmd_unregister(const QStringList &args)
{
    const QString question = args[0];
    if (!m_data.contains(question)) {
        say(QLatin1String("20 %1 doesn't exist").arg(question));
        return;
    }
    if (m_input.contains(question)) {
        say(QLatin1String("20 %1 is busy, cannot unregister right now").arg(question));
        return;
    }
    m_data.remove(question);
    say(QLatin1String("0 ok"));
}

void DebconfFrontend::cmd_metaget(const QStringList &args)
{
    const QString question = args[0];
    const QString fieldName = args[1];
    if (!m_data.contains(question)) {
        say(QLatin1String("20 %1 doesn't exist").arg(question));
        return;
    }
    const auto properties = m_data[question];
    const auto key = propertyKeyFromString(fieldName);
    if (!properties.contains(key)) {
        say(QLatin1String("20 %1 does not exist").arg(fieldName));
        return;
    }
    const QString data = properties[key];
    say(QLatin1String("0 ") + data);
}

void DebconfFrontend::cmd_exist(const QStringList &args)
{
    const QString question = args[0];
    say(QLatin1String("0 ") + (m_data.contains(question) ? QLatin1String("true") : QLatin1String("false")));
}

void DebconfFrontend::next()
{
    m_input.clear();
    say(QLatin1String("0 ok, got the answers"));
}

void DebconfFrontend::back()
{
    m_input.clear();
    say(QLatin1String("30 go back"));
}

void DebconfFrontend::cancel()
{
    reset();
}

void DebconfFrontend::cmd_title(const QStringList &args)
{
    const QString param = args[0];
    if (!property(param, Description).isEmpty()) {
        m_title = property(param, Description);
    } else {
        m_title = param;
    }
    qCDebug(DEBCONF) << "DEBCONF: TITLE " << m_title;
    say(QLatin1String("0 ok"));
}

void DebconfFrontend::cmd_data(const QStringList &args)
{
    // We get strings like
    // aiccu/brokername description Tunnel broker:
    // item = "aiccu/brokername"
    // type = "description"
    // rest = "Tunnel broker:"
    const QString item = args[0];
    const QString type = args[1];
    const QString value = args[2];

    m_data[item][propertyKeyFromString(type)] = value;
    qCDebug(DEBCONF) << "# NOTED: [" << item << "] [" << type << "] " << value;
    say(QStringLiteral( "0 ok" ));
}

void DebconfFrontend::cmd_subst(const QStringList &args)
{
    // We get strings like
    // aiccu/brokername brokers AARNet, Hexago / Freenet6, SixXS, Wanadoo France
    // item = "aiccu/brokername"
    // type = "brokers"
    // value = "AARNet, Hexago / Freenet6, SixXS, Wanadoo France"
    const QString item = args[0];
    const QString type = args[1];
    const QString value = args[2];

    m_subst[item][type] = value;
    qCDebug(DEBCONF) << "# SUBST: [" << item << "] [" << type << "] " << value;
    say(QLatin1String("0 ok"));
}

void DebconfFrontend::cmd_x_ping(const QStringList &args)
{
    Q_UNUSED(args);
    say(QLatin1String("0 pong"));
}

void DebconfFrontend::cmd_version(const QStringList &args)
{
    const QString param = args[0];
    if ( !param.isEmpty() ) {
        const QString major_version_str = param.section(QLatin1Char('.'), 0, 0);
        bool ok = false;
        int major_version = major_version_str.toInt( &ok );
        if ( !ok || (major_version != 2) ) {
            say(QLatin1String("30 wrong or too old protocol version"));
            return;
        }
    }
    //This debconf frontend is suposed to use the version 2.1 of the protocol.
    say(QLatin1String("0 2.1"));
}

void DebconfFrontend::cmd_x_loadtemplatefile(const QStringList &args)
{
    QFile template_file(args[0]);
    if (template_file.open(QFile::ReadOnly)) {
        QTextStream template_stream(&template_file);
        QString line = QLatin1String("");
        int linecount = 0;
        QHash<QString,QString> field_short_value;
        QHash<QString,QString> field_long_value;
        QString last_field_name;
        while ( !line.isNull() ) {
            ++linecount;
            line = template_stream.readLine();
            qCDebug(DEBCONF) << linecount << line;
            if ( line.isEmpty() ) {
                if (!last_field_name.isEmpty()) {
                    //Submit last block values.
                    qCDebug(DEBCONF) << "submit" << last_field_name;
                    const QString item = field_short_value[QLatin1String("template")];
                    const QString type = field_short_value[QLatin1String("type")];
                    const QString short_description = field_short_value[QLatin1String("description")];
                    const QString long_description = field_long_value[QLatin1String("description")];

                    m_data[item][DebconfFrontend::Type] = type;
                    m_data[item][DebconfFrontend::Description] = short_description;
                    m_data[item][DebconfFrontend::ExtendedDescription] = long_description;
                    
                    //Clear data.
                    field_short_value.clear();
                    field_long_value.clear();
                    last_field_name.clear();
                }
            } else {
                if (!line.startsWith(QLatin1Char(' '))) {
                    last_field_name = line.section(QLatin1String(": "), 0, 0).toLower();
                    field_short_value[last_field_name] = line.section(QLatin1String(": "), 1);
                } else {
                    if ( field_long_value[last_field_name].isEmpty() ){
                        field_long_value[last_field_name] = line.remove(0, 1);
                    } else {
                        field_long_value[last_field_name].append(QLatin1Char('\n'));
                        if ( !(line.trimmed() == QLatin1String(".")) ) {
                            field_long_value[last_field_name].append(line.remove(0, 1));
                        }
                    }
                }
            }
        }
    } else {
        say(QLatin1String("30 couldn't open file"));
        return;
    }
    say(QLatin1String("0 ok"));
}

void DebconfFrontend::cmd_info(const QStringList &args)
{
    const QString &key = args[0];
    if (key.isEmpty()) {
        // this is the Perl debconf behaviour
        say(QLatin1String("0 ok"));
        return;
    }
    if (!m_data.contains(key)) {
        say(QLatin1String("20 question doesn't exist"));
        return;
    }
    m_side_info = m_data[key][PropertyKey::Description];
    say(QLatin1String("0 ok"));
}

void DebconfFrontend::cmd_fget(const QStringList &args)
{
    // We get strings like
    // foo/bar seen false
    // question = "foo/bar"
    // flag = "seen"
    const QString question = args[0];
    const QString flag = args[1];

    if (m_flags[question][flag]) {
        say(QLatin1String("0 true"));
    } else {
        say(QLatin1String("0 false"));
    }
}

void DebconfFrontend::cmd_fset(const QStringList &args)
{
    // We get strings like
    // foo/bar seen false
    // question = "foo/bar"
    // flag = "seen"
    // value = "false"
    const QString question = args[0];
    const QString flag = args[1];
    const QString value = args[2];

    if ( value == QLatin1String("false") ) {
        m_flags[question][flag] = false;
    } else {
        m_flags[question][flag] = true;
    }
    say(QLatin1String("0 ok"));
}

void DebconfFrontend::cmd_beginblock(const QStringList &args)
{
    Q_UNUSED(args)
    m_making_block = true;
    say(QLatin1String("0 ok"));
}

void DebconfFrontend::cmd_endblock(const QStringList &args)
{
    Q_UNUSED(args)
    if (m_making_block)
        m_input.append(QStringLiteral("div"));
    m_making_block = false;
    say(QLatin1String("0 ok"));
}

void DebconfFrontend::cmd_stop(const QStringList &args)
{
     Q_UNUSED(args)
     //Do nothing.
}

bool DebconfFrontend::process()
{
    QTextStream in(getReadDevice());
    QString line = in.readLine();

    if (line.isEmpty()) {
        return false;
    }

    const QString command = line.section(QLatin1Char(' '), 0, 0);
    const QString value = line.section(QLatin1Char(' '), 1);

    qCDebug(DEBCONF) << "DEBCONF <--- [" << command << "] " << value;
    const Cmd *c = commands;
    while (c->cmd != nullptr) {
        if (command == QLatin1String(c->cmd)) {
            QStringList args{};
            if (c->num_args_min > 0) {
                int args_scanned = 0;
                for (int i = 0; i < c->num_args_min; i++) {
                    const int end_index = (i < (c->num_args_min - 1)) ? i : -1;
                    const QString arg = value.section(QLatin1Char(' '), i, end_index);
                    if (!arg.isEmpty()) args_scanned++;
                    args.append(arg);
                }

                if (args_scanned < c->num_args_min) {
                    say(QLatin1String("20 Incorrect number of arguments"));
                    return false;
                }
            } else {
                args.append(value);
            }
            (this->*(c->run))(args);
            return true;
        }
        ++ c;
    }
    say(QLatin1String("20 Unsupported command \"%1\" received").arg(command.toLower()));
    return false;
}

DebconfFrontendSocket::DebconfFrontendSocket(const QString &socketName, QObject *parent)
  : DebconfFrontend(parent), m_socket(nullptr)
{
    m_server = new QLocalServer(this);
    QFile::remove(socketName);
    m_server->listen(socketName);
    connect(m_server, &QLocalServer::newConnection, this, &DebconfFrontendSocket::newConnection);
}

DebconfFrontendSocket::~DebconfFrontendSocket()
{
    QFile::remove(m_server->fullServerName());
}

void DebconfFrontendSocket::newConnection()
{
    if (m_socket) {
        QLocalSocket *socket = m_server->nextPendingConnection();
        socket->disconnectFromServer();
        socket->deleteLater();
        return;
    }

    m_socket = m_server->nextPendingConnection();
    if (m_socket) {
        connect(m_socket, &QLocalSocket::readyRead, this, &DebconfFrontendSocket::process);
        connect(m_socket, &QLocalSocket::disconnected, this, &DebconfFrontendSocket::disconnected);
    }
}

void DebconfFrontendSocket::reset()
{
    if (m_socket) {
        m_socket->deleteLater();
        m_socket = nullptr;
    }
    DebconfFrontend::reset();
}

void DebconfFrontendSocket::cancel()
{
    if (m_socket) {
        m_socket->disconnectFromServer();
    }
    DebconfFrontend::cancel();
}

DebconfFrontendFifo::DebconfFrontendFifo(int readfd, int writefd, QObject *parent)
  : DebconfFrontend(parent)
{
    m_readf = new QFile(this);
    // Use QFile::open(fh,mode) method for opening read file descriptor as
    // sequential files opened with QFile::open(fd,mode) are not handled
    // properly.
    FILE *readfh = ::fdopen(readfd, "rb");
    m_readf->open(readfh, QIODevice::ReadOnly);

    m_writef = new QFile(this);
    m_writef->open(writefd, QIODevice::WriteOnly);
    // QIODevice::readyReady() does not work with QFile
    // http://bugreports.qt.nokia.com/browse/QTBUG-16089
    m_readnotifier = new QSocketNotifier(readfd, QSocketNotifier::Read, this);
    connect(m_readnotifier, &QSocketNotifier::activated, this, &DebconfFrontendFifo::process);
}

void DebconfFrontendFifo::reset()
{
    if (m_readf) {
        // Close file descriptors
        int readfd = m_readf->handle();
        int writefd = m_writef->handle();
        m_readnotifier->setEnabled(false);
        m_readf->close();
        m_writef->close();
        m_readf = m_writef = nullptr;

        // Use C library calls because QFile::close() won't close them actually
        ::close(readfd);
        ::close(writefd);
    }
    DebconfFrontend::reset();
}

void DebconfFrontendFifo::cancel()
{
    disconnected();
}

bool DebconfFrontendFifo::process()
{
    // We will get notification when the other end is closed
    if (m_readf->atEnd()) {
        cancel();
        return false;
    }
    return DebconfFrontend::process();
}

}

#include "moc_debconf.cpp"
