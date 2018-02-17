#include "configurationdialog_mock.h"

#include "configurationdialog.h"

extern template struct trompeloeil::reporter<trompeloeil::specialized>;

ConfigurationDialogMock configurationDialogMockInstance;

ConfigurationDialog::ConfigurationDialog(double* PUser) {}

HINSTANCE ConfigurationDialog::dllInstance = NULL;

std::string GetInputText(HWND dlg, int resid) {
  return "";
}

BOOL CALLBACK DialogProc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp) {
  return false;
}

void ConfigurationDialog::Get() {
  configurationDialogMockInstance.Get();
}
