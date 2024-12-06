

#include "utils.h"
#include "llvm/ADT/SCCIterator.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
void getSCCList(Function &currF, vector<vector<BasicBlock*>> &List){
    Function* F = &currF;
    for(auto I = scc_begin(F),IE = scc_end(F); I!=IE; ++I){
        const vector<BasicBlock *> &constvec = *I;
        vector<BasicBlock*> SCC(constvec.rbegin(), constvec.rend());
        List.push_back(move(SCC));
    }
    reverse(List.begin(),List.end());
}
bool isAvailableExternally(Function &F){
    GlobalValue::LinkageTypes L = F.getLinkage();
    return GlobalValue::isAvailableExternallyLinkage(L);
}
void fixCallGraph(Module &M, CallGraph *CG){
    for(auto  &F: M.getFunctionList()){
        if(F.isDeclaration() || isAvailableExternally(F)){
            continue;
        }
        for(auto &BB:F){
            for(auto &I: BB) {
                if (CallBase * CB = dyn_cast<CallBase>(&I)) {
                    if (CB->isInlineAsm()) continue;

                    Function *Called = dyn_cast<Function>(CB->getCalledOperand()->stripPointerCasts());
                    if (!Called || Called->isDeclaration() || isAvailableExternally(*Called) ||
                        Called->isIntrinsic())
                        continue;

                    if (CB->getCalledFunction() == nullptr) {
                        CallGraphNode *Node = CG->getOrInsertFunction(&F);
                        Node->addCalledFunction(CB, CG->getOrInsertFunction(Called));
                    }
                }
                //we still need to add the function pointer analysis
                //such as the mov [somewhere], Function Addr
                //to the CallGraph
                //in order to fix the pointer call
                if(isa<StoreInst>(I)) {
                    if (dyn_cast<PointerType>(I.getOperand(0)->getType())) {
                        auto *ptr = I.getOperand(0)->getType();
                        if (ptr == nullptr) continue;
                        if (ptr->getPointerElementType()->isFunctionTy()) {
                            auto *Called = dyn_cast<Function>(I.getOperand(0)->stripPointerCasts());
                            if (!Called || Called->isDeclaration() || isAvailableExternally(*Called) ||
                                Called->isIntrinsic())
                                continue;

                            CallGraphNode *Node = CG->getOrInsertFunction(&F);
                            Node->addCalledFunction(nullptr, CG->getOrInsertFunction(Called));
                        }
                    }
                }//end function pointer

            }
        }
    }
}