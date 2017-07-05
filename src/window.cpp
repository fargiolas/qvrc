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

#include "glwidget.h"
#include "transfuncwidget.h"
#include "presetmanager.h"
#include "window.h"
#include <QVBoxLayout>
#include <QComboBox>
#include <QPushButton>
#include <QFileDialog>
#include <QDoubleSpinBox>
#include <QSize>
#include <QLabel>
#include <QFormLayout>
#include <QGroupBox>
#include <QColorDialog>

#include "colorbutton.h"


ColorButton::ColorButton(const QColor &color)
{
    setColor(color);
    //setFixedSize(36, 24);
    setIconSize(QSize(24, 12));
}

void ColorButton::setColor(const QColor &color)
{
    pixmap = QPixmap(QSize(24, 12));
    pixmap.fill(color);

    setIcon(QIcon(pixmap));
}

Window::Window(InitOptions &opt)
{
    prman = new PresetManager("presets");

    QWidget *centralwidget = new QWidget();
    GLWidget *glWidget = new GLWidget(opt);

    QGroupBox *tfgroup = new QGroupBox("Transfer Function");
    QVBoxLayout *tflayout = new QVBoxLayout();
    QGroupBox *sgroup = new QGroupBox("Shading and Compositing");
    QVBoxLayout *vlayout = new QVBoxLayout();
    QHBoxLayout *hlayout = new QHBoxLayout();
    hlayout->setMargin(0);
    hlayout->setSpacing(0);
    QHBoxLayout *playout = new QHBoxLayout();
    QFormLayout *flayout = new QFormLayout();
    flayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    preset_combo = new QComboBox();
    for(int i=0; i<prman->presets.size(); i++) {
        // qInfo() << prman->presets[i]->name;
        preset_combo->addItem(prman->presets[i]->filename, i);
        // foreach(TransFuncPoint *pt, prman->presets[i]->alpha_points)
        //     qInfo() << pt->p;
    }

    QPushButton *save = new QPushButton("Save");

    playout->addWidget(preset_combo, 1);
    playout->addWidget(save);

    hlayout->addWidget(glWidget,1);
    hlayout->addLayout(vlayout);

    tf = new TransFuncWidget(prman);
    tflayout->addWidget(tf);
    tfgroup->setLayout(tflayout);
    tflayout->addLayout(playout);
    vlayout->addWidget(tfgroup);
    vlayout->setMargin(2);

    sgroup->setLayout(flayout);
    vlayout->addWidget(sgroup);

    /* general settings */
    QLabel *shading_label = new QLabel("Shading");
    shading_combo = new QComboBox();
    shading_combo->addItem("Blinn-Phong");
    shading_combo->addItem("Blinn-Phong + Edges");
    shading_combo->addItem("Blinn-Phong + Edges + Toon");
    shading_combo->addItem("None");
    flayout->addRow(shading_label, shading_combo);

    QLabel *comp_label = new QLabel("Compositing");
    comp_combo = new QComboBox();
    comp_combo->addItem("Front to back");
    comp_combo->addItem("MIP");
    comp_combo->addItem("M̶I̶D̶A̶ (not working)");
    comp_combo->addItem("debug: ray start");
    comp_combo->addItem("debug: ray end");
    comp_combo->addItem("debug: ray dir");
    flayout->addRow(comp_label, comp_combo);

    QLabel *background_color_label = new QLabel("Background color");
    background_color_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    background_color_button = new ColorButton(glWidget->get_background_color());
    flayout->addRow(background_color_label, background_color_button);

    QLabel *light_color_label = new QLabel("Light color");
    light_color_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    light_color_button = new ColorButton(glWidget->get_light_color());
    flayout->addRow(light_color_label, light_color_button);

    QLabel *ambient_label = new QLabel("Ambient reflectance");
    ambient_spinbox = new QDoubleSpinBox();
    ambient_spinbox->setRange(0.0, 5.0);
    ambient_spinbox->setSingleStep(0.05);
    ambient_spinbox->setValue(glWidget->get_ambient_reflectance());
    flayout->addRow(ambient_label, ambient_spinbox);

    QLabel *diffuse_label = new QLabel("Diffuse reflectance");
    diffuse_spinbox = new QDoubleSpinBox();
    diffuse_spinbox->setRange(0.0, 5.0);
    diffuse_spinbox->setSingleStep(0.05);
    diffuse_spinbox->setValue(glWidget->get_diffuse_reflectance());
    flayout->addRow(diffuse_label, diffuse_spinbox);

    QLabel *specular_label = new QLabel("Specular reflectance");
    specular_spinbox = new QDoubleSpinBox();
    specular_spinbox->setRange(0.0, 5.0);
    specular_spinbox->setSingleStep(0.05);
    specular_spinbox->setValue(glWidget->get_specular_reflectance());
    flayout->addRow(specular_label, specular_spinbox);


    /* stretch to the bottom */
    vlayout->addStretch();

    centralwidget->setLayout(hlayout);
    setCentralWidget(centralwidget);

    /* signals */
    connect(tf, &TransFuncWidget::transfer_function_ready,
            glWidget, &GLWidget::new_transfer_function, Qt::QueuedConnection);
    connect(tf, &TransFuncWidget::fast_rendering_hint,
            glWidget, &GLWidget::set_fast_rendering, Qt::QueuedConnection);

    /* seriously? this is legal syntax?! I hate C++ */
    connect(preset_combo, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &Window::preset_selected);
    connect(save, &QPushButton::clicked,
            this, &Window::save_preset);

    /* dual signals here, as I want to avoid to spawn a color dialog
     * from glwidget */
    connect(background_color_button, &QToolButton::clicked,
            this, &Window::set_background_color);
    connect(this, &Window::background_color_ready,
            glWidget, &GLWidget::set_background_color, Qt::QueuedConnection);
    /* same here */
    connect(light_color_button, &QToolButton::clicked,
            this, &Window::set_light_color);
    connect(this, &Window::light_color_ready,
            glWidget, &GLWidget::set_light_color, Qt::QueuedConnection);


    connect(shading_combo,
            static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            glWidget,
            &GLWidget::set_shading_mode,
            Qt::QueuedConnection);

    connect(comp_combo,
            static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            glWidget,
            &GLWidget::set_compositing_mode,
            Qt::QueuedConnection);

    connect(ambient_spinbox, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            glWidget, &GLWidget::set_ambient_reflectance, Qt::QueuedConnection);

    connect(diffuse_spinbox, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            glWidget, &GLWidget::set_diffuse_reflectance, Qt::QueuedConnection);

    connect(specular_spinbox, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            glWidget, &GLWidget::set_specular_reflectance, Qt::QueuedConnection);



}

void Window::save_preset()
{
    Preset *p = prman->presets[prman->selected];
    QString newname = QFileDialog::getSaveFileName(this, "Save File",
                                                   "presets",
                                                   "JSon files (*.json)");

    /* buggy convoluted logic here, save the old one which
     * has changed, to the new filename, reload the new filename as a
     * new preset, and now the old preset is lost... what do we do
     * about it? fixme fixme fixme... rebuild the combo? */
    if (newname != NULL)
    {
        p->saveJson(newname);
        prman->presets << new Preset(newname);

        QFileInfo finfo(newname);
        if (finfo.fileName() != p->filename)
        {
            preset_combo->addItem(prman->presets[prman->presets.size()-1]->filename,
                           prman->presets.size()-1);
            preset_combo->setCurrentIndex(preset_combo->count()-1);
        }

    }
}

void Window::preset_selected(int i)
{
    int selected = preset_combo->itemData(i).toInt();

    tf->update_preset(selected);
}


void Window::set_background_color()
{
    const QColor color = QColorDialog::getColor(Qt::white, this,
                                                "Select Color");
    if (color.isValid()) {
        // qInfo() << color;
        emit background_color_ready(color);

        background_color_button->setColor(color);
    }
}

void Window::set_light_color()
{
    const QColor color = QColorDialog::getColor(Qt::white, this,
                                                "Select Color");
    if (color.isValid()) {
        // qInfo() << color;
        emit light_color_ready(color);

        light_color_button->setColor(color);
    }
}

void Window::keyReleaseEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Q:
        close();
        break;
    default:
        QWidget::keyReleaseEvent(event);
    }
}
