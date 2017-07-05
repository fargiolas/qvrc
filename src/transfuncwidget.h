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

#ifndef TRANS_FUNC_WIDGET_H
#define TRANS_FUNC_WIDGET_H

#include <QWidget>
#include <QColor>
#include <QPointF>
#include <QPaintEvent>
#include <QFrame>
#include <QSettings>

#include "presetmanager.h"

#define TF_CHANNEL_SIZE 4096

/* Parent, container widget, point global state, layout and signals */
class TransFuncWidget : public QFrame
{
    Q_OBJECT

public:
    TransFuncWidget(QWidget *parent) : QFrame(parent) {};
    TransFuncWidget(PresetManager *prman);
    ~TransFuncWidget();

    void update_preset(int i);


public slots:
    void new_rgb_data(float *rgb_data, int len);
    void new_alpha_data(float *alpha_data, int len);
    void forward_fast_rendering_hint(bool hint);

signals:
    void preset_selected(int i);
    void transfer_function_ready(float *tf_data, int len);
    void fast_rendering_hint(bool hint);

private:
    PresetManager *prman;
    QSettings *settings;
    float *tf_data;
    int tf_len;
};

#endif // TRANS_FUNC_WIDGET_H
