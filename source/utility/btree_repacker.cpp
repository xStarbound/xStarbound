#include "StarBTreeDatabase.hpp"
#include "StarTime.hpp"
#include "StarFile.hpp"
#include "StarVersionOptionParser.hpp"

#ifdef STAR_USE_RPMALLOC
#include "rpmalloc.h"
#endif

using namespace Star;

int main(int argc, char** argv) {
#ifdef STAR_USE_RPMALLOC
  ::rpmalloc_initialize();
#endif
  try {
    double startTime = Time::monotonicTime();

    VersionOptionParser optParse;
    optParse.setSummary("Repacks a Starbound BTree file to shrink its file size");
    optParse.addArgument("input file path", OptionParser::Required, "Path to the BTree to be repacked");
    optParse.addArgument("output filename", OptionParser::Optional, "Output BTree file");

    auto opts = optParse.commandParseOrDie(argc, argv);

    String bTreePath = opts.arguments.at(0);
    String outputFilename = opts.arguments.get(1, bTreePath + ".repack");

    outputFilename = File::relativeTo(File::fullPath(File::dirName(outputFilename)), File::baseName(outputFilename));
    //open the old db
    BTreeDatabase db;
    db.setIODevice(std::move(File::open(bTreePath, IOMode::Read)));
    db.open();

    //make a new db
    BTreeDatabase newDb;
    newDb.setBlockSize(db.blockSize());
    newDb.setContentIdentifier(db.contentIdentifier());
    newDb.setKeySize(db.keySize());
    newDb.setAutoCommit(false);

    newDb.setIODevice(std::move(File::open(outputFilename, IOMode::ReadWrite | IOMode::Truncate)));
    newDb.open();
    coutf("Repacking {}...\n", bTreePath);
    //copy the data over
    unsigned count = 0;
    db.forAll([&count, &newDb](ByteArray key, ByteArray data) {
      newDb.insert(key, data);
      ++count;
    });

    //close the old db
    db.close();
    //commit and close the new db
    newDb.commit();
    newDb.close();

    coutf("Repacked BTree to {} in {}s\n", outputFilename, Time::monotonicTime() - startTime);
    return 0;

  } catch (std::exception const& e) {
    cerrf("Exception caught: {}\n", outputException(e, true));
    return 1;
  }
}
