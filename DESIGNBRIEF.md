## Function Pass Design:

My function pass design is dependent on two loops: the `BasicBlock` loop and the instructions loop. In the former, I am simply looping through each `BasicBlock` in the current function. The latter is where all of the work takes place. In this for-each loop, I'm looping through each instruction in the `BasicBlock`. I do this by pulling an iterator from the `BasicBlock` using the `llvm::BasicBlock::begin()` function, which returns a `llvm::BasicBlock::iterator`. I decided to loop through the instructions in this way because I decided to use `ReplaceInstWithValue()` to do all the leg work of identifying all instances of an instruction and replacing them with the desired value, and this function takes an `llvm::BasicBlock::iterator` as a parameter. It is also easy to grab that function's first parameter, `llvm::BasicBlock::InstListType`, from an `llvm::BasicBlock::iterator` using the `getParent()` then `getInstList()` methods built into the iterator class.

Then, in the instruction loop, I call four major functions. Two of these check if the instruction is a **Binary Operation** or **Comparative Operation** - using `llvm::isa<llvm::BinaryOperator>()` and `llvm::isa<llvm::ICmpInst>()`, respectively - between two constants - using `llvm::isa<llvm::ConstantInt>()` of each operand. These checks are important as they allow me to do casts later on without the risk of crashing if the cast is undoable. The next two functions do the work of replacing instructions with values. They use the aforementioned casts, `.getOpcode()` (for **Binary Operations**), and `.getPredicate()` (for **Comparative Operations**). For a binary operation, I get the value that will be passed to `ReplaceInstWithValue()` using ``llvm::ConstantInt::getSigned()``, which as I understand it, essentially turns the passed in number into a value that plays nice with the specified type. I use a switch-case statement and the corresponding enumerated types for comparisons. I **return false** if the *opcode* or *predicate* could not be matched. At the end of the function, I call `ReplaceInstWithValue()` and **return true** if that point was reached.

Finally, upon return from a successful fold, I reset the iterator to the top and use `goto` to jump straight to the top of the loop without incrementing as this prevents starting at the second instruction, rather than the first.

## Steady State:
Because of my implementation, the average number of passes it takes to reach steady state is not very interesting. Because I simply visit the next unvisited instruction each iteration, even when an instruction is replaced, the number of passes is simply n, where there are n-instructions to be visited and potentially optimized.

## Functions:
Nearly all the functions I used are described in the design description (that's why it's so long!)

## Problems/Issues:
The most significant problem I encountered was figuring out the best way to loop through the instructions. I think the approach I ended up taking works well, and makes it easy to call ReplaceInstWithValue, but perhaps requires more casting than other approaches.

## To Run:

    make clean; make
    ./optimize.sh filename.ll