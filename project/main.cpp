#include "pch.h"
#include "src/core/Window.h"
#include "src/graphics/renderers/Renderer.h"

static Window s_window;
static Renderer s_renderer;

int main()
{
#if defined(DEBUG) && defined(WIN32)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    srand((unsigned int)time(0));
    s_window.init("softbody simulation", 1280, 720);
    s_renderer.init(s_window);

    while (!s_window.shouldClose())
    {
        s_window.update();
        s_renderer.render();
    }

    s_renderer.cleanup();
    s_window.cleanup();
}