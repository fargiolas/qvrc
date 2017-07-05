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

/* parameters */
in vec3 ray_in;
in mat3 normalmatrix;
out vec4 outcolor;

/* uniforms */
uniform sampler2D backtex;
uniform sampler3D voltex;
uniform sampler1D tftex;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

uniform float screen_width;
uniform float screen_height;

uniform vec3 scale;

uniform vec3 light_color;
uniform float ka;
uniform float kd;
uniform float ks;

uniform float nsamples;
uniform int compositing_mode;
uniform int shading_mode;

/* consts */
const float DELTA = 0.005;
const float SHADING_THRES = 0.10;

/* globals */
float stepsize;



/* TODO: check randomness, explore noise texture alternative */
float rand() {
    /* the internet **really** likes this one, still no source to be
     * found, probably Rey 1998, cited by TestU01 but nothing
     * downloadable */
    return fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

/* calculate voxel gradient using central differences approximation */
/*  f' = ( f(x+h)-f(x-h) ) / 2*h */
vec3 gradient_central_diff(sampler3D sampler, vec3 pos, float delta)
{
    vec3 fl, fh;

    fl.x = texture(sampler, pos - vec3(delta*scale.x, 0.0, 0.0)).r;
    fl.y = texture(sampler, pos - vec3(0.0, delta*scale.y, 0.0)).r;
    fl.z = texture(sampler, pos - vec3(0.0, 0.0, delta*scale.z)).r;

    fh.x = texture(sampler, pos + vec3(delta*scale.x, 0.0, 0.0)).r;
    fh.y = texture(sampler, pos + vec3(0.0, delta*scale.y, 0.0)).r;
    fh.z = texture(sampler, pos + vec3(0.0, 0.0, delta*scale.z)).r;

    /* well we should really divide it by 2h here, but we'll use it
     * for the normals anyway, it's ok to just normalize it here */
    return normalize(fh - fl);
}

/* Standard compositing, alpha blend next pixel with the previous */
vec4 composite_front_to_back(vec4 incolor, vec4 outcolor)
{
    /* opacity correction for varying stepsize */
    /* Engel et. al.: "Real-Time Volume Graphics" - § 1.4.3 and 9.1.3 */
    incolor.a = 1.0 - pow(1.0 - incolor.a, stepsize*200.0);

    /* associate color and opacity (Blinn 1994) */
    incolor.rgb *= incolor.a;

    outcolor += (1.0 - outcolor.a) * incolor;

    return outcolor;
}

/* Maximum Intensity Projection, save only the brightest/most opaque
 * voxel in the current direction */
vec4 composite_mip(vec4 incolor, vec4 outcolor)
{
    if (incolor.a > outcolor.a)
        return incolor;
    else
        return outcolor;
}


/* not really sure this works as expected */
/* Bruckner 2009, Instant Volume Visualization using Maximum Intensity
 * Difference Accumulation */
/* the code seems right, but I'm not so sure about f_max_i, he talks
 * about the "current maximum along the direction" */
vec4 composite_mida(vec4 incolor, vec4 outcolor, float f_P_i, inout float f_max_i)
{
    /* opacity correction for varying stepsize */
    /* Engel et. al.: "Real-Time Volume Graphics" - § 1.4.3 and 9.1.3 */
    incolor.a = 1.0 - pow(1.0 - incolor.a, stepsize*200.0);

    /* associate color and opacity (Blinn 1994) */
    incolor.rgb *= incolor.a;

    float delta_i = 0.0;

    if (f_P_i > f_max_i) {
        delta_i = f_P_i - f_max_i;
        f_max_i = f_P_i;
    }

    float beta_i = 1.0 - delta_i;

    outcolor = beta_i * outcolor + (1.0 - beta_i * outcolor.a) * incolor;

    return outcolor;
}

vec3 blinn_phong_shading(vec3 N, vec3 V, vec3 L)
{
    vec3 ambient_light_color = vec3(0.3, 0.3, 0.3);
    float shininess = 100.0;

    vec3 H = normalize(L + V);

    float diffuse_factor = max(0, dot(L, N));
    float specular_factor = pow(max(dot(H, N), 0), shininess);

    vec3 ambient = ka * ambient_light_color;
    vec3 diffuse = kd * light_color * diffuse_factor;
    vec3 specular = ks * light_color * specular_factor;

    return ambient + diffuse + specular;
}

vec3 blinn_phong_toon_shading(vec3 N, vec3 V, vec3 L)
{
    vec3 ambient_light_color = vec3(0.3, 0.3, 0.3);
    float shininess = 100.0;

    vec3 H = normalize(L + V);

    const float A = 0.1;
    const float B = 0.3;
    const float C = 0.6;
    const float D = 1.0;

    float diffuse_factor = max(0, dot(L, N));
    float specular_factor = pow(max(dot(H, N), 0), shininess);

    /* posterize diffusion */
    if (diffuse_factor < A) diffuse_factor = 0.0;
    else if (diffuse_factor < B) diffuse_factor = B;
    else if (diffuse_factor < C) diffuse_factor = C;
    else diffuse_factor = D;

    /* harsh cut reflections */
    specular_factor = step(0.2, specular_factor);

    vec3 ambient = ka * ambient_light_color;
    vec3 diffuse = kd * light_color * diffuse_factor;
    vec3 specular = ks * light_color * specular_factor;

    return ambient + diffuse + specular;
}

void main()
{
    /* screen to normalized viewport coordinates */
    vec2 norm_coord = gl_FragCoord.st / vec2(screen_width, screen_height);
    /* start position is saved into the cube colors */
    vec3 start = ray_in;
    /* retrieve end position from first pass results */
    vec3 end = texture(backtex, norm_coord).xyz;

    vec3 direction = end - start;
    float len = length(direction);
    stepsize = len / nsamples;
    direction = normalize(direction);
    vec3 delta = direction * stepsize;

    vec3 pos = start;

    /* dithering of the starting position */
    /* hides the woodgrain/staircase aliasing cause by undersampling
     * the volume while marching the ray */
    /* does nothing (little?) to prevent transfer function and shading
     * aliasing */
    pos  = pos + delta * rand();

    vec3 eyePosition = view[3].xyz;
    vec3 lightPosition = eyePosition - vec3(0, 0, 4);

    float intensity;
    float f_max_i = 0; /* for mida */

    outcolor = vec4(0.0);
    vec4 color = vec4(0.0);

    /* debugging modes */
    if (compositing_mode == 3) {
        outcolor = vec4(start, 1.0);
        return;
    } else if (compositing_mode == 4) {
        outcolor = vec4(end, 1.0);
        return;
    } else if (compositing_mode == 5) {
        outcolor = vec4(abs(end - start), length(end - start));
        return;
    }


    /* marching loop */
    for(int i = 0; i < nsamples && len > 0; i++, pos+=delta, len-=stepsize) {
        /* sample intensity from the 3D texture */
        intensity = texture(voltex, pos).r;
        /* map intensity to transfer function LUT */
        color = texture(tftex, intensity);


        /* shading_mode
         *   0: Blinn Phong shading
         *   1: Blinn Phong with edge enhancement
         *   2: Blinn Phong with toon shading
         *   3: No shading
         */
        if ((color.a > SHADING_THRES) &&
            (shading_mode != 3) &&
            (nsamples > 500)) {
            /* everything in world space */
            vec3 N = gradient_central_diff(voltex, pos, DELTA);

            vec3 pos_world = vec3(model * vec4(pos, 1.0));

            N = normalize(vec3(normalmatrix * N));

            vec3 L = normalize(lightPosition - pos_world);
            vec3 V = normalize(eyePosition - pos_world);

            if (shading_mode == 0) {
                color.rgb += blinn_phong_shading(N, V, L);
            } else {
                if (shading_mode == 1)
                    color.rgb += blinn_phong_shading(N, V, L);
                else if (shading_mode == 2)
                    color.rgb += blinn_phong_toon_shading(N, V, L);

                /* enhance edges when the gradient is almost
                 * perpendicular to the viewing direction */
                float dv = dot(V, N);
                float ev = pow(1.0 - abs(dv), 0.3);

                float et = 0.1;

                if (ev >= et)
                    // color.rgb = vec3(0);
                    color.rgb = mix(color.rgb, vec3(0), pow((ev-et)/(1. - et), 6));
            }
        }

        /* does this affect performance? */
        /* in the old days there was no real branching, but it
         * seems we're past that nowawadays */
        if (compositing_mode == 0)
            outcolor = composite_front_to_back(color, outcolor);
        else if (compositing_mode == 1)
            outcolor = composite_mip(color, outcolor);
        else
            outcolor = composite_mida(color, outcolor, intensity, f_max_i);

        /* early ray termination */
        if (outcolor.a > 0.95) {
            break;
        }
    }
}
