/*
 * Copyright (C) 2010 Daniel Nicoletti <dantti12@gmail.com>
 *           (C) 2011 Modestas Vainius <modax@debian.org>
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

#include <QtCore/QRegExp>

#include <KApplication>
#include <KAboutData>
#include <KCmdLineArgs>
#include <KDebug>

#include <iostream>

#include <DebconfGui.h>

using namespace DebconfKde;

int main(int argc, char **argv)
{
    KAboutData aboutData("debkonf",
                         QByteArray(),
                         ki18n("Debconf KDE"),
                         "0.1",
                         ki18n("Debconf frontend for KDE"),
                         KAboutData::License_LGPL);

    KCmdLineArgs::init(argc, argv, &aboutData);

    KCmdLineOptions options;
    options.add("socket-path <path_to_socket>", ki18n("Path to where the socket should be created"));
    options.add("fifo-fds <read_fd,write_fd>", ki18n("FIFO file descriptors for communication with Debconf"));
    KCmdLineArgs::addCmdLineOptions(options);

    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

    KApplication app;
    DebconfGui *dcf = 0;

    if (args->isSet("fifo-fds")) {
        int readfd, writefd;
        QRegExp regex(QLatin1String("(\\d+),(\\d+)"));
        if (regex.exactMatch(args->getOption("fifo-fds"))) {
            readfd = regex.cap(1).toInt();
            writefd = regex.cap(2).toInt();

            dcf = new DebconfGui(readfd, writefd);
            dcf->connect(dcf, SIGNAL(activated()), SLOT(show()));
            // Once FIFO pipes are closed, they cannot be reopened. Hence we
            // should terminate as well.
            dcf->connect(dcf, SIGNAL(deactivated()), SLOT(close()));
        } else {
            kFatal() << "Incorrect value of the --fifo-fds parameter";
        }
    } else {
        QString path;
        if (args->isSet("socket-path")) {
            path = args->getOption("socket-path");
        } else {
            path = QLatin1String("/tmp/debkonf-sock");
        }
        dcf = new DebconfGui(path);
        std::cout << "export DEBIAN_FRONTEND=passthrough" << std::endl;
        std::cout << "export DEBCONF_PIPE=" << path.toUtf8().data() << std::endl;

        dcf->connect(dcf, SIGNAL(activated()), SLOT(show()));
        dcf->connect(dcf, SIGNAL(deactivated()), SLOT(hide()));
    }

    if (!dcf)
        return 1;

    app.setTopWidget(dcf);

    return app.exec();
}
