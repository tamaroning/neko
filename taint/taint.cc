#include "llvm/IR/Function.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include <queue>
#include <unordered_set>

using namespace llvm;

// Custom hash function for llvm::StringRef
namespace std {
template <> struct hash<llvm::StringRef> {
  size_t operator()(const llvm::StringRef &str) const {
    return std::hash<std::string>{}(str.str());
  }
};
} // namespace std

namespace {

// List of functions that are considered sources of taint
const std::unordered_set<StringRef> taint_sources = {"__isoc99_scanf",
                                                     "getenv_s"};

void visitor(Function &f) {
  outs() << "Running Taint Analysis on function: " << f.getName() << "\n";

  std::unordered_set<Value *> taintedValues;
  std::queue<Value *> worklist;

  // Initialize tainted values (e.g., function arguments)
  // find callsites of taint sources
  for (auto &bb : f) {
    for (auto &inst : bb) {
      if (auto *call = dyn_cast<CallInst>(&inst)) {
        if (Function *callee = call->getCalledFunction()) {
          outs() << "Callee name: " << callee->getName() << "\n";

          if (taint_sources.count(callee->getName())) {
            for (auto &arg : call->operands()) {
              errs() << "Tainted value: " << *arg << "\n";
              worklist.push(arg);
            }
          }
        }
      }
    }
  }

  // Propagate taint through the function
  while (!worklist.empty()) {
    Value *current = worklist.front();
    worklist.pop();

    for (auto *user : current->users()) {
      if (auto *inst = dyn_cast<Instruction>(user)) {
        if (taintedValues.insert(inst).second) { // If newly tainted
          worklist.push(inst);
          errs() << "Tainted instruction: " << *inst << "\n";
          inst->getDebugLoc().print(errs());
        }
      }
    }
  }
}

class TaintAnalysisPass : public PassInfoMixin<TaintAnalysisPass> {
public:
  // Run the pass on a given function
  PreservedAnalyses run(Function &f, FunctionAnalysisManager &) {
    visitor(f);
    return PreservedAnalyses::all();
  }

  static bool isRequired() { return true; }
};

} // end anonymous namespace

llvm::PassPluginLibraryInfo getTaintPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "Taint", LLVM_VERSION_STRING,
          [](PassBuilder &pb) {
            pb.registerPipelineParsingCallback(
                [](StringRef name, FunctionPassManager &fpm,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (name == "taint") {
                    fpm.addPass(TaintAnalysisPass());
                    return true;
                  }
                  return false;
                });
          }};
}

// This is the core interface for pass plugins. It guarantees that 'opt' will
// be able to recognize HelloWorld when added to the pass pipeline on the
// command line, i.e. via '-passes=hello-world'
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getTaintPluginInfo();
}
