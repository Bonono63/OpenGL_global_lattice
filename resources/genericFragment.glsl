#version 460 core
in vec3 uv;
out vec4 FragColor;

uniform sampler3D TEXTURE;

uniform float TIME;
uniform vec2 RESOLUTION;

void main()
{
        TIME;
        RESOLUTION;
        
        vec4 color = texture(TEXTURE, uv);
        if (color.a != 1.0)
                discard;
        FragColor = color;
}
