#version 450 core

layout(location = 0) in vec2 in_position;


layout(location = 0) out vec2 out_position;

void main() {
    out_position = in_position;
    gl_Position = vec4(in_position, 0.0, 1.0);
}