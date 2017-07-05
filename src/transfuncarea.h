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

#ifndef TRANS_FUNC_AREA_H
#define TRANS_FUNC_AREA_H

#include <QWidget>
#include <QColor>
#include <QPointF>
#include <QPaintEvent>

#include "transfuncwidget.h"
#include "presetmanager.h"


QPointF norm_to_rect(QPointF p, QRect r);
QPointF rect_to_norm(QPointF p, QRect r, bool fixed_y = false);


/* Drawing area for the color gradient */
class TransFuncArea : public QWidget
{
    Q_OBJECT

public:
    TransFuncArea(QWidget *parent = 0);

    QSize minimumSizeHint() const Q_DECL_OVERRIDE;
    QSize sizeHint() const Q_DECL_OVERRIDE;

signals:
    void fast_rendering_hint(bool hint);

protected:
    void mousePressEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *e) Q_DECL_OVERRIDE;

    int click_to_point(QPointF click);
    int get_selected_idx();
    int set_selected_idx(int i);


    PresetManager *prman;
    TransFuncWidget *tfw;

    QVector<TransFuncPoint *> points;
    int active_point;
    QRect drawing_area;

    qreal point_radius;
    qreal offset;

    bool fast_rendering = false;
};

#endif // TRANS_FUNC_AREA_H
