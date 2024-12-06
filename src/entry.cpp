#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/Support/raw_ostream.h>
#include "llvm/Analysis/CFG.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/ADT/SCCIterator.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"

#include "GlobalVistor.h"
#include "AliasContext.h"
#include "AliasInstvistor.h"
#include "Field.h"
#include "IO_field.h"
#include "json.hpp"
#include <iostream>
#include <fstream>
using namespace llvm;
//enum FunctionTag{
//    Hybrid,
//    Read,
//    WriteBack,
//    Process,
//};
//struct  SCCState{
//    SmallVector<BasicBlock*,100> bb_list;
//    map<BasicBlock*,SmallVector<Function *,10>> SCC_bb_succ_funciton;
//};
//struct FunctionState{
//    StringRef func_name;
//    SmallVector<SCCState,100> SCC;
//    int weight;
//    FunctionTag func_tag;
//    pair<Instruction *, int> cmp_tag;
//    vector<Function *> func_list;
//};
//extern std::unordered_map<StringRef, FunctionStructInfo*> FunctionMap;
std::vector<std::set<Function *>> SCCFunctionList;
namespace {

    class FunctionInfoPass final : public PassInfoMixin<FunctionInfoPass> {
        CallGraph *CG;
        std::set<Function *> SCCFunctions;
        bool final_flag = false;
       // vector<FunctionState*> func_state;
    public:
        void examineCallGraph(CallGraph &CG) {
            for (auto &Node : CG) {
                const Function *Func = Node.first;
                CallGraphNode *CGN = Node.second.get();

                if (Func) {
                    outs() << "Function: " << Func->getName() << "\n";
                } else {
                    
                    outs() << "External Node or Indirect Call\n";
                }
                for (auto &Call : *CGN) {
                    if (Call.second->getFunction()) {
                        outs() << "  Calls: " << Call.second->getFunction()->getName() << "\n";
                    } else {
                        outs() << "  Calls: External or Indirect Call\n";
                    }
                }
            }
        }

        PreservedAnalyses run([[maybe_unused]] Module &M, ModuleAnalysisManager &MAM) {
            final_flag= true;
            outs() << "Dataflow information Pass"
                   << "\n";
            CG = &MAM.getResult<CallGraphAnalysis>(M);
            fixCallGraph(M,CG);

            //examineCallGraph(*CG);
            //we need to fix the CG by add the new nodes
            //and for the call node, we still need to add more
            //function pointer analysis
            runOnModule(M);
            return PreservedAnalyses::all();
        }
        void getAnalysisUsage(AnalysisUsage &AU) const  {
            AU.addRequired<CallGraphWrapperPass>();
            AU.addRequired<DominatorTreeWrapperPass>();
            //AU.addRequired<PostDominatorTreeWrapperPass>();
        }

        void collectSCC(CallGraphSCC &SCC){
            llvm::outs()<<"enter Callgraph scc unit \n";
            std::set<Function* > Funtions;
            for(CallGraphNode *CGN: SCC){
                //Funtions.insert(CGN->getFunction());
                Function* Caller = CGN->getFunction(); 
                if (Caller && !Caller->isDeclaration() && !Caller->isIntrinsic()) {
                    llvm::outs() << "Caller: " << Caller->getName() << "\n";
                } else if (!Caller) {
                    llvm::outs() << "Caller is an external or indirect call\n";
                }

                for (auto &Call : *CGN) {
                    if (Call.second->getFunction()) {
                        //outs() << "  Calls: " << Call.second->getFunction()->getName() << "\n";
                        if(Call.second->getFunction()->isIntrinsic()) continue;
                        Funtions.insert(Call.second->getFunction());
                    } else {
                        outs() << "  Calls: External or Indirect Call\n";
                    }
                }
            }
            for(auto &f: Funtions){
                llvm::outs()<<"\t"<<f->getName()<<"\n";
            }
            if(Funtions.size()>1){
                //we find a SCC unit in CallGraph
                for(Function *f : Funtions){
                    SCCFunctions.insert(f);
                }
                SCCFunctionList.emplace_back(SCCFunctions);
            }
        }

        bool runOnModule(Module &m){
            getchar();
            for(auto& f:m){
                field_operation_analyze(f);
//                if(f.getName().startswith("edu")){
//                    llvm::outs()<<f.getName()<<"     ";
//                    field_operation_analyze(f);
//                    //IO_field_taint(f);
//                }else
//                    continue;
                FunctionFieldAnalysis(f);
            }
            //getchar();
            //return false;
            scc_iterator<CallGraph*> CGI = scc_begin(CG);
            CallGraphSCC CurSCC(*CG, &CGI);

            while(!CGI.isAtEnd()){
                const std::vector<CallGraphNode *> &NodeVec = *CGI;

                CurSCC.initialize(NodeVec);
                ++CGI;
                collectSCC(CurSCC);
            }
            //Pass::getAnalysis<>()
            //CallGraph &CG = getAnalysis<CallGraphWrapperPass>().getCallGraph();
            //            ofstream ofile("result.txt");
//            nlohmann::json jfile;

//            for(auto &f : m){
//                FunctionFieldAnalysis(f);
//                nlohmann::json j;
//                int compare_inst_num = 0;
//                int store_inst_num = 0;
//                int load_inst_num = 0;
//                int operator_inst_num = 0;
//                int scc_state_num = 0;
//                for(auto &b: f){
//                    for(auto &i: b){
//                        if(isa<CmpInst>(i)){
//                            compare_inst_num+=1;
//                        } else if(isa<StoreInst>(i)){
//                            store_inst_num++;
//                        } else if(isa<LoadInst>(i)){
//                            load_inst_num++;
//                        }else if(isa<BinaryOperator>(i) ){
//                            operator_inst_num++;
//                        }
//                    }
//                }
//                vector<vector<BasicBlock*>> SCCList;
//                if(f.isDeclaration()){continue;}
//                getSCCList(f,SCCList);
//
//                outs()<<f.getName()<<" :[\n";
//                j["Function"] = nlohmann::json::object();
//                //j["Function"]["Name"]  = nlohmann::json::array();
//                j["Function"]["Name"]=f.getName();
//                j["Function"]["SCCList"] = nlohmann::json::object();
//                j["Function"]["SCCList"]["CMP"] =nlohmann::json::object();
//                j["Function"]["SCCList"]["CMP"]["Tag"] =nlohmann::json::array();
//                j["Function"]["CallList"] =nlohmann::json::array();
//
//
//
//                //j["Function"]["SCCList"]["CMP"]["Tag"]["FunctionCall"] =nlohmann::json::array();
//                if(SCCList.size()>=1) {
//                    outs() << "     SCC list:[\n";
//                    for (auto &BBList: SCCList) {
//                        if(BBList.size()==1){continue;}
//                        for (auto &bb: BBList) {
//                            for (auto &i: *bb) {
//                                if(isa<CmpInst>(i)){
//                                    scc_state_num++;
//                                }
//                                DILocation *loc = i.getDebugLoc();
//                                if (loc != nullptr && (isa<CmpInst>(i) || isa<SwitchInst>(i))) {
//                                    outs() << "          Inst: " << i << " SCC block source loc\n           line:" << (int) loc->getLine()
//                                           << " , column: "
//                                           << (int) loc->getColumn() << " Hash tag: "
//                                           << (loc->getLine() * 33 + loc->getColumn()) * 33 << "\n";
//                                    //fcall["Hash"] =
//                                    //j["Function"]["SCCList"]["CMP"]["Tag"].push_back((loc->getLine() * 33 + loc->getColumn()) * 33);
//                                    nlohmann::json fcall;
//                                    int hash =(loc->getLine() * 33 + loc->getColumn()) * 33;
//                                    if(!fcall.contains(to_string(hash))){
//                                        fcall[to_string(hash)] = nlohmann::json::array();
//                                    }
//
//                                    Instruction *terminator = bb->getTerminator();
//                                    unsigned  int succnum = terminator->getNumSuccessors();
//
//                                    for(int i =0; i< succnum;i++){
//                                        BasicBlock *succ = terminator->getSuccessor(i);
//                                        //std::cout << "Successor Block: " << succ->getName().str() << "\n";
//
//                                        for (Instruction &I : *succ) {
//                                            if (auto *call = dyn_cast<CallInst>(&I)) {
//
//                                                if (Function *calledFunc = call->getCalledFunction() ) {
//                                                    std::cout << "               Function call: " << calledFunc->getName().str() << "\n";
//                                                    //fcall["Hash"] =(loc->getLine() * 33 + loc->getColumn()) * 33;
//
//
//                                                    //fcall["FunctionCall"] = nlohmann::json::array();
//                                                    fcall[to_string(hash)].push_back(calledFunc->getName());
//                                                    //j["Function"]["SCCList"]["CMP"]["Tag"]["FunctionCall"].push_back(calledFunc->getName());
//                                                }
//
//                                            }
//                                        }
//
//
//                                    }//succ
//                                    if(!fcall.is_null())
//                                        j["Function"]["SCCList"]["CMP"]["Tag"].push_back(fcall);
//
//                                }
//                            }
//
//                        }
//                    }
//                    outs() << "     ]\n";
//                }
//                //alias vistor
////                if(f.getName().str()=="main"){
////                    int a;
////                    cin>>a;
////                    analyze(m,f);
////                }
//    //function call graph generator
//            for(auto &bb : f){
//                for(auto &inst: bb){
//                    if(isa<CallInst>(inst)){
//
//                        auto *cb = dyn_cast<CallBase>(&inst);
//                        if(cb== nullptr) continue;
//                        if(cb->getCalledFunction()== nullptr) continue;
//                        if(cb->getCalledFunction()->isDeclaration()){continue;}
//                        outs()<<"       call: "<<cb->getCalledFunction()->getName()<<";\n";
//                        j["Function"]["CallList"].push_back(cb->getCalledFunction()->getName());
//                    } else if(isa<StoreInst>(inst)){
//                       // if(isa<PointerType>(inst.getType()))
//                        if(dyn_cast<PointerType>(inst.getOperand(0)->getType())) {
//                            auto *ptr = inst.getOperand(0)->getType();
//                            if(ptr== nullptr ) continue;
//                            if(ptr->getPointerElementType()->isFunctionTy()) {
//                                auto *func = dyn_cast<Function>(inst.getOperand(0)->stripPointerCasts());
//                                if(func->hasName()){
//                                outs() << "      Function Pointer store " << func->getName() << "; \n";
//                                j["Function"]["CallList"].push_back(func->getName());
//                                }
//                            }
//                        }
//                    }
//                }
//            }
//            outs()<<"SCC CMP INST NUM:"<<scc_state_num<<"\n";
//            j["Function"]["SCC_CMP_INST"] = scc_state_num;
//            outs()<<"CMP INST NUM: "<<compare_inst_num<<"\n";
//            j["Function"]["CMP_INST_NUM"] = compare_inst_num;
//            if(scc_state_num<compare_inst_num && scc_state_num>=1){
//                outs()<<"Find the defence check before the logic\n";
//            }
//            outs()<<"OPT INST NUM: "<<operator_inst_num<<"\n";
//            outs()<<"]\n";
//            jfile["Flist"].push_back(j);
//            }
//
//            //return false;
//            ofile<<setw(4)<<jfile;
        }
        void analyze(Module &m,Function &f);
//        ~FunctionInfoPass(){
//            if(FunctionMap.size()>=1){
//                llvm::outs()<<"aaa\n";
//            }
//        }
        ~FunctionInfoPass(){
            if(final_flag){
                llvm::outs()<<"Key SCC infor:  \n";
                if(SCCFunctionList.size()>0){
                   for(auto scc_function_list: SCCFunctionList){
                       processFunctionSet(scc_function_list,*CG);
                       summarizeAllConflictFunctions(*CG);
                       printAllConflictFunctions();
                   }
                } else{
                    llvm::errs()<<"no scc function\n";
                }
            }
        }
    }; // class FunctionInfoPass
    void FunctionInfoPass::analyze(Module &m, Function &f) {
        GlobalVistor<AliasContext> globalVistor(m,f);
        globalVistor.addCallback<AliasInstVistor>();
        globalVistor.analyze();

    }

} // anonymous namespace

extern "C" PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return {
            .APIVersion = LLVM_PLUGIN_API_VERSION,
            .PluginName = "FunctionInfo",
            .PluginVersion = LLVM_VERSION_STRING,
            .RegisterPassBuilderCallbacks =
            [](PassBuilder &PB) {
                PB.registerPipelineParsingCallback(
                        [](StringRef Name, ModulePassManager &MPM,
                           ArrayRef<PassBuilder::PipelineElement>) -> bool {
                            if (Name == "function-info") {
                                MPM.addPass(FunctionInfoPass());
                                return true;
                            }
                            return false;
                        });
            } // RegisterPassBuilderCallbacks
    };        // struct PassPluginLibraryInfo
}