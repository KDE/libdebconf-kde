/*
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

#include <KApplication>
#include <KAboutData>
#include <KCmdLineArgs>

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
    KCmdLineArgs::addCmdLineOptions(options);

    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

    QString path;
    if (args->isSet("socket-path")) {
        path = args->getOption("socket-path");
    } else {
        path = QLatin1String("/tmp/debkonf-sock");
    }

    KApplication app;
    DebconfGui *dcf = new DebconfGui(path);
    app.setTopWidget(dcf);

    dcf->connect(dcf, SIGNAL(activated()), SLOT(show()));
    dcf->connect(dcf, SIGNAL(deactivated()), SLOT(hide()));

    std::cout << "export DEBIAN_FRONTEND=passthrough" << std::endl;
    std::cout << "export DEBCONF_PIPE=" << path.toUtf8().data() << std::endl;

    return app.exec();
}
