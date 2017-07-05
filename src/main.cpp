/*
 *  qvrc - a GLSL volume rendering engine
 *  well... engine... let's say prototype/proof of concept... hack?
 *
 *  Copyright (C) 2017 Filippo Argiolas <filippo.argiolas@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *  02110-1301 USA.
 */

#include <QApplication>
#include <QSurfaceFormat>
#include <QCommandLineParser>
#include <QCommandLineOption>

#include "window.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("qvrc");
    QCoreApplication::setApplicationVersion("0.1");

    QCommandLineParser parser;
    parser.setApplicationDescription("GLSL Volume Raycasting");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption fname_opt(QStringList() << "f" << "filename",
                                 "Raw volumetric data filename",
                                 "path/to/filename.raw",
                                 "datasets/head256.raw");
    parser.addOption(fname_opt);

    QCommandLineOption size_opt(QStringList() << "s" << "size",
                                 "Voxel data size",
                                 "width,height,depth",
                                 "256,256,225");
    parser.addOption(size_opt);

    QCommandLineOption scale_opt(QStringList() << "x" << "scale",
                                "Voxel scale / aspect ratio",
                                "xscale,yscale,zscale",
                                "1.0,1.0,1.0");
    parser.addOption(scale_opt);


    QCommandLineOption bit_depth_opt(QStringList() << "d" << "bitdepth",
                                     "Voxel bit depth",
                                     "8,10,12,16",
                                     "12");
    parser.addOption(bit_depth_opt);


    parser.process(app);

    InitOptions opt;

    opt.filename = parser.value(fname_opt);

    QString size = parser.value(size_opt);
    QStringList l = size.split(",");
    opt.width = l[0].toInt();
    opt.height = l[1].toInt();
    opt.depth = l[2].toInt();

    QString scale = parser.value(scale_opt);
    l = scale.split(",");
    opt.xscale = l[0].toFloat();
    opt.yscale = l[1].toFloat();
    opt.zscale = l[2].toFloat();

    QString bit_depth = parser.value(bit_depth_opt);
    opt.bit_depth = bit_depth.toInt();

    QSurfaceFormat fmt;
    fmt.setDepthBufferSize(24);
    // fmt.setSamples(4); // complicates everything with offscreen rendering
    fmt.setVersion(3, 2);
    fmt.setProfile(QSurfaceFormat::CoreProfile);

    QSurfaceFormat::setDefaultFormat(fmt);

    Window window(opt);
    window.resize(window.sizeHint());

    window.show();

    return app.exec();
}
