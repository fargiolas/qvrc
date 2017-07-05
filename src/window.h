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

#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>
#include <QMainWindow>
#include <QComboBox>
#include <QDoubleSpinBox>
#include "glwidget.h"
#include "presetmanager.h"
#include "transfuncwidget.h"
#include "colorbutton.h"

#include "util.h"

class Window : public QMainWindow
{
    Q_OBJECT

public:
    Window(InitOptions &opt);

public slots:
    void preset_selected(int i);
    void save_preset();
    void set_background_color();
    void set_light_color();

protected:
    void keyReleaseEvent(QKeyEvent *event) Q_DECL_OVERRIDE;

signals:
    void background_color_ready(const QColor &color);
    void light_color_ready(const QColor &color);

private:
    PresetManager *prman;
    GLWidget *glWidget;
    QComboBox *preset_combo;
    TransFuncWidget *tf;

    QComboBox *shading_combo;
    QComboBox *comp_combo;
    ColorButton *background_color_button;
    ColorButton *light_color_button;
    QDoubleSpinBox *ambient_spinbox;
    QDoubleSpinBox *diffuse_spinbox;
    QDoubleSpinBox *specular_spinbox;
};

#endif
