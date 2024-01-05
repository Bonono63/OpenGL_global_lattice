#version 460 core
in vec3 vertexColor;
in vec2 uv;
out vec4 FragColor;

uniform sampler2D textures;

uniform float TIME;
uniform vec2 RESOLUTION;

vec3 palette( float t ) {
    vec3 a = vec3(0.5, 0.5, 0.5);
    vec3 b = vec3(0.5, 0.5, 0.5);
    vec3 c = vec3(1.0, 1.0, 1.0);
    vec3 d = vec3(0.263,0.416,0.557);

    return a + b*cos( 6.28318*(c*t+d) );
}

void main()
{
        FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
}
