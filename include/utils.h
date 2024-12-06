

#ifndef PASSANALYSISTEST_UTILS_H
#define PASSANALYSISTEST_UTILS_H



#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Function.h"
#include "llvm/ADT/SCCIterator.h"
#include "llvm/IR/CFG.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/CallGraphSCCPass.h"
using namespace std;
using namespace llvm;
#define DEBUG true
#ifndef OP
#define OP errs()
#endif
typedef int  FieldIdTy;
typedef string FieldNameTy;
typedef string FunctionNameTy;



void getSCCList(Function &currF, vector<vector<BasicBlock*>> &List);
void fixCallGraph(Module &M, CallGraph *CG);
bool isAvailableExternally(Function &F);
#endif //PASSANALYSISTEST_UTILS_H
