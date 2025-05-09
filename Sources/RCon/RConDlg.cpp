/* Copyright (c) 2002-2012 Croteam Ltd. 
This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published by
the Free Software Foundation


This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

// RConDlg.cpp : implementation file
//

#include "stdafx.h"
#include <Engine/Network/CommunicationInterface.h>
#include "RCon.h"
#include "RConDlg.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRConDlg dialog

CRConDlg::CRConDlg(CWnd* pParent /*=NULL*/)
  : CDialog(CRConDlg::IDD, pParent)
{
  //{{AFX_DATA_INIT(CRConDlg)
  m_strLog = _T("");
  //}}AFX_DATA_INIT
  // Note that LoadIcon does not require a subsequent DestroyIcon in Win32
  m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRConDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CRConDlg)
  DDX_Text(pDX, IDC_LOG, m_strLog);
  //}}AFX_DATA_MAP

  // keep the last line visible
  CEdit *pctrlLog = (CEdit *) (GetDlgItem(IDC_LOG));
  if( pctrlLog != NULL)
  {
    int iLines=pctrlLog->GetLineCount();
    pctrlLog->LineScroll(iLines);
  }
}

BEGIN_MESSAGE_MAP(CRConDlg, CDialog)
  //{{AFX_MSG_MAP(CRConDlg)
  ON_WM_PAINT()
  ON_WM_QUERYDRAGICON()
  ON_WM_CLOSE()
  ON_WM_TIMER()
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRConDlg message handlers

BOOL CRConDlg::OnInitDialog()
{
  CDialog::OnInitDialog();

  // Set the icon for this dialog.  The framework does this automatically
  //  when the application's main window is not a dialog
  SetIcon(m_hIcon, TRUE);			// Set big icon
  SetIcon(m_hIcon, FALSE);		// Set small icon

  // TODO: Add extra initialization here

  SetTimer(0, 10, NULL);

  return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CRConDlg::OnPaint() 
{
  if (IsIconic()) {
    CPaintDC dc(this); // device context for painting

    SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

    // Center icon in client rectangle
    int cxIcon = GetSystemMetrics(SM_CXICON);
    int cyIcon = GetSystemMetrics(SM_CYICON);
    CRect rect;
    GetClientRect(&rect);
    int x = (rect.Width() - cxIcon + 1) / 2;
    int y = (rect.Height() - cyIcon + 1) / 2;

    // Draw the icon
    dc.DrawIcon(x, y, m_hIcon);

  } else {
    CDialog::OnPaint();
  }
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CRConDlg::OnQueryDragIcon()
{
  return (HCURSOR) m_hIcon;
}

// [Cecil] Entered command history
static CTString _aCommandHistory[32];
static const INDEX _ctCommands = ARRAYCOUNT(_aCommandHistory);

// [Cecil] Currently selected command in history (-1 allows going up to 0 and retrieving the first command)
static INDEX _iInCommandHistory = -1;

// [Cecil] Special command parsers
static CString *_pLogString = NULL;

static void OptionConnect(const CommandLineArgs_t &aArgs) {
  // Connect to a new server
  theApp.m_ulHost = StringToAddress(aArgs[0]);
  theApp.m_uwPort = strtoul(aArgs[1].ConstData(), NULL, 10);
  theApp.m_strPass = aArgs[2];

  CTString strReport(0, "Server: %s:%u\r\nReady for commands...\r\n", aArgs[0].ConstData(), theApp.m_uwPort);
  *_pLogString += strReport.ConstData();
};

static BOOL ParseSpecialCommands(CTString &strCommand, CString &strLog) {
  if (strCommand == "") return FALSE;

  // Start a new dedicated server with any arguments
  if (strCommand.RemovePrefix("/start ")) {
    STARTUPINFOA si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    CTString strCmd = _fnmFullExecutablePath.FileDir();
    strCommand = " " + strCommand;

  #if defined(SE1_STATIC_BUILD)
    strCmd += "DedicatedServer-Static.exe";
  #else
    strCmd += "DedicatedServer-Dynamic.exe";
  #endif

    if (CreateProcessA(strCmd.ConstData(), strCommand.Data(), NULL, NULL, FALSE, CREATE_DEFAULT_ERROR_MODE, NULL, NULL, &si, &pi)) {
      // Close unnecessarily opened handles
      CloseHandle(pi.hProcess);
      CloseHandle(pi.hThread);

    } else {
      // Report error
      CTString strError(0, "Cannot start dedicated server: %s\r\n", GetWindowsError(GetLastError()).ConstData());
      strLog += strError.ConstData();
    }
    return TRUE;

  // Show usage example of some commands
  } else if (strCommand == "/start") {
    strLog += "Usage: /start <configname> [<modname>]\r\n";
    return TRUE;

  } else if (strCommand == "/connect") {
    strLog += "Usage: /connect <hostname> <port> <password>\r\n";
    return TRUE;

  // List available commands
  } else if (strCommand == "/help") {
    strLog += "RCon can be used to remotely run console commands on a dedicated server that you are connected to. "
      "It also offers its own special commands that are parsed by RCon itself instead of the engine shell.\r\n"
      "\r\n"
      "Available commands:\r\n"
      "    /connect - connect to a different server with a different password\r\n"
      "    /help - display this help text\r\n"
      "    /start - start a new dedicated server\r\n"
      "Type any command without arguments to see its usage example.\r\n";
  }

  return FALSE;
};

BOOL CRConDlg::PreTranslateMessage(MSG* pMsg) 
{
  CWnd *pwndCommand = GetDlgItem(IDC_COMMAND);

  // [Cecil] Pressed a key in the command field
  if (pMsg->message == WM_KEYDOWN && CWnd::GetFocus() == pwndCommand) {
    UpdateData(TRUE);

    WPARAM &iKey = pMsg->wParam;

    if (iKey == VK_RETURN) {
      CString strConCommand;
      pwndCommand->GetWindowText(strConCommand);

      // send chat string to user(s)
      m_strLog += ">" + strConCommand + "\r\n";
      pwndCommand->SetWindowText(_T(""));

      // [Cecil] Get a meaningful command from the console
      CTString strCommand = MfcStringToCT(strConCommand);
      strCommand.TrimSpacesLeft();
      strCommand.TrimSpacesRight();

      // [Cecil] Add new command to the history and push it up once
      if (strCommand != "") {
        const INDEX iLast = _ctCommands - 1;

        for (INDEX i = 0; i < iLast; i++) {
          const INDEX iNext = iLast - i;
          const INDEX iThis = iLast - i - 1;

          _aCommandHistory[iNext] = _aCommandHistory[iThis];
        }

        _aCommandHistory[0] = strCommand;
      }

      // [Cecil] Reset selected command
      _iInCommandHistory = -1;

      // [Cecil] Try to parse special RCon commands with variable arguments
      _pLogString = &m_strLog;

      if (!ParseSpecialCommands(strCommand, m_strLog)) {
        // Try the command line with fixed amounts of arguments
        CommandLineSetup cmd(strCommand.ConstData());
        cmd.AddCommand("/connect", &OptionConnect, 3);

        CTString strCmdOutput = "";

        // Parse command line and display the result
        if (cmd.Parse(strCmdOutput)) {
          strCmdOutput.ReplaceSubstr("\n", "\r\n");
          m_strLog += strCmdOutput.ConstData();

        // Display the error if couldn't parse
        } else if (strCmdOutput != "") {
          strCmdOutput.ReplaceSubstr("\n", "\r\n");
          m_strLog += strCmdOutput.ConstData();

        // Send the command to the server
        } else {
          CNetworkMessage nm(MSG_EXTRA);
          nm << CTString(0, "rcmd %u \"%s\" %s\n", theApp.m_ulCode, theApp.m_strPass.ConstData(), strCommand.ConstData());
          _pNetwork->SendBroadcast(nm, theApp.m_ulHost, theApp.m_uwPort);
          _cmiComm.Client_Update();
        }
      }

      UpdateData(FALSE);

    // [Cecil] Go up the command history
    } else if (iKey == VK_UP) {
      // Don't go further if the next command is empty
      const CTString &strNext = _aCommandHistory[ClampDn(_iInCommandHistory + 1, 0)];

      if (_iInCommandHistory < _ctCommands - 1 && strNext != "") {
        _iInCommandHistory++;

        // Update command field
        pwndCommand->SetWindowText(CString(_aCommandHistory[_iInCommandHistory].ConstData()));
        UpdateData(FALSE);

        // Go to the end by mimicking the End press
        iKey = VK_END;
      }

    // [Cecil] Go down the command history
    } else if (iKey == VK_DOWN) {
      if (_iInCommandHistory > 0) {
        _iInCommandHistory--;

        // Update command field
        pwndCommand->SetWindowText(CString(_aCommandHistory[_iInCommandHistory].ConstData()));
        UpdateData(FALSE);

        // Go to the end by mimicking the End press
        iKey = VK_END;
      }
    }
  }

  return CDialog::PreTranslateMessage(pMsg);
}

void CRConDlg::OnCancel() 
{
  MinimizeApp();
}

void CRConDlg::OnOK() 
{
}

void CRConDlg::OnClose() 
{
  PostMessage(WM_QUIT);
  CDialog::OnClose();
}

void CRConDlg::OnTimer(UINT_PTR nIDEvent) 
{
  // repeat
  BOOL bChanged = FALSE;
  FOREVER {
    CNetworkMessage nmReceived;

    _cmiComm.Client_Update();
    ULONG ulFrom;
    UWORD uwPort;
    BOOL bHasMsg = _pNetwork->ReceiveBroadcast(nmReceived, ulFrom, uwPort);
    // if there are no more messages
    if (!bHasMsg) {
      // finish
      break;
    }

    // if this message is not valid rcon message
    if (nmReceived.GetType()!=MSG_EXTRA) {
      // skip it
      continue;
    }
    // get the string from the message
    CTString strMsg;
    nmReceived>>strMsg;

    // accept logs only
    if (!strMsg.RemovePrefix("log ")) {
      continue;
    }
    ULONG ulCode;
    INDEX iLine;
    char strLine[256];
    strMsg.ScanF("%u %d %256[^\n]", &ulCode, &iLine, strLine);
    if (ulCode!=theApp.m_ulCode) {
      continue;
    }

    m_strLog += strLine;
    m_strLog += "\r\n";
    bChanged = TRUE;
  }
  if (bChanged) {
    UpdateData(FALSE);
  }

  CDialog::OnTimer(nIDEvent);
}

