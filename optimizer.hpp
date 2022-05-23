/* optimizer.hpp - header file for optimizer.cpp
 *  Damian Medina, May 2022, CS57
 *      Extended from the provided examples
*/

#pragma once

#include <llvm/Pass.h>        // for llvm::FunctionPass
#include <llvm/IR/Function.h> // for llvm::Function

// Change the DEBUG_TYPE define to the friendly name of your pass...for some reason?
#define DEBUG_TYPE "optimizerpass"

struct OptimizerPass : public llvm::FunctionPass {
    static char ID;
    // This constructor just calls the parent class's constructor.
    OptimizerPass() : llvm::FunctionPass(ID) {}

    bool runOnFunction(llvm::Function &F);
};
