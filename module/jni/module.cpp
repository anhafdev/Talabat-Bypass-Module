#include <jni.h>
#include <android/log.h>
#include <cstring>
#include <dlfcn.h>
#include <cstdlib>
#include <ctime>
#include <string>
#include <unistd.h>
#include <sys/system_properties.h>
#include "zygisk.hpp"

#define LOG_TAG "TalabatBypass"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

using zygisk::Api;
using zygisk::AppSpecializeArgs;

// --- Dynamic Random Generators ---
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

// --- Native Hook Pointers ---
static int (*orig_system_property_get)(const char* name, char* value);
static int new_system_property_get(const char* name, char* value) {
    if (strcmp(name, "ro.serialno") == 0 || strcmp(name, "ro.boot.serialno") == 0) {
        std::string fake = random_serial();
        strcpy(value, fake.c_str());
        return strlen(value);
    }
    if (strcmp(name, "ro.product.model") == 0) {
        strcpy(value, "SM-G991B");
        return strlen(value);
    }
    return orig_system_property_get(name, value);
}

static int (*orig_kill)(pid_t pid, int sig);
static int new_kill(pid_t pid, int sig) {
    if (pid == getpid()) {
        LOGD("Blocked kill() on self!");
        return 0; 
    }
    return orig_kill(pid, sig);
}

static void (*orig_exit)(int status);
static void new_exit(int status) {
    LOGD("Blocked exit() call!");
}

class TalabatBypass : public zygisk::ModuleBase {
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
                LOGD("Talabat detected! Applying Ultimate Bypass...");
                
                hook_java_hardware_ids();
                
                // Register Native Hooks (PLT)
                api->pltHookRegister("libc.so", "__system_property_get", (void*)new_system_property_get, (void**)&orig_system_property_get);
                api->pltHookRegister("libc.so", "kill", (void*)new_kill, (void**)&orig_kill);
                api->pltHookRegister("libc.so", "exit", (void*)new_exit, (void**)&orig_exit);
            }
            env->ReleaseStringUTFChars(args->nice_name, process);
        }
    }

    void postAppSpecialize(const AppSpecializeArgs *) override {
        api->pltHookCommit();
    }

private:
    Api *api;
    JNIEnv *env;

    void hook_java_hardware_ids() {
        JNINativeMethod secureMethods[] = {
            {(char*)"getString", (char*)"(Landroid/content/ContentResolver;Ljava/lang/String;)Ljava/lang/String;", (void*)new_getString}
        };
        api->hookJniNativeMethods(env, "android/provider/Settings$Secure", secureMethods, 1);

        JNINativeMethod telephonyMethods[] = {
            {(char*)"getImei", (char*)"()Ljava/lang/String;", (void*)new_getImei},
            {(char*)"getDeviceId", (char*)"()Ljava/lang/String;", (void*)new_getImei}
        };
        api->hookJniNativeMethods(env, "android/telephony/TelephonyManager", telephonyMethods, 2);

        JNINativeMethod buildMethods[] = {
            {(char*)"getSerial", (char*)"()Ljava/lang/String;", (void*)new_getSerial}
        };
        api->hookJniNativeMethods(env, "android/os/Build", buildMethods, 1);

        JNINativeMethod locationMethods[] = {
            {(char*)"isFromMockProvider", (char*)"()Z", (void*)new_isFromMockProvider},
            {(char*)"isMock", (char*)"()Z", (void*)new_isMock}
        };
        api->hookJniNativeMethods(env, "android/location/Location", locationMethods, 2);
    }

    static jstring new_getString(JNIEnv *env, jobject thiz, jobject resolver, jstring name) {
        const char *nameStr = env->GetStringUTFChars(name, nullptr);
        if (strcmp(nameStr, "android_id") == 0) {
            env->ReleaseStringUTFChars(name, nameStr);
            return env->NewStringUTF(random_android_id().c_str());
        }
        env->ReleaseStringUTFChars(name, nameStr);
        return nullptr;
    }

    static jstring new_getImei(JNIEnv *env, jobject thiz) {
        return env->NewStringUTF(random_imei().c_str());
    }

    static jstring new_getSerial(JNIEnv *env) {
        return env->NewStringUTF(random_serial().c_str());
    }

    static jboolean new_isFromMockProvider(JNIEnv *env, jobject thiz) {
        return JNI_FALSE; 
    }

    static jboolean new_isMock(JNIEnv *env, jobject thiz) {
        return JNI_FALSE;
    }
};

REGISTER_ZYGISK_MODULE(TalabatBypass)
