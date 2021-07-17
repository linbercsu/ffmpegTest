//
// Created by ZhaoLinlin on 2021/7/9.
//

#include <jni.h>
#include <sys/ucontext.h>
#include <dlfcn.h>
#include <android/log.h>
#include <string.h>
#include <malloc.h>
#include <time.h>
#include <threads.h>
#include <unwind.h>

static pthread_key_t keyJvmDetach;

static jmethodID callbackMethod = NULL;
static jclass callbackClass = NULL;
static JavaVM *callbackVM = NULL;

typedef struct backtrace_state_t {
    void** current;
    void** end;
}backtrace_state_t;
/**
 * callback used when using <unwind.h> to get the trace for the current context
 */
_Unwind_Reason_Code unwind_callback(struct _Unwind_Context *context, void *arg) {
    backtrace_state_t *state = (backtrace_state_t *) arg;
    _Unwind_Word pc = _Unwind_GetIP(context);
    if (pc) {
        if (state->current == state->end) {
            return _URC_END_OF_STACK;
        } else {
            *state->current = (void*)pc;
            state->current++;
//            *state->current++ = pc;
        }
    }
    return _URC_NO_REASON;
}

/**
 * uses built in <unwind.h> to get the trace for the current context
 */
 /*
size_t capture_backtrace(void **buffer, size_t max) {
    backtrace_state_t state = {buffer, buffer + max};
    _Unwind_Backtrace(unwind_callback, &state);
    return state.current - buffer;
}
*/

/*
static inline void dump_backtrace() {
//    va_list ap;
//    va_start(ap, msg);
//    __android_log_vprint(ANDROID_LOG_DEBUG, "backtrace", msg, ap);
//    va_end(ap);

    const size_t count = 30;
    void* buffer[count];
    backtrace_state_t state = {buffer, buffer + count};
    _Unwind_Backtrace(unwind_callback, &state);

    size_t max = state.current - buffer;

    for (size_t idx = 0; idx < max; ++idx) {
        const void* addr = buffer[idx];
        const char* symbol = "";
        const char* lib = "";

        Dl_info info;
        if (dladdr(addr, &info) && info.dli_fname != NULL) {
            symbol = info.dli_sname;
            lib = info.dli_fname;
        }

        __android_log_print(6, "test", "# %02ld: %p, %p %s (%s)",
                            idx, addr, (void*)((char*)addr - (char*)info.dli_fbase), symbol, lib );
    }
}
 */

JNIEnv *fromVM() {
    JNIEnv *env = NULL;

    int err = (*callbackVM)->GetEnv(callbackVM, (void **) &env, JNI_VERSION_1_2);
    if (err != JNI_OK) {
        if (err != JNI_EDETACHED) {
            if (err == JNI_EVERSION) {
            } else {
            }
        }

        err = (*callbackVM)->AttachCurrentThread(callbackVM, &env, NULL);
        if (err != JNI_OK) {
        }

        // Register detach function on thread exit.
        err = pthread_setspecific(keyJvmDetach, callbackVM);
        if (err != 0) {
        }

    }

    return env;
}

static inline uintptr_t pc_from_ucontext(ucontext_t *uc) {
#if defined(__aarch64__)
    return uc->uc_mcontext.pc;
#elif (defined(__arm__))
    return uc->uc_mcontext.arm_pc;
#elif (defined(__x86_64__))
    return uc->uc_mcontext.gregs[REG_RIP];
#elif (defined(__i386))
  return uc->uc_mcontext.gregs[REG_EIP];
#elif (defined (__ppc__)) || (defined (__powerpc__))
  return uc->uc_mcontext.regs->nip;
#elif (defined(__hppa__))
  return uc->uc_mcontext.sc_iaoq[0] & ~0x3UL;
#elif (defined(__sparc__) && defined (__arch64__))
  return uc->uc_mcontext.mc_gregs[MC_PC];
#elif (defined(__sparc__) && !defined (__arch64__))
  return uc->uc_mcontext.gregs[REG_PC];
#else
#error "Architecture is unknown, please report me!"
#endif
}

static int arch() {
#if defined(__aarch64__)
    return 1;
#elif (defined(__arm__))
    return 0;
#elif (defined(__x86_64__))
    return 2;
#elif (defined(__i386))
  return 3;
#endif

    return 9;
}

static const int signal_array[] = {SIGILL, SIGABRT, SIGBUS, SIGFPE, SIGSEGV, SIGSTKFLT, SIGSYS};
#define SIGNALS_LEN 7

static struct sigaction old_signal_handlers[SIGNALS_LEN];
static char *targetDir = NULL;

static const char *find_name(const char *lib_name) {
    const char *name = lib_name;
    int index = -1;
    int i = 0;
    while (*name != 0) {
        if (*name == '/') {
            index = i;
        }
        name++;

        i++;
    }

    if (index != -1) {
        name = lib_name + index + 1;
    }

    return name;
}

static void mx_write_to_file(const char *content) {
    time_t rawtime;
    time(&rawtime);

    char path[1024];
    memset(path, 0, 1024);

#if defined(__LP64__)
    sprintf(path, "%s/%s_%ld.txt", targetDir, "crash", rawtime);
#else
    sprintf(path, "%s/%s_%d.log", targetDir, name, rawtime);
#endif

    FILE *pFile = fopen(path, "w");
    if (pFile == NULL)
        return;

    fwrite(content, 1, strlen(content), pFile);
    fflush(pFile);

    fclose(pFile);
}

/*
static void mx_write(const char *lib_name, int code, int si_code, uintptr_t pc) {
    __android_log_print(6, "mx-crash", "%s", lib_name);
    time_t rawtime;
    time(&rawtime);

    const char *name = find_name(lib_name);

    char path[1024];
    memset(path, 0, 1024);

#if defined(__LP64__)
    sprintf(path, "%s/%s_%ld.txt", targetDir, name, rawtime);
#else
    sprintf(path, "%s/%s_%d.log", targetDir, name, rawtime);
#endif

//    FILE *pFile = fopen(path, "w");
//    if (pFile == NULL)
//        return;

    char str[32];
    memset(str, 0, 32);
#if defined(__LP64__)
    sprintf(str, "%s:%d:%d:%d:%ld", name, arch(), code, si_code, pc);
#else
    sprintf(str, "%s:%d:%d:%d:%d", name, arch(), code, si_code, pc);
#endif
    __android_log_print(6, "mx-crash", "1  %s", str);

    JNIEnv *env = fromVM();

    jstring log = (*env)->NewStringUTF(env, str);
    __android_log_print(6, "mx-crash", "write  %d", __LINE__);
    (*env)->CallStaticVoidMethod(env, callbackClass, callbackMethod, log);
    __android_log_print(6, "mx-crash", "write  %d", __LINE__);
//    __android_log_print(6, "mx-crash", "%s", str);

//    fwrite(str, 1, strlen(str), pFile);
//    fflush(pFile);
//
//    fclose(pFile);
}
*/

static inline void format(char* str, const char* libName, const char* symbol, void* pc) {
    if (libName == NULL || libName[0] == 0)
        libName = "(unknown)";

    if (symbol == NULL || symbol[0] == 0) {
        symbol = "(unknown)";
    }

    sprintf(str, "%s:%s:%p\n", libName, symbol, pc);
}

static void mx_signal_handle(int code, siginfo_t *si, void *t) {
    
    int strSize = 1920;
    char stackStr[strSize];
    memset(stackStr, 0, strSize);
    char* str = &stackStr[0];

    uintptr_t pc = pc_from_ucontext((ucontext_t *) t);


    Dl_info info;
    memset(&info, 0, sizeof(info));
    if (dladdr((void *) pc, &info) != 0 && info.dli_fname != NULL) {
        const uintptr_t addr_relative =
                ((uintptr_t) pc - (uintptr_t) info.dli_fbase);
//      
//        __android_log_print(6, "test", "%ld, %p, %p, %s", pc, info.dli_fbase, addr_relative, info.dli_fname);
        const char *name = find_name(info.dli_fname);
        const char* symbol = info.dli_sname;
        format(str, name, symbol, (void*)addr_relative);
        str = &stackStr[strlen(stackStr)];
//        mx_write(info.dli_fname, code, si->si_code, addr_relative);
    }

//    dump_backtrace();

    const size_t count = 30;
    void* buffer[count];
    backtrace_state_t state = {buffer, buffer + count};
    _Unwind_Backtrace(unwind_callback, &state);

    size_t max = state.current - buffer;

    for (size_t idx = 0; idx < max; ++idx) {
        const void* addr = buffer[idx];
        const char* symbol = "";

        Dl_info info1;
        if (dladdr(addr, &info1) && info1.dli_fname != NULL) {
            symbol = info1.dli_sname;
            const char *lib1 = find_name(info1.dli_fname);
            void *idAddress = (void *) ((char *) addr - (char *) info1.dli_fbase);

            format(str, lib1, symbol, idAddress);

            size_t length = strlen(stackStr);
            if (length > strSize + 200) {
                break;
            }
            str = &stackStr[strlen(stackStr)];
        }
    }

//    mx_write_to_file(path);
    JNIEnv *env = fromVM();

    jstring log = (*env)->NewStringUTF(env, stackStr);
    (*env)->CallStaticVoidMethod(env, callbackClass, callbackMethod, log);

    for (int i = 0; i < SIGNALS_LEN; i++) {
        if (signal_array[i] == code) {
            old_signal_handlers[i].sa_sigaction(code, si, t);
            break;
        }
    }

    /*
    char *head_cpu = nullptr;

    asprintf(&head_cpu, "r0 %08lx  r1 %08lx  r2 %08lx  r3 %08lx\n"
                        "r4 %08lx  r5 %08lx  r6 %08lx  r7 %08lx\n"
                        "r8 %08lx  r9 %08lx  sl %08lx  fp %08lx\n"
                        "ip %08lx  sp %08lx  lr %08lx  pc %08lx  cpsr %08lx\n",
             t->uc_mcontext.arm_r0, t->uc_mcontext.arm_r1, t->uc_mcontext.arm_r2,
             t->uc_mcontext.arm_r3, t->uc_mcontext.arm_r4, t->uc_mcontext.arm_r5,
             t->uc_mcontext.arm_r6, t->uc_mcontext.arm_r7, t->uc_mcontext.arm_r8,
             t->uc_mcontext.arm_r9, t->uc_mcontext.arm_r10, t->uc_mcontext.arm_fp,
             t->uc_mcontext.arm_ip, t->uc_mcontext.arm_sp, t->uc_mcontext.arm_lr,
             t->uc_mcontext.arm_pc, t->uc_mcontext.arm_cpsr);
             */
}


JNIEXPORT void mx_crash_collect_init(const char *dir) {

    size_t length = strlen(dir);
    targetDir = malloc(length + 1);
    memset(targetDir, 0, length + 1);
    memcpy(targetDir, dir, length);

    stack_t stack;
    memset(&stack, 0, sizeof(stack));
/* Reserver the system default stack size. We don't need that much by the way. */
    stack.ss_size = SIGSTKSZ * 5;
    stack.ss_sp = malloc(stack.ss_size * 5);
    stack.ss_flags = 0;
/* Install alternate stack size. Be sure the memory region is valid until you revert it. */
    if (stack.ss_sp != NULL && sigaltstack(&stack, NULL) == 0) {
    }


    struct sigaction handler;
    handler.sa_sigaction = mx_signal_handle;
    handler.sa_flags = SA_SIGINFO;

    for (int i = 0; i < SIGNALS_LEN; ++i) {
        sigaction(signal_array[i], &handler, &old_signal_handlers[i]);
    }
}

JNIEXPORT void JNICALL
Java_com_mxtech_NativeCrashCollector_nativeInitClass(
        JNIEnv *env,
        jclass clazz) {
    callbackClass = (*env)->NewGlobalRef(env, clazz);
//    callbackClass = clazz;
    callbackMethod = (*env)->GetStaticMethodID(env, clazz, "onNativeCrash",
                                               "(Ljava/lang/String;)V");
    (*env)->GetJavaVM(env, &callbackVM);
//    jboolean copy;
//    const char *dirStr = (*env)->GetStringUTFChars(env, dir, &copy);
//
    mx_crash_collect_init("/sdcard/test1/hls");

//    (*env)->ReleaseStringUTFChars(env, dir, dirStr);
}