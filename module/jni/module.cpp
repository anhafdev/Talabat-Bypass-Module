#include <jni.h>
#include <android/log.h>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <string>
#include <sys/system_properties.h>
#include "zygisk.hpp"

#define LOG_TAG "SystemService"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

using zygisk::Api;
using zygisk::AppSpecializeArgs;

std::string random_imei() {
    std::string s = "35";
    for (int i = 0; i < 13; ++i) s += std::to_string(rand() % 10);
    return s;
}
std::string random_android_id() {
    const char* chars = "0123456789abcdef";
    std::string s = "";
    for (int i = 0; i < 16; ++i) s += chars[rand() % 16];
    return s;
}
std::string random_serial() {
    const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::string s = "";
    for (int i = 0; i < 10; ++i) s += chars[rand() % 36];
    return s;
}

// নেটিভ সিস্টেম প্রপার্টি হুক
static int (*orig_system_property_get)(const char* name, char* value);
static int new_system_property_get(const char* name, char* value) {
    if (strcmp(name, "ro.serialno") == 0 || strcmp(name, "ro.boot.serialno") == 0) {
        strcpy(value, random_serial().c_str()); 
        return strlen(value);
    }
    if (strcmp(name, "ro.product.model") == 0) {
        strcpy(value, "Pixel 6"); 
        return strlen(value);
    }
    if (strcmp(name, "ro.product.brand") == 0) {
        strcpy(value, "google");
        return strlen(value);
    }
    if (strcmp(name, "ro.debuggable") == 0) {
        strcpy(value, "0"); 
        return 1;
    }
    if (strcmp(name, "ro.build.tags") == 0) {
        strcpy(value, "release-keys");
        return strlen(value);
    }
    return orig_system_property_get(name, value);
}

class SystemService : public zygisk::ModuleBase {
public:
    void onLoad(Api *api, JNIEnv *env) override {
        this->api = api;
        srand(time(NULL)); 
    }

    void preAppSpecialize(AppSpecializeArgs *args) override {
        const char *process = env->GetStringUTFChars(args->nice_name, nullptr);
        if (process) {
            if (strstr(process, "com.logistics.rider.talabat")) {
                LOGD("Talabat detected. Injecting Native Hardware Layer...");
                api->pltHookRegister("libc.so", "__system_property_get", (void*)new_system_property_get, (void**)&orig_system_property_get);
            }
            env->ReleaseStringUTFChars(args->nice_name, process);
        }
    }

    void postAppSpecialize(const AppSpecializeArgs *args) override {
        api->pltHookCommit();
    }

private:
    Api *api;
    JNIEnv *env;
};

REGISTER_ZYGISK_MODULE(SystemService)
