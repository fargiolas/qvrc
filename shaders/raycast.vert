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

#version 330

layout (location = 0) in vec3 vertex_position;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

uniform vec3 origin;

out vec3 ray_in;
out mat3 normalmatrix;

void main()
{
    gl_Position = projection * view * model * vec4(vertex_position, 1.0);
    ray_in = vertex_position;

    /* transform local normals to world space */
    normalmatrix = mat3(transpose(inverse(model)));
    /* in eye space it would be */
    /* normalmatrix = mat3(transpose(inverse(view * model))); */
}
