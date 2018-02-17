#include <afxwin.h>  // we are an MFC app so directly including Windows.h is not allowed

#include "configuration.h"

struct ConfigurationDialog {
  Configuration configuration;
  static HINSTANCE dllInstance;
  bool got;

  explicit ConfigurationDialog(double* PUser);

  void Get();
};
