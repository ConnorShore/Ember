#shader vertex
#version 450 core

out vec2 TexCoord;

layout(location = 0) in vec3 v_Position;
layout(location = 2) in vec2 v_TextureCoord;

void main()
{
    TexCoord = v_TextureCoord;
    gl_Position = vec4(v_Position, 1.0);
}

#shader fragment
#version 450 core

layout(location = 0) out vec4 OutColor;
layout(location = 1) out vec4 BrightColor;

in vec2 TexCoord;

uniform sampler2D u_Scene;
uniform sampler2D u_BrightScene;

// Split ID Buffers
uniform isampler2D u_OpaqueIDBuffer;
uniform isampler2D u_ForwardIDBuffer;

uniform sampler2D u_DepthBuffer;

uniform int u_SelectedEntityID;
uniform vec3 u_OutlineColor;
uniform float u_OutlineThickness;

// Helper function to unify the ID buffers
int GetEntityID(vec2 uv)
{
    int forwardID = texture(u_ForwardIDBuffer, uv).r;
    if (forwardID != INVALID_ENTITY_ID)
    {
        return forwardID;
    }
    return texture(u_OpaqueIDBuffer, uv).r;
}

void main()
{           
    vec3 scene = texture(u_Scene, TexCoord).rgb;
    vec3 brightScene = texture(u_BrightScene, TexCoord).rgb;

    // Use our new unified ID function!
    int centerID = GetEntityID(TexCoord);

    // If the center pixel IS the selected entity, we just draw the normal scene.
    if (centerID == u_SelectedEntityID)
    {
        OutColor = vec4(scene, 1.0);
        BrightColor = vec4(brightScene, 1.0);
        return;
    }

    // Get the size of a single texel. We can just use the Opaque buffer size as they match.
    vec2 tex_offset = 1.0 / textureSize(u_OpaqueIDBuffer, 0); 
    
    // Make outline u_OutlineThickness pixels wide
    tex_offset *= u_OutlineThickness;

    // Sample the neighbors using our unified ID getter
    int up    = GetEntityID(TexCoord + vec2(0.0, tex_offset.y));
    int down  = GetEntityID(TexCoord - vec2(0.0, tex_offset.y));
    int right = GetEntityID(TexCoord + vec2(tex_offset.x, 0.0));
    int left  = GetEntityID(TexCoord - vec2(tex_offset.x, 0.0));

    // Need to check depth here as well to prevent outlines showing for closer objects
    bool isOuterEdge = false;
    float centerDepth = texture(u_DepthBuffer, TexCoord).r;

    // If the center is not the entity, but a neighbor IS, we are on the edge!
    if(up == u_SelectedEntityID)
    {
        float neighborDepth = texture(u_DepthBuffer, TexCoord + vec2(0.0, tex_offset.y)).r;
        if (centerDepth >= neighborDepth) isOuterEdge = true;
    }
    
    if(down == u_SelectedEntityID)
    {
        float neighborDepth = texture(u_DepthBuffer, TexCoord - vec2(0.0, tex_offset.y)).r;
        if (centerDepth >= neighborDepth) isOuterEdge = true;
    }
    
    if(right == u_SelectedEntityID)
    {
        float neighborDepth = texture(u_DepthBuffer, TexCoord + vec2(tex_offset.x, 0.0)).r;
        if (centerDepth >= neighborDepth) isOuterEdge = true;
    }
    
    if(left == u_SelectedEntityID)
    {
        float neighborDepth = texture(u_DepthBuffer, TexCoord - vec2(tex_offset.x, 0.0)).r;
        if (centerDepth >= neighborDepth) isOuterEdge = true;
    }

    if(isOuterEdge)
    {
        OutColor = vec4(u_OutlineColor, 1.0);
        BrightColor = vec4(brightScene, 1.0);
        return;
    }

    // Draw scene normally if not edge
    OutColor = vec4(scene, 1.0);
    BrightColor = vec4(brightScene, 1.0);
}