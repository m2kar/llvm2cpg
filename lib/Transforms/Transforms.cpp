#include "llvm2cpg/Transforms/Transforms.h"
#include "llvm2cpg/Transforms/CustomPasses.h"

#include "llvm2cpg/Logger/CPGLogger.h"
#include <llvm/IR/Function.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Pass.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Transforms/IPO/FunctionAttrs.h>
#include <llvm/Transforms/IPO/InferFunctionAttrs.h>
#include <llvm/Transforms/Utils/Local.h>
#include <llvm2cpg/Traversals/ObjCTraversal.h>

using namespace llvm2cpg;
#include <llvm/Support/raw_ostream.h>

void Transforms::transformBitcode(llvm::Module &bitcode) {

  llvm::PassBuilder passBuilder;
  bool DebugPM = false;
  llvm::LoopAnalysisManager loopAnalysisManager(DebugPM);
  llvm::FunctionAnalysisManager functionAnalysisManager(DebugPM);
  llvm::CGSCCAnalysisManager ipoAnalysisManager(DebugPM);
  llvm::ModuleAnalysisManager moduleAnalysisManager(DebugPM);
  {
    // TODO: use custom pipeline
    // may want to use objC-aware passes (named *ARC*)
    llvm::AAManager AAManager = passBuilder.buildDefaultAAPipeline();
    functionAnalysisManager.registerPass([&] { return std::move(AAManager); });
  }
  passBuilder.registerModuleAnalyses(moduleAnalysisManager);
  passBuilder.registerCGSCCAnalyses(ipoAnalysisManager);
  passBuilder.registerFunctionAnalyses(functionAnalysisManager);
  passBuilder.registerLoopAnalyses(loopAnalysisManager);
  passBuilder.crossRegisterProxies(
      loopAnalysisManager, functionAnalysisManager, ipoAnalysisManager, moduleAnalysisManager);
  // end of boilerplate

  llvm::ModulePassManager modulePassManager(DebugPM);

  modulePassManager.addPass(llvm::VerifierPass());
  {
    llvm::FunctionPassManager FPM(DebugPM);
    modulePassManager.addPass(llvm::createModuleToFunctionPassAdaptor(std::move(FPM)));
  }
  modulePassManager.addPass(customPasses::StripOptNonePass());

  // More or less cribbed from standard pipeline. The passes are supposed to run in exactly this
  // order. optionally could run multiple times.
  modulePassManager.addPass(llvm::InferFunctionAttrsPass());
  {
    llvm::CGSCCPassManager IPOPM(DebugPM);
    IPOPM.addPass(llvm::PostOrderFunctionAttrsPass());
    modulePassManager.addPass(llvm::createModuleToPostOrderCGSCCPassAdaptor(std::move(IPOPM)));
  }
  modulePassManager.addPass(llvm::ReversePostOrderFunctionAttrsPass());
  {
    llvm::FunctionPassManager FPM(DebugPM);
    FPM.addPass(customPasses::DemotePhiPass());
    if (inlineAP) {
      FPM.addPass(customPasses::LoadInlinePass());
    }
    modulePassManager.addPass(llvm::createModuleToFunctionPassAdaptor(std::move(FPM)));
  }

  modulePassManager.run(bitcode, moduleAnalysisManager);

  renameOpaqueObjCTypes(bitcode);
  logger.doNothing();
}

Transforms::Transforms(CPGLogger &log, bool inlineAP) : logger(log), inlineAP(inlineAP) {}

void Transforms::destructPHINodes(llvm::Function &function) {
  std::vector<llvm::PHINode *> worklist;
  for (llvm::Instruction &instruction : llvm::instructions(function)) {
    if (auto phi = llvm::dyn_cast<llvm::PHINode>(&instruction)) {
      worklist.push_back(phi);
    }
  }

  for (llvm::PHINode *phi : worklist) {
    llvm::DemotePHIToStack(phi);
  }
}

void Transforms::renameOpaqueObjCTypes(llvm::Module &bitcode) {
  ObjCTraversal traversal(&bitcode);
  std::vector<const llvm::ConstantStruct *> worklist = traversal.objcClasses();

  for (const llvm::ConstantStruct *objcClass : worklist) {
    const llvm::ConstantStruct *objcROClass = traversal.objcClassROCounterpart(objcClass);
    std::string className = traversal.objcClassName(objcROClass);

    std::vector<std::pair<std::string, llvm::Function *>> methods =
        traversal.objcMethods(objcROClass);
    for (auto &methodPair : methods) {
      llvm::FunctionType *type = methodPair.second->getFunctionType();
      assert(type->getNumParams() >= 2 &&
             "ObjC method expected to have implicit parameters (self and _cmd)");
      auto *selfType = llvm::cast<llvm::PointerType>(type->getParamType(0));
      auto *selfStruct = llvm::cast<llvm::StructType>(selfType->getPointerElementType());
      assert(selfStruct->isOpaque() && "ObjC class types expected to be opaque");
      selfStruct->setName(className);
    }
  }
}
