mkdir -p build

DEV_COMPILER_FLAGS='-DHANDMADE_SLOW=1 -DHANDMADE_INTERNAL=1
-g -Werror -Wall -Wextra -Wpedantic
-Wno-unused-parameter
-Wno-unused-variable -Wno-unused-but-set-variable -Wno-c++20-designator -Wno-unused-function
-Wno-null-dereference -Wno-macro-redefined -Wno-writable-strings -Wno-nested-anon-types -Wno-language-extension-token -Wno-gnu-anonymous-struct 
-fno-rtti -fno-exceptions -ffunction-sections -fdata-sections'

DEV_LINKER_FLAGS="-Wl,--gc-sections"

clang++ $DEV_COMPILER_FLAGS -o build/handmade code/linux32-handmade.cpp $DEV_LINKER_FLAGS -lxcb -lxcb-randr
clang++ $DEV_COMPILER_FLAGS -o build/handmade.so code/handmade.cpp $DEV_LINKER_FLAGS -shared
