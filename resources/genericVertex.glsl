#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 VertexColor;
layout (location = 2) in vec2 UV;

out vec3 vertexColor;
out vec2 uv;

void main()
{   
        vertexColor = VertexColor;
        uv = UV;
        gl_Position = vec4(aPos, 1.0f);
}
