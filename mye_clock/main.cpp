// ---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "main.h"
#include "../mye/mye.h"
#include "functions.h"
#include "ClockThread.h"
#include <stdio.h>
// ---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TForm1 *Form1;
//mye_GetWindowDC_api getwindowdc;
HWND hwnd;
HDC hdc;
HMODULE hLib;
extern mye_DockWindow_api dockwindow;

// ---------------------------------------------------------------------------
void __fastcall TForm1::handleCustomMessage(TMessage &Message)
{

    if(Message.Msg == WM_ERASEBKGND)
    {
        Message.Result = true;
        return;
    } else if(Message.Msg == WM_PAINT || Message.Msg == WM_NCPAINT)
    {
        Message.Result = false;
        return;
    }
    static long id = 0;

    static DWORD sttick = 0;
    char tm[1024];
    DWORD curtick = GetTickCount()-sttick;
    SYSTEMTIME st;
    GetLocalTime(&st);
    static char sep[2] = ":";

    curtick = GetTickCount();
    if (curtick >= sttick+300)
    {
        sttick = curtick;
        sep[0] = (sep[0] == 0x20?':':0x20);
    }
    sprintf(tm, "%02d%s%02d%s%02d", st.wHour, sep, st.wMinute, sep, st.wSecond);
    // cnv->TextOutA(0, 0, tm);
    PAINTSTRUCT ps;
    // BeginPaint(hwnd,&ps);
    // if (!TextOutA(cnv->Handle, 0, 500, tm, strlen(tm)))
    // deb("textout: %s", fmterr());

    LOGFONT logFont;

    logFont.lfHeight = -(0.5 + 1.0 * Form1->Font->Size * 96 / 72);
    logFont.lfWidth = 0;
    logFont.lfEscapement = 0;
    logFont.lfOrientation = 0;
    logFont.lfWeight = Form1->Font->Style.Contains(fsBold) ? 700:400;
    logFont.lfItalic = Form1->Font->Style.Contains(fsItalic) ? TRUE:FALSE;
    logFont.lfUnderline = Form1->Font->Style.Contains(fsUnderline) ? TRUE:FALSE;
    logFont.lfStrikeOut = Form1->Font->Style.Contains(fsStrikeOut) ? TRUE:FALSE;
    logFont.lfCharSet = DEFAULT_CHARSET;
    logFont.lfOutPrecision = OUT_TT_PRECIS;
    logFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    logFont.lfQuality = NONANTIALIASED_QUALITY; // 1
    logFont.lfPitchAndFamily = FIXED_PITCH;
    char str[2035];
    strcpy(logFont.lfFaceName, deunicode(Form1->Font->Name.c_str(), str, sizeof(str)));
    HFONT hFont = NULL, hFontOld = NULL;
    hFont = CreateFontIndirect(&logFont);
    if (!hFont)
        deb("failed to create font: %s", fmterr());
    // HDC dc = GetWindowDC(Form1->Handle);
    hFontOld = (HFONT)SelectObject(Form1->pb->Canvas->Handle, hFont); // 2
    // Form1->Canvas->TextOut(0, 0, tm);
    //
    Form1->pb->Canvas->Font->Color = clWhite;
    Form1->pb->Canvas->Brush->Style = bsClear;
    // Form1->pb->Canvas->TextOutA(1,1,tm);
    // deb("out %s",tm);
    SetTextColor(Form1->pb->Canvas->Handle, clWhite);

    RECT rect;
    rect.left = 0;
    rect.right = Form1->pb->Width;
    rect.top = 1;
    rect.bottom = Form1->pb->Height;
    // FillRect(Form1->pb->Canvas->Handle,&rect, (HBRUSH) (COLOR_WINDOW+1));
    if(!TextOutA(Form1->pb->Canvas->Handle, 3,0,tm,strlen(tm)))
        deb("textout: %s",fmterr());
    //LockWindowUpdate(NULL);
    //int ret = DrawText(Form1->pb->Canvas->Handle, tm, strlen(tm), &rect, DT_CENTER|DT_NOCLIP|DT_EDITCONTROL);
    //if (!ret)
    //    deb("drawtext: %s", fmterr());
    // Form1->tedit->Text = tm;
    switch(st.wDayOfWeek)
    {
        case 1:
        strcpy(tm, "�����������");
        break;
        case 2:
        strcpy(tm, "�������");
        break;
        case 3:
        strcpy(tm, "�����");
        break;
        case 4:
        strcpy(tm, "�������");
        break;
        case 5:
        strcpy(tm, "�������");
        break;
        case 6:
        strcpy(tm, "�������");
        break;
        case 0:
        strcpy(tm, "�����������");
        break;
        default:
        sprintf(tm,"unk: %d",st.wDayOfWeek);
        break;
    }
    rect.top = Form1->Canvas->TextHeight(tm)+1;

    DeleteObject(hFont);
    logFont.lfHeight = -(0.5 + 1.0 * Form1->Font->Size * 96 / 90);
    logFont.lfWidth = 0;
    logFont.lfEscapement = 0;
    logFont.lfOrientation = 0;
    logFont.lfWeight = Form1->Font->Style.Contains(fsBold) ? 700:400;
    logFont.lfItalic = Form1->Font->Style.Contains(fsItalic) ? TRUE:FALSE;
    logFont.lfUnderline = Form1->Font->Style.Contains(fsUnderline) ? TRUE:FALSE;
    logFont.lfStrikeOut = Form1->Font->Style.Contains(fsStrikeOut) ? TRUE:FALSE;
    logFont.lfCharSet = DEFAULT_CHARSET;
    logFont.lfOutPrecision = OUT_TT_PRECIS;
    logFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    logFont.lfQuality = NONANTIALIASED_QUALITY; // 1
    logFont.lfPitchAndFamily = FIXED_PITCH;

    strcpy(logFont.lfFaceName, "tahoma");

    hFont = CreateFontIndirect(&logFont);
    if (!hFont)
        deb("failed to create font: %s", fmterr());
    // HDC dc = GetWindowDC(Form1->Handle);
    hFontOld = (HFONT)SelectObject(Form1->pb->Canvas->Handle, hFont);
    //if(!TextOutA(Form1->pb->Canvas->Handle, 0,Form1->Canvas->TextHeight(tm)+1,tm,strlen(tm)))
    //    deb("textout: %s",fmterr());
    int ret = DrawText(Form1->pb->Canvas->Handle, tm, strlen(tm), &rect, DT_CENTER|DT_NOCLIP|DT_EDITCONTROL);
    if (!ret)
        deb("drawtext: %s", fmterr());
    // Form1->dedit->Text = tm;
    // SelectObject(dc, hFontOld);
    // ReleaseDC(Form1->tedit->Handle,dc);
    DeleteObject(hFont);
    SelectObject(Form1->pb->Canvas->Handle, hFontOld);

    // Form1->pb->Canvas->Font->Color=clBlack;
    // Form1->pb->Canvas->TextOutA(0,0,tm);
    // EndPaint(hwnd,&ps);
    // cnv->Unlock();
    Message.Result = true;
}

__fastcall TForm1::TForm1(TComponent *Owner):TForm(Owner)
{
    Hide();

    hLib = LoadLibrary("mye.dll");
   // getwindowdc = (mye_GetWindowDC_api)GetProcAddress(hLib, "_mye_GetWindowDC@12");
 //   if (!getwindowdc)
  //      ExitProcess(0);
    dockwindow = (mye_DockWindow_api)GetProcAddress(hLib, "_mye_DockWindow@16");
//    deb("getwindowdc @ 0x%p", getwindowdc);
    // hwnd = getwindowdc(0, 10, 20);
    // deb("hwnd: %x", hwnd);

     ClockThread *ct = new ClockThread(1);
     ct->Start();

    // TCanvas *cnv;
    // DWORD sttick = GetTickCount();
    // DWORD curtick;

    // cnv = new TCanvas();
    // hdc = GetWindowDC(hwnd);
    // cnv->Handle = hdc;
    // deb("canvas from window %x hdc %x", hwnd, hdc);

}

// ---------------------------------------------------------------------------

void __fastcall TForm1::FormClick(TObject *Sender)
{
    MessageBox(Application->MainFormHandle, "Clock click", "ok", MB_OK);
}
// ---------------------------------------------------------------------------

void __fastcall TForm1::teditClick(TObject *Sender)
{
    MessageBox(Application->MainFormHandle, "Clock click", "ok", MB_OK);
}
// ---------------------------------------------------------------------------

void __fastcall TForm1::FormPaint(TObject *Sender)
{
    onpaint();
}
// ---------------------------------------------------------------------------

void __fastcall TForm1::pbMouseDown(TObject *Sender, TMouseButton Button, TShiftState Shift, int X, int Y)
{
    MessageBox(NULL, "clock click", "click", MB_OK);
}
// ---------------------------------------------------------------------------

