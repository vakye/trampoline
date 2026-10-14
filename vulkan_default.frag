
#version 450

layout(set = 1, binding = 0) uniform sampler2D Textures[512];

layout(location = 0) in VertexShaderOutput
{
    vec2 TexCoord;
    vec4 Color;
    float TextureIndex;
} In;

layout(location = 0) out vec4 FragmentColor;

void main()
{
    vec4 Texel = texture(Textures[uint(In.TextureIndex)], In.TexCoord);
    FragmentColor = Texel * In.Color;
}

