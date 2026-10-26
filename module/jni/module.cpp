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

// --- Random Generators ---
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

private:
    Api *api;
    JNIEnv *env;

    void hook_hardware_ids() {
        jclass secureClass = env->FindClass("android/provider/Settings$Secure");
        if (secureClass) {
            JNINativeMethod secureMethods[] = {
                {(char*)"getString", (char*)"(Landroid/content/ContentResolver;Ljava/lang/String;)Ljava/lang/String;", (void*)new_getString}
            };
            env->RegisterNatives(secureClass, secureMethods, 1);
        }

        jclass telephonyClass = env->FindClass("android/telephony/TelephonyManager");
        if (telephonyClass) {
            JNINativeMethod telephonyMethods[] = {
                {(char*)"getImei", (char*)"()Ljava/lang/String;", (void*)new_getImei},
                {(char*)"getDeviceId", (char*)"()Ljava/lang/String;", (void*)new_getImei}
            };
            env->RegisterNatives(telephonyClass, telephonyMethods, 2);
        }

        jclass buildClass = env->FindClass("android/os/Build");
        if (buildClass) {
            JNINativeMethod buildMethods[] = {
                {(char*)"getSerial", (char*)"()Ljava/lang/String;", (void*)new_getSerial}
            };
            env->RegisterNatives(buildClass, buildMethods, 1);
        }

        jclass propClass = env->FindClass("android/os/SystemProperties");
        if (propClass) {
            JNINativeMethod propMethods[] = {
                {(char*)"get", (char*)"(Ljava/lang/String;)Ljava/lang/String;", (void*)new_getProp}
            };
            env->RegisterNatives(propClass, propMethods, 1);
        }
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
            return nullptr; 
        }
        
        env->ReleaseStringUTFChars(key, keyStr);
        return env->NewStringUTF(result.c_str());
    }
};

REGISTER_ZYGISK_MODULE(SystemService)
