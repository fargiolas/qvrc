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

#ifndef TRANS_FUNC_ALPHAAREA_H
#define TRANS_FUNC_ALPHAAREA_H

#include <QWidget>
#include <QColor>
#include <QPointF>
#include <QPaintEvent>

#include "presetmanager.h"
#include "transfuncwidget.h"
#include "transfuncarea.h"

/* Drawing area for the color gradient */
class TransFuncAlphaArea : public TransFuncArea
{
    Q_OBJECT

public:
    TransFuncAlphaArea(PresetManager *pr);
    ~TransFuncAlphaArea();

    void update_transfer_function();

public slots:
    void update_preset(int i);

signals:
    void transfer_function_ready(float *tf_data, int sz);

protected:
    void paintEvent(QPaintEvent *);
    void mouseMoveEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
    void mouseDoubleClickEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
    void keyReleaseEvent(QKeyEvent *e) Q_DECL_OVERRIDE;
    QSize sizeHint() const Q_DECL_OVERRIDE;

private:
    float *tf_data;
    int tf_len;
};

#endif // TRANS_FUNC_ALPHAAREA_H
