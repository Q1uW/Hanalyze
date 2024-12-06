
#ifndef PASS4DATAFLOW_IO_FIELD_H
#define PASS4DATAFLOW_IO_FIELD_H
#include "utils.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Operator.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/CFG.h"
#include "llvm/ADT/SmallSet.h"

#include <queue>
void field_operation_analyze(Function &f);
void IO_field_taint(Function &F);
void checkForParamUsage(Value *V, Function &F);
bool isUsedBy(Value *V, Value *Target);
#endif //PASS4DATAFLOW_IO_FIELD_H
