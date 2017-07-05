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
#include "transfunclutarea.h"
#include "util.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPaintEvent>
#include <QPainter>
#include <QLinearGradient>
#include <QPen>
#include <QColorDialog>

/* color gradient widget */
TransFuncLutArea::TransFuncLutArea(PresetManager *pm)
{
    this->prman = pm;

    Preset *p = prman->presets[prman->selected];
    points = p->lut_points;

    active_point = -1;

    tf_len = TF_CHANNEL_SIZE;
    tf_data = (float *) malloc(3*tf_len * sizeof(float));
    memset(tf_data, 0.0, 3*tf_len * sizeof(float));

    setFocusPolicy(Qt::ClickFocus);
}

void TransFuncLutArea::update_preset(int i)
{
    Preset *p = prman->presets[i];
    points = p->lut_points;

    active_point = -1;
    update_transfer_function();
    update();
}

TransFuncLutArea::~TransFuncLutArea() {
    free(tf_data);
}

void TransFuncLutArea::update_transfer_function()
{
    for (int i=0; i<points.size()-1; i++) {
        int x1 = points.at(i)->p.x() * (tf_len-1);
        int x2 = points.at(i+1)->p.x() * (tf_len-1);

        float r1 = points.at(i)->c.redF();
        float r2 = points.at(i+1)->c.redF();
        float g1 = points.at(i)->c.greenF();
        float g2 = points.at(i+1)->c.greenF();
        float b1 = points.at(i)->c.blueF();
        float b2 = points.at(i+1)->c.blueF();

        for (int j=x1; j<=x2; j++) {
            float x = (float)(j - x1)/(float)(x2-x1);
            // x = smoothstep(0.0, 1.0, x);
            tf_data[3*j] = lerp(r1, r2, x);
            tf_data[3*j+1] = lerp(g1, g2, x);
            tf_data[3*j+2] = lerp(b1, b2, x);
        }
    }

    prman->presets[prman->selected]->lut_points = points;

    emit transfer_function_ready(tf_data, tf_len);
}

void TransFuncLutArea::paintEvent(QPaintEvent *)
{
    drawing_area.setX(rect().x());
    drawing_area.setY(rect().y());
    drawing_area.setWidth(rect().width());
    drawing_area.setHeight(rect().height());

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QLinearGradient gradient(drawing_area.topLeft(), drawing_area.topRight());

    for (int i=0; i<points.size(); i++) {
        gradient.setColorAt(points[i]->p.x(), points[i]->c);
    }

    painter.fillRect(drawing_area, QBrush(gradient));

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

void TransFuncLutArea::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (active_point < 0) {
        QColor color = QColorDialog::getColor(Qt::white, this,
                                                    "Select Color",
                                                    QColorDialog::DontUseNativeDialog);
        if (color.isValid()) {
            points << new TransFuncPoint(rect_to_norm(e->pos(), drawing_area, true), QColor(color));
            qSort(points.begin(), points.end(), trans_func_point_compare_x);
        }
    }
    else {
        const QColor color = QColorDialog::getColor(points.at(active_point)->c, this,
                                                    "Select Color",
                                                    QColorDialog::DontUseNativeDialog);
        if (color.isValid()) {
            points[active_point]->c = color;

            active_point = -1;
        }
    }

    update_transfer_function();
    update();
}


void TransFuncLutArea::mouseMoveEvent(QMouseEvent *e)
{
    if (active_point < 0)
        return;

    qSort(points.begin(), points.end(), trans_func_point_compare_x);
    active_point = get_selected_idx();

    QPointF newpos = rect_to_norm(e->pos(), drawing_area, true);

    /* don't touch side points */
    if ((active_point == 0) || (active_point == points.size()-1))
        newpos.setX(points[active_point]->p.x());

    points[active_point]->set_point(newpos);

    emit fast_rendering_hint(true);

    update_transfer_function();
    update();
}


void TransFuncLutArea::keyReleaseEvent(QKeyEvent *e)
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

QSize TransFuncLutArea::sizeHint() const
{
    return QSize(250, 36);
}
