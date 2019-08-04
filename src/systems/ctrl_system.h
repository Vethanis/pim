
struct sapp_event;

namespace CtrlSystem
{
    void Init();
    void Update(float dt);
    void Shutdown();
    void OnEvent(const sapp_event* evt, bool keyboardCaptured);
};
