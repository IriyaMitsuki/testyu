cmake_minimum_required(VERSION 3.10)

project(MyUnityHack C CXX)

# Добавляем поддиректории с исходниками ImGui
# Это нужно, чтобы CMake нашел все .cpp файлы ImGui
add_subdirectory(includes/imgui)

# Создаем нашу библиотеку Tool2 (будет libTool2.so)
add_library(Tool2 SHARED main.cpp)

# Указываем, где искать заголовочные файлы
target_include_directories(Tool2 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/includes/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/includes/dobby
)

# Указываем, где искать скомпилированные библиотеки
target_link_directories(Tool2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/libs/${ANDROID_ABI})

# Линкуем Tool2 с другими библиотеками
target_link_libraries(Tool2
    # Линкуемся с ImGui, который мы добавили через add_subdirectory
    imgui
    # Наша прекомпилированная Dobby
    dobby
    # Стандартные библиотеки Android
    log      # для логов
    EGL      # для хука рендеринга
    GLESv3   # для рендеринга ImGui
    android
)
