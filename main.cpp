#include <windows.h>
#include <cmath>
#include <ctime>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "winmm.lib")

#define SAMPLE_RATE 8000     
#define BUFFER_SIZE 2500000   
#define PHASE_DURATION 30000   

typedef union _RGBQUAD_CUSTOM {
    COLORREF rgb;
    struct { BYTE r; BYTE g; BYTE b; BYTE Reserved; };
} RGBQUAD_CUSTOM, *PRGBQUAD_CUSTOM;

struct BouncingCircle {
    int x, y, vx, vy, size;
};

void FillBeatBuffer(unsigned char* buffer, int size, int phase) {
    for (int t = 0; t < size; t++) {
        int value;
        switch (phase % 16) {
            case 0:
                value = 5*t&t>>7|3*t&4*t>>10;
                break;
            case 1:
                value = t*(t&8192?7:5)*(6-(3&t>>8)+(3&t>>9))>>(3&-t>>(t&2048?2:11))|t>>3;
                break;
            case 2:
                value = (2*t&255)*(-t>>6&t>>7)>>8|t&t>>1|t&t>>1;
                break;
            case 3:
                value = t*((t>>11|t%16*t>>8)&t>>9&18)|-t/16+64;
                break;
            case 4:
                value = (t>>9^(t>>9)-1^1)%13*t;
                break;
            case 5:
                value = t*(3+(1^t>>10&5))*(5+(3&t>>14))>>(t>>8&3);
                break;
            case 6:
                value = 2*(t>>5&t)-(t>>5)+t*(t>>14&14);
                break;
            case 7:
                value = t*(t^t+(t>>15|1)^(t-1280^t)>>10);
                break;
            case 8:
                value = t*((t>>9|t>>13)&15)&129;
                break;
            case 9:
                value = (t>>6|t|t>>(t>>16))*10+((t>>11)&7);
                break;
            case 10:
                value = t*((t&4096?t%65536<59392?7:t>>6:16)+(1&t>>14))>>(3&-t>>(t&2048?2:10))|t>>(t&16384?t&4096?4:3:2);
                break;
            case 11:
                value = t*((t>>12|t>>8)&63&t>>4);
                break;
            case 12:
                value = t/8>>(t>>9)*t/((t>>14&3)+4);
                break;
            case 13:
                value = (t>>9^(t>>9)-1^1)%13*t;
                break;
            case 14:
                value = t*(t^t>>20*(t>>11));
                break;
            default:
                value = t*(t>>9|t>>13)&16;
                break;
        }
        buffer[t] = static_cast<unsigned char>(value);
    }
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PWSTR pCmdLine, _In_ int nShowCmd) {
    (void)hInstance; (void)hPrevInstance; (void)pCmdLine; (void)nShowCmd;
    srand(static_cast<unsigned int>(time(0)));

    if (MessageBoxW(NULL, L"SOLARIS: CLEANED EDITION.\n\nOnly original XOR fractal preserved.\n14 isolated phases loaded.\n\nPress [YES] to execute.", 
        L"Solaris.exe - Cleaned", MB_YESNO | MB_ICONWARNING) != IDYES) return 0;

    int w = GetSystemMetrics(SM_CXSCREEN);
    int h = GetSystemMetrics(SM_CYSCREEN);
    double color_timer = 0;
    int last_phase = -1;

    // Bouncing circles
    BouncingCircle circles[6];
    for (int i = 0; i < 6; i++) {
        circles[i].x = rand() % w;
        circles[i].y = rand() % h;
        circles[i].vx = (rand() % 20 - 10);
        circles[i].vy = (rand() % 20 - 10);
        circles[i].size = 50 + rand() % 100;
    }

    // --- ЗВУКОВИЙ РУШІЙ ---
    HWAVEOUT hWaveOut;
    WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, SAMPLE_RATE, SAMPLE_RATE, 1, 8, 0 };
    waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);

    unsigned char* buffer = new unsigned char[BUFFER_SIZE];
    FillBeatBuffer(buffer, BUFFER_SIZE, 0);

    WAVEHDR header;
    ZeroMemory(&header, sizeof(WAVEHDR));
    header.lpData = reinterpret_cast<LPSTR>(buffer);
    header.dwBufferLength = BUFFER_SIZE;
    header.dwFlags = WHDR_BEGINLOOP | WHDR_ENDLOOP;
    header.dwLoops = 0xFFFFFFFF; 

    waveOutPrepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
    waveOutWrite(hWaveOut, &header, sizeof(WAVEHDR));

    auto playPhaseBeat = [&](int phase) {
        waveOutReset(hWaveOut);
        waveOutUnprepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
        FillBeatBuffer(buffer, BUFFER_SIZE, phase);
        waveOutPrepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
        waveOutWrite(hWaveOut, &header, sizeof(WAVEHDR));
    };

    DWORD startTime = GetTickCount();

    // --- ГОЛОВНИЙ ЦИКЛ ---
    while (!(GetAsyncKeyState(VK_ESCAPE) & 0x8000)) {
        HDC hdc = GetDC(0);
        int r = rand() % 100;
        DWORD elapsed = GetTickCount() - startTime; 

        int current_phase = elapsed / PHASE_DURATION;

        // ЖОРСТКЕ ОЧИЩЕННЯ ПРИ ЗМІНІ ФАЗИ
        if (current_phase != last_phase) {
            RedrawWindow(NULL, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
            playPhaseBeat(current_phase);
            last_phase = current_phase;
        }

        color_timer += 0.15;
        COLORREF current_color = RGB((int)(127 + 127 * sin(color_timer)), (int)(127 + 127 * sin(color_timer + 2)), (int)(127 + 127 * sin(color_timer + 4)));

        // Update and draw bouncing circles
        for (int i = 0; i < 6; i++) {
            circles[i].x += circles[i].vx;
            circles[i].y += circles[i].vy;
            if (circles[i].x <= 0 || circles[i].x >= w) circles[i].vx = -circles[i].vx;
            if (circles[i].y <= 0 || circles[i].y >= h) circles[i].vy = -circles[i].vy;
            circles[i].x = (circles[i].x < 0) ? 0 : (circles[i].x > w) ? w : circles[i].x;
            circles[i].y = (circles[i].y < 0) ? 0 : (circles[i].y > h) ? h : circles[i].y;
            HBRUSH circleBrush = CreateSolidBrush(RGB(rand() % 255, rand() % 255, rand() % 255));
            SelectObject(hdc, circleBrush);
            Ellipse(hdc, circles[i].x - circles[i].size/2, circles[i].y - circles[i].size/2, circles[i].x + circles[i].size/2, circles[i].y + circles[i].size/2);
            DeleteObject(circleBrush);
        }

        if (r % 2 == 0) {
            POINT pt;
            if (GetCursorPos(&pt)) SetCursorPos(pt.x + (rand() % 61 - 30), pt.y + (rand() % 61 - 30));
        }

        // ==========================================================
        // БАЗОВІ ФАЗИ
        // ==========================================================
        if (current_phase == 0) {
            if (r < 60) StretchBlt(hdc, 15, 15, w - 30, h - 30, hdc, 0, 0, w, h, SRCCOPY);
        }
        else if (current_phase == 1) {
            if (r % 3 == 0) {
                HBRUSH br = CreateSolidBrush(current_color);
                HBRUSH oldBr = (HBRUSH)SelectObject(hdc, br);
                if (r % 2 == 0) Rectangle(hdc, rand()%w, rand()%h, rand()%w, rand()%h); else Ellipse(hdc, rand()%w, rand()%h, rand()%w, rand()%h);
                SelectObject(hdc, oldBr); DeleteObject(br);
            }
        }
        else if (current_phase == 2) {
            if (r % 2 == 0) { int tx = rand() % w; BitBlt(hdc, tx, 1, 10, h, hdc, tx, 0, SRCCOPY); }
        }
        else if (current_phase == 3) {
            if (r % 2 == 0) {
                SetBkMode(hdc, TRANSPARENT); SetTextColor(hdc, current_color);
                HFONT hFont = CreateFontW(rand() % 150 + 50, 0, rand() % 3600, rand() % 3600, FW_BLACK, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
                HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
                TextOutW(hdc, rand() % w, rand() % h, L"SOLARIS", 7);
                SelectObject(hdc, hOldFont); DeleteObject(hFont);
            }
        }
        else if (current_phase == 4) {
            if (r % 2 == 0) {
                int y = rand() % h; int wave_offset = (int)(sin((elapsed + y) * 0.005) * 35); 
                BitBlt(hdc, wave_offset, y, w, rand() % 20 + 5, hdc, 0, y, SRCCOPY);
            }
        }
        else if (current_phase == 5) {
            if (r % 2 == 0) { int melt_x = rand() % w; BitBlt(hdc, melt_x, rand() % 40 + 10, rand() % 120 + 30, h, hdc, melt_x, 0, SRCCOPY); }
        }
        // XOR Фрактал (ЗАЛИШАЄМО)
        else if (current_phase == 6) {
            if (r % 8 == 0) {
                BITMAPINFO bmi = { 0 }; bmi.bmiHeader.biSize = sizeof(BITMAPINFO); bmi.bmiHeader.biBitCount = 32; bmi.bmiHeader.biPlanes = 1; bmi.bmiHeader.biWidth = w; bmi.bmiHeader.biHeight = h;
                PRGBQUAD_CUSTOM rgbScreen = { 0 }; HDC hdcMem = CreateCompatibleDC(hdc); HBITMAP hbmTemp = CreateDIBSection(hdc, &bmi, NULL, (void**)&rgbScreen, NULL, NULL);
                SelectObject(hdcMem, hbmTemp); BitBlt(hdcMem, 0, 0, w, h, hdc, 0, 0, SRCCOPY);
                for (int i = 0; i < w * h; i+=3) { rgbScreen[i].rgb += (i % w) ^ (i / w); }
                BitBlt(hdc, 0, 0, w, h, hdcMem, 0, 0, SRCCOPY); DeleteObject(hbmTemp); DeleteDC(hdcMem);
            }
        }
        // ==========================================================
        // ІНШІ ЕФЕКТИ
        // ==========================================================
        else if (current_phase == 7) {
            // Фрактал та складні обчислення видалено.
            // Замінено на простий ефект інверсії кольорів.
            RECT rc = { 0, 0, w, h };
            InvertRect(hdc, &rc);
        }
        else if (current_phase == 8) {
            // Фрактал та складні обчислення видалено.
            // Замінено на просту заливку екрану кольором.
            HBRUSH hBrush = CreateSolidBrush(current_color);
            PatBlt(hdc, 0, 0, w, h, PATCOPY);
            DeleteObject(hBrush);
        }
        
        else if (current_phase == 9) {
            if (r % 1 == 0) { int rx = rand() % w; BitBlt(hdc, rx, 10, 100, h, hdc, rx, 0, SRCCOPY); }
        }
        else if (current_phase == 10) {
            if (r % 1 == 0) { BitBlt(hdc, 0, 0, w, h, hdc, -30, 0, SRCCOPY); BitBlt(hdc, 0, 0, w, h, hdc, w - 30, 0, SRCCOPY); }
        }
        else if (current_phase == 11) {
            if (r % 2 == 0) StretchBlt(hdc, -20, 0, w + 40, h, hdc, 0, 0, w, h, SRCCOPY);
        }
        else if (current_phase == 12) {
            if (r % 2 == 0) {
                int size = rand() % 600 + 200; int x = rand() % w, y = rand() % h;
                HRGN hrgn = CreateEllipticRgn(x - size/2, y - size/2, x + size/2, y + size/2);
                SelectClipRgn(hdc, hrgn); 
                BitBlt(hdc, x - size/2, y - size/2, size, size, hdc, x - size/2, y - size/2, NOTSRCCOPY);
                SelectClipRgn(hdc, NULL); DeleteObject(hrgn);
            }
        }
        // ==========================================================
        // ФІНАЛЬНА ФАЗА (MATVIJ)
        // ==========================================================
        else {
            if (r % 2 == 0) {
                SetBkMode(hdc, TRANSPARENT); SetTextColor(hdc, RGB(255, 0, 0)); 
                HFONT hFont = CreateFontW(rand() % 250 + 100, 0, rand() % 3600, rand() % 3600, FW_BLACK, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
                HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
                TextOutW(hdc, rand() % w, rand() % h, L"MATVIJ", 6);
                SelectObject(hdc, hOldFont); DeleteObject(hFont);
            }
            if (r % 2 == 0) {
                BitBlt(hdc, rand() % w, rand() % h, rand() % 600, rand() % 600, hdc, rand() % w, rand() % h, SRCINVERT);
            }
            if (r % 3 == 0) {
                int melt_x = rand() % w; BitBlt(hdc, melt_x, rand() % 50 + 20, rand() % 200 + 50, h, hdc, melt_x, 0, SRCCOPY);
            }
        }

        ReleaseDC(0, hdc);
        Sleep(5); 
    }

    waveOutReset(hWaveOut);
    waveOutUnprepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
    waveOutClose(hWaveOut);
    delete[] buffer;
    InvalidateRect(0, 0, TRUE);

    return 0;
}

// Provide WinMain/wWinMain so the project can link as a Windows (GUI) subsystem
// while keeping the main() entry point. This prevents the linker error
// "unresolved external symbol WinMain referenced in function invoke_main".
// main is now implemented as wWinMain; remove redundant wrappers.
