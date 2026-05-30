#include <windows.h>
#include <vector>
#include <cmath>
#include <ctime>
#include <algorithm>

// Твої лінкери
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "winmm.lib")

struct Bubble {
    float x, y, radius;
    float speedX, speedY;
    COLORREF color;

    Bubble(int screenWidth, int screenHeight) {
        radius = (rand() % 30) + 20;
        x = rand() % screenWidth;
        y = screenHeight + radius;
        speedX = ((rand() % 100) - 50) / 50.0f;
        speedY = -((rand() % 100) + 50) / 40.0f; // Рух вгору
        color = RGB(135, 206, 235); // Aero Blue
    }

    void update() {
        x += speedX;
        y += speedY;
    }
};

std::vector<Bubble> bubbles;
bool running = true;

// Обробка подій вікна
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_LBUTTONDOWN: {
            int mouseX = LOWORD(lParam);
            int mouseY = HIWORD(lParam);

            // Перевірка на натискання бульбашки (зворотний цикл)
            for (int i = bubbles.size() - 1; i >= 0; --i) {
                float dx = mouseX - bubbles[i].x;
                float dy = mouseY - bubbles[i].y;
                float distance = std::sqrt(dx * dx + dy * dy);

                if (distance <= bubbles[i].radius) {
                    bubbles.erase(bubbles.begin() + i);
                    break; // Видаляємо одну за клік
                }
            }
            return 0;
        }
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) running = false;
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            running = false;
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    srand(time(0));

    // Налаштування повного екрану
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = L"BubbleClass";
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(WS_EX_TOPMOST, L"BubbleClass", L"Aero Bubbles", 
                               WS_POPUP | WS_VISIBLE, 0, 0, screenWidth, screenHeight, 
                               NULL, NULL, hInstance, NULL);

    HDC hdc = GetDC(hwnd);
    HDC hMemDC = CreateCompatibleDC(hdc);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdc, screenWidth, screenHeight);
    SelectObject(hMemDC, hBitmap);

    MSG msg = {0};
    while (running) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Спавн нових бульбашок
        if (rand() % 15 == 0) {
            bubbles.emplace_back(screenWidth, screenHeight);
        }

        // Очищення фону (темно-синій "космічний")
        HBRUSH bgBrush = CreateSolidBrush(RGB(10, 20, 40));
        RECT rect = {0, 0, screenWidth, screenHeight};
        FillRect(hMemDC, &rect, bgBrush);
        DeleteObject(bgBrush);

        // Оновлення та малювання бульбашок
        for (int i = 0; i < bubbles.size(); ++i) {
            bubbles[i].update();

            // Малювання "Aero" стилю
            HBRUSH hBrush = CreateSolidBrush(bubbles[i].color);
            HPEN hPen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255)); // Біла окантовка
            SelectObject(hMemDC, hBrush);
            SelectObject(hMemDC, hPen);

            Ellipse(hMemDC, bubbles[i].x - bubbles[i].radius, bubbles[i].y - bubbles[i].radius,
                            bubbles[i].x + bubbles[i].radius, bubbles[i].y + bubbles[i].radius);

            DeleteObject(hBrush);
            DeleteObject(hPen);
        }

        // Видалення бульбашок, що вилетіли за екран
        bubbles.erase(std::remove_if(bubbles.begin(), bubbles.end(),
            [](const Bubble& b) { return b.y + b.radius < 0; }), bubbles.end());

        // Копіювання з буфера на екран (Double Buffering щоб не мерехтіло)
        BitBlt(hdc, 0, 0, screenWidth, screenHeight, hMemDC, 0, 0, SRCCOPY);
        
        Sleep(10); // Обмеження FPS
    }

    ReleaseDC(hwnd, hdc);
    return 0;
}