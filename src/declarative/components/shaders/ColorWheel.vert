/*
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 * SPDX-FileCopyrightText: 2021-2024 Eike Hein <sho@eikehein.com>
 */

#version 450

layout(set = 0, binding = 0) uniform UBO {
    mat4 qt_Matrix;
};

layout(location = 0) in vec4 qt_Vertex;
layout(location = 1) in vec2 qt_MultiTexCoord0;

layout(location = 0) out vec2 coord;

void main() {
    coord = qt_MultiTexCoord0;
    gl_Position = qt_Matrix * qt_Vertex;
}
