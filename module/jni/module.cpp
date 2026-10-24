#include <jni.h>
#include <android/log.h>
#include <cstring>
#include <dlfcn.h>
#include <cstdlib>
#include <ctime>
#include <string>
#include "zygisk.hpp"

#define LOG_TAG "TalabatBypass"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

using zygisk::Api;
using zygisk::AppSpecializeArgs;

// প্রতিবার অ্যাপ ওপেন হলে নতুন র‍্যান্ডম আইডি জেনারেট করার ফাংশন
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
        srand(time(NULL)); // র‍্যান্ডম জেনারেটর সিড সেট করা
    }

    void preAppSpecialize(AppSpecializeArgs *args) override {
        const char *process = env->GetStringUTFChars(args->nice_name, nullptr);
        if (process) {
            if (strstr(process, "com.logistics.rider.talabat")) {
                LOGD("Talabat detected! Applying Dynamic Hardware Spoofing & Mock Location Bypass...");
                hook_hardware_ids();
            }
            env->ReleaseStringUTFChars(args->nice_name, process);
        }
    }

private:
    Api *api;
    JNIEnv *env;

    void hook_hardware_ids() {
        // 1. Android ID Spoofing
        JNINativeMethod secureMethods[] = {
            {(char*)"getString", (char*)"(Landroid/content/ContentResolver;Ljava/lang/String;)Ljava/lang/String;", (void*)new_getString}
        };
        api->hookJniNativeMethods(env, "android/provider/Settings$Secure", secureMethods, 1);

        // 2. IMEI Spoofing
        JNINativeMethod telephonyMethods[] = {
            {(char*)"getImei", (char*)"()Ljava/lang/String;", (void*)new_getImei},
            {(char*)"getDeviceId", (char*)"()Ljava/lang/String;", (void*)new_getImei}
        };
        api->hookJniNativeMethods(env, "android/telephony/TelephonyManager", telephonyMethods, 2);

        // 3. Serial Spoofing
        JNINativeMethod buildMethods[] = {
            {(char*)"getSerial", (char*)"()Ljava/lang/String;", (void*)new_getSerial}
        };
        api->hookJniNativeMethods(env, "android/os/Build", buildMethods, 1);

        // 4. Mock Location (Fake GPS) Bypass
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
            std::string fake_id = random_android_id();
            return env->NewStringUTF(fake_id.c_str());
        }
        env->ReleaseStringUTFChars(name, nameStr);
        return nullptr;
    }

    static jstring new_getImei(JNIEnv *env, jobject thiz) {
        std::string fake_imei = random_imei();
        return env->NewStringUTF(fake_imei.c_str());
    }

    static jstring new_getSerial(JNIEnv *env) {
        std::string fake_serial = random_serial();
        return env->NewStringUTF(fake_serial.c_str());
    }

    static jboolean new_isFromMockProvider(JNIEnv *env, jobject thiz) {
        LOGD("Bypassed isFromMockProvider!");
        return JNI_FALSE; 
    }

    static jboolean new_isMock(JNIEnv *env, jobject thiz) {
        LOGD("Bypassed isMock!");
        return JNI_FALSE;
    }
};

REGISTER_ZYGISK_MODULE(TalabatBypass)
