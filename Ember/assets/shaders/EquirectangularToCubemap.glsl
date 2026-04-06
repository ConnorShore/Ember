#shader vertex
#version 450 core

out vec3 LocalPosition;

layout(location = 0) in vec3 v_Position;
layout(location = 2) in vec2 v_TextureCoord;

uniform mat4 u_Projection;
uniform mat4 u_View;

void main()
{
    LocalPosition = v_Position;
    gl_Position = u_Projection * u_View * vec4(LocalPosition, 1.0);
}

#shader fragment
#version 450 core

out vec4 OutColor;

in vec3 LocalPosition;

uniform sampler2D u_EquirectangularMap;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main()
{
    vec2 uv = SampleSphericalMap(normalize(LocalPosition));
    vec3 color = texture(u_EquirectangularMap, uv).rgb;
    
    OutColor = vec4(color, 1.0);
} 
