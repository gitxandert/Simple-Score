// Simple Score.cpp : Defines the entry point for the application.
//

#include <windows.h>
#include <CommCtrl.h>
#include <gdiplus.h>
#include <iostream>
#include <filesystem>
#include <vector>
#include "Simple Score.h"
#pragma comment(lib, "Gdiplus.lib")

using namespace Gdiplus;

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

RECT clientRect;                                // client rect to create compatible bitmap for double buffering
int clientWidth{ 1000 };                        // variable to store width of (resized) screen
int clientHeight{ 800 };                        // variable to store height of (resized) screen
float scaleX;
float scaleY;

SCORE score{};                                  // class for holding and drawing standard music notation
DRAW_SHAPES drawShapes{};                       // class for holding and drawing shapes
HBITMAP hBitmap{ NULL };                        // bitmap for double buffering
HDC memDC{ NULL };                              // memory device context for double buffering
HWND gWindow{ NULL };                           // global window for main window
HWND hSymbolsDialog{ NULL };                    // global window for modeless symbols dialog
HWND hPaletteDialog{ NULL };                    // global window for modeless palette dialog
HWND hTextDialog{ NULL };                       // global window for modeless text dialog

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    Palette(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    Symbols(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    TextDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

void                LoadFontsIntoComboBox(HWND hComboBox);
void                updatePalette(Shape* shape);  // updates palette when selecting shapes



void loadLelandFont()
{
    std::filesystem::path fontPath{ std::filesystem::current_path() / L"Leland.otf" };

    DWORD result{ static_cast<DWORD>(AddFontResourceEx(fontPath.c_str(), FR_PRIVATE, nullptr)) };
    if (result == 0)
    {
        DWORD lastError = GetLastError();
        std::wstringstream errorMsg;
        errorMsg << L"Failed to load Leland font. Error Code: " << lastError;
        MessageBox(nullptr, errorMsg.str().c_str(), L"Error", MB_OK | MB_ICONERROR);
    }
    else
    {
        score.setFont();
    }
}

void unloadLelandFont()
{
    std::filesystem::path fontPath{ std::filesystem::current_path() / L"Leland.otf" };
    RemoveFontResourceEx(fontPath.c_str(), FR_PRIVATE, nullptr);
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    SetProcessDPIAware();

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    //initialize gdiplus
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    loadLelandFont();

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_SIMPLESCORE, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SIMPLESCORE));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    unloadLelandFont();
    drawShapes.deleteShapes();
    score.deleteScore();
    GdiplusShutdown(gdiplusToken);

    return (int) msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SIMPLESCORE));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_SIMPLESCORE);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   gWindow = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, clientWidth, clientHeight, nullptr, nullptr, hInstance, nullptr);

   if (!gWindow)
   {
      return FALSE;
   }

   ShowWindow(gWindow, nCmdShow);
   UpdateWindow(gWindow);

   return TRUE;
}

static void showPalette(HWND& hWnd)
{
    if (!hPaletteDialog) // Check if the dialog is already open
    {
        hPaletteDialog = CreateDialog(hInst, MAKEINTRESOURCE(IDD_PALETTE), hWnd, Palette);
        ShowWindow(hPaletteDialog, SW_SHOW);
    }
}

static void hidePalette(HWND& hWnd)
{
    if (hPaletteDialog) // Check if the dialog is already open
    {
        ShowWindow(hPaletteDialog, SW_HIDE);
        hPaletteDialog = NULL;
    }
}

static void showSymbols(HWND& hWnd)
{
    if (!hSymbolsDialog) // Check if the dialog is already open
    {
        hSymbolsDialog = CreateDialog(hInst, MAKEINTRESOURCE(IDD_SYMBOLS), hWnd, Symbols);
        ShowWindow(hSymbolsDialog, SW_SHOW);
    }
}

static void hideSymbols(HWND& hWnd)
{
    if (hSymbolsDialog) // Check if the dialog is already open
    {
        ShowWindow(hSymbolsDialog, SW_HIDE);
        hSymbolsDialog = NULL;
    }
}

static void showTextDlg(HWND& hWnd)
{
    if (!hTextDialog)
    {
        hTextDialog = CreateDialog(hInst, MAKEINTRESOURCE(IDD_TXTDLG), hWnd, TextDialog);
        ShowWindow(hTextDialog, SW_SHOW);
    }
}

static void hideTextDlg(HWND& hWnd)
{
    if (hTextDialog)
    {
        ShowWindow(hTextDialog, SW_HIDE);
        hTextDialog = NULL;
    }
}


//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        // Parse the menu selections:
        switch (wmId)
        {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case ID_SCORE_MEASURE:
            hidePalette(hWnd);
            hideTextDlg(hWnd);
            score.setElement(SCORE::MEASURE);
            drawShapes.setShape(DRAW_SHAPES::NONE);
            drawShapes.unSelect();
            break;
        case ID_SCORE_SYMBOLS:
            hidePalette(hWnd);
            hideTextDlg(hWnd);
            showSymbols(hWnd);
            score.setElement(SCORE::SYMBOL);
            drawShapes.setShape(DRAW_SHAPES::NONE);
            drawShapes.unSelect();
            break;
        case ID_SCORE_TEXT:
            hidePalette(hWnd);
            hideSymbols(hWnd);
            showTextDlg(hWnd);
            score.setElement(SCORE::TEXT);
            drawShapes.setShape(DRAW_SHAPES::NONE);
            drawShapes.unSelect();
            break;
        case ID_SCORE_SELECT:
            hidePalette(hWnd);
            hideTextDlg(hWnd);
            showSymbols(hWnd);
            score.setElement(SCORE::SELECT);
            drawShapes.setShape(DRAW_SHAPES::NONE);
            drawShapes.unSelect();
            break;
        case ID_DRAW_LINE:
            hideSymbols(hWnd);
            hideTextDlg(hWnd);
            showPalette(hWnd);
            drawShapes.setShape(DRAW_SHAPES::LINE);
            drawShapes.unSelect();
            score.setElement(SCORE::NONE);
            break;
        case ID_DRAW_ELLIPSE:
            hideSymbols(hWnd);
            hideTextDlg(hWnd);
            showPalette(hWnd);
            drawShapes.setShape(DRAW_SHAPES::ELLIPSE);
            drawShapes.unSelect();
            score.setElement(SCORE::NONE);
            break;
        case ID_DRAW_RECTANGLE:
            hideSymbols(hWnd);
            hideTextDlg(hWnd);
            showPalette(hWnd);
            drawShapes.setShape(DRAW_SHAPES::RECT);
            drawShapes.unSelect();
            score.setElement(SCORE::NONE);
            break;
        case ID_DRAW_TRIANGLE:
            hideSymbols(hWnd);
            hideTextDlg(hWnd);
            showPalette(hWnd);
            drawShapes.setShape(DRAW_SHAPES::TRIANGLE);
            drawShapes.unSelect();
            score.setElement(SCORE::NONE);
            break;
        case ID_DRAW_SKETCH:
            hideSymbols(hWnd);
            hideTextDlg(hWnd);
            showPalette(hWnd);
            drawShapes.setShape(DRAW_SHAPES::SKETCH);
            drawShapes.unSelect();
            score.setElement(SCORE::NONE);
            break;
        case ID_DRAW_SELECT:
            hideSymbols(hWnd);
            hideTextDlg(hWnd);
            showPalette(hWnd);
            drawShapes.setShape(DRAW_SHAPES::SELECT);
            score.setElement(SCORE::NONE);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }

        InvalidateRect(hWnd, NULL, TRUE);

        break;
    }
    case WM_LBUTTONDOWN:
    {
        // get mouse position
        int x{ LOWORD(lParam) };
        int y{ HIWORD(lParam) };

        if (drawShapes.getShape() != DRAW_SHAPES::NONE)
        {
            if (drawShapes.getShape() == DRAW_SHAPES::SELECT)
            {
                // check if a shape is selected, or start drawing select rect
                drawShapes.selectShape(x, y);
                if (!drawShapes.isSelecting())
                {
                    updatePalette(drawShapes.getSelected());
                }
            }
            else
            {
                // set drawing state to true
                drawShapes.setDrawing(true);
            }

            // save current mouse coordinates as origin (corner, vertex, center, etc.) of shape 
            drawShapes.setOrigin(PointF(static_cast<REAL>(x), static_cast<REAL>(y)));

            // save mouse position also as "current" to abstract width and height
            drawShapes.setCurrent(PointF(static_cast<REAL>(x), static_cast<REAL>(y)));
        }
        else if (score.getElement() != SCORE::NONE)
        {
            if (score.getElement() == SCORE::SELECT)
            {
                score.selectElement(x, y);
            }
            else
            {
                score.setDrawing(true);
            }

            score.setOrigin(PointF(static_cast<REAL>(x), static_cast<REAL>(y)));
            score.setCurrent(PointF(static_cast<REAL>(x), static_cast<REAL>(y)));
        }

        InvalidateRect(hWnd, NULL, TRUE);

        break;
    }
    case WM_MOUSEMOVE:
        if (drawShapes.isDrawing())
        {
            // get mouse position
            int x{ LOWORD(lParam) };
            int y{ HIWORD(lParam) };

            // save mouse position as "current" to abstract width and height
            drawShapes.setCurrent(PointF(static_cast<REAL>(x), static_cast<REAL>(y)));

            InvalidateRect(hWnd, NULL, TRUE);
        }
        else if (drawShapes.isMoving())
        {
            // get mouse position
            int x{ LOWORD(lParam) };
            int y{ HIWORD(lParam) };

            // compare mouse position with shape position
            drawShapes.moveShape(x, y);

            InvalidateRect(hWnd, NULL, TRUE);
        }
        else if (drawShapes.isSelecting())
        {
            // get mouse position
            int x{ LOWORD(lParam) };
            int y{ HIWORD(lParam) };

            // save mouse position as "current" to abstract width and height of select rect
            drawShapes.setCurrent(PointF(static_cast<REAL>(x), static_cast<REAL>(y)));

            InvalidateRect(hWnd, NULL, TRUE);
        }
        else if (score.isDrawing())
        {
            // get mouse position
            int x{ LOWORD(lParam) };
            int y{ HIWORD(lParam) };

            // save mouse position as "current" to abstract width and height of select rect
            score.setCurrent(PointF(static_cast<REAL>(x), static_cast<REAL>(y)));

            InvalidateRect(hWnd, NULL, TRUE);
        }
        else if (score.isMoving())
        {
            // get mouse position
            int x{ LOWORD(lParam) };
            int y{ HIWORD(lParam) };

            // compare mouse position with element position
            score.moveElement(x, y);

            InvalidateRect(hWnd, NULL, TRUE);
        }
        else if (score.isSelecting())
        {
            // get mouse position
            int x{ LOWORD(lParam) };
            int y{ HIWORD(lParam) };

            // save mouse position as "current" to abstract width and height of select rect
            score.setCurrent(PointF(static_cast<REAL>(x), static_cast<REAL>(y)));

            InvalidateRect(hWnd, NULL, TRUE);
        }

        break;
    case WM_LBUTTONUP:
        if (drawShapes.isMoving())
        {
            drawShapes.dropShape();
        }
        else if (drawShapes.isSelecting())
        {
            // get mouse position
            int x{ LOWORD(lParam) };
            int y{ HIWORD(lParam) };

            // save mouse position as "current" to abstract width and height
            drawShapes.setCurrent(PointF(static_cast<REAL>(x), static_cast<REAL>(y)));

            drawShapes.selectShapes();

            drawShapes.stopSelecting();

            InvalidateRect(hWnd, NULL, TRUE);
        }
        else if (drawShapes.isDrawing())
        {
            // get mouse position
            int x{ LOWORD(lParam) };
            int y{ HIWORD(lParam) };

            // save mouse position as "current" to abstract width and height
            drawShapes.setCurrent(PointF(static_cast<REAL>(x), static_cast<REAL>(y)));

            // store currently drawn shape
            switch (drawShapes.getShape())
            {
            case DRAW_SHAPES::LINE:
                drawShapes.addShape(std::make_shared<LineShape>(PointF(drawShapes.getOX(), drawShapes.getOY()), PointF(drawShapes.getCX(), drawShapes.getCY()), drawShapes.getColor(), drawShapes.getWidth()));
                drawShapes.setSelect();
                break;
            case DRAW_SHAPES::RECT:
                drawShapes.addShape(std::make_shared<RectShape>(PointF(drawShapes.getOX(), drawShapes.getOY()), drawShapes.getCX() - drawShapes.getOX(), drawShapes.getCY() - drawShapes.getOY(), drawShapes.getColor(), drawShapes.getWidth(), drawShapes.getFillMode()));
                drawShapes.setSelect();
                break;
            case DRAW_SHAPES::ELLIPSE:
                drawShapes.addShape(std::make_shared<EllipseShape>(PointF(drawShapes.getOX(), drawShapes.getOY()), drawShapes.getCX() - drawShapes.getOX(), drawShapes.getCY() - drawShapes.getOY(), drawShapes.getColor(), drawShapes.getWidth(), drawShapes.getFillMode()));
                drawShapes.setSelect();
                break;
            case DRAW_SHAPES::TRIANGLE:
                drawShapes.addShape(std::make_shared<TriShape>(PointF(drawShapes.getCX(), drawShapes.getCY()), PointF(drawShapes.getOX(), drawShapes.getOY()), PointF(drawShapes.getCX(), drawShapes.getOY()), drawShapes.getColor(), drawShapes.getWidth(), drawShapes.getFillMode()));
                drawShapes.setSelect();
                break;
            case DRAW_SHAPES::SKETCH:
                drawShapes.addSketch();
                drawShapes.setSelect();
                break;
            }

            drawShapes.setDrawing(false);

            InvalidateRect(hWnd, NULL, TRUE);
        }
        else if (score.isMoving())
        {
            score.dropElement();
        }
        else if (score.isSelecting())
        {
            // get mouse position
            int x{ LOWORD(lParam) };
            int y{ HIWORD(lParam) };

            // save mouse position as "current" to abstract width and height
            score.setCurrent(PointF(static_cast<REAL>(x), static_cast<REAL>(y)));

            score.selectElements();

            score.stopSelecting();

            InvalidateRect(hWnd, NULL, TRUE);

        }
        else if (score.isDrawing())
        {
            // get mouse position
            int x{ LOWORD(lParam) };
            int y{ HIWORD(lParam) };

            // save mouse position as "current" to abstract width and height
            score.setCurrent(PointF(static_cast<REAL>(x), static_cast<REAL>(y)));

            // store currently drawn shape
            switch (score.getElement())
            {
            case SCORE::MEASURE:
                score.addElement(std::make_shared<Measure>(score.getOrigin(), score.getCurrent().X - score.getOrigin().X));
                score.setSelect();
                break;
            case SCORE::SYMBOL:
                score.addElement(std::make_shared<Symbol>(score.getOrigin(), score.getCurrent().Y - score.getOrigin().Y, score.getSymbol()));
                score.setSelect();
                break;
            default:
                break;
            }

            score.setDrawing(false);

            InvalidateRect(hWnd, NULL, TRUE);
        }

        break;

    case WM_KEYDOWN:
        if (drawShapes.getShape() == DRAW_SHAPES::SELECT)
        {
            switch (wParam)
            {
            case VK_LEFT:
                drawShapes.moveShapes(DRAW_SHAPES::LEFT);
                break;
            case VK_RIGHT:
                drawShapes.moveShapes(DRAW_SHAPES::RIGHT);
                break;
            case VK_UP:
                drawShapes.moveShapes(DRAW_SHAPES::UP);
                break;
            case VK_DOWN:
                drawShapes.moveShapes(DRAW_SHAPES::DOWN);
                break;
            }

            InvalidateRect(gWindow, NULL, TRUE);
        }
        else if (score.getElement() == SCORE::SELECT)
        {
            switch (wParam)
            {
            case VK_LEFT:
                score.moveElements(SCORE::LEFT);
                break;
            case VK_RIGHT:
                score.moveElements(SCORE::RIGHT);
                break;
            case VK_UP:
                score.moveElements(SCORE::UP);
                break;
            case VK_DOWN:
                score.moveElements(SCORE::DOWN);
                break;
            }

            InvalidateRect(gWindow, NULL, TRUE);
        }

        break;

    case WM_ERASEBKGND:
        return 1;

    case WM_CREATE:
    {
        // variables for retrieving DPI
        int dpiX, dpiY;

        // set memory DC
        HDC hdc{ GetDC(hWnd) };
        memDC = CreateCompatibleDC(hdc);

        // get dpi settings for scale
        dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
        dpiY = GetDeviceCaps(hdc, LOGPIXELSY);

        // scale according to dpi divided by assumed resolution
        scaleX = dpiX / 96.0f;
        scaleY = dpiY / 96.0f;

        ReleaseDC(hWnd, hdc);

        // create compatible bitmap with client size
        if (hBitmap) DeleteObject(hBitmap);
        hBitmap = CreateCompatibleBitmap(GetDC(hWnd), clientWidth * scaleX, clientHeight * scaleY);
        SelectObject(memDC, hBitmap);

        break;
    }
    case WM_SIZE:
        // get new client area size
        GetClientRect(hWnd, &clientRect);
        clientWidth = LOWORD(lParam);
        clientHeight = HIWORD(lParam);

        // update compatible bitmap with new client size
        if (hBitmap) DeleteObject(hBitmap);
        hBitmap = CreateCompatibleBitmap(GetDC(hWnd), clientWidth * scaleX, clientHeight * scaleY);
        SelectObject(memDC, hBitmap);

        break;

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            // draw onto the memory DC with GDI+
            Graphics graphics(memDC);
            graphics.SetSmoothingMode(SmoothingModeAntiAlias);
            graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);
            graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
            graphics.Clear(Color(255, 255, 255));

            // draw stored shapes
            drawShapes.drawStoredShapes(graphics);

            // draw current shape, if left mouse button is depressed while in a shape mode
            if (drawShapes.isDrawing())
            {
                drawShapes.drawCurrentShape(graphics);
            }

            score.drawStoredElements(graphics);

            if (score.isDrawing())
            {
                score.drawCurrentElement(graphics);
            }

            if (drawShapes.getShape() == DRAW_SHAPES::SELECT)
            {
                drawShapes.selectRect(graphics);
            }
            else if (score.getElement() == SCORE::SELECT)
            {
                score.selectRect(graphics);
            }

            // blit memory DC to main DC
            BitBlt(hdc, 0, 0, ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top, memDC, 0, 0, SRCCOPY);

            EndPaint(hWnd, &ps);
        }

        break;

    case WM_DESTROY:
        // clean up resources
        DeleteObject(hBitmap);
        DeleteDC(memDC);
        PostQuitMessage(0);

        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

void updatePalette(Shape* shape)
{
    SendMessage(GetDlgItem(hPaletteDialog, IDC_RSLIDER), TBM_SETPOS, TRUE, shape->getColor().GetRed());
    drawShapes.setR(shape->getColor().GetRed());

    SendMessage(GetDlgItem(hPaletteDialog, IDC_GSLIDER), TBM_SETPOS, TRUE, shape->getColor().GetGreen());
    drawShapes.setG(shape->getColor().GetGreen());

    SendMessage(GetDlgItem(hPaletteDialog, IDC_BSLIDER), TBM_SETPOS, TRUE, shape->getColor().GetBlue());
    drawShapes.setB(shape->getColor().GetBlue());

    SendMessage(GetDlgItem(hPaletteDialog, IDC_OSLIDER), TBM_SETPOS, TRUE, shape->getColor().GetAlpha());
    drawShapes.setAlpha(shape->getColor().GetAlpha());

    SendMessage(GetDlgItem(hPaletteDialog, IDC_TSLIDER), TBM_SETPOS, TRUE, shape->getWidth());
    drawShapes.setWidth(shape->getWidth());


    SendMessage(GetDlgItem(hPaletteDialog, IDC_FILLBOX), BM_SETCHECK, shape->isFilled() ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(GetDlgItem(hPaletteDialog, IDC_LOCKBOX), BM_SETCHECK, shape->isLocked() ? BST_CHECKED : BST_UNCHECKED, 0);

    InvalidateRect(hPaletteDialog, NULL, TRUE);
}

// message handler for palette
INT_PTR CALLBACK Palette(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        HWND oSlider = GetDlgItem(hwndDlg, IDC_OSLIDER); // initialize opacity slider values
        SendMessage(oSlider, TBM_SETRANGE, TRUE, MAKELPARAM(0, 255)); // Set range from 0 to 255
        SendMessage(oSlider, TBM_SETPOS, TRUE, 255); // Set initial position to 255 (fully opaque)

        HWND rSlider = GetDlgItem(hwndDlg, IDC_RSLIDER); // initialize red slider values
        SendMessage(rSlider, TBM_SETRANGE, TRUE, MAKELPARAM(0, 255)); // Set range from 0 to 255
        SendMessage(rSlider, TBM_SETPOS, TRUE, 0); // Set initial position to 0

        HWND gSlider = GetDlgItem(hwndDlg, IDC_GSLIDER); // initialize green slider values
        SendMessage(gSlider, TBM_SETRANGE, TRUE, MAKELPARAM(0, 255)); // Set range from 0 to 255
        SendMessage(gSlider, TBM_SETPOS, TRUE, 0); // Set initial position to 0

        HWND bSlider = GetDlgItem(hwndDlg, IDC_BSLIDER); // initialize blue slider values
        SendMessage(bSlider, TBM_SETRANGE, TRUE, MAKELPARAM(0, 255)); // Set range from 0 to 255
        SendMessage(bSlider, TBM_SETPOS, TRUE, 0); // Set initial position to 0

        return (INT_PTR)TRUE;
    }
    case WM_DRAWITEM:
    {
        LPDRAWITEMSTRUCT dis{ (LPDRAWITEMSTRUCT)lParam };
        HDC hdc = dis->hDC;
        RECT rect = dis->rcItem;

        if (dis->CtlID == IDC_CCOLOR)
        {
            FillRect(hdc, &rect, drawShapes.getBrush());
            
            return TRUE;
        }

        break;

    }
    case WM_HSCROLL:
    {
        // Get the current position of the slider
        int position = SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);

        if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_TSLIDER))
        {
            // Set width of currently-drawn shape
            drawShapes.setWidth(position);
            drawShapes.setSelectWidth();

            InvalidateRect(HWND(GetDlgItem(hwndDlg, IDC_CCOLOR)), NULL, TRUE);
            InvalidateRect(gWindow, NULL, TRUE);
        }
        else if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_OSLIDER))
        {
            // Set alpha value of currently-drawn color
            drawShapes.setAlpha(uint8_t(position));
            drawShapes.setSelectColor();

            InvalidateRect(HWND(GetDlgItem(hwndDlg, IDC_CCOLOR)), NULL, TRUE);
            InvalidateRect(gWindow, NULL, TRUE);
        }
        else if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_RSLIDER))
        {
            // Set red value of currently-drawn color
            drawShapes.setR(uint8_t(position));
            drawShapes.setSelectColor();

            InvalidateRect(HWND(GetDlgItem(hwndDlg, IDC_CCOLOR)), NULL, TRUE);
            InvalidateRect(gWindow, NULL, TRUE);
        }
        else if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_GSLIDER))
        {
            // Set green value of currently-drawn color
            drawShapes.setG(uint8_t(position));
            drawShapes.setSelectColor();

            InvalidateRect(HWND(GetDlgItem(hwndDlg, IDC_CCOLOR)), NULL, TRUE);
            InvalidateRect(gWindow, NULL, TRUE);
        }
        else if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_BSLIDER))
        {
            // Set blue value of currently-drawn color
            drawShapes.setB(uint8_t(position));
            drawShapes.setSelectColor();

            InvalidateRect(HWND(GetDlgItem(hwndDlg, IDC_CCOLOR)), NULL, TRUE);
            InvalidateRect(gWindow, NULL, TRUE);
        }

        return TRUE;
    }

    case WM_COMMAND:
    {
        if (LOWORD(wParam) == IDC_FILLBOX) // Check the ID of your checkbox
        {
            if (HIWORD(wParam) == BN_CLICKED) // Check if the event is a button click
            {
                // The checkbox state can be queried here
                HWND hCheckBox = (HWND)lParam; // lParam contains the handle to the checkbox
                int state = SendMessage(hCheckBox, BM_GETCHECK, 0, 0);

                if (state == BST_CHECKED) {
                    drawShapes.setFillMode(TRUE);
                    drawShapes.setSelectFill();
                }
                else
                {
                    drawShapes.setFillMode(FALSE);
                    drawShapes.setSelectFill();
                }

                InvalidateRect(gWindow, NULL, TRUE);
            }
        }
        else if (LOWORD(wParam) == IDC_LOCKBOX)
        {
            if (HIWORD(wParam) == BN_CLICKED)
            {
                // query checkbox state
                HWND hLockBox{ (HWND)lParam };
                int state{ static_cast<int>(SendMessage(hLockBox, BM_GETCHECK, 0, 0)) };

                if (state == BST_CHECKED)
                {
                    drawShapes.lockSelectedShape();
                }
                else
                {
                    drawShapes.unlockSelectedShape();
                }
            }
        }
        else if (LOWORD(wParam) == IDC_POPSHAPE)
        {
            drawShapes.popUp();
            InvalidateRect(gWindow, NULL, TRUE);
        }
        else if (LOWORD(wParam) == IDC_PUSHSHAPE)
        {
            drawShapes.pushDown();
            InvalidateRect(gWindow, NULL, TRUE);
        }
        else if (LOWORD(wParam) == IDC_DELETE)
        {
           drawShapes.removeShape();
           InvalidateRect(gWindow, NULL, TRUE);
        }
        break;
    }
    case WM_CLOSE:
        hPaletteDialog = NULL; // Reset the global handle
        DestroyWindow(hwndDlg); // Destroy the dialog
        EndDialog(hwndDlg, 0);
        return TRUE;
    }
    return (INT_PTR)FALSE;
}

// message handler for symbols dialog
INT_PTR CALLBACK Symbols(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        HWND oSlider = GetDlgItem(hwndDlg, IDC_SIZE); // initialize size slider values
        SendMessage(oSlider, TBM_SETRANGE, TRUE, MAKELPARAM(0, 500)); // Set range from 0 to 255
        SendMessage(oSlider, TBM_SETPOS, TRUE, 0); // Set initial position to 0

        HWND symbols = GetDlgItem(hwndDlg, IDC_SYMBOLS);
        SendMessage(symbols, WM_SETFONT, (WPARAM)score.getLeland(), TRUE);

        score.setGlyphs(symbols);

        SendMessage(symbols, CB_RESETCONTENT, 0, 0);
        InvalidateRect(symbols, NULL, TRUE);
        UpdateWindow(symbols);
        return (INT_PTR)TRUE;
    }
    case WM_DRAWITEM:
    {
        LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lParam;
        if (dis->CtlType == ODT_LISTBOX)
        {
            HDC hdc = dis->hDC;
            RECT rc = dis->rcItem;

            // Set background color
            FillRect(hdc, &rc, (HBRUSH)(COLOR_WINDOW + 1));

            // Set text color
            SetTextColor(hdc, RGB(0, 0, 0)); // Black

            // Draw the item text (glyph)
            WCHAR glyph[2] = { score.getGlyphs()[dis->itemID][0], L'\0' }; // Null-terminated string
            DrawText(hdc, glyph, -1, &rc, DT_VCENTER | DT_CENTER | DT_NOPREFIX);
        }
        return TRUE;
    }
    case WM_HSCROLL:
        if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_SIZE))
        {
            // Get the current position of the slider
            int position = SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
            // Set size of currently-drawn text
            score.setSize(position);

            InvalidateRect(gWindow, NULL, TRUE);
        }
        break;

    case WM_COMMAND:
    {
        HWND symbols = GetDlgItem(hwndDlg, IDC_SYMBOLS);
        int wmId = LOWORD(wParam);

        switch (wmId)
        {
        case IDC_SYMBOLS: // Assuming this is the ID for your symbols list
            if (HIWORD(wParam) == CBN_SELCHANGE) // Check for selection change
            {
                int index = SendMessage(symbols, LB_GETCURSEL, 0, 0);
                score.setSymbol(index);
            }
            break;
        case IDC_LOCKSYM:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                // query checkbox state
                HWND hLockBox{ (HWND)lParam };
                int state{ static_cast<int>(SendMessage(hLockBox, BM_GETCHECK, 0, 0)) };

                if (state == BST_CHECKED)
                {
                    score.lockElement();
                }
                else
                {
                    score.unlockElement();
                }
            }
            break;

        case IDC_DELETESYM:
            score.removeElement();
            InvalidateRect(gWindow, NULL, TRUE);
            break;

        default:
            break;
        }

        break;
    }
    case WM_CLOSE:
        hSymbolsDialog = NULL; // Reset the global handle
        DestroyWindow(hwndDlg); // Destroy the dialog
        EndDialog(hwndDlg, 0);
        return TRUE;
    }
    return (INT_PTR)FALSE;
}

//code for text loading

INT_PTR CALLBACK TextDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
    {
        // Get the combo box handle
        HWND hFontBox = GetDlgItem(hDlg, IDC_FONTBOX);

        SendMessage(hFontBox, CB_SETDROPPEDWIDTH, 200, 600);

        // Load the fonts into the combo box
        LoadFontsIntoComboBox(hFontBox);
        return (INT_PTR)TRUE;
    }

    case WM_COMMAND:
        break;

    case WM_CLOSE:
        hTextDialog = NULL; // Reset the global handle
        DestroyWindow(hDlg); // Destroy the dialog
        EndDialog(hDlg, 0);
        return TRUE;
    }
    return (INT_PTR)FALSE;
}

int CALLBACK EnumFontFamExProc(
    const LOGFONT* lpelf,
    const TEXTMETRIC* lpntm,
    DWORD FontType,
    LPARAM lParam)
{
    auto* fonts = reinterpret_cast<std::vector<std::wstring>*>(lParam);
    fonts->emplace_back(lpelf->lfFaceName);
    return 1; // Continue enumeration
}

void LoadFontsIntoComboBox(HWND hComboBox)
{
    std::vector<std::wstring> fontNames;

    LOGFONT logFont = {};
    logFont.lfCharSet = DEFAULT_CHARSET;

    HDC hdc = GetDC(hComboBox); // Get the device context for the combo box

    EnumFontFamiliesEx(
        hdc,
        &logFont,
        EnumFontFamExProc,
        reinterpret_cast<LPARAM>(&fontNames),
        0);

    for (const auto& fontName : fontNames)
    {
        SendMessage(hComboBox, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(fontName.c_str()));
    }

    SendMessage(hComboBox, CB_SETCURSEL, 0, 0); // Optionally select the first item

    ReleaseDC(hComboBox, hdc); // Release the device context
}

