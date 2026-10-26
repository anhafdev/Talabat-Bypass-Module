#pragma once
#include <jni.h>
#include <sys/types.h>

namespace zygisk {

struct AppSpecializeArgs {
    jstring nice_name;
    jstring app_data_dir;
};

struct ServerSpecializeArgs {
    jint argc;
    char **argv;
};

struct Api {
    virtual void connectCompanion() = 0;
    virtual void pltHookRegister(dev_t dev, ino_t inode, const char *symbol, void *newFunc, void **oldFunc) = 0;
    virtual void pltHookCommit() = 0;
};

class ModuleBase {
public:
    virtual void onLoad(Api *api, JNIEnv *env) {}
    virtual void preAppSpecialize(AppSpecializeArgs *args) {}
    virtual void postAppSpecialize(const AppSpecializeArgs *args) {}
    virtual void preServerSpecialize(ServerSpecializeArgs *args) {}
    virtual void postServerSpecialize(const ServerSpecializeArgs *args) {}
    virtual ~ModuleBase() = default;
};

} // namespace zygisk

#define REGISTER_ZYGISK_MODULE(clazz) \
extern "C" __attribute__((visibility("default"))) void zygisk_module_entry(zygisk::Api *api, JNIEnv *env) { \
    static clazz module; \
    module.onLoad(api, env); \
}
