# Compilatori-pt.2---1-assignment

## Come compilare il passo:
In /build:  
export LLVM_DIR=/usr/lib/llvm-19/bin  
cmake -DLT_LLVM_INSTALL_DIR=$LLVM_DIR ..  
make  
export PATH=/usr/lib/llvm-19/bin:$PATH  
opt -load-pass-plugin ./libLocalOpts.so -passes=local-opts ../test/Foo.ll -o ../test/Foo.optimized.bc  
llvm-dis ../test/Foo.optimized.bc -o ../test/Foo.optimized.ll  
