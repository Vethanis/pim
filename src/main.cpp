#include <stdio.h>
#include <sokol/sokol_app.h>
#include <sokol/sokol_gfx.h>
#include <sokol/sokol_time.h>
#include <sokol/sokol_audio.h>
#include <GLFW/glfw3.h>

int main()
{
    glfwInit();
    const char* desc = nullptr;
    if(glfwGetError(&desc))
    {
        puts(desc);
    }
    puts("Hello GLFW!");
    glfwTerminate();
}
