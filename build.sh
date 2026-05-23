mkdir -p build

clang++ -DHANDMADE_SLOW=1 -DHANDMADE_INTERNAL=1 -g \
  -Werror -Wall -Wextra -Wpedantic -Wno-null-dereference \
  -Wno-macro-redefined -Wno-writable-strings -Wno-nested-anon-types\
  -Wno-language-extension-token -Wno-gnu-anonymous-struct\
  -fno-rtti -fno-exceptions \
  -ffunction-sections -fdata-sections\
  -o build/handmade code/linux32-handmade.cpp -lxcb -Wl,--gc-sections
