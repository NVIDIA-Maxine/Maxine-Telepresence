R""(
#version 420 core

out vec2 texCoords;

void main()
{
    // compute vertex position and texture coordinate directly from vertex id
    // without using any vertex array buffer
    // following https://www.saschawillems.de/blog/2016/08/13/vulkan-tutorial-on-rendering-a-fullscreen-quad-without-buffers/
    texCoords = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    gl_Position = vec4(texCoords * 2.0f + -1.0f, 0.0f, 1.0f);
}
)""
