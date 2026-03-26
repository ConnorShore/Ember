#shader vertex
#version 450 core

layout(location = 0) in vec3 v_Position;

uniform mat4 u_InverseView;
uniform mat4 u_InverseProjection;

out vec3 NearPoint;
out vec3 FarPoint;

vec3 UnprojectPoint(vec3 point)
{
    vec4 unprojectedPoint = u_InverseView * u_InverseProjection * vec4(point, 1.0);
    return unprojectedPoint.xyz / unprojectedPoint.w;
}

void main()
{
    NearPoint = UnprojectPoint(vec3(v_Position.xy, -1.0)); // Near plane
    FarPoint = UnprojectPoint(vec3(v_Position.xy, 1.0));   // Far plane

    gl_Position = vec4(v_Position.x, v_Position.y, 0.0, 1.0);
}

#shader fragment
#version 450 core

layout(location = 0) out vec4 OutColor;
layout(location = 1) out vec4 BrightColor;
layout(location = 2) out int EntityID;

in vec3 NearPoint;
in vec3 FarPoint;

layout(std140, binding = 0) uniform CameraData
{
    mat4 u_ViewProjection;
};

uniform vec3 u_CameraPos;

float drawGrid(vec2 fragCoord, float scale) 
{
    vec2 coord = fragCoord * scale;
    vec2 derivative = fwidth(coord);
    
    // fract() creates a repeating 0 to 1 gradient. 
    // We shape it into a perfectly 1-pixel wide line using the hardware derivative
    vec2 grid = abs(fract(coord - 0.5) - 0.5) / derivative;
    
    float line = min(grid.x, grid.y);
    return 1.0 - min(line, 1.0);
}

void main()
{
    // Find where the ray hit y=0, need to re-order this equation to solve for t:
    // R(t) = NearPoint + t * (FarPoint - NearPoint)
    float t = -NearPoint.y / (FarPoint.y - NearPoint.y);
    if (t < 0.0) 
    {
        // If t < 0, the ray didin't hit the floor (pointing up)
        discard;
    }
    
    // Find exactly where the in 3d space the ray intersected with the floor
    // (i.e. plug t into:  R(t) = NearPoint + t * (FarPoint - NearPoint)
    vec3 fragPos3D = NearPoint + t * (FarPoint - NearPoint);
    
    // Determine grid color based on the position of the fragment
    // Draw two overlapping grids
    // Thin lines every 1 unit, Thick lines every 10 units (scale 0.1)
    float thinLine = drawGrid(fragPos3D.xz, 1.0);
    float thickLine = drawGrid(fragPos3D.xz, 0.1);
    
    // Combine them into a base color (Thick lines are brighter)
    vec3 gridColor = vec3(0.5) * thinLine + vec3(0.8) * thickLine;
    float alpha = max(thinLine * 0.3, thickLine * 0.8);
    
    // Draw the main World Axes (X axis = Red, Z axis = Blue)
    // We use the same fwidth trick to make them exactly 1.5 pixels wide!
    float axisX = 1.0 - min(abs(fragPos3D.z) / (fwidth(fragPos3D.z) * 1.5), 1.0);
    float axisZ = 1.0 - min(abs(fragPos3D.x) / (fwidth(fragPos3D.x) * 1.5), 1.0);
    
    if (axisX > 0.0) {
        gridColor = mix(gridColor, vec3(1.0, 0.2, 0.2), axisX);
        alpha = max(alpha, axisX);
    }
    if (axisZ > 0.0) {
        gridColor = mix(gridColor, vec3(0.2, 0.4, 1.0), axisZ);
        alpha = max(alpha, axisZ);
    }
    
    // Fade out in the distance so it seamlessly blends into the background
    float fadeDistance = 50.0; // Adjust this to push the horizon further back!
    float fading = max(0.0, 1.0 - (length(fragPos3D - u_CameraPos) / fadeDistance));
    alpha *= fading;

    // Discard fully transparent pixels to save GPU fill-rate
    if (alpha < 0.01) {
        discard;
    }

    vec4 finalGridColor = vec4(gridColor, alpha);

    // Manually calculate and set depth (since depth is 0 for the screenQuad)
    // so we need to trick the GPU to thinking its at a specific depth
    vec4 clipSpacePos = u_ViewProjection * vec4(fragPos3D, 1.0);
    float ndcDepth = clipSpacePos.z / clipSpacePos.w;
    
    // Map from [-1, 1] to [0, 1] for the OpenGL depth buffer
    gl_FragDepth = (((gl_DepthRange.diff) * ndcDepth) + gl_DepthRange.near + gl_DepthRange.far) / 2.0;

    // Write output buffers
    OutColor = finalGridColor;
    BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
    EntityID = INVALID_ENTITY_ID;
}