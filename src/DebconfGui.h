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

#ifndef DEBCONF_GUI_H
#define DEBCONF_GUI_H

#include <QtGui/QWidget>

#include <kdemacros.h>

namespace DebconfKde {

/**
 * \class DebconfGui DebconfGui.h DebconfGui
 * \author Daniel Nicoletti <dantti12@gmail.com>
 *
 * \brief Widget to present debconf elements
 *
 * This class provides a widget subclass that
 * can present Debconf elements (questions), using
 * a socket file.
 *
 * For this class to be useful the programs that are going
 * to use debconf to present questions must have the environment
 * variables DEBIAN_FRONTEND set to passthrough and DEBCONF_PIPE
 * to the path set on the constructor (\p socketName). Then when
 * a new connection arrives this class will take care of
 * talking the debconf protocol and emit activated() so that
 * this widget should be shown, and deactivated() when it should
 * be hidden.
 *
 * \note It is possible to let this class work automatically by
 * connecting the activated() signal on the QWidget::show() slot and
 * deactivated() on the QWidget::hide() slot.
 *
 * \note This class must not be deleted after deactivated() signal
 * is emitted, since new packages need to talk to the same socket.
 * Only delete it after you are sure no more operations ended.
 */
class DebconfGuiPrivate;
class KDE_EXPORT DebconfGui : public QWidget
{
    Q_OBJECT
public:
    /**
     * Constructor that takes a file path (\p socketName) to create
     * a new socket.
     * \warning Be adivised that this class will delete the path pointed
     * by \p socketName. A good location would be /tmp/debconf-$PID.
     */
    explicit DebconfGui(const QString &socketName, QWidget *parent = 0);

    /**
     * Constructor that prepares for communication with Debconf via FIFO pipes.
     * Read (\p readfd) and write (\p writefd) file descriptors should be open
     * and connected to Debconf which speaks Passthrough frontend protocol.
     */
    explicit DebconfGui(int readfd, int writefd, QWidget *parent = 0);

Q_SIGNALS:
    /**
     * This signal is emitted when a new debconf element (question)
     * needs to be displayed.
     */
    void activated();
    /**
     * This signal is emitted when there are no more debconf element
     * (questions) to show. If FIFO pipes are used for communication, it means
     * that pipes were closed and futher communication is no longer possible.
     * If socket is used, more questions might still be shown in future so do
     * not delete this class.
     */
    void deactivated();

private Q_SLOTS:
    void cmd_go(const QString &title, const QStringList &input);
    void cmd_progress(const QString &param);

    void on_nextPB_clicked();
    void on_backPB_clicked();
    void on_cancelPB_clicked();

protected:
    /**
     * Reimplemented function to cancel the question if the user closes
     * the window.
     */
    void closeEvent(QCloseEvent *event);

    DebconfGuiPrivate * const d_ptr;

private:
    Q_DECLARE_PRIVATE(DebconfGui)

    /**
      * This routine is called by all constructors to perform common
      * initialization.
      */
    void init();
};


}

#endif
