#include "FileType.h"
#include <llvm/Support/CommandLine.h>
#include <llvm2cpg/CPG/BitcodeLoader.h>
#include <llvm2cpg/CPG/CPG.h>
#include <llvm2cpg/CPG/Version.h>
#include <llvm2cpg/CPGWriter/CPGProtoWriter.h>
#include <llvm2cpg/Logger/CPGLogger.h>
#include <sstream>
#include <string>

llvm::cl::OptionCategory CPGProtoWriterCategory("llvm2cpg");

llvm::cl::list<std::string> BitcodePaths(llvm::cl::Positional, llvm::cl::OneOrMore,
                                         llvm::cl::desc("Bitcode files"),
                                         llvm::cl::cat(CPGProtoWriterCategory));

llvm::cl::opt<std::string>
    OutputDirectory("output-dir", llvm::cl::Optional,
                    llvm::cl::desc("Where to store cpg.bin.zip (defaults to '.')"),
                    llvm::cl::cat(CPGProtoWriterCategory), llvm::cl::init("."));

llvm::cl::opt<std::string> OutputName("output-name", llvm::cl::Optional,
                                      llvm::cl::desc("Output filename (defaults to 'cpg.bin.zip')"),
                                      llvm::cl::cat(CPGProtoWriterCategory),
                                      llvm::cl::init("cpg.bin.zip"));

llvm::cl::opt<std::string>
    Output("output", llvm::cl::Optional,
           llvm::cl::desc("Output file path. Overrides --output-dir and --output-name"),
           llvm::cl::cat(CPGProtoWriterCategory));

llvm::cl::opt<bool>
    APInliner("inline", llvm::cl::Optional,
              llvm::cl::desc("Enable inlining of access paths (loads, pointer arithmetic)"),
              llvm::cl::cat(CPGProtoWriterCategory), llvm::cl::init(true));

int main(int argc, char **argv) {
  llvm::cl::SetVersionPrinter(llvm2cpg::printVersionInformationStream);
  llvm::cl::HideUnrelatedOptions(CPGProtoWriterCategory);
  llvm::cl::ParseCommandLineOptions(argc, argv);

  llvm2cpg::CPGLogger logger;
  logger.uiInfo(std::string("More details: ") + logger.getLogPath());

  std::stringstream logIntro;
  logIntro << "Generated by llvm2cpg (" << llvm2cpg::llvm2cpgVersionString() << " / "
           << llvm2cpg::llvm2cpgCommitString() << ")";
  logger.logInfo(logIntro.str());

  llvm2cpg::BitcodeLoader loader(logger);
  std::vector<std::unique_ptr<llvm::Module>> modules;

  llvm2cpg::CPG cpg(logger, APInliner.getValue());
  for (size_t i = 0; i < BitcodePaths.size(); i++) {
    std::string path = BitcodePaths[i];
    logger.uiInfo(std::string("Loading ") + path);
    llvm2cpg::FileType type = getFileType(logger, path);
    switch (type) {
    case llvm2cpg::FileType::Unsupported: {
      logger.logWarning(std::string("Skipping unsupported file ") + path);
    } break;
    case llvm2cpg::FileType::Bitcode: {
      logger.logInfo(std::string("Parsing bitcode file ") + path);
      std::unique_ptr<llvm::Module> module = loader.loadBitcode(path);
      modules.push_back(std::move(module));
    } break;
    case llvm2cpg::FileType::Binary: {
      logger.logInfo(std::string("Attempting to extract bitcode from ") + path);
      for (std::unique_ptr<llvm::Module> &module : loader.extractBitcode(path)) {
        modules.push_back(std::move(module));
      }
    } break;
    case llvm2cpg::FileType::LLVM_IR: {
      logger.logInfo(std::string("Parsing IR file ") + path);
      std::unique_ptr<llvm::Module> module = loader.loadIR(path);
      if (module) {
        modules.push_back(std::move(module));
      }
    } break;
    }
  }
  for (std::unique_ptr<llvm::Module> &module : modules) {
    cpg.addBitcode(module.get());
  }

  std::string output = Output.getValue();
  if (output.empty()) {
    output = OutputDirectory.getValue() + '/' + OutputName.getValue();
  }

  llvm2cpg::CPGProtoWriter writer(logger, output);
  writer.writeCpg(cpg);

  logger.uiInfo("Shutting down");
  return 0;
}
