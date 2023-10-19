#include <jni.h>
#include <string>
#include <android/log.h>
#include <chrono>

#define TAG            "GCSuppress"
#define LOG(fmt, ...) __android_log_print(ANDROID_LOG_INFO, TAG, fmt, ##__VA_ARGS__)

struct JavaVMExtShadow {
    void *functions;
    void *runtime_;
};

struct RuntimeShadow {
    void *heap;
    void *jit_arena_pool_;
    void *arena_pool_;
    void *low_4gb_arena_pool_;
    void *linear_alloc_;
    size_t max_spins_before_thin_lock_inflation_;
    void *monitor_list_;
    void *monitor_pool_;
    void *thread_list_;
    void *intern_table_;
    void *class_linker_;
    void *signal_catcher_;
    std::string stack_trace_file_;
    void *java_vm_;
};

template<typename T>
int findOffset(void *start, int regionStart, int regionEnd, T value) {

    if (NULL == start || regionEnd <= 0 || regionStart < 0) {
        return -1;
    }
    char *c_start = (char *) start;

    for (int i = regionStart; i < regionEnd; i += 4) {
        T *current_value = (T *) (c_start + i);
        if (value == *current_value) {
            LOG("found offset: %d", i);
            return i;
        }
    }
    return -2;
}

void suppress(JNIEnv *env, jobject thiz, float targetHeapUtilization) {
    // 获取开始时间点
    auto start = std::chrono::high_resolution_clock::now();
    JavaVM *vm;
    env->GetJavaVM(&vm);
    JavaVMExtShadow *vmExt = (JavaVMExtShadow *) vm;
    void *runtime = vmExt->runtime_;
    // 先搜索到已知的 java_vm_ 在 Runtime 结构体的偏移量
    int vm_offset = findOffset(runtime, 0, 2000, (void *) vmExt);
    RuntimeShadow shadow;
    // 计算 java_vm_ 和 heap 的偏移量
    int offset = (char *) &(shadow.java_vm_) - (char *) &(shadow.heap);
    if (offset < 0) return;
    // 获取到 heap 的指针
    void **heap = (void **) ((char *) runtime + (vm_offset - offset));
    int utilization_offset = findOffset(*heap, 0, 2000, (double) targetHeapUtilization);
    if (utilization_offset < 0) return;

    size_t *max_free_ = (size_t *) ((char *) *heap + utilization_offset - sizeof(size_t));
    size_t *min_free_ = (size_t *) ((char *) *heap + utilization_offset - 2 * sizeof(size_t));

    *max_free_ = *max_free_ * 2;
    *min_free_ = *min_free_ * 2;
    LOG("max_free_ : %d", *max_free_);
    LOG("min_free_ : %d", *min_free_);

    // 计算时间差，并将其转换为纳秒
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    LOG("offset hook cost : %lld μs", duration.count());
}

static const JNINativeMethod gMethods[] = {
        {"nativeSuppress", "(F)V", (void *) suppress},
};


extern "C" int JNI_OnLoad(JavaVM *vm, void *unused) {

    JNIEnv *env = nullptr;
    jint result = JNI_ERR;
    if (vm->GetEnv((void **) (&env), JNI_VERSION_1_6) != JNI_OK) {
        return result;
    }

    jclass c = env->FindClass("com/example/gcsuppress/GCSuppress");
    if (c == nullptr) {
        const char *msg = "Native registration unable to find class; aborting...";
        env->FatalError(msg);
    }

    int numMethods = sizeof(gMethods) / sizeof(gMethods[0]);
    //采用RegisterNatives进行动态注册
    if (env->RegisterNatives(c, gMethods, numMethods) < 0) {
        const char *msg = "RegisterNatives failed; aborting...";
        env->FatalError(msg);
    }
    return JNI_VERSION_1_6;
}