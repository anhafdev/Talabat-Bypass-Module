#include <jni.h>
#include <android/log.h>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <string>
#include "zygisk.hpp"

#define LOG_TAG "TalabatBypass"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

using zygisk::Api;
using zygisk::AppSpecializeArgs;

// ডাইনামিক র‍্যান্ডম আইডি জেনারেটর
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
                LOGD("Talabat detected! Applying Hardware Spoofing & Prop Hiding...");
                hook_hardware_ids();
                hook_system_properties();
            }
            env->ReleaseStringUTFChars(args->nice_name, process);
        }
    }

private:
    Api *api;
    JNIEnv *env;

    void hook_hardware_ids() {
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

    // এই নতুন ফাংশনটি Talabat-এর সিস্টেম প্রপার্টি চেক ব্লক করবে
    void hook_system_properties() {
        JNINativeMethod propMethods[] = {
            {(char*)"get", (char*)"(Ljava/lang/String;)Ljava/lang/String;", (void*)new_getProp}
        };
        api->hookJniNativeMethods(env, "android/os/SystemProperties", propMethods, 1);
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

    // Talabat যখন ro.serialno বা ro.build.tags চাইবে, তখন ফেক ডেটা পাবে
    static jstring new_getProp(JNIEnv *env, jclass clazz, jstring key) {
        const char *keyStr = env->GetStringUTFChars(key, nullptr);
        if (strcmp(keyStr, "ro.serialno") == 0 || strcmp(keyStr, "ro.boot.serialno") == 0) {
            env->ReleaseStringUTFChars(key, keyStr);
            return env->NewStringUTF(random_serial().c_str());
        }
        if (strcmp(keyStr, "ro.debuggable") == 0) {
            env->ReleaseStringUTFChars(key, keyStr);
            return env->NewStringUTF("0"); 
        }
        if (strcmp(keyStr, "ro.build.tags") == 0) {
            env->ReleaseStringUTFChars(key, keyStr);
            return env->NewStringUTF("release-keys"); 
        }
        env->ReleaseStringUTFChars(key, keyStr);
        return nullptr; // ডিফল্ট আচরণের জন্য
    }
};

REGISTER_ZYGISK_MODULE(TalabatBypass)
