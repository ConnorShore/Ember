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
    mat4 rotView = mat4(mat3(u_View)); // remove translation from the view matrix
    vec4 clipPos = u_Projection * rotView * vec4(LocalPosition, 1.0);
    gl_Position = clipPos.xyww;
}

#shader fragment
#version 450 core

layout(location = 0) out vec4 OutColor;
layout(location = 1) out vec4 OutEmission;
layout(location = 2) out int OutEntityID;

in vec3 LocalPosition;

uniform samplerCube u_EnvironmentMap;
uniform int u_EntityID;
  
void main()
{
    vec3 envColor = texture(u_EnvironmentMap, LocalPosition).rgb;
    OutColor = vec4(envColor, 1.0);
    
    // Output black for emission so the sky doesn't violently explode when you add Bloom!
    OutEmission = vec4(0.0f, 0.0f, 0.0f, 1.0); 
    
    // Output the invalid ID so clicking the sky selects nothing
    OutEntityID = u_EntityID; 
}