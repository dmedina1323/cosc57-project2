/*  optimizer.cpp - Constructs a pass to optimize binary and comparative operations
 *  using constant folding and constant propagation
 *
 *  Damian Medina, May 2022, CS57 
 *      Extended from the provided examples (namely codegen and cee2a)
*/
#include "optimizer.hpp"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"  // for Basic Block and ReplaceWithInst
#include <llvm/IR/Constants.h>                      // for llvm::ConstantInt
#include <llvm/IR/Function.h>                       // for llvm::Function
#include <llvm/IR/Instruction.h>                    // for llvm::Instruction
#include <llvm/IR/LegacyPassManager.h>              // for llvm::legacy::PassManagerBase
#include <llvm/IR/Type.h>                           // for llvm::Type
#include <llvm/IR/Use.h>                            // for llvm::Use
#include <llvm/IR/Value.h>                          // for llvm::Value
#include <llvm/Pass.h>                              // for llvm::RegisterPass<T>
#include <llvm/Support/raw_ostream.h>               // for llvm::errs
#include <llvm/Transforms/IPO/PassManagerBuilder.h> // for llvm::RegisterStandardPasses and llvm::PassManagerBuilder
#include "llvm/IR/InstrTypes.h"                     // for BinaryOperator and ICmpInst
#include "llvm/IR/InstIterator.h"                   // for .begin() and .end()
#include "llvm/IR/InstVisitor.h"                    // Maybe don't needß

/************************ canFoldBinOp *****************************/
/* Checks if the passed in instruction is a binary operation, and if
   it is, if that operation is between two constants
 */
bool canFoldBinOp(llvm::BasicBlock::iterator &it){
    bool isBinOp = llvm::isa<llvm::BinaryOperator>(*it);
    return (isBinOp && llvm::isa<llvm::ConstantInt>((*it).getOperand(0)) && llvm::isa<llvm::ConstantInt>((*it).getOperand(1)));
}

/************************ foldBinOp *****************************/
/* Called when the instruction is a binary operation
 *
 * Determines the exact binary operation using .getOpcode() and
 *      does the corresponding arithmetic. 
 *
 * Stores the result in "value", uses it and the passed in 
 *      iterator to replace all instances of this instruction with
 *      the correct value
*/
bool foldBinOp(llvm::BasicBlock::iterator &it){
    auto const &op = llvm::cast<llvm::BinaryOperator>(*it);

    // Grab left and right hand side
    llvm::ConstantInt const &lhs = llvm::cast<llvm::ConstantInt>(*op.getOperand(0));
    llvm::ConstantInt const &rhs = llvm::cast<llvm::ConstantInt>(*op.getOperand(1));

    // Grab operator type
    llvm::IntegerType *const binOp = lhs.getType();
    llvm::ConstantInt* value = nullptr;

    // Which binop? 
    switch(op.getOpcode()){
        case llvm::Instruction::Add :
            value = llvm::ConstantInt::getSigned(binOp, lhs.getSExtValue() + rhs.getSExtValue());
            break;
        case llvm::Instruction::Sub :
            value = llvm::ConstantInt::getSigned(binOp, lhs.getSExtValue() - rhs.getSExtValue());
            break;
        case llvm::Instruction::Mul :
            value = llvm::ConstantInt::getSigned(binOp, lhs.getSExtValue() * rhs.getSExtValue());
            break;
        case llvm::Instruction::SDiv :
            value = llvm::ConstantInt::getSigned(binOp, lhs.getSExtValue() / rhs.getSExtValue());
            break;
        default:
            return false;
    }

    // This should never be the case, but thorough checking
    if (value != nullptr) {
        /****** Replace instructions accordingly ********/
        llvm::ReplaceInstWithValue(it->getParent()->getInstList(), it, value);
        return true;
    }
    return false;
}

/************************ canFoldComp *****************************/
/* Checks if the passed in instruction is a comparsion between two constants */
bool canFoldComp(llvm::BasicBlock::iterator &it){
    bool isComp = llvm::isa<llvm::ICmpInst>(*it);
    return (isComp && llvm::isa<llvm::ConstantInt>((*it).getOperand(0)) && llvm::isa<llvm::ConstantInt>((*it).getOperand(1)));
}

/************************ foldComp *****************************/
/* Called when the instruction is a comparison
 *
 * Determines the exact comparsion using .getPredicate() and
 *      returns the corresponding boolean. 
 *
 * Stores the result in bool_value and uses it to retrieve the llvm
 *      values for true and false using .getTrue() and .getFalse()
 *
 * Uses that value to replace instructions with ReplaceInstWithValue()
*/
bool foldComp(llvm::BasicBlock::iterator &it){
    auto const &op = llvm::cast<llvm::ICmpInst>(*it);
    bool bool_value = false;

    // Grab left and right hand side
    llvm::ConstantInt const &lhs = llvm::cast<llvm::ConstantInt>(*op.getOperand(0));
    llvm::ConstantInt const &rhs = llvm::cast<llvm::ConstantInt>(*op.getOperand(1));

    // Grab operator type
    llvm::Constant* value = nullptr;

    // Which comparison? 
    switch(op.getPredicate()) {

        // == 
        case llvm::ICmpInst::ICMP_EQ :
            bool_value = (lhs.getSExtValue() == rhs.getSExtValue());
            break;

        // != 
        case llvm::ICmpInst::ICMP_NE  :
            bool_value = (lhs.getSExtValue() != rhs.getSExtValue());
            break;
        
        // >
        case llvm::ICmpInst::ICMP_SGT  :
            bool_value = (lhs.getSExtValue() > rhs.getSExtValue());
            break;

        // >= 
        case llvm::ICmpInst::ICMP_SGE  :
            bool_value = (lhs.getSExtValue() >= rhs.getSExtValue());
            break;
        
        // <
        case llvm::ICmpInst::ICMP_SLT :
            bool_value = (lhs.getSExtValue() < rhs.getSExtValue());
            break;
        
        // <=
        case llvm::ICmpInst::ICMP_SLE :
            bool_value = (lhs.getSExtValue() <= rhs.getSExtValue());
            break;
        default:
            return false;
    }
    // Set value
    value = (bool_value) ? llvm::ConstantInt::getTrue(op.getType()) : llvm::ConstantInt::getFalse(op.getType());

    /****** Replace instructions accordingly ********/
    llvm::ReplaceInstWithValue(it->getParent()->getInstList(), it, value);
    return true;
}

/************************ runOnFunction *****************************/
bool OptimizerPass::runOnFunction(llvm::Function &function) {

    // Loop over blocks in function
    for (llvm::BasicBlock &block : function) {
        // Loop over instructions in block
        LOOP:for (auto iter = block.begin(); iter != block.end(); iter++){
            /* Binary Operations */
            // Is this a foldable binary operation?
            if (canFoldBinOp(iter)) {
                // Yes? Then, fold it! 
                if (foldBinOp(iter)) {
                    iter = block.begin();   // Restart at the beginning of the block
                    goto LOOP;              // Restart at the first instruction, not the second
                }
            }
            /* Relational Expressions */
            // Is this a foldable comparison?
            else if (canFoldComp(iter)){
                // Yes? Then, fold it!
                if (foldComp(iter)){
                    iter = block.begin();   // Restart at the beginning of the block
                    goto LOOP;              // Restart at the first instruction, not the second
                }
            }
        }
    }

    return false; // returning false means the overall CFG has not changed
}

/* Registering, building, and enabling this pass. Adding to the pass manager. (I think?) */
char OptimizerPass::ID = 0; // This is (somehow) necessary.
static llvm::RegisterPass<OptimizerPass> opt("optimizerpass", "Damian's optimizer pass", false, false);

void add_optimizer_to_the_pm(llvm::PassManagerBuilder const &, llvm::legacy::PassManagerBase &PM) { PM.add(new OptimizerPass()); };

static llvm::RegisterStandardPasses RegisterMyPass(llvm::PassManagerBuilder::EP_FullLinkTimeOptimizationLast, add_optimizer_to_the_pm);
