#include <boost/filesystem.hpp>

#include "TestTypedPropertyGraph.h"
#include "katana/GraphTopology.h"
#include "katana/Iterators.h"
#include "katana/Logging.h"
#include "katana/PropertyGraph.h"
#include "katana/Result.h"
#include "katana/SharedMemSys.h"
#include "katana/analytics/Utils.h"
#include "llvm/Support/CommandLine.h"
#include "stdio.h"
#include "storage-format-version-optional-topologies.h"
#include "tsuba/RDG.h"

namespace cll = llvm::cl;
namespace fs = boost::filesystem;

/*
 * Tests to validate optional topology storage added in storage_format_version=3
 * Ensure that when we convert an RDG with storage_format_version = 1
 * we can also add & store optional topologies
 * Input can be any rdg with storage_format_version == 1
 */

static cll::opt<std::string> ldbc_003InputFile(
    cll::Positional, cll::desc("<ldbc_003 input file>"), cll::Required);

int
main(int argc, char** argv) {
  katana::SharedMemSys sys;
  cll::ParseCommandLineOptions(argc, argv);

  TestOptionalTopologyStorageEdgeShuffleTopology(ldbc_003InputFile);
  TestOptionalTopologyStorageShuffleTopology(ldbc_003InputFile);
  TestOptionalTopologyStorageEdgeTypeAwareTopology(ldbc_003InputFile);
  return 0;
}
