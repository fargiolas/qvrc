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

#ifndef UTIL_H
#define UTIL_H

#include <QString>

#define MIN(X,Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X,Y) (((X) > (Y)) ? (X) : (Y))
#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

double smoothstep(double edge0, double edge1, double x);
double lerp(double edge0, double edge1, double x);


typedef struct _InitOptions
{
    QString filename;

    unsigned int width;
    unsigned int height;
    unsigned int depth;

    unsigned int bit_depth;

    float xscale;
    float yscale;
    float zscale;
} InitOptions;

#endif /* UTIL_H */
