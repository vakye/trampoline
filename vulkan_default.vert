
#version 450

struct vertex
{
    float X, Y;
    float U, V;
    float R, G, B, A;
};

layout(set = 0, binding = 0) readonly buffer VertexBuffer
{
    vertex Vertices[];
};

layout(push_constant) uniform PushConstants
{
    mat4 Projection;
};

layout(location = 0) out VertexShaderOutput
{
    vec2 TexCoord;
    vec4 Color;
} Out;

void main()
{
    vertex V = Vertices[gl_VertexIndex];

    vec2 Position = vec2(V.X, V.Y);    
    vec2 TexCoord = vec2(V.U, V.V);    
    vec4 Color    = vec4(V.R, V.G, V.B, V.A);

    gl_Position = Projection * vec4(Position, 0.0, 1.0);
    Out.TexCoord = TexCoord;
    Out.Color = Color;
}

