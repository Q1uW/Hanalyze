
#include "IO_field.h"

void checkForComparisons(Argument *Arg, GetElementPtrInst *GEP) {
    BasicBlock *Block = GEP->getParent();
    std::queue<BasicBlock *> queue; // Queue for BFS
    llvm::SmallSet<BasicBlock *, 8> visited; // Set to keep track of visited blocks

    // Start BFS with the block containing the GEP
    queue.push(Block);
    visited.insert(Block);

    while (!queue.empty()) {
        BasicBlock *currentBlock = queue.front();
        queue.pop();

        // Check each instruction in the current block for comparisons
        for (Instruction &Inst : *currentBlock) {
            if (auto *Cmp = dyn_cast<ICmpInst>(&Inst)) {
                for (unsigned i = 0; i < Cmp->getNumOperands(); ++i) {
                    if (Cmp->getOperand(i) == Arg) {
                        if (auto *Const = dyn_cast<ConstantInt>(Cmp->getOperand(1 - i))) {
                            llvm::outs() << "  Argument " << Arg->getName() << " compared with constant " << Const->getValue()
                                         << " in instruction " << *Cmp << " at block " << currentBlock->getName() << "\n";
                        }
                    }
                }
            }
        }

        // Enqueue all predecessors that have not been visited yet
        for (BasicBlock *Pred : predecessors(currentBlock)) {
            if (visited.insert(Pred).second) { // Insert returns a pair, and .second is true if the element was new
                queue.push(Pred);
            }
        }
    }
}
void field_operation_analyze(Function &F){

            for (auto &B : F) {
                for (auto &I : B) {
                    if (auto *GEP = dyn_cast<GetElementPtrInst>(&I)) {
                        if (GEP->getSourceElementType()->isStructTy()) {
                            //llvm::outs() << "Found GEP on struct in function " << F.getName() << ": " << *GEP << "\n";

                            //llvm::outs() << "  Operands:\n";
                            for (unsigned i = 0; i < GEP->getNumOperands(); ++i) {
                                //llvm::outs() << "    Operand " << i << ": " << *(GEP->getOperand(i)) << "\n";

                                if (auto *Arg = dyn_cast<Argument>(GEP->getOperand(i))) {
                                    Type *ArgType = Arg->getType();
                                    if (ArgType->isIntegerTy(64)) {
                                        llvm::outs() << "    Operand " << i << " is a function parameter of type uint64_t: " << Arg->getName() << "\n";
                                        if (DISubprogram *SP = F.getSubprogram()) {
                                            llvm::outs() << "Source File: " << SP->getFilename() <<" Function Name: "<<F.getName()<< "\n";
                                        } else {
                                            llvm::outs() << "Source File: No debug info available\n";
                                        }
                                        checkForComparisons(Arg, GEP);
                                    }
                                }
                            }
                            bool isUsedForWriting = false;
                            for (auto *U : GEP->users()) {
                                if (auto *SI = dyn_cast<StoreInst>(U)) {
                                    if (SI->getPointerOperand() == GEP) {
                                        isUsedForWriting = true;
                                        //llvm::outs() << "  Result used for writing by: " << *SI << "\n";
                                    }
                                }
                            }

                            if (!isUsedForWriting) {
                                //llvm::outs() << "  Result not used for writing.\n";
                            }

                            if (GEP->hasIndices()) {
                                auto *IndexedType = GEP->getResultElementType();
                                if (IndexedType->isPointerTy() || IndexedType->isArrayTy()) {
                                    //llvm::outs() << "  Indexed field is of type: " << *IndexedType << "\n";
                                }
                            }
                        }
                    }
                }
            }
}
void IO_field_taint(Function &F){
    for (auto &B : F) {
        for (auto &I : B) {
            if (auto *GEP = dyn_cast<GetElementPtrInst>(&I)) {
                for (auto *User : GEP->users()) {
                    if (auto *Store = dyn_cast<StoreInst>(User)) {
                        if (Store->getPointerOperand() == GEP) {
                            checkForParamUsage(Store->getValueOperand(), F);
                        }
                    }
                }
            }
        }
    }
}
void checkForParamUsage(Value *V, Function &F) {
    for (auto &Arg : F.args()) {
        if (isUsedBy(V, &Arg)) {
            llvm::outs() << "Function " << F.getName() << " indirectly writes parameter "
                         << Arg.getName() << " via GEP to memory.\n";
        }
    }
}

bool isUsedBy(Value *V, Value *Target) {
    if (V == Target) {
        return true;
    }
    if (auto *Op = dyn_cast<Operator>(V)) {
        for (unsigned i = 0, e = Op->getNumOperands(); i != e; ++i) {
            if (isUsedBy(Op->getOperand(i), Target)) {
                return true;
            }
        }
    }
    return false;
}