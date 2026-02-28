/*
 * dsky_gui.c -- Win32 GDI graphical DSKY backend
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port
 * Renders the DSKY as a graphical window using native Win32 GDI.
 */

#include "dsky_backend.h"

#ifdef _WIN32

#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include "dsky.h"
#include "dsky_gui.h"
#include "agc_cpu.h"

extern void pinball_keypress(int keycode);

#define REF_W  400
#define REF_H  650
#define REF_RES 950

#define COL_PANEL       RGB(43,43,43)
#define COL_DISP_BG     RGB(10,10,10)
#define COL_SEG_ON      RGB(0,230,0)
#define COL_SEG_OFF     RGB(0,28,0)
#define COL_LIGHT_ON    RGB(255,170,0)
#define COL_LIGHT_OFF   RGB(30,25,0)
#define COL_LABEL       RGB(200,200,200)
#define COL_BTN_FACE    RGB(80,80,80)
#define COL_BTN_DOWN    RGB(55,55,55)
#define COL_BTN_TEXT    RGB(220,220,220)
#define COL_SEP         RGB(60,60,60)

#define SEG_W_REF  16
#define SEG_H_REF  28
#define SEG_T_REF  3
#define DIG_SP_REF 22

#define SA 0x01
#define SB 0x02
#define SC 0x04
#define SD 0x08
#define SE 0x10
#define SF 0x20
#define SG 0x40

static const unsigned char seg_tab[10] = {
    SA|SB|SC|SD|SE|SF,    SB|SC,
    SA|SB|SD|SE|SG,       SA|SB|SC|SD|SG,
    SB|SC|SF|SG,          SA|SC|SD|SF|SG,
    SA|SC|SD|SE|SF|SG,    SA|SB|SC,
    SA|SB|SC|SD|SE|SF|SG, SA|SB|SC|SD|SF|SG
};

typedef struct { int x,y,w,h; const char *label; int kc; } gbtn_t;
#define NBTN 19
static const gbtn_t btns[NBTN] = {
    { 20,430,170,34,"VERB",   DSKY_KEY_VERB},
    {210,430,170,34,"NOUN",   DSKY_KEY_NOUN},
    { 20,472, 60,34,"+",      DSKY_KEY_PLUS},
    { 88,472, 60,34,"7",      DSKY_KEY_7},
    {156,472, 60,34,"8",      DSKY_KEY_8},
    {224,472, 60,34,"9",      DSKY_KEY_9},
    {292,472, 88,34,"CLR",    DSKY_KEY_CLR},
    { 20,514, 60,34,"-",      DSKY_KEY_MINUS},
    { 88,514, 60,34,"4",      DSKY_KEY_4},
    {156,514, 60,34,"5",      DSKY_KEY_5},
    {224,514, 60,34,"6",      DSKY_KEY_6},
    {292,514, 88,34,"PRO",    DSKY_KEY_PRO},
    { 20,556, 60,34,"0",      DSKY_KEY_0},
    { 88,556, 60,34,"1",      DSKY_KEY_1},
    {156,556, 60,34,"2",      DSKY_KEY_2},
    {224,556, 60,34,"3",      DSKY_KEY_3},
    {292,556, 88,34,"KEY REL",DSKY_KEY_KREL},
    { 20,598,170,34,"ENTR",   DSKY_KEY_ENTR},
    {210,598,170,34,"RSET",   DSKY_KEY_RSET}
};

static HWND g_hwnd = NULL;
static int g_running = 0;
static dsky_display_t g_prev;
static int g_dirty = 1;
static HFONT g_fnt_light, g_fnt_label, g_fnt_btn;
static int g_pressed_btn = -1;
static int g_sf_num = 1, g_sf_den = 1;
#define S(v) ((v)*g_sf_num/g_sf_den)
static int g_seg_w, g_seg_h, g_seg_t, g_dig_sp;
static gbtn_t sbtns[NBTN];

static void frect(HDC dc, int x, int y, int w, int h, COLORREF c)
{
    RECT r; HBRUSH br;
    r.left=x; r.top=y; r.right=x+w; r.bottom=y+h;
    br = CreateSolidBrush(c);
    FillRect(dc, &r, br);
    DeleteObject(br);
}

static void draw_seg(HDC dc, int x, int y, int digit)
{
    unsigned char s;
    COLORREF on=COL_SEG_ON, off=COL_SEG_OFF;
    int hw = g_seg_w - 2*g_seg_t;
    int hh = (g_seg_h - 3*g_seg_t) / 2;
    s = (digit>=0 && digit<=9) ? seg_tab[digit] : 0;
    frect(dc, x+g_seg_t, y,                           hw, g_seg_t, (s&SA)?on:off);
    frect(dc, x+g_seg_w-g_seg_t, y+g_seg_t,          g_seg_t, hh, (s&SB)?on:off);
    frect(dc, x+g_seg_w-g_seg_t, y+g_seg_t+hh+g_seg_t, g_seg_t, hh, (s&SC)?on:off);
    frect(dc, x+g_seg_t, y+g_seg_h-g_seg_t,          hw, g_seg_t, (s&SD)?on:off);
    frect(dc, x, y+g_seg_t+hh+g_seg_t,               g_seg_t, hh, (s&SE)?on:off);
    frect(dc, x, y+g_seg_t,                           g_seg_t, hh, (s&SF)?on:off);
    frect(dc, x+g_seg_t, y+g_seg_t+hh,               hw, g_seg_t, (s&SG)?on:off);
}

static void draw_sign(HDC dc, int x, int y, int sign)
{
    COLORREF on=COL_SEG_ON, off=COL_SEG_OFF;
    int cx=x+g_seg_w/2, cy=y+g_seg_h/2;
    frect(dc, cx-g_seg_w/2, cy-g_seg_t/2, g_seg_w, g_seg_t, sign?on:off);
    frect(dc, cx-g_seg_t/2, cy-g_seg_w/2, g_seg_t, g_seg_w, (sign>0)?on:off);
}

static void draw_light(HDC dc, int x, int y, int w, int h, int on, const char *lbl)
{
    COLORREF bg = on ? COL_LIGHT_ON : COL_LIGHT_OFF;
    RECT r; HFONT old;
    frect(dc, x, y, w, h, bg);
    r.left=x; r.top=y; r.right=x+w; r.bottom=y+h;
    FrameRect(dc, &r, (HBRUSH)GetStockObject(GRAY_BRUSH));
    old = (HFONT)SelectObject(dc, g_fnt_light);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, on ? RGB(0,0,0) : RGB(80,70,0));
    DrawTextA(dc, lbl, -1, &r, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    SelectObject(dc, old);
}

static void draw_label(HDC dc, int x, int y, const char *txt)
{
    HFONT old = (HFONT)SelectObject(dc, g_fnt_label);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, COL_LABEL);
    TextOutA(dc, x, y, txt, (int)strlen(txt));
    SelectObject(dc, old);
}

static void draw_btn(HDC dc, const gbtn_t *b, int pressed)
{
    RECT r; HFONT old; HBRUSH br;
    r.left=b->x; r.top=b->y; r.right=b->x+b->w; r.bottom=b->y+b->h;
    br = CreateSolidBrush(pressed ? COL_BTN_DOWN : COL_BTN_FACE);
    FillRect(dc, &r, br);
    DeleteObject(br);
    DrawEdge(dc, &r, pressed ? EDGE_SUNKEN : EDGE_RAISED, BF_RECT);
    old = (HFONT)SelectObject(dc, g_fnt_btn);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, COL_BTN_TEXT);
    DrawTextA(dc, b->label, -1, &r, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    SelectObject(dc, old);
}

static void paint_dc(HDC dc)
{
    int i, dx, dy;
    frect(dc, 0, 0, S(REF_W), S(REF_H), COL_PANEL);

    /* Status lights: 4 rows x 3 cols */
    draw_light(dc, S(20),S(12),S(108),S(26), dsky_display.light_uplink_acty,"UPLINK ACTY");
    draw_light(dc, S(140),S(12),S(108),S(26),dsky_display.light_temp,       "TEMP");
    draw_light(dc, S(260),S(12),S(108),S(26),dsky_display.light_prog_alarm, "PROG");
    draw_light(dc, S(20),S(44),S(108),S(26), dsky_display.light_gimbal_lock,"GIMBAL LOCK");
    draw_light(dc, S(140),S(44),S(108),S(26),dsky_display.light_stby,       "STBY");
    draw_light(dc, S(260),S(44),S(108),S(26),dsky_display.light_restart,    "RESTART");
    draw_light(dc, S(20),S(76),S(108),S(26), dsky_display.light_no_att,     "NO ATT");
    draw_light(dc, S(140),S(76),S(108),S(26),dsky_display.light_key_rel,    "KEY REL");
    draw_light(dc, S(260),S(76),S(108),S(26),dsky_display.light_tracker,    "TRACKER");
    draw_light(dc, S(20),S(108),S(108),S(26),dsky_display.light_opr_err,    "OPR ERR");
    draw_light(dc, S(140),S(108),S(108),S(26),dsky_display.light_vel,       "VEL");
    draw_light(dc, S(260),S(108),S(108),S(26),dsky_display.light_alt,       "ALT");

    /* Display area */
    frect(dc, S(15), S(148), S(368), S(240), COL_DISP_BG);

    /* COMP ACTY */
    draw_light(dc, S(22),S(155),S(88),S(22), dsky_display.light_comp_acty, "COMP ACTY");

    /* PROG */
    draw_label(dc, S(220),S(158), "PROG");
    draw_seg(dc, S(280),S(158), dsky_display.prog[0]);
    draw_seg(dc, S(280)+g_dig_sp,S(158), dsky_display.prog[1]);

    frect(dc, S(20),S(192), S(358),S(2), COL_SEP);

    /* VERB */
    draw_label(dc, S(25),S(200), "VERB");
    draw_seg(dc, S(80),S(200), dsky_display.verb[0]);
    draw_seg(dc, S(80)+g_dig_sp,S(200), dsky_display.verb[1]);

    /* NOUN */
    draw_label(dc, S(220),S(200), "NOUN");
    draw_seg(dc, S(280),S(200), dsky_display.noun[0]);
    draw_seg(dc, S(280)+g_dig_sp,S(200), dsky_display.noun[1]);

    frect(dc, S(20),S(234), S(358),S(2), COL_SEP);

    /* R1 */
    draw_label(dc, S(25),S(252), "R1");
    dx=S(70); dy=S(252);
    draw_sign(dc, dx, dy, dsky_display.r1_sign);
    for(i=0;i<5;i++) draw_seg(dc, dx+g_dig_sp+i*g_dig_sp, dy, dsky_display.r1[i]);

    /* R2 */
    draw_label(dc, S(25),S(298), "R2");
    dx=S(70); dy=S(298);
    draw_sign(dc, dx, dy, dsky_display.r2_sign);
    for(i=0;i<5;i++) draw_seg(dc, dx+g_dig_sp+i*g_dig_sp, dy, dsky_display.r2[i]);

    /* R3 */
    draw_label(dc, S(25),S(344), "R3");
    dx=S(70); dy=S(344);
    draw_sign(dc, dx, dy, dsky_display.r3_sign);
    for(i=0;i<5;i++) draw_seg(dc, dx+g_dig_sp+i*g_dig_sp, dy, dsky_display.r3[i]);

    frect(dc, S(15),S(420), S(368),S(2), COL_SEP);

    /* Keypad */
    for(i=0;i<NBTN;i++) draw_btn(dc, &sbtns[i], i==g_pressed_btn);
}

static int hit_btn(int mx, int my)
{
    int i;
    for(i=0;i<NBTN;i++){
        if(mx>=sbtns[i].x && mx<sbtns[i].x+sbtns[i].w &&
           my>=sbtns[i].y && my<sbtns[i].y+sbtns[i].h)
            return i;
    }
    return -1;
}

static void send_key(int kc)
{
    dsky_submit_key(kc);
}

static int map_char(int ch)
{
    switch(ch){
        case '0': return DSKY_KEY_0;
        case '1': return DSKY_KEY_1;
        case '2': return DSKY_KEY_2;
        case '3': return DSKY_KEY_3;
        case '4': return DSKY_KEY_4;
        case '5': return DSKY_KEY_5;
        case '6': return DSKY_KEY_6;
        case '7': return DSKY_KEY_7;
        case '8': return DSKY_KEY_8;
        case '9': return DSKY_KEY_9;
        case 'v': case 'V': return DSKY_KEY_VERB;
        case 'n': case 'N': return DSKY_KEY_NOUN;
        case '+': case '=': return DSKY_KEY_PLUS;
        case '-': case '_': return DSKY_KEY_MINUS;
        case 'e': case 'E': case '\r': case '\n':
            return DSKY_KEY_ENTR;
        case 'c': case 'C': return DSKY_KEY_CLR;
        case 'k': case 'K': return DSKY_KEY_KREL;
        case 'r': case 'R': return DSKY_KEY_RSET;
    }
    return -1;
}

static LRESULT CALLBACK wndproc(HWND hw, UINT msg, WPARAM wp, LPARAM lp)
{
    switch(msg){
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc, mem;
        HBITMAP bmp, obmp;
        RECT cr;
        hdc = BeginPaint(hw, &ps);
        GetClientRect(hw, &cr);
        mem = CreateCompatibleDC(hdc);
        bmp = CreateCompatibleBitmap(hdc, cr.right, cr.bottom);
        obmp = (HBITMAP)SelectObject(mem, bmp);
        paint_dc(mem);
        BitBlt(hdc, 0,0, cr.right, cr.bottom, mem, 0,0, SRCCOPY);
        SelectObject(mem, obmp);
        DeleteObject(bmp);
        DeleteDC(mem);
        EndPaint(hw, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_CHAR:
    {
        int ch = (int)wp;
        int kc;
        if(ch=='q'||ch=='Q'){ exit(0); }
        if(ch=='p'||ch=='P'){ dsky_submit_key(DSKY_KEY_PRO); return 0; }
        kc = map_char(ch);
        if(kc>=0) send_key(kc);
        return 0;
    }
    case WM_LBUTTONDOWN:
    {
        int idx = hit_btn(LOWORD(lp), HIWORD(lp));
        if(idx>=0){
            g_pressed_btn = idx;
            InvalidateRect(hw, NULL, FALSE);
            send_key(sbtns[idx].kc);
        }
        return 0;
    }
    case WM_LBUTTONUP:
        if(g_pressed_btn>=0){
            g_pressed_btn = -1;
            InvalidateRect(hw, NULL, FALSE);
        }
        return 0;
    case WM_CLOSE:
        exit(0);
    case WM_DESTROY:
        PostQuitMessage(0);
        g_running = 0;
        return 0;
    }
    return DefWindowProcA(hw, msg, wp, lp);
}

/* ----------------------------------------------------------------
 * Backend API
 * ---------------------------------------------------------------- */

static void gui_init(void)
{
    WNDCLASSEXA wc;
    RECT rc;
    int sw, sh, ww, wh, i;
    HINSTANCE hi = GetModuleHandle(NULL);

    SetProcessDPIAware();

    /* Screen-resolution-based scaling (reference: 1080p) */
    g_sf_num = GetSystemMetrics(SM_CYSCREEN);
    g_sf_den = REF_RES;

    /* Pre-scale segment dimensions */
    g_seg_w  = S(SEG_W_REF);
    g_seg_h  = S(SEG_H_REF);
    g_seg_t  = S(SEG_T_REF);
    g_dig_sp = S(DIG_SP_REF);

    /* Pre-scale button array */
    for(i=0;i<NBTN;i++){
        sbtns[i].x = S(btns[i].x);
        sbtns[i].y = S(btns[i].y);
        sbtns[i].w = S(btns[i].w);
        sbtns[i].h = S(btns[i].h);
        sbtns[i].label = btns[i].label;
        sbtns[i].kc = btns[i].kc;
    }

    sw = S(REF_W);
    sh = S(REF_H);

    g_fnt_light = CreateFontA(S(13),0,0,0,FW_BOLD,0,0,0,
        ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,DEFAULT_PITCH|FF_SWISS,"Consolas");
    g_fnt_label = CreateFontA(S(14),0,0,0,FW_BOLD,0,0,0,
        ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,DEFAULT_PITCH|FF_SWISS,"Consolas");
    g_fnt_btn = CreateFontA(S(15),0,0,0,FW_BOLD,0,0,0,
        ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,DEFAULT_PITCH|FF_SWISS,"Consolas");

    memset(&wc,0,sizeof(wc));
    wc.cbSize       = sizeof(wc);
    wc.style        = CS_HREDRAW|CS_VREDRAW;
    wc.lpfnWndProc  = wndproc;
    wc.hInstance     = hi;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszClassName = "DSKY_GUI";
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassExA(&wc);

    rc.left=0; rc.top=0; rc.right=sw; rc.bottom=sh;
    AdjustWindowRect(&rc, WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU, FALSE);
    ww = rc.right-rc.left;
    wh = rc.bottom-rc.top;

    g_hwnd = CreateWindowExA(0,"DSKY_GUI","COMANCHE 055 - DSKY",
        WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,
        (GetSystemMetrics(SM_CXSCREEN)-ww)/2,
        (GetSystemMetrics(SM_CYSCREEN)-wh)/2, ww, wh,
        NULL,NULL,hi,NULL);

    ShowWindow(g_hwnd, SW_SHOWNORMAL);
    UpdateWindow(g_hwnd);
    BringWindowToTop(g_hwnd);
    SetForegroundWindow(g_hwnd);
    SetActiveWindow(g_hwnd);
    SetFocus(g_hwnd);
    g_running = 1;
    g_dirty = 1;
    memset(&g_prev, 0, sizeof(g_prev));
}

static void gui_update(void)
{
    if(g_dirty || memcmp(&dsky_display, &g_prev, sizeof(dsky_display))!=0){
        InvalidateRect(g_hwnd, NULL, FALSE);
        g_prev = dsky_display;
        g_dirty = 0;
    }
}

static void gui_poll(void)
{
    MSG m;
    while(PeekMessageA(&m, NULL, 0, 0, PM_REMOVE)){
        if(m.message==WM_QUIT){ exit(0); }
        TranslateMessage(&m);
        DispatchMessageA(&m);
    }
}

static void gui_cleanup(void)
{
    if(g_fnt_light){ DeleteObject(g_fnt_light); g_fnt_light=NULL; }
    if(g_fnt_label){ DeleteObject(g_fnt_label); g_fnt_label=NULL; }
    if(g_fnt_btn)  { DeleteObject(g_fnt_btn);   g_fnt_btn=NULL;   }
    if(g_hwnd){ DestroyWindow(g_hwnd); g_hwnd=NULL; }
    g_running = 0;
}

static void gui_sleep(int ms) { Sleep(ms); }

dsky_backend_t dsky_gui_backend = {
    gui_init, gui_update, gui_poll, gui_cleanup, gui_sleep
};

#else /* !_WIN32 */

/* ----------------------------------------------------------------
 * Non-Windows stub implementation
 * Prevents empty translation unit errors on non-Windows platforms
 * ---------------------------------------------------------------- */

static void stub_init(void) {}
static void stub_update(void) {}
static void stub_poll(void) {}
static void stub_cleanup(void) {}
static void stub_sleep(int ms) { (void)ms; }

dsky_backend_t dsky_gui_backend = {
    stub_init, stub_update, stub_poll, stub_cleanup, stub_sleep
};

#endif /* _WIN32 */
