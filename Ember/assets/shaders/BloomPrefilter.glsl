#shader vertex
#version 450 core

layout(location = 0) in vec3 v_Position;
layout(location = 2) in vec2 v_TextureCoord;

out vec2 TexCoord;

void main()
{
    TexCoord = v_TextureCoord;
    gl_Position = vec4(v_Position, 1.0);
}

#shader fragment
#version 450 core

in vec2 TexCoord;
out vec4 OutColor;

uniform sampler2D u_Scene;

uniform float u_Threshold; // The cutoff point (e.g., 1.0)
uniform float u_Knee;      // The smoothing curve (e.g., 0.1 to 0.5)

void main()
{
    vec3 color = texture(u_Scene, TexCoord).rgb;
    
    // Find the brightness (luminance) of the pixel
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    
    // Quadratic Soft Knee Math
    // This creates a smooth ramp instead of a harsh, jagged cutoff
    float knee = u_Threshold * u_Knee + 0.0001; // Epsilon prevents division by zero
    vec3 curve = vec3(u_Threshold - knee, knee * 2.0, 0.25 / knee);
    
    float rq = clamp(brightness - curve.x, 0.0, curve.y);
    rq = (rq * rq) * curve.z;
    
    // Apply the threshold mask back to the original color
    float multiplier = max(rq, brightness - u_Threshold) / max(brightness, 0.0001);
    
    // Any pixel darker than the threshold becomes pure black (vec3(0.0))
    OutColor = vec4(color * multiplier, 1.0);
}