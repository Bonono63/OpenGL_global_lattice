#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 UV;

out vec3 uv;

uniform float TIME;
uniform vec2 RESOLUTION;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
        TIME;
        RESOLUTION;

        uv = UV;
        gl_Position = projection * view * model * vec4(aPos, 1.0f);
}
