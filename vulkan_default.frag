
#version 450

layout(set = 1, binding = 0) uniform sampler2D Textures[512];

layout(location = 0) in flat uint InTextureIndex;
layout(location = 1) in VertexShaderVarying
{
    vec2 TexCoord;
    vec4 Color;
} In;

layout(location = 0) out vec4 FragmentColor;

void main()
{
    vec4 Texel = texture(Textures[InTextureIndex], In.TexCoord);
    FragmentColor = Texel * In.Color;
}

