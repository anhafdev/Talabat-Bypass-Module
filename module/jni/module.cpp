#include <jni.h>
#include <android/log.h>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <string>
#include "zygisk.hpp"

#define LOG_TAG "SystemService"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

using zygisk::Api;
using zygisk::AppSpecializeArgs;

std::string random_serial() {
    const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::string s = "";
    for (int i = 0; i < 10; ++i) s += chars[rand() % 36];
    return s;
}

class SystemService : public zygisk::ModuleBase {
public:
    void onLoad(Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
        srand(time(NULL)); 
    }

    void preAppSpecialize(AppSpecializeArgs *args) override {
        const char *process = env->GetStringUTFChars(args->nice_name, nullptr);
        if (process) {
            if (strstr(process, "com.logistics.rider.talabat")) {
                LOGD("Talabat detected. Injecting Hardware Layer...");
                hook_hardware_ids();
            }
            env->ReleaseStringUTFChars(args->nice_name, process);
        }
    }

    void postAppSpecialize(const AppSpecializeArgs *args) override {}

private:
    Api *api;
    JNIEnv *env;

    void hook_hardware_ids() {
        // SystemProperties.get হলো একটি নেটিভ মেথড, যা হুক করা ১০০% সেফ
        JNINativeMethod propMethods[] = {
            {(char*)"get", (char*)"(Ljava/lang/String;)Ljava/lang/String;", (void*)new_getProp}
        };
        api->hookJniNativeMethods(env, "android/os/SystemProperties", propMethods, 1);
    }

    static jstring new_getProp(JNIEnv *env, jclass clazz, jstring key) {
        const char *keyStr = env->GetStringUTFChars(key, nullptr);
        std::string result;
        
        if (strcmp(keyStr, "ro.serialno") == 0 || strcmp(keyStr, "ro.boot.serialno") == 0) {
            result = random_serial();
        } else if (strcmp(keyStr, "ro.product.model") == 0) {
            result = "Pixel 6"; 
        } else if (strcmp(keyStr, "ro.product.brand") == 0) {
            result = "google";
        } else if (strcmp(keyStr, "ro.debuggable") == 0) {
            result = "0";
        } else if (strcmp(keyStr, "ro.build.tags") == 0) {
            result = "release-keys";
        } else {
            env->ReleaseStringUTFChars(key, keyStr);
            return nullptr; // ডিফল্ট আচরণের জন্য
        }
        
        env->ReleaseStringUTFChars(key, keyStr);
        return env->NewStringUTF(result.c_str());
    }
};

REGISTER_ZYGISK_MODULE(SystemService)
