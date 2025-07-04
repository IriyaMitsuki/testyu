#include <jni.h>
#include <pthread.h>
#include <unistd.h>
#include <dlfcn.h>
#include <android/log.h>

// Подключаем Dobby и ImGui
#include "includes/dobby/dobby.h"
#include "includes/imgui/imgui.h"
#include "includes/imgui/imgui_impl_android.h"
#include "includes/imgui/imgui_impl_opengl3.h"

#define LOG_TAG "MyUnityHack"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// Глобальные переменные для нашего хака
bool isMenuVisible = true;
bool isFlyHackEnabled = false;

// Адреса и указатели на оригинальные функции
void *il2cpp_handle = nullptr;

// Определяем структуру Vector3, как в Unity
struct Vector3 {
    float x, y, z;
};

// Прототип функции Player_Update, которую мы будем хукать
// Важно: сигнатура (аргументы и возвращаемое значение) должна точно совпадать!
// В данном случае `void Player_Update(void *instance)`
void (*orig_Player_Update)(void *instance);

void hooked_Player_Update(void *instance) {
    // Вызываем оригинальную функцию, чтобы игра не сломалась
    orig_Player_Update(instance);

    // `instance` - это указатель на C# объект класса Player
    // Если флайхак включен, манипулируем Transform'ом
    if (isFlyHackEnabled && instance) {
        // Получаем компонент Transform. Смещение нужно найти реверс-инжинирингом.
        // Это самое сложное место. 0x3C - просто пример!
        void *transform = *(void **)((uintptr_t)instance + 0x3C); 

        if (transform) {
            // Устанавливаем новую позицию. Смещение set_position тоже нужно найти.
            // 0x90 - тоже пример!
            auto set_position = (void (*)(void*, Vector3))((uintptr_t)il2cpp_handle + 0xABCDEF);

            // Для примера, просто поднимаем игрока вверх
            Vector3 currentPos = { 100.0f, 200.0f, 100.0f }; // Пример координат
            // set_position(transform, currentPos);
        }
    }
}


// Хук на eglSwapBuffers для отрисовки меню
EGLBoolean (*orig_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);

EGLBoolean hooked_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    // Инициализация ImGui при первом вызове
    static bool imgui_initialized = false;
    if (!imgui_initialized) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        
        ImGui::StyleColorsDark();
        
        ImGui_ImplAndroid_Init(nullptr); // Инициализация для Android
        ImGui_ImplOpenGL3_Init("#version 300 es");

        // Устанавливаем размер шрифта
        io.Fonts->AddFontDefault();

        imgui_initialized = true;
        LOGI("ImGui Initialized!");
    }

    // Начинаем новый кадр ImGui
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame();
    ImGui::NewFrame();

    // Рисуем наше меню
    if (isMenuVisible) {
        ImGui::Begin("My Test Menu");
        ImGui::Checkbox("Fly Hack", &isFlyHackEnabled);
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
    }
    
    // Рендерим ImGui
    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetRenderData());

    // Вызываем оригинальную функцию, чтобы кадр игры тоже отрисовался
    return orig_eglSwapBuffers(dpy, surface);
}


// Основной поток, где происходит вся магия
void* hack_thread(void*) {
    LOGI("Hack thread started!");
    
    // Ждем, пока библиотека libil2cpp.so загрузится в память
    do {
        sleep(1);
        il2cpp_handle = dlopen("libil2cpp.so", RTLD_NOLOAD);
    } while (!il2cpp_handle);

    LOGI("libil2cpp.so found at %p", il2cpp_handle);

    // Хукаем рендеринг для меню (более надежный способ)
    void* egl_handle = dlopen("libEGL.so", RTLD_LAZY);
    if (egl_handle) {
        void* eglSwapBuffers_addr = dlsym(egl_handle, "eglSwapBuffers");
        DobbyHook(eglSwapBuffers_addr, (dobby_dummy_func_t)hooked_eglSwapBuffers, (dobby_dummy_func_t*)&orig_eglSwapBuffers);
        LOGI("eglSwapBuffers hooked!");
    }

    // Хукаем Player.Update
    // ВАЖНО: 0xDEADBEEF - это ПРИМЕР! Вам нужно найти реальный адрес (смещение)
    // функции Player.Update в libil2cpp.so с помощью IDA Pro, Ghidra или Il2CppDumper.
    uintptr_t player_update_addr = (uintptr_t)il2cpp_handle + 0xDEADBEEF; 
    DobbyHook((void*)player_update_addr, (dobby_dummy_func_t)hooked_Player_Update, (dobby_dummy_func_t*)&orig_Player_Update);

    LOGI("Player_Update hooked!");

    return nullptr;
}


// Точка входа в библиотеку
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("Library loaded! Starting hack thread...");
    
    pthread_t pt;
    pthread_create(&pt, nullptr, hack_thread, nullptr);
    
    return JNI_VERSION_1_6;
}
