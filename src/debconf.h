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

#ifndef DEBCONF_H
#define DEBCONF_H

#include <QtCore/QHash>
#include <QtCore/QStringList>
#include <QtCore/QMetaEnum>
#include <QtCore/QMetaObject>
#include <QtCore/QFile>
#include <QtNetwork/QLocalSocket>
#include <QtNetwork/QLocalServer>

class QSocketNotifier;

namespace DebconfKde {

/**
  * An abstract class which talks Debconf Passthrough frontend protocol. It
  * does not implement underlying I/O method specifics.
  */
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

    explicit DebconfFrontend(QObject *parent = nullptr);
    virtual ~DebconfFrontend();

    QString value(const QString &key) const;
    void setValue(const QString &key, const QString &value);

    QString property(const QString &key, PropertyKey p) const;

    TypeKey type(const QString &string) const;

    /**
      * Send \p string to Debconf over established connection
      */
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
    virtual void cancel();

    inline QString getSideInfo() const { return m_side_info; }

Q_SIGNALS:
    void go(const QString &title, const QStringList &input);
    void progress(const QString &param);
    /**
      * Emitted when connection with Debconf is terminated.
      */
    void finished();
    void backup(bool capable);

protected Q_SLOTS:
    /**
     * This slot should be called when there is new data available.
     */
    virtual bool process();

    /**
     * This slot should be called when connection with Debconf is terminated.
     */
    virtual void disconnected();

protected:
    /**
      * This method must be overridden by the derivative class and return
      * current QIODevice which should be used to read data from Debconf.
      */
    virtual QIODevice* getReadDevice() const = 0;
    /**
      * This method must be overridden by the derivative class and return
      * current QIODevice which should be used to write data to Debconf.
      */
    virtual QIODevice* getWriteDevice() const = 0;
    /**
     * This is called to clean up internal data once connection with
     * Debconf is terminated.
     */
    virtual void reset();

private:
    void cmd_capb(const QStringList &args);
    void cmd_set(const QStringList &args);
    void cmd_get(const QStringList &args);
    void cmd_input(const QStringList &args);
    void cmd_go(const QStringList &);
    void cmd_title(const QStringList &args);
    void cmd_data(const QStringList &args);
    void cmd_subst(const QStringList &args);
    void cmd_progress(const QStringList &args);
    void cmd_x_ping(const QStringList &args);
    void cmd_version(const QStringList &args);
    void cmd_x_loadtemplatefile(const QStringList &args);
    void cmd_info(const QStringList &args);
    void cmd_fget(const QStringList &args);
    void cmd_fset(const QStringList &args);
    void cmd_beginblock(const QStringList &args);
    void cmd_endblock(const QStringList &args);
    void cmd_stop(const QStringList &args);
    void cmd_register(const QStringList &args);
    void cmd_unregister(const QStringList &args);
    void cmd_metaget(const QStringList &args);
    void cmd_exist(const QStringList &args);
    struct Cmd {
        const char *cmd;
        void (DebconfFrontend::*run)(const QStringList &);
        int num_args_min;
    };
    static const Cmd commands[];

    // TODO this is apparently very much untested
    QString substitute(const QString &key, const QString &rest) const;
    /**
     * Transforms the string camel cased and return it's enum given the enum name
     */
    template<class T> static int enumFromString(const QString &str, const char *enumName);
    PropertyKey propertyKeyFromString(const QString &string);

    typedef QHash<PropertyKey, QString> Properties;
    typedef QHash<QString, QString> Substitutions;
    typedef QHash<QString, bool> Flags;

    QHash<QString, Properties>    m_data;
    QHash<QString, Substitutions> m_subst;
    QHash<QString, QString>       m_values;
    QHash<QString, Flags>         m_flags;
    QString m_title;
    QString m_side_info;
    QStringList m_input;
    bool m_making_block;
};

/**
  * DebconfFrontend which communicates with Debconf over UNIX socket. Even when
  * finished signal is emitted, DeconfFrontend will reset and continue to
  * listen for new connections on the socket.
  */
class DebconfFrontendSocket : public DebconfFrontend {
    Q_OBJECT

public:
    /**
      * Instantiates the class and starts listening for new connections on the
      * socket at \p socketName path. Please note that any file at \p socketName
      * will be removed if it exists prior to the call of this constructor.
      */
    explicit DebconfFrontendSocket(const QString &socketName, QObject *parent = nullptr);

    /**
      * Removes socket when the object is destroyed.
      */
    virtual ~DebconfFrontendSocket();

    /**
      * Overridden to trigger termination of the current connection.
      */
    void cancel() override;

protected:
    inline QIODevice* getReadDevice() const override { return m_socket; }
    inline QIODevice* getWriteDevice() const override { return m_socket; }
    void reset() override;

private Q_SLOTS:
    /**
     * Called when a new connection is received on the socket
     * If we are already handlyng a connection we refuse the others
     */
    void newConnection();

private:
    QLocalServer *m_server;
    QLocalSocket *m_socket;
};

/**
  * DebconfFrontend which communicates with Debconf over FIFO pipes. Once
  * finished signal is emitted, the frontend is no longer usable as pipes
  * have been been closed by then.
  */
class DebconfFrontendFifo : public DebconfFrontend {

public:
    /**
      * Instantiates the class and prepares for communication with Debconf over
      * \p readfd (read) and \p writefd (write) FIFO file descriptors.
      */
    explicit DebconfFrontendFifo(int readfd, int writefd, QObject *parent = nullptr);

    /**
      * Overridden to trigger full disconnection
      */
    void cancel() override;

protected:
    QIODevice* getReadDevice() const override { return m_readf; }
    QIODevice* getWriteDevice() const override { return m_writef; }
    void reset() override;
    bool process() override;

private:
    QFile *m_readf;
    QFile *m_writef;
    QSocketNotifier *m_readnotifier;
};

}

#endif
