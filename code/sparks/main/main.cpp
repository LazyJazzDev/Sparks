#include "iostream"
#include "sparks/app/app.h"
#include "sparks/assets/assets.h"
#include "sparks/utils/utils.h"

int main() {
  sparks::AppSettings app_settings;
  sparks::Application app(app_settings);
  app.Run();
}
