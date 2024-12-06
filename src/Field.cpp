
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/Support/Casting.h"
#include "llvm/IR/Constants.h"
#include "Field.h"
#include "utils.h"
using namespace llvm;
std::unordered_map<std::string , FunctionStructInfo*> FunctionInfoMap;
//std::unordered_map<std::string , FunctionStructInfo*> FunctionMap;

void addFunctionInfo(std::string functionName, FunctionStructInfo* info) {
    FunctionInfoMap[functionName] = info;
}
int convertIndex(llvm::Value *value){
    if (const llvm::ConstantInt* CI = llvm::dyn_cast<llvm::ConstantInt>(value)) {

        int intValue = static_cast<int>(CI->getSExtValue());
        return intValue;
    } else {
        llvm::errs() << "The provided value is not a ConstantInt.\n";
    }
    return -1;
}
void propagateSCCInfo(Function &f, FunctionStructInfo * function_info){
    //vector<vector<BasicBlock*>> list;
    getSCCList(f,function_info->SCClist);

    for(auto &l: function_info->SCClist){
        if(l.size()>1){
            //find a scc unit
            function_info->hasSCC = true;
            for(auto &sccbb: l){
                for(auto &sccinst: *sccbb){
                    if(isa<CallInst>(sccinst)){
                        //find a call inside scc
                        CallInst *callInst = dyn_cast<CallInst>(&sccinst);
                        if (Function *calledFunc = callInst->getCalledFunction()) {
                            //function_info->addCalleeFunction(calledFunc);
                            function_info->SCC_callee_function_list.push_back(calledFunc);
                            FunctionInfoMap[calledFunc->getName().str()]->setPropagateSCCFlag();
                        } else {
                            llvm::errs() << "Call instruction found in function '" << f.getName();
                            llvm::errs() << "' calls an indirect function (e.g., through a function pointer).\n";
                        }
                    }
                }
            }
        }
    }
}
std::string getFunctionFileName(const llvm::Function &F) {

    if (!F.getSubprogram()) return "";


    llvm::DISubprogram *subprog = F.getSubprogram();


    return subprog->getFile()->getFilename().str();
}

bool checkUses(Value *V) {
    for (auto *U : V->users()) {
        if (isa<GetElementPtrInst>(U)) {
            return false;
        }
        if (Instruction *UserInst = dyn_cast<Instruction>(U)) {
            if (!minimizeStructure(UserInst)) {
                return false;
            }
        }
    }
    return true;
}
bool minimizeStructure(Value *V) {
    return checkUses(V);
}

void FunctionFieldAnalysis(Function &f) {
    if(f.isDeclaration())
        return;
    if(FunctionInfoMap.find(f.getName().str())!=FunctionInfoMap.end()){
        return;
    }

    FunctionStructInfo *function_info = nullptr;
    auto it = FunctionInfoMap.find(f.getName().str());
    if(it == FunctionInfoMap.end()) {

        function_info = new FunctionStructInfo(f.getName(), getFunctionFileName(f));
        FunctionInfoMap[f.getName().str()] = function_info; 
    } else {

        function_info = it->second;
    }


    propagateSCCInfo(f, function_info);

    auto &function_type_field = function_info->type_field;
    auto &function_type_set = function_info->type_set;


    for(auto& bb : f) {
        for (auto &inst : bb) {
            if (isa<GetElementPtrInst>(inst)) {
                GetElementPtrInst *gepInst = dyn_cast<GetElementPtrInst>(&inst);


                Type *basePtrType = gepInst->getPointerOperandType();


                if (PointerType *ptrType = dyn_cast<PointerType>(basePtrType)) {
                    Type *elementType = ptrType->getElementType();


                    if (StructType *structType = dyn_cast<StructType>(elementType) ) {
                     
                        if(structType== nullptr){
                            continue;
                        }
                        
                        for (auto *U : inst.users()) {
                            
                            if (Instruction *UserInst = dyn_cast<Instruction>(U)) {
                                
                                continue;
                            }
                        }
                        if (structType->hasName()) {
                            //llvm::outs() << "Struct name: " << structType->getName() << "\n";
                            if(function_type_set.find(structType)==function_type_set.end()) {
                                function_type_set.insert(structType);
                            }
                        }
                        //完成初始化。 插入偏移
                        if (gepInst->getNumOperands() >= 3) {
                            llvm::Value* firstIndex = gepInst->getOperand(1);
                            llvm::Value* secondIndex = gepInst->getOperand(2);
                            int first_index =convertIndex(firstIndex);
                            int second_index = convertIndex(secondIndex);
//                            llvm::outs()<<inst<<"\n";
//                            llvm::outs()<<"first index: "<<first_index<<" second index: "<<second_index<<" \n";
                            if(first_index!=-1 && second_index!=-1){
                                auto field_object = new FieldTypeAndOffset(first_index);
                                function_type_field[structType].push_back(field_object);
                                field_object->addOffset(second_index);
                            }

                        } else{
                            llvm::errs()<<"fatal error in getting element index\n";
                        }

                    }
                }
            }
            else if(isa<CallInst>(inst)){
                CallInst *callInst = dyn_cast<CallInst>(&inst);
                if (Function *calledFunc = callInst->getCalledFunction()) {
                    function_info->addCalleeFunction(calledFunc);
                } else {
                    llvm::errs() << "Call instruction found in function '" << f.getName();
                    llvm::errs() << "' calls an indirect function (e.g., through a function pointer).\n";
                }
            } else if(isa<InvokeInst>(inst)){
                llvm::outs()<<"Invo \n "<<inst<<"\n";
            }
        }
    }
}

bool hasOverlap(const FunctionStructInfo& f1, const FunctionStructInfo& f2) {
    for (const auto& pair1 : f1.type_field) {
        auto type = pair1.first;
        const auto& fieldsInfo1 = pair1.second;

        auto it = f2.type_field.find(type);
        if (it != f2.type_field.end()) {
            const auto& fieldsInfo2 = it->second;

            for (const auto& field1 : fieldsInfo1) {
                for (const auto& field2 : fieldsInfo2) {
                    std::vector<int> overlap;
                    std::set_intersection(field1->offsets.begin(), field1->offsets.end(),
                                          field2->offsets.begin(), field2->offsets.end(),
                                          std::back_inserter(overlap));
                    if (!overlap.empty()) {
                        return true; 
                    }
                }
            }
        }
    }
    return false;
}
extern std::vector<std::set<Function *>> SCCFunctionList;
std::vector<std::unordered_set<Function*>> globalConflictFunctionSets;
std::unordered_set<Function*> allConflictFunctions;


void processFunctionSet(const std::set<Function*>& functions, CallGraph& CG) {
    std::unordered_set<Function*> conflictSet;


    for (auto it1 = functions.begin(); it1 != functions.end(); ++it1) {
        for (auto it2 = std::next(it1); it2 != functions.end(); ++it2) {
            Function* F1 = *it1;
            Function* F2 = *it2;
            auto f1InfoIt = FunctionInfoMap.find(F1->getName().str());
            auto f2InfoIt = FunctionInfoMap.find(F2->getName().str());


            if (f1InfoIt != FunctionInfoMap.end() && f2InfoIt != FunctionInfoMap.end()) {
                if (hasOverlap(*f1InfoIt->second, *f2InfoIt->second)) {
                    conflictSet.insert(F1);
                    conflictSet.insert(F2);
                }
            }
        }
    }

 
    if (!conflictSet.empty()) {
        globalConflictFunctionSets.push_back(conflictSet);
    }
}

void summarizeAllConflictFunctions(CallGraph& CG) {
    //allConflictFunctions.clear();
    for (const auto& conflictSet : globalConflictFunctionSets) {
        for (Function* F : conflictSet) {
     
            allConflictFunctions.insert(F);
      
            auto* CGN = CG.getOrInsertFunction(F);
            for (auto& Pred : *CGN) {
                if (F == Pred.second->getFunction()) {
                    //we find the caller function in CallGraph
                    allConflictFunctions.insert(CGN->getFunction());
                }
            }
        }
    }
}
void printAllConflictFunctions() {
    for (Function* F : allConflictFunctions) {
        llvm::outs() << F->getName() << "\n";
    }
}
