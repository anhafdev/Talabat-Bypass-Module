#include <jni.h>
#include <android/log.h>
#include <cstring>
#include <dlfcn.h>
#include <sys/system_properties.h>
#include "zygisk.hpp"

#define LOG_TAG "TalabatBypass"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

using zygisk::Api;
using zygisk::AppSpecializeArgs;

// Native hook for system properties
static int (*orig_system_property_get)(const char* name, char* value);
static int new_system_property_get(const char* name, char* value) {
    // Spoof Serial Number
    if (strcmp(name, "ro.serialno") == 0 || strcmp(name, "ro.boot.serialno") == 0) {
        strcpy(value, "R58M12345ABC"); // Fake Serial
        LOGD("Spoofed %s to %s", name, value);
        return strlen(value);
    }
    // Spoof Device Model (Optional: makes it look like a clean Samsung)
    if (strcmp(name, "ro.product.model") == 0) {
        strcpy(value, "SM-G991B"); // Samsung S21
        return strlen(value);
    }
    return orig_system_property_get(name, value);
}

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
                LOGD("Talabat detected! Applying Native Hardware Spoofing...");
                
                // Register PLT hook for __system_property_get
                api->pltHookRegister("libc.so", "__system_property_get", 
                                     (void*)new_system_property_get, 
                                     (void**)&orig_system_property_get);
                
                // Hook Java Hardware IDs as well
                hook_java_hardware_ids();
            }
            env->ReleaseStringUTFChars(args->nice_name, process);
        }
    }

    void postAppSpecialize(const AppSpecializeArgs *) override {
        api->pltHookCommit(); // Commit native hooks
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
    }

    static jstring new_getString(JNIEnv *env, jobject thiz, jobject resolver, jstring name) {
        const char *nameStr = env->GetStringUTFChars(name, nullptr);
        if (strcmp(nameStr, "android_id") == 0) {
            env->ReleaseStringUTFChars(name, nameStr);
            return env->NewStringUTF("a1b2c3d4e5f67890"); // Fake Android ID
        }
        env->ReleaseStringUTFChars(name, nameStr);
        return nullptr; 
    }

    static jstring new_getImei(JNIEnv *env, jobject thiz) {
        return env->NewStringUTF("358123456789012"); // Fake IMEI
    }
};

REGISTER_ZYGISK_MODULE(TalabatBypass)
