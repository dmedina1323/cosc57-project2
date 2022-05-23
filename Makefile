LLVM-VERSION:=-10
CXX := clang++$(LLVM-VERSION)
CC := clang$(LLVM-VERSION)
OPT = opt$(LLVM-VERSION)
LLVM-CONFIG := llvm-config$(LLVM-VERSION)
CFLAGS := `$(LLVM-CONFIG) --cflags` -fPIC -I/usr/include/llvm-10/
CXXFLAGS := `$(LLVM-CONFIG) --cxxflags` -fPIC -I/usr/include/llvm-10/
STYLE := "{BasedOnStyle: llvm, IndentWidth: 4, ColumnLimit: 200}"

# In order to echo the literal text '$1' into a file, I have to double-escape it as '\$$1'.
# That took too long to figure out!
# $(CC) -O -S -Xclang -disable-llvm-passes -emit-llvm \$$1 -o /dev/stdout
optimize.sh: optimizer.so
	echo "$(OPT) -S -load=./optimizer.so --mem2reg --optimizerpass \$$1" > optimize.sh
	chmod +x optimize.sh

optimizer.so: optimizer.o
	$(CC) $(CFLAGS) -shared optimizer.o -o optimizer.so

optimizer.o: optimizer.cpp
	$(CXX) $(CXXFLAGS) -c optimizer.cpp -o $@

clean:
	rm -f optimizer.so optimizer.o optimize.sh
