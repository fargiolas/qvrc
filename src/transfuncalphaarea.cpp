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
#include "transfuncalphaarea.h"
#include "util.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPaintEvent>
#include <QPainter>
#include <QLinearGradient>
#include <QPen>
#include <QColorDialog>

/* color gradient widget */
TransFuncAlphaArea::TransFuncAlphaArea(PresetManager *pm)
{
    this->prman = pm;

    Preset *p = prman->presets[prman->selected];
    points = p->alpha_points;

    active_point = -1;

    tf_len = TF_CHANNEL_SIZE;
    tf_data = (float *) malloc(tf_len * sizeof(float));

    setFocusPolicy(Qt::ClickFocus);
}

void TransFuncAlphaArea::update_preset(int i)
{
    Preset *p = prman->presets[i];
    points = p->alpha_points;

    active_point = -1;
    update_transfer_function();
    update();
}

TransFuncAlphaArea::~TransFuncAlphaArea() {
    free(tf_data);
}

void TransFuncAlphaArea::update_transfer_function()
{
    for (int i=0; i<points.size()-1; i++) {
        int x1 = points.at(i)->p.x() * (tf_len-1);
        int x2 = points.at(i+1)->p.x() * (tf_len-1);

        float y1 = (1.0-points.at(i)->p.y());
        float y2 = (1.0-points.at(i+1)->p.y());

        // printf("x1: %d x2: %d\n", x1, x2);
        // printf("y1: %f y2: %f\n", y1, y2);

        for (int j=x1; j<=x2; j++) {
            float x = (float)(j - x1)/(float)(x2-x1);
            // x = smoothstep(0.0, 1.0, x);
            tf_data[j] = lerp(y1, y2, x);
        }
    }

    prman->presets[prman->selected]->alpha_points = points;

    emit transfer_function_ready(tf_data, tf_len);
}

void TransFuncAlphaArea::paintEvent(QPaintEvent *)
{
    drawing_area.setX(rect().x());
    drawing_area.setY(rect().y());
    drawing_area.setWidth(rect().width());
    drawing_area.setHeight(rect().height());

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QLinearGradient gradient(drawing_area.topLeft(), drawing_area.topRight());

    QBrush checker = QBrush(QColor(0, 0, 0, 120));
    checker.setStyle(Qt::CrossPattern);
    painter.fillRect(drawing_area, checker);

    QPainterPath path;
    path.moveTo(norm_to_rect(points[0]->p, drawing_area));
    for (int i=1; i<points.size(); i++) {
        path.lineTo(norm_to_rect(points[i]->p, drawing_area));
    }

    QPainterPathStroker stroker;
    stroker.setWidth(2);
    QPainterPath stroke = stroker.createStroke(path);
    painter.fillPath(stroke, QColor(200, 50, 0, 150));

    for (int i=0; i<points.size(); i++) {
        TransFuncPoint *p = points.at(i);

        if (i == active_point) {
            QPen pen = QPen(QColor(20, 20, 50));
            pen.setWidth(2);
            painter.setPen(pen);
            painter.setBrush(QColor(255, 255, 255));
        } else {
            QPen pen = QPen(QColor(20, 20, 50));
            painter.setPen(pen);
            painter.setBrush(QColor(200, 200, 220, 255));
        }

        QPointF rectp = norm_to_rect(p->p, drawing_area);
        painter.drawEllipse(rectp.x() - point_radius,
                            rectp.y() - point_radius,
                            2*point_radius, 2*point_radius);
    }
}

void TransFuncAlphaArea::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (active_point < 0) {
        points << new TransFuncPoint(rect_to_norm(e->pos(), drawing_area));
        qSort(points.begin(), points.end(), trans_func_point_compare_x);
    } else {
        active_point = -1;
    }

    update_transfer_function();
    update();
}

void TransFuncAlphaArea::mouseMoveEvent(QMouseEvent *e)
{
    if (active_point < 0)
        return;

    qSort(points.begin(), points.end(), trans_func_point_compare_x);
    active_point = get_selected_idx();


    QPointF newpos = rect_to_norm(e->pos(), drawing_area);

    /* don't touch side points */
    if ((active_point == 0) || (active_point == points.size()-1))
        newpos.setX(points[active_point]->p.x());

    points[active_point]->set_point(newpos);

    emit fast_rendering_hint(true);

    update_transfer_function();
    update();
}

void TransFuncAlphaArea::keyReleaseEvent(QKeyEvent *e)
{
    if ((e->key() == Qt::Key_Delete) &&
        (active_point > 0) && (active_point < (points.size()-1))) {
        if (points.size() > 1)
            points.remove(active_point);
        active_point = -1;

        update_transfer_function();
        update();
    }
    else
        QWidget::keyReleaseEvent(e);
}

QSize TransFuncAlphaArea::sizeHint() const
{
    return QSize(250, 80);
}
