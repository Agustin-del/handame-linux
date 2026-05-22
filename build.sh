mkdir -p build

clang++ -DHANDMADE_SLOW=1 -DHANDMADE_INTERNAL=1 -g -Wall -o build/handmade code/linux32-handmade.cpp -lxcb
