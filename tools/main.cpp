/*
 * Copyright (C) 2010-2018 Daniel Nicoletti <dantti12@gmail.com>
 *           (C) 2011 Modestas Vainius <modax@debian.org>
 *           (C) 2018 Harald Sitter <sitter@kde.org>
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

#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QRegExp>
#include <QSocketNotifier>

#include <KAboutData>
#include <KLocalizedString>

#include <iostream>

#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <DebconfGui.h>

using namespace DebconfKde;

// Handle SIGQUIT. Clients (e.g. packagekit) may use QUIT which would otherwise
// result in a core dump.
// Qt methods aren't signal-safe, so we'll defer the handling via a socket
// notification.
static void setupQuitHandler() {
    static int quitFD[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, quitFD)) {
       qErrnoWarning("Failed to create socket");
    }
    auto notifier = new QSocketNotifier(quitFD[1], QSocketNotifier::Read, qApp);
    QObject::connect(notifier, &QSocketNotifier::activated, [notifier]() {
        notifier->setEnabled(false);
        char c;
        read(quitFD[1], &c, sizeof(c));
        qApp->quit();
    });

    struct sigaction sa;
    sa.sa_handler = [](int) -> void {
        char c = 1;
        write(quitFD[0], &c, sizeof(c));
    };
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGQUIT);
    sa.sa_flags = 0;
    sa.sa_flags |= SA_RESTART;

    if (sigaction(SIGQUIT, &sa, 0) != 0) {
       qErrnoWarning("Failed to set quit handler");
    }
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    setupQuitHandler();

    KLocalizedString::setApplicationDomain("libdebconf-kde");

    KAboutData aboutData(QStringLiteral("debconf-kde-helper"),
                         i18nc("@title", "Debconf KDE"),
                         QStringLiteral(PROJECT_VERSION),
                         i18nc("@info", "Debconf frontend based on Qt"),
                         KAboutLicense::LicenseKey::LGPL);
    KAboutData::setApplicationData(aboutData);

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption socketOption(QStringLiteral("socket-path"),
                                    i18nc("@info:shell", "Path to where the socket should be created"),
                                    i18nc("@info:shell value name", "path_to_socket"),
                                    QStringLiteral("/tmp/debkonf-sock"));
    parser.addOption(socketOption);
    QCommandLineOption fifoFdsOption(QStringLiteral("fifo-fds"),
                                    i18nc("@info:shell", "FIFO file descriptors for communication with Debconf"),
                                    i18nc("@info:shell value name", "read_fd,write_fd"));
    parser.addOption(fifoFdsOption);
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    DebconfGui *dcf = nullptr;
    if (parser.isSet(fifoFdsOption)) {
        int readfd, writefd;
        QRegExp regex(QLatin1String("(\\d+),(\\d+)"));
        if (regex.exactMatch(parser.value(fifoFdsOption))) {
            readfd = regex.cap(1).toInt();
            writefd = regex.cap(2).toInt();

            dcf = new DebconfGui(readfd, writefd);
            dcf->connect(dcf, &DebconfGui::activated, dcf, &DebconfGui::show);
            // Once FIFO pipes are closed, they cannot be reopened. Hence we
            // should terminate as well.
            dcf->connect(dcf, &DebconfGui::deactivated, dcf, &DebconfGui::close);
        } else {
            qFatal("Incorrect value of the --fifo-fds parameter");
        }
    } else {
        QString path = parser.value(socketOption);
        dcf = new DebconfGui(path);
        std::cout << "export DEBIAN_FRONTEND=passthrough" << std::endl;
        std::cout << "export DEBCONF_PIPE=" << path.toUtf8().data() << std::endl;

        dcf->connect(dcf, &DebconfGui::activated, dcf, &DebconfGui::show);
        dcf->connect(dcf, &DebconfGui::deactivated, dcf, &DebconfGui::hide);
    }

    if (!dcf)
        return 1;

    return app.exec();
}
