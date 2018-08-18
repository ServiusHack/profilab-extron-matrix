#include "configurationdialog.h"
#include "resource.h"

#include <commdlg.h>

#include "listserialports.h"

ConfigurationDialog::ConfigurationDialog(double* PUser)
  : got(false)
  , configuration(PUser)
{}

HINSTANCE ConfigurationDialog::dllInstance = NULL;

std::string GetInputText(HWND dlg, int resid)
{
  HWND hc = GetDlgItem(dlg, resid);
  int n = GetWindowTextLength(hc) + 1;
  std::string s(n, 0);
  GetWindowText(hc, &s[0], n);
  s.erase(s.length() - 1);
  return s;
}

BOOL CALLBACK DialogProc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp)
{
  static ConfigurationDialog* getter = 0;
  switch (message) {
    case WM_INITDIALOG: {
      getter = reinterpret_cast<ConfigurationDialog*>(lp);
      SetWindowText(GetDlgItem(hwnd, IDC_COMPORT),
                    getter->configuration.comPort.c_str());
      SetWindowText(GetDlgItem(hwnd, IDC_INPUTS),
                    std::to_string(getter->configuration.inputs).c_str());
      SetWindowText(GetDlgItem(hwnd, IDC_OUTPUTS),
                    std::to_string(getter->configuration.outputs).c_str());

      for (const std::string& port : listSerialPorts()) {
        SendDlgItemMessage(
          hwnd, IDC_COMPORT, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)port.c_str());
      }

      CheckDlgButton(
        hwnd, IDC_INPUTNAMEPINS, getter->configuration.includeInputNames);
      CheckDlgButton(
        hwnd, IDC_OUTPUTNAMEPINS, getter->configuration.includeOutputNames);
      return TRUE;
    }
    case WM_COMMAND: {
      int ctl = LOWORD(wp);
      int event = HIWORD(wp);
      if (ctl == IDCANCEL && event == BN_CLICKED) {
        getter->got = false;
        DestroyWindow(hwnd);
        return TRUE;
      } else if (ctl == IDOK && event == BN_CLICKED) {
        getter->configuration.comPort = GetInputText(hwnd, IDC_COMPORT);

        getter->configuration.inputs =
          std::stoi(GetInputText(hwnd, IDC_INPUTS));
        getter->configuration.outputs =
          std::stoi(GetInputText(hwnd, IDC_OUTPUTS));

        getter->configuration.includeInputNames =
          SendDlgItemMessage(hwnd, IDC_INPUTNAMEPINS, BM_GETCHECK, 0, 0) ==
          BST_CHECKED;
        getter->configuration.includeOutputNames =
          SendDlgItemMessage(hwnd, IDC_OUTPUTNAMEPINS, BM_GETCHECK, 0, 0) ==
          BST_CHECKED;

        getter->got = true;
        DestroyWindow(hwnd);
        return TRUE;
      }
      return FALSE;
    }
    case WM_DESTROY:
      PostQuitMessage(0);
      return TRUE;
    case WM_CLOSE:
      DestroyWindow(hwnd);
      return TRUE;
  }

  return FALSE;
}

void ConfigurationDialog::Get()
{
  HWND dlg = CreateDialogParam(dllInstance,
                               MAKEINTRESOURCE(IDD_CONFIGURATION_DIALOG),
                               0,
                               DialogProc,
                               (LPARAM)this);
  MSG msg;
  while (GetMessage(&msg, 0, 0, 0)) {
    if (!IsDialogMessage(dlg, &msg)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
}
