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

#include "debconf.h"

#include <QtCore/QSocketNotifier>
#include <QtCore/QRegExp>
#include <QtCore/QStringBuilder>
#include <QtCore/QFile>
#include <cstdio>
#include <unistd.h>

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
    { "X_PING", &DebconfFrontend::cmd_x_ping },
    { "VERSION", &DebconfFrontend::cmd_version },
    { "X_LOADTEMPLATEFILE", &DebconfFrontend::cmd_x_loadtemplatefile },
    { "INFO", &DebconfFrontend::cmd_info },
    { "FSET", &DebconfFrontend::cmd_fset },
    { 0, 0 } };

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
    emit finished();
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
        enumValue = e.keyToValue(QString(QLatin1String( "Unknown" )).append(QLatin1String( enumName )).toAscii().data());
        kDebug() << "enumFromString (" <<QLatin1String( enumName ) << ") : converted" << realName << "to" << QString(QLatin1String( "Unknown" )).append(QLatin1String( enumName )) << ", enum value" << enumValue;
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
    emit backup(false);
    m_data.clear();
    m_subst.clear();
    m_values.clear();
}

void DebconfFrontend::say(const QString &string)
{
    kDebug() << "DEBCONF ---> " << string;
    QTextStream out(getWriteDevice());
    out << string << "\n";
    out.flush();
}

QString DebconfFrontend::substitute(const QString &key, const QString &rest) const
{
    Substitutions sub = m_subst[key];
    QString result, var, escape;
    QRegExp rx(QLatin1String( "^(.*)(\\\\)?\\$\\{([^\\{\\}]+)\\}(.*)$" ));
    QString last(rest);
    int pos = 0;
    while ( (pos = rx.indexIn(rest, pos)) != -1) {
        kDebug() << "var found! at" << pos;
        result += rx.cap(1);
        escape = rx.cap(2);
        var = rx.cap(3);
        last = rx.cap(4);
        if (!escape.isEmpty()) {
            result += QString(QLatin1Literal( "${" ) % var % QLatin1Char( '}' ));
        } else {
            result += sub.value(var);
        }
        pos += rx.matchedLength();
    }
    return result + last;
}

QString DebconfFrontend::property(const QString &key, PropertyKey p) const
{
    QString r = m_data.value(key).value(p);
    if (p == Description || p == Choices) {
        return substitute(key, r);
    }
    return r;
}

void DebconfFrontend::cmd_capb(const QString &caps)
{
    emit backup(caps.split(QLatin1String( ", " )).contains(QLatin1String( "backup" )));
    say(QLatin1String( "0 backup" ));
}

void DebconfFrontend::cmd_set(const QString &param)
{
    QString item = param.section(QLatin1Char( ' ' ), 0, 0);
    QString value = param.section(QLatin1Char( ' ' ), 1);
    m_values[item] = value;
    kDebug() << "# SET: [" << item << "] " << value;
    say(QLatin1String( "0 ok" ));
}

void DebconfFrontend::cmd_get(const QString &param)
{
    say(QLatin1String( "0 " ) + m_values.value(param));
}

void DebconfFrontend::cmd_input(const QString &param)
{
    m_input.append(param.section(QLatin1Char( ' ' ), 1));
    say(QLatin1String( "0 will ask" ));
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
    kDebug() << "DEBCONF: PROGRESS " << param;
    emit progress(param);
}

void DebconfFrontend::next()
{
    m_input.clear();
    say(QLatin1String( "0 ok, got the answers" ));
}

void DebconfFrontend::back()
{
    m_input.clear();
    say(QLatin1String( "30 go back" ));
}

void DebconfFrontend::cancel()
{
    reset();
}

void DebconfFrontend::cmd_title(const QString &param)
{
    if (!property(param, Description).isEmpty()) {
        m_title = property(param, Description);
    } else {
        m_title = param;
    }
    kDebug() << "DEBCONF: TITLE " << m_title;
    say(QLatin1String( "0 ok" ));
}

void DebconfFrontend::cmd_data(const QString &param)
{
    // We get scrings like
    // aiccu/brokername description Tunnel broker:
    // item = "aiccu/brokername"
    // type = "description"
    // rest = "Tunnel broker:"
    QString item = param.section(QLatin1Char( ' ' ), 0, 0);
    QString type = param.section(QLatin1Char( ' ' ), 1, 1);
    QString value = param.section(QLatin1Char( ' ' ), 2);

    m_data[item][propertyKeyFromString(type)] = value;
    kDebug() << "# NOTED: [" << item << "] [" << type << "] " << value;
    say(QLatin1String( "0 ok" ));
}

void DebconfFrontend::cmd_subst(const QString &param)
{
    // We get scrings like
    // aiccu/brokername brokers AARNet, Hexago / Freenet6, SixXS, Wanadoo France
    // item = "aiccu/brokername"
    // type = "brokers"
    // value = "AARNet, Hexago / Freenet6, SixXS, Wanadoo France"
    QString item = param.section(QLatin1Char( ' ' ), 0, 0);
    QString type = param.section(QLatin1Char( ' ' ), 1, 1);
    QString value = param.section(QLatin1Char( ' ' ), 2);

    m_subst[item][type] = value;
    kDebug() << "# SUBST: [" << item << "] [" << type << "] " << value;
    say(QLatin1String( "0 ok" ));
}

void DebconfFrontend::cmd_x_ping(const QString &param)
{
    Q_UNUSED(param);
    say(QLatin1String( "0 pong" ));
}

void DebconfFrontend::cmd_version(const QString &param)
{
    if ( !param.isEmpty() ) {
        QString major_version_str = param.section(QLatin1Char( '.' ), 0, 0);
        bool ok = false;
        int major_version = major_version_str.toInt( &ok );
        if ( !ok || (major_version != 2) ) {
            say(QLatin1String( "30 wrong or too old protocol version" ));
            return;
        }
    }
    //This debconf frontend is suposed to use the version 2.1 of the protocol.
    say(QLatin1String( "0 2.1" ));
}

void DebconfFrontend::cmd_x_loadtemplatefile(const QString &param)
{
    QFile template_file(param);
    if (template_file.open(QFile::ReadOnly)) {
        QTextStream template_stream(&template_file);
        QString line = QLatin1String("");
        int linecount = 0;
        QHash <QString,QString> field_short_value;
        QHash <QString,QString> field_long_value;
	QString last_field_name;
        while ( !line.isNull() ) {
            ++linecount;
            line = template_stream.readLine();
            kDebug() << linecount << line;
            if ( line.isEmpty() ) {
                if (!last_field_name.isEmpty()) {
                    //Submit last block values.
                    kDebug() << "submit" << last_field_name;
                    QString item = field_short_value[QLatin1String("template")];
                    QString type = field_short_value[QLatin1String("type")];
                    QString short_description = field_short_value[QLatin1String("description")];
                    QString long_description = field_long_value[QLatin1String("description")];

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
        say(QLatin1String( "30 couldn't open file" ));
        return;
    }
    say(QLatin1String( "0 ok" ));
}

void DebconfFrontend::cmd_info(const QString &param)
{
    //FIXME: this is a dummy command, we should actually do something
    //with param.
    say(QLatin1String( "0 ok" ));
}

void DebconfFrontend::cmd_fset(const QString &param)
{
    //FIXME: this is a dummy command, we should actually do something
    //with param.
    say(QLatin1String( "0 ok" ));
}

bool DebconfFrontend::process()
{
    QTextStream in(getReadDevice());
    QString line = in.readLine();

    if (line.isEmpty()) {
        return false;
    }

    QString command = line.section(QLatin1Char( ' ' ), 0, 0);
    QString value = line.section(QLatin1Char( ' ' ), 1);

    kDebug() << "DEBCONF <--- [" << command << "] " << value;
    const Cmd *c = commands;
    while (c->cmd != 0) {
        if (command == QLatin1String( c->cmd )) {
            (this->*(c->run))(value);
            return true;
        }
        ++ c;
    }
    return false;
}

DebconfFrontendSocket::DebconfFrontendSocket(const QString &socketName, QObject *parent)
  : DebconfFrontend(parent), m_socket(0)
{
    m_server = new QLocalServer(this);
    QFile::remove(socketName);
    m_server->listen(socketName);
    connect(m_server, SIGNAL(newConnection()), this, SLOT(newConnection()));
}

DebconfFrontendSocket::~DebconfFrontendSocket()
{
    QFile::remove(m_server->fullServerName());
}

void DebconfFrontendSocket::newConnection()
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

void DebconfFrontendSocket::reset()
{
    if (m_socket) {
        m_socket->deleteLater();
        m_socket = 0;
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
    connect(m_readnotifier, SIGNAL(activated(int)), this, SLOT(process()));
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
        m_readf = m_writef = 0;

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

#include "debconf.moc"
