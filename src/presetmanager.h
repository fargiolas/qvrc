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

#ifndef PRESET_MANAGER_H
#define PRESET_MANAGER_H

#include <QObject>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVector>
#include <QPointF>
#include <QColor>

class TransFuncPoint : public QObject
{
    Q_OBJECT

public:
    TransFuncPoint(QObject *parent = 0) : QObject(parent) {}
    TransFuncPoint(QPointF p, QColor c);
    TransFuncPoint(QPointF p);

    QPointF p;
    QColor c;

    void set_point(QPointF point);
    void set_color(QColor color);

    bool is_selected();
    void set_selected(bool status);

private:
    TransFuncPoint(const TransFuncPoint&);
    TransFuncPoint& operator=(const TransFuncPoint&);
    bool selected;
};

inline bool trans_func_point_compare_x(TransFuncPoint *p1, TransFuncPoint *p2)
{
    return p1->p.x() <= p2->p.x();
}

/* yeah i know it's nothing more than a struct */
/* who cares */
class Preset: public QObject
{
    Q_OBJECT

public:
    Preset(QObject *parent): QObject(parent) { printf("new preset\n"); };
    Preset();
    Preset(const QString &path);
    ~Preset();

    QString filename;
    QString name;

    QColor bgcolor;
    const QVector<TransFuncPoint*> &getLutPoints();
    QVector<TransFuncPoint *> lut_points;
    QVector<TransFuncPoint *> alpha_points;

    void saveJson(const QString &path);
    void loadJson(const QString &path);

    /*
      float offset;
      struct light stuff
    */
private:
    QJsonDocument doc;
};

class PresetManager: public QObject
{
    Q_OBJECT

public:
    PresetManager(QObject *parent = 0): QObject(parent) {}
    PresetManager(const QString &presets_dir);
    QVector<Preset *> presets;

    int selected;

private:
    QDir dir;
};


#endif // PRESET_MANAGER_H
