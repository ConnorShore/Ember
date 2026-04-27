#shader vertex
#version 450 core
layout(location = 0) in vec3 v_Position;
layout(location = 2) in vec2 v_TextureCoord;
out vec2 TexCoord;
void main() {
    TexCoord = v_TextureCoord;
    gl_Position = vec4(v_Position, 1.0);
}

#shader fragment
#version 450 core

in vec2 TexCoord;

out vec4 OutColor;

uniform sampler2D u_Scene;
uniform sampler2D u_Depth;

uniform float u_FogDensity;
uniform vec3  u_FogColor;
uniform float u_FogStart;

uniform float u_NearClip; 
uniform float u_FarClip;  

float LinearizeDepth(float depth) 
{
    float z = depth * 2.0 - 1.0; 
    return (2.0 * u_NearClip * u_FarClip) / (u_FarClip + u_NearClip - z * (u_FarClip - u_NearClip));
}

void main() 
{
    vec3 color = texture(u_Scene, TexCoord).rgb;
    float depth = texture(u_Depth, TexCoord).r;

    // FIX 1: Safer skybox check to prevent glitches
    if (depth > 0.9999) {
        OutColor = vec4(color, 1.0);
        return;
    }

    float distance = LinearizeDepth(depth);

    // FIX 2: Push the fog back!
    // Subtract the start distance. If it's negative, clamp it to 0.0.
    float fogDistance = max(distance - u_FogStart, 0.0);

    // Calculate Fog
    float fogFactor = exp(-pow(u_FogDensity * fogDistance, 2.0));
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    OutColor = vec4(mix(u_FogColor, color, fogFactor), 1.0);
}