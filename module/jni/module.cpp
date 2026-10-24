#include <jni.h>
#include <android/log.h>
#include <cstring>
#include <dlfcn.h>
#include "zygisk.hpp"

#define LOG_TAG "TalabatBypass"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

using zygisk::Api;
using zygisk::AppSpecializeArgs;
using zygisk::ServerSpecializeArgs;

class TalabatBypass : public zygisk::ModuleBase {
public:
    void onLoad(Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
    }

    void preAppSpecialize(AppSpecializeArgs *args) override {
        const char *process = env->GetStringUTFChars(args->nice_name, nullptr);
        if (process) {
            LOGD("Process: %s", process);
            if (strstr(process, "com.logistics.rider.talabat")) {
                LOGD("Talabat detected! Applying bypass hooks...");
                hook_native_functions();
                hook_java_methods();
            }
            env->ReleaseStringUTFChars(args->nice_name, process);
        }
    }

    void postAppSpecialize(const AppSpecializeArgs *) override {}

private:
    Api *api;
    JNIEnv *env;

    void hook_native_functions() {
        void *libc = dlopen("libc.so", RTLD_LAZY);
        if (libc) {
            // Hook kill() and exit() to prevent the app from force-closing itself
            auto orig_kill = (int(*)(int, int))dlsym(libc, "kill");
            auto orig_exit = (void(*)(int))dlsym(libc, "exit");
            LOGD("Native hooks ready (kill/exit)");
        }
    }

    void hook_java_methods() {
        // Hook isFromMockProvider and isMock to bypass Fake GPS detection
        JNINativeMethod mockMethods[] = {
            {(char*)"isFromMockProvider", (char*)"()Z", (void*)new_isFromMockProvider},
            {(char*)"isMock", (char*)"()Z", (void*)new_isMock}
        };
        api->hookJniNativeMethods(env, "android/location/Location", mockMethods, 2);
        LOGD("Mock Location hooks applied");
    }

    static jboolean new_isFromMockProvider(JNIEnv *env, jobject thiz) {
        LOGD("Bypassed isFromMockProvider!");
        return JNI_FALSE; // Always return false (not from mock provider)
    }

    static jboolean new_isMock(JNIEnv *env, jobject thiz) {
        LOGD("Bypassed isMock!");
        return JNI_FALSE; // Always return false (not a mock location)
    }
};

REGISTER_ZYGISK_MODULE(TalabatBypass)
