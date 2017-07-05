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

#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFunctions_3_2_Core>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QMouseEvent>
#include <QMatrix4x4>
#include <QVector2D>
#include <QQuaternion>
#include <QColor>
#include <QTimer>

#include "util.h"

QT_FORWARD_DECLARE_CLASS(QOpenGLShaderProgram)

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_2_Core
{
    Q_OBJECT

public:
    GLWidget(QWidget *parent = 0) : QOpenGLWidget(parent) {}
    GLWidget(InitOptions &opt);
    ~GLWidget();

    QSize minimumSizeHint() const Q_DECL_OVERRIDE;
    QSize sizeHint() const Q_DECL_OVERRIDE;

    const QColor &get_background_color();

    const QColor &get_light_color();
    double get_diffuse_reflectance();
    double get_ambient_reflectance();
    double get_specular_reflectance();

public slots:
    void set_background_color(const QColor &color);

    void set_shading_mode(int mode);
    void set_compositing_mode(int mode);

    void set_light_color (const QColor &color);
    void set_diffuse_reflectance (double kd);
    void set_ambient_reflectance (double ka);
    void set_specular_reflectance (double ks);

    void set_fast_rendering(bool fr);
    void new_transfer_function(float *data, int len);
    void update_timer_timeout();

protected:
    void initializeGL() Q_DECL_OVERRIDE;
    void paintGL() Q_DECL_OVERRIDE;
    void resizeGL(int width, int height) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void wheelEvent(QWheelEvent *event) Q_DECL_OVERRIDE;

private:
    GLuint load_volume_texture(const char *path, GLuint w, GLuint h, GLuint d, unsigned int bit_depth);
    GLuint load_transfer_function(const char *path);
    GLuint load_transfer_function_from_data(float *data, size_t sz);
    void init_target_texture(int w, int h);
    void init_fbo(int w, int h);
    void render_cube(QOpenGLShaderProgram *shader, GLuint cull_face);
    QVector3D arc_ball_vector(QVector2D v);

    InitOptions opt;

    QOpenGLShaderProgram *distance_shader;
    QOpenGLShaderProgram *raycast_shader;

    QMatrix4x4 proj;
    QMatrix4x4 model;
    QMatrix4x4 view;

    GLuint vao;
    GLuint db;
    GLuint fbo;
    GLuint target_texture;

    GLuint volume_texture;
    GLuint transfer_function;

    int cur_width;
    int cur_height;

    int x_angle;
    int y_angle;
    int z_angle;

    bool fast_rendering;
    int nsamples;

    QPoint click_position;

    QQuaternion rotation;
    QVector2D last_mouse_position;
    int mouse_wheel_delta;

    float depth;

    int shading_mode;
    int compositing_mode;
    float background_color[4];
    float light_color[3]; /* no alpha here */
    double ambient_reflectance;
    double diffuse_reflectance;
    double specular_reflectance;

    QTimer *update_timer;
};

#endif
