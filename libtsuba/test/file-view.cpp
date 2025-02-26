#include <boost/filesystem.hpp>

#include "katana/Result.h"
#include "katana/URI.h"
#include "tsuba/FileView.h"
#include "tsuba/file.h"
#include "tsuba/tsuba.h"

namespace fs = boost::filesystem;

katana::Result<void>
TestEmpty(const std::string& path) {
  if (boost::system::error_code err; !fs::create_directories(path, err)) {
    if (err) {
      return KATANA_ERROR(
          std::error_code(err.value(), err.category()),
          "creating parent directories: {}", err.message());
    }
  }

  auto uri = KATANA_CHECKED(katana::Uri::MakeFromFile(path));
  auto empty_uri = uri.Join("empty_file");

  KATANA_CHECKED(tsuba::FileStore(empty_uri.string(), std::string("")));
  tsuba::FileView fv;

  KATANA_CHECKED(fv.Bind(empty_uri.string(), true));

  KATANA_LOG_ASSERT(fv.size() == 0);

  return katana::ResultSuccess();
}

katana::Result<void>
TestAll(const std::string& path) {
  KATANA_CHECKED_CONTEXT(TestEmpty(path), "TestEmpty");

  return katana::ResultSuccess();
}

int
main(int argc, char* argv[]) {
  if (auto init_good = tsuba::Init(); !init_good) {
    KATANA_LOG_FATAL("tsuba::Init: {}", init_good.error());
  }

  if (argc <= 1) {
    KATANA_LOG_FATAL("{} <empty dir>", argv[0]);
  }

  auto res = TestAll(argv[1]);
  if (!res) {
    KATANA_LOG_FATAL("test failed: {}", res.error());
  }

  if (auto fini_good = tsuba::Fini(); !fini_good) {
    KATANA_LOG_FATAL("tsuba::Fini: {}", fini_good.error());
  }

  return 0;
}
