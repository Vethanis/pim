
struct sapp_event;

namespace Systems
{
    void Init();
    void Update();
    void Shutdown();
    void OnEvent(const sapp_event* evt);
};
