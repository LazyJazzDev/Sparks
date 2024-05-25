#include "iostream"
#include "sparks/app/app.h"
#include "sparks/assets/assets.h"
#include "sparks/utils/utils.h"

int main() {
  std::cout << sparks::FileProbe::GetInstance() << std::endl;
  std::cout << sparks::FindAssetsFile("README.md") << std::endl;
}
