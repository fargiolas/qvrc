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

#include "transfuncwidget.h"
#include "transfuncarea.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPaintEvent>
#include <QPainter>
#include <QLinearGradient>
#include <QPen>
#include <QColorDialog>

/* utils */
QPointF norm_to_rect(QPointF p, QRect r)
{
    return QPointF(p.x() * r.width(), p.y() * r.height());
}
QPointF rect_to_norm(QPointF p, QRect r,
                     bool fixed_y)
{
    qreal newx = qBound(0.0, p.x() / r.width(), 1.0);
    qreal newy;

    if (fixed_y)
        newy = 0.5;
    else
        newy = qBound(0.0, p.y() / r.height(), 1.0);

    return QPointF(newx, newy);
}

/* transfuncarea base class */
TransFuncArea::TransFuncArea(QWidget *parent) :
    QWidget(parent)
{
    point_radius = 6.0;
    fast_rendering = false;
}

QSize TransFuncArea::minimumSizeHint() const
{
    return QSize(100, 20);
}
QSize TransFuncArea::sizeHint() const
{
    return QSize(250, 80);
}

int TransFuncArea::click_to_point(QPointF click)
{
    qreal prevd = 100.; /* something big */
    int found = -1;
    for (int i=0; i<points.size(); i++) {
        qreal d = QLineF(norm_to_rect(points.at(i)->p, drawing_area),
                         click).length();

        if ((d < prevd) && (d <= point_radius * 2))
            found = i;
        prevd = d;
    }

    return found;
}

int TransFuncArea::get_selected_idx()
{
    int selected = -1;
    for (int i=0; i<points.size(); i++) {
        if (points[i]->is_selected()) selected = i;
    }

    return selected;
}

int TransFuncArea::set_selected_idx(int idx)
{
    for (int i=0; i<points.size(); i++) {
        if (idx == i)
            points[i]->set_selected(true);
        else
            points[i]->set_selected(false);
    }

    return idx;
}

void TransFuncArea::mousePressEvent(QMouseEvent *e)
{
    active_point = set_selected_idx(click_to_point(e->pos()));

    // if (active_point < 0)
    //     printf("no point near\n");
    // else
    //     printf("found: %d\n", active_point);

    update();
}

void TransFuncArea::mouseReleaseEvent(QMouseEvent *e)
{
    Q_UNUSED(e);

    fast_rendering = false;
    emit fast_rendering_hint(false);

    update();
}
