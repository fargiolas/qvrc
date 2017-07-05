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

#include <math.h>
#include <stdint.h>

#define NSAMPLES_HIGH 4000
#define NSAMPLES_LOW  100


/* construct and init defaults */
GLWidget::GLWidget(InitOptions &opt)
{
    this->opt = opt;

    fast_rendering = false;
    nsamples = NSAMPLES_HIGH;

    /* default eye depth */
    depth = 1.1f;
    mouse_wheel_delta = 0;

    /* front to back dvr and blinn phong shading */
    compositing_mode = 0;
    shading_mode = 0;

    /* black background */
    background_color[0] = 0.0;
    background_color[1] = 0.0;
    background_color[2] = 0.0;
    background_color[3] = 1.0;

    /* white spotlight */
    light_color[0] = 1.0;
    light_color[1] = 1.0;
    light_color[2] = 1.0;

    /* material */
    ambient_reflectance = 0.05;
    diffuse_reflectance = 0.3;
    specular_reflectance = 0.45;

    update_timer = new QTimer(this);
    update_timer->setSingleShot(true);
    connect(update_timer, SIGNAL(timeout()), this, SLOT(update_timer_timeout()));
}

/* clean up resources */
GLWidget::~GLWidget()
{
    delete distance_shader;
    delete raycast_shader;
    delete update_timer;

    glDeleteTextures(1, &volume_texture);
    glDeleteTextures(1, &transfer_function);
    glDeleteTextures(1, &target_texture);
    glDeleteRenderbuffers(1, &db);
    glDeleteFramebuffers(1, &fbo);
    glDeleteVertexArrays(1, &vao);
}

QSize GLWidget::minimumSizeHint() const { return QSize(50, 50); }
QSize GLWidget::sizeHint() const { return QSize(600, 600); }

/* reduce sampling rate when the UI asks for fast rendering, usually
 * because either the model is rotating or something like that and we
 * need interactive framerates */
void GLWidget::set_fast_rendering(bool s)
{
    if (s != fast_rendering) {
        fast_rendering = s;
        nsamples = s ? NSAMPLES_LOW : NSAMPLES_HIGH;
        update();
    }
}

void GLWidget::set_compositing_mode(int mode)
{
    compositing_mode = mode;
    update();
}

void GLWidget::set_shading_mode(int mode)
{
    shading_mode = mode;
    update();
}

void GLWidget::set_background_color(const QColor &color)
{
    background_color[0] = color.redF();
    background_color[1] = color.greenF();
    background_color[2] = color.blueF();
    background_color[3] = color.alphaF();

    makeCurrent();
    glClearColor(background_color[0], background_color[1],
                 background_color[2], background_color[3]);

    doneCurrent();

    update();
}

const QColor & GLWidget::get_background_color()
{
    static QColor bgqcolor =  QColor(background_color[0] * 255,
                                     background_color[1] * 255,
                                     background_color[2] * 255,
                                     background_color[3] * 255);

    return bgqcolor;
}

void GLWidget::set_light_color(const QColor &color)
{
    light_color[0] = color.redF();
    light_color[1] = color.greenF();
    light_color[2] = color.blueF();

    update();
}

const QColor & GLWidget::get_light_color()
{
    static QColor bgqcolor =  QColor(light_color[0] * 255,
                                     light_color[1] * 255,
                                     light_color[2] * 255);

    return bgqcolor;
}

void GLWidget::set_ambient_reflectance(double ka)
{
    ambient_reflectance = ka;
    update();
}
double GLWidget::get_ambient_reflectance()
{
    return ambient_reflectance;
}

void GLWidget::set_diffuse_reflectance(double ka)
{
    diffuse_reflectance = ka;
    update();
}
double GLWidget::get_diffuse_reflectance()
{
    return diffuse_reflectance;
}

void GLWidget::set_specular_reflectance(double ka)
{
    specular_reflectance = ka;
    update();
}
double GLWidget::get_specular_reflectance()
{
    return specular_reflectance;
}

void GLWidget::new_transfer_function(float *data, int len)
{
    glDeleteTextures(1, &transfer_function);
    transfer_function = load_transfer_function_from_data(data, len);

    update();
}

// -----------------------------------------------------------------------
//    TEXTURE LOADERS
// -----------------------------------------------------------------------

/* Load 8bit raw luminance data into a 3D texture */
GLuint load_volume_texture_8bit(const char *path, GLuint w, GLuint h, GLuint d)
{
    /* FIXME: duplicated code */
    FILE *f;
    GLuint tex;

    f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "couldn't open: %s\n", path);
        exit(1);
    }

    size_t chunk = sizeof(uint8_t);
    size_t len = w*h*d*chunk;

    uint8_t *volume_data;
    volume_data = (uint8_t *) malloc(len);

    /* read the whole file at once, maybe I should read it chunk by
     * chunk or byte by byte */
    if (fread(volume_data, 1, len, f) != len) {
        fprintf(stderr, "premature eof or reading error: %s\n", path);
        exit(1);
    }

    /* standard texture initialization, nothing fancy */
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_3D, tex);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    /* align to single byte */
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    /* uint8_t -> GL_RED, late OpenGL deprecated luminance texture,
     * you have to use a single channel now */
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RED,
                 w, h, d, 0, GL_RED, GL_UNSIGNED_BYTE, volume_data);

    free(volume_data);

    return tex;
}

/* Load 16bit raw luminance data into a 3D texture

   @shift: amount of padding bits to remove, most medical data comes
   in 16bit textures but only the first 10 or 12 bit actually contain
   any data
*/
GLuint load_volume_texture_16bit(const char *path, GLuint w, GLuint h, GLuint d, uint8_t shift)
{
    /* FIXME: duplicated code */
    FILE *f;
    GLuint tex;

    f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "couldn't open: %s\n", path);
        exit(1);
    }

    size_t chunk = sizeof(uint16_t);
    size_t len = w*h*d*chunk;
    int array_len = w*h*d;

    uint16_t *volume_data;
    volume_data = (uint16_t *) malloc(len);

    if (fread(volume_data, 1, len, f) != len) {
        fprintf(stderr, "premature eof or reading error: %s\n", path);
        exit(1);
    }

    /* assume data is already saturated in the [0, 2^(bit_depth)]
     * range and rescale it to fill 16bit */
    /* maybe we should just rescale [min,max] to [0, 2^16] */
    for (int i=0; i<array_len; i++) {
        volume_data[i] <<= shift;
    }

    /* standard init, nothing fancy */
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_3D, tex);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    /* single byte row alignment */
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    /* uint16_t -> GL_R16, not really sure we need to enforce the
     * 16bit internal format */
    glTexImage3D(GL_TEXTURE_3D, 0, GL_R16,
                 w, h, d, 0, GL_RED, GL_UNSIGNED_SHORT, volume_data);

    free(volume_data);

    return tex;
}

/* 3D texture loader wrapper */
GLuint GLWidget::load_volume_texture(const char *path, GLuint w, GLuint h, GLuint d,
                                     unsigned int bit_depth)
{
    switch (bit_depth) {
    case 8:
        return load_volume_texture_8bit(path, w, h, d);
    case 10: /* not tested */
    case 12:
    case 16:
        return load_volume_texture_16bit(path, w, h, d, 16 - bit_depth);
    default:
        fprintf(stderr, "unsupported bit depth: %d\n", bit_depth);
        exit(1);
    }
}

/* 1D texture loader for transfer function */
GLuint GLWidget::load_transfer_function_from_data(float *data, size_t sz)
{
    /* if no data is given init a default linear ramp across rgb
     * channels and a threshold alpha */
    float *default_tf = NULL;

    if (data == NULL) {
        default_tf = (float *) malloc(4 * sz * sizeof(float));
        for (size_t i = 0; i < sz; i++) {
            for (int j=0; j<3; j++)
                default_tf[i*4 + j] = 0.0;
        }
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_1D, tex);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    /* TODO: explore different transfer function precisions */
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA16F, sz, 0, GL_RGBA, GL_FLOAT,
                 data != NULL ? data : default_tf);

    if (default_tf != NULL)
        free(default_tf);

    return tex;
}

// -----------------------------------------------------------------------
//    BUFFER, TARGETS, ETC
// -----------------------------------------------------------------------

/* target texture for the first rendering pass */
void GLWidget::init_target_texture(int w, int h)
{
    /* check this shit */
    glDeleteTextures(1, &target_texture);
    glGenTextures(1, &target_texture);
    glBindTexture(GL_TEXTURE_2D, target_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    /* try to keep a good precision in the intermediate rendering steps */
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
}

/* framebuffer object for two pass rendering */
void GLWidget::init_fbo(int w, int h)
{
    glDeleteRenderbuffers(1, &db);
    glDeleteFramebuffers(1, &fbo);

    /* render buffer for face culling */
    glGenRenderbuffers(1, &db);
    glBindRenderbuffer(GL_RENDERBUFFER, db);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w, h);

    /* frame buffer for rendering */
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER,
                           GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D,
                           target_texture,
                           0);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                              GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER,
                              db);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "Something wrong with the framebuffer... \n");
        exit(1);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

/* (too) big do it all GL init function */
void GLWidget::initializeGL()
{
    /* QT way of enabling OGL API */
    initializeOpenGLFunctions();

    printf("Renderer: %s\n", glGetString(GL_RENDERER));
    printf("OpenGL version: %s\n", glGetString(GL_VERSION));

    set_fast_rendering(false);

    /* load textures */
    volume_texture = load_volume_texture(opt.filename.toUtf8().data(),
                                         opt.width, opt.height, opt.depth, opt.bit_depth);
    transfer_function = load_transfer_function_from_data(NULL, 256);

    /* init transformation matrices */
    proj.setToIdentity(); // see resizeGL as it's viewport dependent

    /* local to world */
    model.setToIdentity();
    /* I should probably get initial rotation from DICOM orientation data */
    rotation = QQuaternion::fromEulerAngles(0, 0, 0);
    // rotation = QQuaternion();
    model.rotate(rotation);
    /* draw in [0,1] because we want local coordinates easily mapped
     * to colors, translate in [-0.5,0.5] to make camera transforms
     * easier */
    model.translate(-0.5, -0.5, -0.5);

    /* simple view, just look at the center object */
    view.setToIdentity();
    view.lookAt({0,0,depth},{0,0,0},{0,1,0});

    /* Geometry initialization, just a cube in [0,1] */
    GLfloat vertices[24] = {
        0.0, 0.0, 0.0,
        0.0, 0.0, 1.0,
        0.0, 1.0, 0.0,
        0.0, 1.0, 1.0,
        1.0, 0.0, 0.0,
        1.0, 0.0, 1.0,
        1.0, 1.0, 0.0,
        1.0, 1.0, 1.0
    };

    /* triangle ordering, arbitrary? */
    GLuint indices[36] = {
        1,5,7, 7,3,1, 0,2,6,
        6,4,0, 0,1,3, 3,2,0,
        7,5,4, 4,6,7, 2,3,7,
        7,6,2, 1,0,4, 4,5,1
    };

    /* load vertices and indices to gpu */
    /* vertex buffer for vertices */
    GLuint points_vbo = 0;
    glGenBuffers(1, &points_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, points_vbo);
    glBufferData(GL_ARRAY_BUFFER, 24 * sizeof(GLfloat), vertices, GL_STATIC_DRAW);

    /* element buffer for indices */
    GLuint points_ebo = 0;
    glGenBuffers(1, &points_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, points_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 36*sizeof(GLuint), indices, GL_STATIC_DRAW);

    /* bind vertices to a vertex array */
    vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, points_vbo);
    /* map vertex position to the first attrib, this will allow to
     * retrieve the local coordinates in the vertex shader */
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, points_ebo);
    glEnableVertexAttribArray(0);

    /* enable alpha blending */
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /* draw background */
    glClearColor(background_color[0], background_color[1],
                 background_color[2], background_color[3]);

    /* initialize first pass shader */
    /* see shaders source code for details */
    /* this just shades our cube with color mapped to local position */
    distance_shader = new QOpenGLShaderProgram;
    distance_shader->addShaderFromSourceFile(QOpenGLShader::Vertex,
                                             "shaders/firstpass.vert");
    distance_shader->addShaderFromSourceFile(QOpenGLShader::Fragment,
                                             "shaders/firstpass.frag");
    distance_shader->link();

    /* and this is where the volume rendering really happens */
    raycast_shader = new QOpenGLShaderProgram;
    raycast_shader->addShaderFromSourceFile(QOpenGLShader::Vertex,
                                            "shaders/raycast.vert");
    raycast_shader->addShaderFromSourceFile(QOpenGLShader::Fragment,
                                            "shaders/raycast.frag");
    raycast_shader->link();
}

/* draw our geometry with the proper culling */
void GLWidget::render_cube(QOpenGLShaderProgram *shader, GLuint cull_face)
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* scene transform happens in the shaders with modern GL */
    GLint proj_loc = shader->uniformLocation("projection");
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, (GLfloat *) proj.data());
    GLint model_loc = shader->uniformLocation("model");
    glUniformMatrix4fv(model_loc, 1, GL_FALSE, (GLfloat *) model.data());
    GLint view_loc = shader->uniformLocation("view");
    glUniformMatrix4fv(view_loc, 1, GL_FALSE, (GLfloat *) view.data());

    glCullFace(cull_face);
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
}

void GLWidget::paintGL()
{
    /* backup current fbo as Qt might be doing something there */
    GLint savedfbo;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &savedfbo);

    /* init model matrix */
    model.setToIdentity();
    model.rotate(rotation);
    model.scale(opt.xscale, opt.yscale, opt.zscale);
    model.translate(-0.5, -0.5, -0.5);

    /* map framebuffer object for offscreen rendering */
    glEnable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* first pass: draw a colored cube with front face culling */
    /* the colors will be the coordinates of the back face we can use
     * as the end points for our raycasting integral */
    distance_shader->bind();
    render_cube(distance_shader, GL_FRONT);
    distance_shader->release();


    /* restore previous framebuffer, we'll render to screen now */
    glBindFramebuffer(GL_FRAMEBUFFER, savedfbo > 0 ? savedfbo : 0);
    raycast_shader->bind();

    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* load for the raycasting fragment shader */
    /* first pass target, now full with position data */
    GLint tex_loc = raycast_shader->uniformLocation("backtex");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, target_texture);
    glUniform1i(tex_loc, 0);
    /* volume data */
    tex_loc = raycast_shader->uniformLocation("voltex");
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, volume_texture);
    glUniform1i(tex_loc, 1);
    /* transfer function */
    tex_loc = raycast_shader->uniformLocation("tftex");
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_1D, transfer_function);
    glUniform1i(tex_loc, 2);

    /* viewport size, needed to get normalized texture coordinates */
    GLint screen_width_loc = raycast_shader->uniformLocation("screen_width");
    GLint screen_height_loc = raycast_shader->uniformLocation("screen_height");
    glUniform1f(screen_width_loc, (GLfloat) cur_width);
    glUniform1f(screen_height_loc, (GLfloat) cur_height);

    /* viewport size, needed to get normalized texture coordinates */
    GLint scale_loc = raycast_shader->uniformLocation("scale");

    GLfloat scale[3];
    scale[0] = opt.xscale;
    scale[1] = opt.yscale;
    scale[2] = opt.zscale;
    glUniform3fv(scale_loc, 1, scale);


    /* how many samples we want in our ray integral */
    GLint nsamples_loc = raycast_shader->uniformLocation("nsamples");
    glUniform1f(nsamples_loc, (GLfloat) nsamples);

    /* compositing mode (front to back, mip, mida), mida doesn't really work */
    GLuint compositing_mode_loc = raycast_shader->uniformLocation("compositing_mode");
    glUniform1i(compositing_mode_loc, (GLint) compositing_mode);

    /* shading mode (blinn phong, toon, none) */
    GLuint shading_mode_loc = raycast_shader->uniformLocation("shading_mode");
    glUniform1i(shading_mode_loc, (GLint) shading_mode);

    /* shading parameters */
    GLint light_color_loc = raycast_shader->uniformLocation("light_color");
    glUniform3fv(light_color_loc, 1, (GLfloat *) light_color);
    GLint ka_loc = raycast_shader->uniformLocation("ka");
    glUniform1f(ka_loc, ambient_reflectance);
    GLint kd_loc = raycast_shader->uniformLocation("kd");
    glUniform1f(kd_loc, diffuse_reflectance);
    GLint ks_loc = raycast_shader->uniformLocation("ks");
    glUniform1f(ks_loc, specular_reflectance);


    /* second pass: render the cube again with backface culling, now
     * the color data stores the starting position for our raycasting
     * computation */
    render_cube(raycast_shader, GL_BACK);
    raycast_shader->release();
}

/* resize callback */
void GLWidget::resizeGL(int w, int h)
{
    /* update viewport dependent stuff */
    cur_width = w;
    cur_height = h;
    /* target texture and fbo */
    init_target_texture(w, h);
    init_fbo(w, h);
    /* projection mapping */
    proj.setToIdentity();
    proj.perspective(67.0f, GLfloat(w) / h, 0.001f, 5.0f);
    // proj.ortho(-0.1, 0.1, -0.1, 0.1, 0.001f, 5.0f);
}

void GLWidget::mousePressEvent(QMouseEvent *event)
{
    last_mouse_position = QVector2D(event->localPos());
    set_fast_rendering(true);
}

void GLWidget::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    set_fast_rendering(false);
}

/* convert viewport to ball coordinates */
QVector3D GLWidget::arc_ball_vector(QVector2D v)
{
    /* normalize in [-1, 1] (view space) */
    QVector2D norm_v = 2.0f * v / QVector2D(cur_width, cur_height) - QVector2D(1.0, 1.0);

    /* viewport y axis is top to bottom, view is cartesian */
    QVector3D P = { norm_v.x(), -norm_v.y(), 0 };

    /* return a point on a sphere centered at 0 with radius 1 equal to
     * the viewport radius */
    float Psq = P.lengthSquared();
    if (Psq <= 1)
        P.setZ(sqrt(1.0 - Psq));
    else
        P.normalize();

    return P;
}

void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton))
        return;

    QVector2D cur_mouse_position = QVector2D(event->localPos());

    /* simplified arcball, or something like that, only works for view
     * centered objects... enough for our simple mouse navigation */
    QVector3D axis;
    QVector3D va = arc_ball_vector(cur_mouse_position);
    QVector3D vb = arc_ball_vector(last_mouse_position);

    /* angle is dot product between the two vectors on the sphere,
     * axis is the normal vector */
    float angle = acos(QVector3D::dotProduct(va,vb));
    angle *= 180. / M_PI;

    axis = QVector3D::crossProduct(vb, va);

    /* update rotation matrix */
    rotation = QQuaternion::fromAxisAndAngle(axis, angle) * rotation;

    last_mouse_position = cur_mouse_position;
    update();
}

/* zoom in, zoom out with mouse wheel */
void GLWidget::wheelEvent(QWheelEvent *event)
{
    if (event->orientation() != Qt::Vertical)
        return;

    if ((event->delta() * mouse_wheel_delta) < 0)
        mouse_wheel_delta = 0;

    mouse_wheel_delta += event->delta();

    if (mouse_wheel_delta >= 120) {
        depth -= 0.05;
        mouse_wheel_delta = 0;
    } else if (mouse_wheel_delta <= -120) {
        depth += 0.05;
        mouse_wheel_delta = 0;
    }


    /* set fast rendering and launch a timer to get back to normal
     * after a while, this way you can zoom in and out with fast
     * rendering and get back to quality rendering when the
     * interaction ends, I could probably use this trick elsewhere */
    update_timer->start(250);
    view.setToIdentity();
    view.lookAt({0,0,depth},{0,0,0},{0,1,0});

    set_fast_rendering(true);
    update(); /* extra update here... */
}

void GLWidget::update_timer_timeout()
{
    set_fast_rendering(false);
}
