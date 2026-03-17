#shader vertex
#version 450 core

layout(location = 0) in vec3 v_Position;

uniform mat4 u_LightViewMat;
uniform mat4 u_Transform;

void main()
{
    gl_Position = u_LightViewMat * u_Transform * vec4(v_Position, 1.0);
}

#shader fragment
#version 450 core

void main()
{
}