#define NO_DECRYPT

#include "Krkr2Lite.h"
#include "FolderDialog.cpp"

#define STATE_FLAG_MASK         0xFFFF0000
#define STATE_FLAG_SKIP_FILE    0x00010000

class CXP3ViewerUI
{
protected:
    HANDLE      m_ExtractThreadHandle;
    LONG        m_ExtractThreadId, m_WaitThreadRetry, m_State;
    HWND        m_hWndXp3List;
    HWND        m_hWndFileList;
    HWND        m_hWndPause;
    HWND        m_hWndDecTlg;
    HWND        m_hWndTlgDec;
    HWND        m_hWndSaveMeta;
    HWND        m_hWndExportType;
    HWND        m_hWndFilter;
    HWND        m_hWndOutputPath;
    HWND        m_hWndBtnSuspend, m_hWndBtnSkip;
    SIZE        m_LastWindowSize, m_MinSize;
    CKrkr2Lite  m_krkr2;

    enum
    {
        STATE_CONTINUE,
        STATE_SUSPEND,
        STATE_EXIT,
    };

    enum
    {
        WM_USER_FIRST = WM_USER,

        WM_USER_PROCESS_FILE,
        WM_USER_QUIT,

        WM_USER_LAST,
    };

public:
    CXP3ViewerUI(TVPXP3ArchiveExtractionFilterFunc Filter)
    {
        m_ExtractThreadHandle   = NULL;
        m_State                 = STATE_CONTINUE;
        m_WaitThreadRetry       = 21;
        m_MinSize.cx            = 0;
        m_MinSize.cy            = 0;
        m_LastWindowSize        = m_MinSize;
        m_krkr2.SetXP3ExtractionFilter((CKrkr2::XP3ExtractionFilterFunc)Filter);
    }

    VOID OnClose(HWND hWnd)
    {
        LARGE_INTEGER TimeOut;

        if (m_State != STATE_EXIT)
        {
            m_State = STATE_EXIT;
            PostThreadMessageW(m_ExtractThreadId, WM_USER_QUIT, 0, 0);
            m_WaitThreadRetry = 21;
        }

        BaseFormatTimeOut(&TimeOut, 100);
        if (--m_WaitThreadRetry != 0 &&
            NtWaitForSingleObject(m_ExtractThreadHandle, FALSE, &TimeOut) == STATUS_TIMEOUT)
        {
            PostMessageW(hWnd, WM_CLOSE, 0, 0);
            return;
        }

        NtClose(m_ExtractThreadHandle);

        DestroyWindow(hWnd);
    }

    VOID OnDestroy(HWND hWnd)
    {
        UNREFERENCED_PARAMETER(hWnd);
        PostQuitMessage(0);
    }

    BOOL OnInitDialog(HWND hWnd, HWND /* hWndFocus */, LPARAM /* lParam */)
    {
        RECT        ClientRect;
        NTSTATUS    Status;
        CLIENT_ID   ThreadId;

        Status = RtlCreateUserThread(
                    NtCurrentProcess(),
                    NULL,
                    FALSE,
                    0,
                    0,
                    0,
                    ExtractThread,
                    this,
                    &m_ExtractThreadHandle,
                    &ThreadId
                 );
        if (!NT_SUCCESS(Status))
        {
            MessageBoxW(hWnd, L"RtlCreateUserThread failed", NULL, 64);
            g_pInfo->InitDone = FALSE;
            Nt_ExitProcess(0);
        }

        SetWindowTextW(hWnd, L"XP3Viewer - built on " MAKE_WSTRING(__DATE__) L" " MAKE_WSTRING(__TIME__));

        m_ExtractThreadId = (ULONG)ThreadId.UniqueThread;

        DragAcceptFiles(hWnd, TRUE);

        m_hWndPause         = GetDlgItem(hWnd, ID_SUSPEND);
        m_hWndXp3List       = GetDlgItem(hWnd, IDC_XP3_LIST);
        m_hWndFileList      = GetDlgItem(hWnd, IDC_FILE_LIST);
        m_hWndDecTlg        = GetDlgItem(hWnd, IDC_CHK_DECTLG);
        m_hWndSaveMeta      = GetDlgItem(hWnd, IDC_CHK_SAVE_META);
        m_hWndTlgDec        = GetDlgItem(hWnd, IDC_CHK_BUILT_IN_TLGDEC);
        m_hWndExportType    = GetDlgItem(hWnd, IDC_COMBO_EXPORT_TYPE);
        m_hWndOutputPath    = GetDlgItem(hWnd, ID_OUTPUT_DIRECTORY);
        m_hWndFilter        = GetDlgItem(hWnd, IDC_EDIT_FILTER);
        m_hWndBtnSuspend    = GetDlgItem(hWnd, ID_SUSPEND);
        m_hWndBtnSkip       = GetDlgItem(hWnd, ID_SKIP_FILE);

        Edit_LimitText(
            m_hWndFilter,
            FIELD_SIZE(MY_FILE_ENTRY_BASE, FileName) / FIELD_SIZE(MY_FILE_ENTRY_BASE, FileName[0])
        );

        ComboBox_AddString(m_hWndExportType, L"32 bit BMP");
        ComboBox_AddString(m_hWndExportType, L"32 bit PNG");
        ComboBox_SetItemData(m_hWndExportType, 0, UNPACKER_FILE_TYPE_BMP);
        ComboBox_SetItemData(m_hWndExportType, 1, UNPACKER_FILE_TYPE_PNG);
        ComboBox_SetCurSel(m_hWndExportType, 0);

        Button_SetCheck(m_hWndDecTlg, TRUE);
        Button_SetCheck(m_hWndTlgDec, FALSE);
        Button_SetCheck(m_hWndSaveMeta, FALSE);
        EnableWindow(m_hWndSaveMeta, FALSE);
        m_krkr2.ClearDecodeFlags(KRKR2_FLAG_BUILT_IN_TLGDEC);
        m_krkr2.ClearDecodeFlags(KRKR2_FLAG_KEEP_RAW);
        m_krkr2.ClearDecodeFlags(KRKR2_FLAG_SAVE_TLG_META);

        GetClientRect(hWnd, &ClientRect);
        m_LastWindowSize.cx = ClientRect.right - ClientRect.left;
        m_LastWindowSize.cy = ClientRect.bottom - ClientRect.top;

        GetWindowRect(hWnd, &ClientRect);
        m_MinSize.cx = ClientRect.right - ClientRect.left;
        m_MinSize.cy = ClientRect.bottom - ClientRect.top;

        OnSize(hWnd, SIZE_RESTORED, m_LastWindowSize.cx, m_LastWindowSize.cy);

        SetStateButtonText(m_State);

        return TRUE;
    }

    VOID SetStateButtonText(BOOL State)
    {
        static WCHAR *ButtonText[] =
        {
            L"&Suspend",
            L"Re&sume",
        };

        SetWindowTextW(m_hWndPause, ButtonText[State & 1]);
    }

    VOID OnCommand(HWND hWnd, INT ID, HWND hWndCtl, UINT codeNotify)
    {
        BOOL Checked;
        UNREFERENCED_PARAMETER(codeNotify);
        UNREFERENCED_PARAMETER(hWndCtl);

        switch (ID)
        {
            case IDCANCEL:
                OnClose(hWnd);
                break;

            case ID_SET_OUTDIR:
                {
                    ml::FolderDialog SelectFolder;

                    if (SelectFolder.DoModule() == 0)
                        break;

                    Static_SetText(m_hWndOutputPath, SelectFolder.GetPathName());
                }
                break;

            case ID_SUSPEND:
                m_State ^= STATE_SUSPEND;
                SetStateButtonText(m_State);
                break;

            case ID_SKIP_FILE:
                _InterlockedOr(&m_State, STATE_FLAG_SKIP_FILE);
                break;

            case IDC_CHK_DECTLG:
                Checked = Button_GetCheck(m_hWndDecTlg);
                EnableWindow(m_hWndTlgDec, Checked);
                EnableWindow(m_hWndSaveMeta, Checked);
                EnableWindow(m_hWndExportType, Checked);
                Checked ? m_krkr2.ClearDecodeFlags(KRKR2_FLAG_KEEP_RAW) : m_krkr2.SetDecodeFlags(KRKR2_FLAG_KEEP_RAW);
                if (!Checked)
                    break;
                NO_BREAK;

            case IDC_CHK_BUILT_IN_TLGDEC:
                Checked = Button_GetCheck(m_hWndTlgDec);
                EnableWindow(m_hWndSaveMeta, Checked);
                Checked ? m_krkr2.SetDecodeFlags(KRKR2_FLAG_BUILT_IN_TLGDEC) : m_krkr2.ClearDecodeFlags(KRKR2_FLAG_BUILT_IN_TLGDEC);
                break;

            case IDC_CHK_SAVE_META:
                Checked = Button_GetCheck(m_hWndSaveMeta);
                if (Checked)
                {
                    m_krkr2.SetDecodeFlags(KRKR2_FLAG_SAVE_TLG_META);
                }
                else
                {
                    m_krkr2.ClearDecodeFlags(KRKR2_FLAG_SAVE_TLG_META);
                }
                break;

            case IDC_COMBO_EXPORT_TYPE:
                switch (codeNotify)
                {
                    case CBN_SELCHANGE:
                        switch (ComboBox_GetItemData(m_hWndExportType, ComboBox_GetCurSel(m_hWndExportType)))
                        {
                            case UNPACKER_FILE_TYPE_PNG:
                                m_krkr2.SetDecodeFlags(KRKR2_FLAG_ENCODE_PNG);
                                break;

                            case UNPACKER_FILE_TYPE_BMP:
                                m_krkr2.ClearDecodeFlags(KRKR2_FLAG_ENCODE_PNG);
                                break;
                        }
                        break;
                }

                break;
        }
    }

    BOOL SuspendOrBreak()
    {
        LARGE_INTEGER TimeOut;

        BaseFormatTimeOut(&TimeOut, 100);
        LOOP_FOREVER
        {
            switch (m_State & ~STATE_FLAG_MASK)
            {
                case STATE_SUSPEND:
                    NtDelayExecution(FALSE, &TimeOut);
                    break;

                default:
                case STATE_CONTINUE:
                    return TRUE;

                case STATE_EXIT:
                    return FALSE;
            }
        }
    }

    VOID ProcessDropFile(HWND hWnd, HDROP hDrop)
    {
        ULONG               StateFlags, OutputLength;
        ULONG64             TotalCount, CurrentCount;
        WCHAR               Progress[0x50];
        WCHAR               Xp3Path[MAX_NTPATH], szPath[MAX_NTPATH], szOutputPath[MAX_NTPATH];
        MY_FILE_INDEX_BASE *pIndex;
        MY_FILE_ENTRY_BASE *pEntry;

        CurrentCount = 0;
        TotalCount   = 0;
        ListBox_ResetContent(m_hWndFileList);
        OutputLength = Static_GetText(m_hWndOutputPath, szOutputPath, countof(szOutputPath));
        if (OutputLength != 0 && szOutputPath[OutputLength - 1] != '\\')
        {
            szOutputPath[OutputLength++] = '\\';
        }

        for (ULONG Index = 0; DragQueryFileW(hDrop, Index, Xp3Path, countof(Xp3Path)); ListBox_DeleteString(m_hWndXp3List, 0), ++Index)
        {
            StateFlags = _InterlockedAnd(&m_State, ~STATE_FLAG_MASK) & STATE_FLAG_MASK;
            if (FLAG_ON(StateFlags, STATE_FLAG_SKIP_FILE))
                continue;

            if (!SuspendOrBreak())
                break;

            if (Nt_GetFileAttributes(Xp3Path) & FILE_ATTRIBUTE_DIRECTORY)
            {
                ULONG i;
                i = ListBox_AddString(m_hWndFileList, L"Packing ...");
                ListBox_SetCurSel(m_hWndFileList, i);
                m_krkr2.Pack(Xp3Path);
                ListBox_DeleteString(m_hWndFileList, i);
                ListBox_AddString(m_hWndFileList, L"Packing ... Done");
                continue;
            }

            if (!m_krkr2.Open(Xp3Path))
                continue;

            if (OutputLength == 0)
            {
                StrCopyW(szOutputPath, Xp3Path);
            }
            else
            {
                StrCopyW(szOutputPath + OutputLength, findnamew(Xp3Path));
            }

            rmext(szOutputPath);

            pIndex = m_krkr2.GetIndex();
            pEntry = pIndex->pEntry;
            TotalCount += pIndex->FileCount.QuadPart;
            for (ULONG64 FileCount = pIndex->FileCount.QuadPart; FileCount; --FileCount)
            {
                ULONG Index;
                BOOL  Success;

                StateFlags = _InterlockedAnd(&m_State, ~STATE_FLAG_MASK) & STATE_FLAG_MASK;
                if (FLAG_ON(StateFlags, STATE_FLAG_SKIP_FILE))
                    break;

                if (!SuspendOrBreak())
                    break;

                swprintf(Progress, L"%I64d / %I64d", ++CurrentCount, TotalCount);
                SetWindowTextW(hWnd, Progress);

                AddFileAutoPath(Xp3Path, pEntry->FileName);

//                Length = swprintf(szPath, L"%s ... ", pEntry->FileName);
//                Index  = ListBox_AddString(m_hWndFileList, szPath);
//                ListBox_SetCurSel(m_hWndFileList, Index);

                Success = m_krkr2.ExtractFile(pEntry, szOutputPath) != 0;

                swprintf(szPath, L"%s ... %s", pEntry->FileName, Success ? L"OK" : L"failed");

//                ListBox_DeleteString(m_hWndFileList, Index);
                Index = ListBox_AddString(m_hWndFileList, szPath);
                ListBox_SetCurSel(m_hWndFileList, Index);

                if ((Index % 0x100) == 0)
                    TVPClearGraphicCache();

                *(PULONG_PTR)&pEntry += pIndex->cbEntrySize;
            }
        }

        TVPClearGraphicCache();
        m_krkr2.ReleaseAll();
        DragFinish(hDrop);
    }

    VOID AddFileAutoPath(PWCHAR Xp3Path, PWCHAR FileName)
    {
        WCHAR ch, *p, *p2, AutoPath[MAX_NTPATH * 2];

        FileName = StrFindCharW(FileName, '>') + 1;

        p = FileName;
        p2 = findnamew(p);
        if (p == p2)
        {
            swprintf(AutoPath, L"%s>", Xp3Path);
            TVPAddAutoPath(AutoPath);
            return;
        }

        p = p2 - 1;
        ch = *p;
        *p = 0;

        swprintf(AutoPath, L"%s>%s/", Xp3Path, FileName);
        TVPAddAutoPath(AutoPath);
        *p = ch;
    }

    static ULONG WINAPI ExtractThread(CXP3ViewerUI *pThis)
    {
        return pThis->ExtractThreadWorker();
    }

    ULONG ExtractThreadWorker()
    {
        MSG             Message;
        LARGE_INTEGER   TimeOut;

        PeekMessageW(&Message, NULL, WM_USER, WM_USER, PM_NOREMOVE);
        BaseFormatTimeOut(&TimeOut, 100);

        CoInitialize(NULL);

        LOOP_FOREVER
        {
            m_State &= ~STATE_FLAG_MASK;
            NtDelayExecution(FALSE, &TimeOut);
            if (!PeekMessageW(&Message, NULL, WM_USER_FIRST, WM_USER_LAST, PM_REMOVE))
            {
                continue;
            }

            switch (Message.message)
            {
                case WM_USER_QUIT:
                    CoUninitialize();
                    return 0;

                case WM_USER_PROCESS_FILE:
                    ProcessDropFile((HWND)Message.wParam, (HDROP)Message.lParam);
                    break;

                default:
                    break;
            }
        }
    }

    VOID OnDropFiles(HWND hWnd, HDROP hDrop)
    {
        WCHAR Xp3Path[MAX_NTPATH];

        for (ULONG Index = 0; DragQueryFileW(hDrop, Index, Xp3Path, countof(Xp3Path)); ++Index)
        {
            ListBox_AddString(m_hWndXp3List, Xp3Path);
        }

        PostThreadMessageW(m_ExtractThreadId, WM_USER_PROCESS_FILE, (WPARAM)hWnd, (LPARAM)hDrop);
    }

    VOID OnLButtonUp(HWND hWnd, INT, INT, UINT)
    {
        SendMessageW(hWnd, WM_NCLBUTTONUP, HTCAPTION, 0);
    }

    VOID OnLButtonDown(HWND hWnd, BOOL, INT, INT, UINT)
    {
        SendMessageW(hWnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
    }

    VOID OnGetMinMaxInfo(HWND, LPMINMAXINFO MinMaxInfo)
    {
        MinMaxInfo->ptMinTrackSize = *(LPPOINT)&m_MinSize;
        MinMaxInfo->ptMaxTrackSize.x = INT_MAX;
        MinMaxInfo->ptMaxTrackSize.y = INT_MAX;
    }

    VOID OnSize(HWND hWnd, UINT State, INT cx, INT cy)
    {
        RECT ControlRect;
        INT  x, y, w, h;

        if (m_LastWindowSize.cx == 0)
            return;

        if (State == SIZE_MINIMIZED)
            return;

        GetWindowRect(m_hWndXp3List, &ControlRect);
        SetWindowPos(
            m_hWndXp3List,
            NULL,
            0,
            0,
            ControlRect.right - ControlRect.left + (cx - m_LastWindowSize.cx),
            ControlRect.bottom - ControlRect.top, // + (cy - m_LastWindowSize.cy),
            SWP_NOMOVE
        );

        GetWindowRect(m_hWndFileList, &ControlRect);

        w = ControlRect.right - ControlRect.left + (cx - m_LastWindowSize.cx);
        h = ControlRect.bottom - ControlRect.top + (cy - m_LastWindowSize.cy);
        SetWindowPos(m_hWndFileList, NULL, 0, 0, w, h, SWP_NOMOVE);

        ScreenToClient(hWnd, (LPPOINT)&ControlRect);
        ScreenToClient(hWnd, (LPPOINT)&ControlRect + 1);

        ULONG ButtonCount = 2;

        ControlRect.bottom = h + ControlRect.top;

        w = cx / ButtonCount / 4;
        x = (cx / ButtonCount - w) / ButtonCount;
        h = (cy - ControlRect.bottom) / ButtonCount;
        y = ControlRect.bottom + (cy - ControlRect.bottom - h) / ButtonCount;
        SetWindowPos(m_hWndBtnSuspend, NULL, x, y, w, h, 0);

        x = cx / ButtonCount + (cx / ButtonCount - w) / ButtonCount;
        SetWindowPos(m_hWndBtnSkip, NULL, x, y, w, h, 0);

        m_LastWindowSize.cx = cx;
        m_LastWindowSize.cy = cy;
    }

    INT_PTR DialogProcWorker(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
    {
        switch (Message)
        {
            HANDLE_MSG(hWnd, WM_COMMAND,        OnCommand);
            HANDLE_MSG(hWnd, WM_DROPFILES,      OnDropFiles);
            HANDLE_MSG(hWnd, WM_LBUTTONDOWN,    OnLButtonDown);
            HANDLE_MSG(hWnd, WM_LBUTTONUP,      OnLButtonUp);
            HANDLE_MSG(hWnd, WM_SIZE,           OnSize);
            HANDLE_MSG(hWnd, WM_GETMINMAXINFO,  OnGetMinMaxInfo);
            HANDLE_MSG(hWnd, WM_CLOSE,          OnClose);
            HANDLE_MSG(hWnd, WM_DESTROY,        OnDestroy);
            HANDLE_MSG(hWnd, WM_INITDIALOG,     OnInitDialog);
        }

        return FALSE;
    }

    static INT_PTR CALLBACK DialogProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
    {
        CXP3ViewerUI *pThis = (CXP3ViewerUI *)GetWindowLongPtrW(hWnd, DWLP_USER);
        return pThis->DialogProcWorker(hWnd, Message, wParam, lParam);
    }

    static INT_PTR CALLBACK StartDialogProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
    {
        if (Message != WM_INITDIALOG)
            return FALSE;

        SetWindowLongPtrW(hWnd, DWLP_USER, (LONG_PTR)lParam);
        SetWindowLongPtrW(hWnd, DWLP_DLGPROC, (LONG_PTR)DialogProc);

        CXP3ViewerUI *pThis = (CXP3ViewerUI *)lParam;
        return pThis->DialogProcWorker(hWnd, Message, wParam, lParam);
    }

    INT_PTR DoModel(HINSTANCE hInstance, LPWSTR lpTemplateID)
    {
        return DialogBoxParamW(
                    hInstance,
                    lpTemplateID,
                    NULL,
                    StartDialogProc,
                    (LPARAM)this
                );
    }
};
