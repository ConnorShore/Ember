#shader vertex
#version 450 core

layout(location = 0) in vec3 v_Position;
layout(location = 5) in uvec4 v_BoneIDs;
layout(location = 6) in vec4 v_BoneWeights;

uniform mat4 u_LightViewMat;
uniform mat4 u_Transform;

uniform mat4 u_BoneMatrices[MAX_BONES];

void main()
{
    mat4 boneTransform = u_BoneMatrices[v_BoneIDs[0]] * v_BoneWeights[0];
    boneTransform     += u_BoneMatrices[v_BoneIDs[1]] * v_BoneWeights[1];
    boneTransform     += u_BoneMatrices[v_BoneIDs[2]] * v_BoneWeights[2];
    boneTransform     += u_BoneMatrices[v_BoneIDs[3]] * v_BoneWeights[3];
    vec4 localPosition = boneTransform * vec4(v_Position, 1.0);
    
    vec4 worldPos = u_Transform * localPosition;
    
    gl_Position = u_LightViewMat * worldPos;
}

#shader fragment
#version 450 core

void main()
{
}