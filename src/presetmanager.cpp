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

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QColor>
#include <QDebug>
#include <QDir>

#include "presetmanager.h"

Preset::Preset()
{
    filename = QString("default.json");
    name = QString("default");

    bgcolor = QColor(0, 0, 0);
    lut_points << new TransFuncPoint(QPointF(0.0, 0.5), QColor(0, 0, 0));
    lut_points << new TransFuncPoint(QPointF(1.0, 0.5), QColor(255, 255, 255));
    alpha_points << new TransFuncPoint(QPointF(0.0, 1.0));
    alpha_points << new TransFuncPoint(QPointF(0.4, 1.0));
    alpha_points << new TransFuncPoint(QPointF(0.401, 0.0));
    alpha_points << new TransFuncPoint(QPointF(1.0, 0.0));
}

Preset::Preset(const QString &path)
{
    loadJson(path);
}

Preset::~Preset() {}

const QVector<TransFuncPoint*> & Preset::getLutPoints()
{
    return lut_points;
}

void Preset::saveJson(const QString &path)
{
    QJsonObject preset;

    QJsonArray alpha_array;
    for (int i=0; i<alpha_points.size(); i++) {
        QJsonObject o;
        o = QJsonObject({
                    { "pos", QJsonValue::fromVariant(alpha_points[i]->p.x()) },
                    { "alpha", QJsonValue::fromVariant(alpha_points[i]->p.y()) }
                });
        alpha_array.push_back(o);
        // qInfo() << "alpha: " << alpha_points[i]->p;
    }

    preset.insert("alpha_points", alpha_array);

    QJsonArray lut_array;
    for (int i=0; i<lut_points.size(); i++) {
        QJsonObject o;
        o = QJsonObject({
                { "pos", QJsonValue::fromVariant(lut_points[i]->p.x()) },
                { "color", QJsonValue::fromVariant(lut_points[i]->c) }});
        lut_array.push_back(o);
        // qInfo() << "lut: " << lut_points[i]->p << lut_points[i]->c;
    }

    preset.insert("lut_points", lut_array);

    preset.insert("name", QJsonValue::fromVariant(name));

    doc = QJsonDocument(preset);

    QFile outfile(path);

    if (!outfile.open(QIODevice::WriteOnly)) {
        qWarning("couldn't open file for writing"); // I'm an error, handle me!
        return;
    }

    outfile.write(doc.toJson());
    outfile.close();
}

void Preset::loadJson(const QString &path)
{
    QFile infile(path);
    if (!infile.open(QIODevice::ReadOnly)) {
        qWarning("couldn't open file for reading"); // I'm an error, handle me!
        return;
    }

    QFileInfo finfo(infile.fileName());
    filename = finfo.fileName();

    // qInfo() << "LOAD JSON: " << filename;

    QByteArray data = infile.readAll();
    infile.close();

    doc = QJsonDocument::fromJson(data);

    // qInfo() << doc.toJson().data();

    QJsonObject obj = doc.object();
    name = obj["name"].toString();

    QJsonArray lut_array = obj["lut_points"].toArray();
    for(int i=0; i<lut_array.size(); i++) {
        QJsonObject point = lut_array[i].toObject();
        double pos = point["pos"].toDouble();
        QVariant v = point["color"].toVariant();
        QColor color = v.value<QColor>();

        lut_points << new TransFuncPoint(QPointF(pos, 0.5), color);
        // qInfo() << "lut: " << lut_points[i]->p << lut_points[i]->c;
    }
    qSort(lut_points.begin(), lut_points.end(), trans_func_point_compare_x);

    QJsonArray alpha_array = obj["alpha_points"].toArray();
    for(int i=0; i<alpha_array.size(); i++) {
        QJsonObject point = alpha_array[i].toObject();
        double pos = point["pos"].toDouble();
        double alpha = point["alpha"].toDouble();

        alpha_points << new TransFuncPoint(QPointF(pos, alpha));
        // qInfo() << "alpha: " << alpha_points[i]->p;
    }
    qSort(alpha_points.begin(), alpha_points.end(), trans_func_point_compare_x);
}

PresetManager::PresetManager(const QString &presets_dir)
{
    dir = QDir(presets_dir);
    QStringList globs;
    globs << "*.json";
    dir.setNameFilters(globs);
    QStringList filenames = dir.entryList(QDir::Files);

    if (filenames.size() < 1)
        presets << new Preset();

    foreach (QString file, filenames) {
        // qInfo() << file;

        presets << new Preset(QString("presets") + "/" + file);
    }

    selected = 0;
}


////////////////////////////////////////////////////////////////////////////////
/* color + point utility class */
TransFuncPoint::TransFuncPoint(QPointF point) {
    this->p = point;
    selected = false;
}

TransFuncPoint::TransFuncPoint(QPointF point, QColor color) {
    this->p = point;
    this->c = color;
    selected = false;
}

bool TransFuncPoint::is_selected() {
    return selected;
}

void TransFuncPoint::set_selected(bool status) {
    selected = status;
}

void TransFuncPoint::set_point(QPointF point) {
    this->p = point;
}

void TransFuncPoint::set_color(QColor color) {
    this->c = color;
}
