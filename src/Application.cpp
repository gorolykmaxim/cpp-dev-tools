#include "Application.hpp"

Application::Application(int argc, char** argv)
    : gui_app(argc, argv),
      runtime(*this),
      threads(gui_app),
      ui() {
  qSetMessagePattern("%{time yyyy-MM-dd h:mm:ss.zzz} %{message}");
}
