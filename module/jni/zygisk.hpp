#pragma once
#include <jni.h>
#include <sys/types.h>
#include <sys/stat.h>

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
    virtual void hookJniNativeMethods(JNIEnv *env, const char *className, JNINativeMethod *methods, int numMethods) = 0;
    virtual ~Api() = default;
};

class ModuleBase {
public:
    virtual void onLoad(Api *api, JNIEnv *env) = 0;
    virtual void preAppSpecialize(AppSpecializeArgs *args) = 0;
    virtual void postAppSpecialize(const AppSpecializeArgs *args) = 0;
    virtual void preServerSpecialize(ServerSpecializeArgs *args) = 0;
    virtual void postServerSpecialize(const ServerSpecializeArgs *args) = 0;
    virtual ~ModuleBase() = default;
};

} // namespace zygisk

#define REGISTER_ZYGISK_MODULE(clazz) \
extern "C" __attribute__((visibility("default"))) void zygisk_module_entry(zygisk::Api *api, JNIEnv *env) { \
    static clazz module; \
    module.onLoad(api, env); \
}
