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

#ifndef COLOR_BUTTON_H
#define COLOR_BUTTON_H

#include <QPixmap>
#include <QIcon>
#include <QSize>
#include <QToolButton>

class ColorButton : public QToolButton
{
    Q_OBJECT

public:
    ColorButton(QWidget *parent = 0): QToolButton(parent)  {}
    ColorButton(const QColor &color);

    void setColor(const QColor &color);

private:
    QPixmap pixmap;
};

#endif /* COLOR_BUTTON_H */
