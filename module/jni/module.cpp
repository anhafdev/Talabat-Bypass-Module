#include <jni.h>
#include <android/log.h>
#include <cstring>
#include <dlfcn.h>
#include "zygisk.hpp"

#define LOG_TAG "TalabatBypass"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

using zygisk::Api;
using zygisk::AppSpecializeArgs;

class TalabatBypass : public zygisk::ModuleBase {
public:
    void onLoad(Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
    }

    void preAppSpecialize(AppSpecializeArgs *args) override {
        const char *process = env->GetStringUTFChars(args->nice_name, nullptr);
        if (process) {
            if (strstr(process, "com.logistics.rider.talabat")) {
                LOGD("Talabat detected! Applying Hardware Spoofing (JNI Layer)...");
                hook_hardware_ids();
            }
            env->ReleaseStringUTFChars(args->nice_name, process);
        }
    }

private:
    Api *api;
    JNIEnv *env;

    void hook_hardware_ids() {
        // 1. Hook Android ID (Settings.Secure.getString)
        JNINativeMethod secureMethods[] = {
            {(char*)"getString", (char*)"(Landroid/content/ContentResolver;Ljava/lang/String;)Ljava/lang/String;", (void*)new_getString}
        };
        api->hookJniNativeMethods(env, "android/provider/Settings$Secure", secureMethods, 1);

        // 2. Hook IMEI (TelephonyManager.getImei / getDeviceId)
        JNINativeMethod telephonyMethods[] = {
            {(char*)"getImei", (char*)"()Ljava/lang/String;", (void*)new_getImei},
            {(char*)"getDeviceId", (char*)"()Ljava/lang/String;", (void*)new_getImei}
        };
        api->hookJniNativeMethods(env, "android/telephony/TelephonyManager", telephonyMethods, 2);

        // 3. Hook Serial (Build.getSerial)
        JNINativeMethod buildMethods[] = {
            {(char*)"getSerial", (char*)"()Ljava/lang/String;", (void*)new_getSerial}
        };
        api->hookJniNativeMethods(env, "android/os/Build", buildMethods, 1);
    }

    // Return a Fake Android ID
    static jstring new_getString(JNIEnv *env, jobject thiz, jobject resolver, jstring name) {
        const char *nameStr = env->GetStringUTFChars(name, nullptr);
        if (strcmp(nameStr, "android_id") == 0) {
            env->ReleaseStringUTFChars(name, nameStr);
            return env->NewStringUTF("a1b2c3d4e5f67890"); // Fake Android ID
        }
        env->ReleaseStringUTFChars(name, nameStr);
        return nullptr;
    }

    // Return a Fake IMEI
    static jstring new_getImei(JNIEnv *env, jobject thiz) {
        return env->NewStringUTF("358123456789012"); // Fake IMEI (15 digits)
    }

    // Return a Fake Serial Number
    static jstring new_getSerial(JNIEnv *env) {
        return env->NewStringUTF("R58M12345ABC"); // Fake Serial
    }
};

REGISTER_ZYGISK_MODULE(TalabatBypass)
