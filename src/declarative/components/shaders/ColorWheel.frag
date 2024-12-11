/*
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 * SPDX-FileCopyrightText: 2021-2024 Eike Hein <sho@eikehein.com>
 */

#version 450

layout(location = 0) in vec2 coord;
layout(location = 0) out vec4 fragColor;

const float TWO_PI = 6.28318530718;

// Convert from HSV to RGB with some cubic smoothing for
// a nicer, more perceptual appearance.
vec3 hsv2rgb(vec3 c) {
    vec3 rgb = clamp(abs(mod(c.x * 6.0 + vec3(0.0, 4.0, 2.0), 6.0) - 3.0) - 1.0, 0.0, 1.0);
    rgb = rgb * rgb * (3.0 - 2.0 * rgb);
    return c.z * mix(vec3(1.0), rgb, c.y);
}

void main() {
    vec2 st = coord;
    vec3 color = vec3(0.0);

    // Convert cartesian to polar coordinates.
    vec2 toCenter = vec2(0.5) - st;
    float angle = atan(toCenter.y, toCenter.x);
    float radius = length(toCenter) * 2.0;

    color = hsv2rgb(vec3((angle / TWO_PI) + 0.5, radius, 1.0));
    fragColor = vec4(color, 1.0);
}
