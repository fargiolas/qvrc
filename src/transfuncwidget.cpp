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
#include "transfuncalphaarea.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QPushButton>
#include <QDebug>


TransFuncWidget::TransFuncWidget(PresetManager *pm)
{
    prman = pm;

    tf_len = TF_CHANNEL_SIZE;
    tf_data = (float *) malloc(4 * tf_len * sizeof(float));
    memset(tf_data, 0, tf_len * sizeof(float));

    // this->setFrameStyle(QFrame::StyledPanel);

    QVBoxLayout *vlayout = new QVBoxLayout();
    QLabel *lutlabel = new QLabel("Color map");
    QLabel *alphalabel = new QLabel("Opacity");
    TransFuncLutArea *lut = new TransFuncLutArea(prman);
    TransFuncAlphaArea *alpha = new TransFuncAlphaArea(prman);

    setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum));

    vlayout->addWidget(lutlabel);
    vlayout->addWidget(lut);
    vlayout->addWidget(alphalabel);
    vlayout->addWidget(alpha);

    connect(lut, &TransFuncLutArea::transfer_function_ready,
            this, &TransFuncWidget::new_rgb_data, Qt::QueuedConnection);
    connect(alpha, &TransFuncAlphaArea::transfer_function_ready,
            this, &TransFuncWidget::new_alpha_data, Qt::QueuedConnection);
    connect(lut, &TransFuncLutArea::fast_rendering_hint,
            this, &TransFuncWidget::forward_fast_rendering_hint);
    connect(lut, &TransFuncLutArea::fast_rendering_hint,
            this, &TransFuncWidget::forward_fast_rendering_hint);
    connect(alpha, &TransFuncAlphaArea::fast_rendering_hint,
            this, &TransFuncWidget::forward_fast_rendering_hint);

    connect(this, &TransFuncWidget::preset_selected,
            lut, &TransFuncLutArea::update_preset);
    connect(this, &TransFuncWidget::preset_selected,
            alpha, &TransFuncAlphaArea::update_preset);

    lut->update_transfer_function();
    alpha->update_transfer_function();

    setLayout(vlayout);
}

TransFuncWidget::~TransFuncWidget() {
    free(tf_data);
}

void TransFuncWidget::update_preset(int i)
{
    prman->selected = i;

    emit preset_selected(i);
}

void TransFuncWidget::new_rgb_data(float *rgb_data, int len)
{
    /* don't trust len, still not sure what to do with it */
    Q_UNUSED(len);

    for (int i=0; i<tf_len; i++) {
        memcpy(tf_data+i*4, rgb_data+i*3, 3*sizeof(float));
    }

    // for (int i=0; i<tf_len; i++)
    //     printf("%d|", tf_data[i]);
    // printf("\n\n");

    // printf("new rgb data\n");

    emit transfer_function_ready(tf_data, tf_len);
}

void TransFuncWidget::new_alpha_data(float *alpha_data, int len)
{
    /* don't trust len, still not sure what to do with it */
    Q_UNUSED(len);

    for (int i=0; i<tf_len; i++) {
        memcpy(tf_data+i*4+3, alpha_data+i, 1*sizeof(float));
    }

    // for (int i=0; i<tf_len; i++)
    //     printf("%d|", tf_data[i]);
    // printf("\n\n");
    // printf("new alpha data\n");

    emit transfer_function_ready(tf_data, tf_len);
}

void TransFuncWidget::forward_fast_rendering_hint(bool hint)
{
    emit fast_rendering_hint(hint);
}
