#pragma once

// Main declaration
#if defined(RENDERER_OPENGL) || defined(RENDERER_VULKAN)
#define MAIN_ENTRYPOINT int main(int argc, char* args[])
#elif defined(RENDERER_DX12)
#define MAIN_ENTRYPOINT int main(int argc, char* args[])
#else
#define MAIN_ENTRYPOINT
#error "Renderer define not found"
#endif