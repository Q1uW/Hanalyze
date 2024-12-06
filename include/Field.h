
#ifndef PASS4DATAFLOW_FIELD_H
#define PASS4DATAFLOW_FIELD_H

#include "llvm/IR/Function.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/CFG.h"
#include "unordered_set"
#include "unordered_map"
#include "algorithm"
#include "set"
#include "vector"

using namespace llvm;
void FunctionFieldAnalysis(Function &f);
struct FunctionStructInfo;
extern std::unordered_map<std::string , FunctionStructInfo*> FunctionInfoMap;
struct FieldTypeAndOffset {
    int field_index;
    std::vector<int> offsets;

    FieldTypeAndOffset() {} 
    FieldTypeAndOffset(int type) : field_index(type) {}

    void addOffset(int offset) {
        if (std::find(offsets.begin(), offsets.end(), offset) == offsets.end()) {
            offsets.push_back(offset);
        }
    }


    friend bool operator==(const FieldTypeAndOffset& lhs, const FieldTypeAndOffset& rhs) {
        return lhs.field_index == rhs.field_index && lhs.offsets == rhs.offsets;
    }


    friend bool operator<(const FieldTypeAndOffset& lhs, const FieldTypeAndOffset& rhs) {
       // if (lhs.fieldType < rhs.fieldType) return true;
        if (rhs.field_index != lhs.field_index) return false;
        return lhs.offsets < rhs.offsets; 
    }
};



typedef struct FunctionStructInfo{
    StringRef function_name;
    std::string function_file;
    std::vector<Function*> callee_function_list;
    std::vector<Function*> SCC_callee_function_list;
    std::unordered_set<Type*> type_set;
    //std::unordered_map<Type*,std::vector<int>> type_field;
    std::unordered_map<llvm::Type*, std::vector<FieldTypeAndOffset*>> type_field; 
    std::vector<std::vector<BasicBlock*>> SCClist;
    bool hasSCC;
    bool inSCC;
    bool callGraphSCC;

    FunctionStructInfo(llvm::StringRef name, std::string file)
            : function_name(name), function_file(file),inSCC(false),hasSCC(false),callGraphSCC(false) {}

    void setPropagateSCCFlag(){
        this->inSCC = true;
        for(auto & func: callee_function_list){
            FunctionInfoMap[func->getName().str()]->setPropagateSCCFlag();
        }
    }
    void mergeTypeSets(std::unordered_set<Type*>& to, const std::unordered_set<Type*>& from) {
        to.insert(from.begin(), from.end());
    }

    void addCalleeFunction(Function* called_function) {
        if (std::find(callee_function_list.begin(), callee_function_list.end(), called_function) == callee_function_list.end()) {
            callee_function_list.push_back(called_function);
        }
    }



    void mergeTypeFields(FunctionStructInfo* other) {
        if (!other) return; 

        for (const auto& pair : other->type_field) {
            llvm::Type* type = pair.first;
            const std::vector<FieldTypeAndOffset*>& fromVec = pair.second;

            std::vector<FieldTypeAndOffset*>& toVec = this->type_field[type];

            for (const auto* fromFTO : fromVec) {
                auto it = std::find_if(toVec.begin(), toVec.end(), [&fromFTO](const FieldTypeAndOffset* toFTO) {
                    return fromFTO->field_index == toFTO->field_index;
                });

                if (it == toVec.end()) {

                    toVec.push_back(new FieldTypeAndOffset(*fromFTO)); 
                } else {
      
                    FieldTypeAndOffset* toFTO = *it;
                    for (int offset : fromFTO->offsets) {
                        toFTO->addOffset(offset);
                    }
                }
            }
        }
    }




}FunctionStructInfo;


bool hasOverlap(const FunctionStructInfo& f1, const FunctionStructInfo& f2);
extern std::vector<std::unordered_set<Function*>> globalConflictFunctionSets;
extern std::unordered_set<Function*> allConflictFunctions;

void handleGraph();
void processFunctionSet(const std::set<Function*>& functions, CallGraph& CG);
void summarizeAllConflictFunctions(CallGraph& CG);
void printAllConflictFunctions();
bool minimizeStructure(Value *V);
bool checkUses(Value *V);
#endif //PASS4DATAFLOW_FIELD_H
