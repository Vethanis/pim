
struct sapp_event;

void systems_init();
void systems_update();
void systems_shutdown();
void systems_onevent(const struct sapp_event* evt);
