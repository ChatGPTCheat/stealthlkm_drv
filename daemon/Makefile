CPP_COMPILER := /opt/android-ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android33-clang++
CPP_FLAGS := -std=c++17 -O3 -Wall -pthread -I../kernel/ -static-libstdc++

all: daemon tester

daemon:
	$(CPP_COMPILER) $(CPP_FLAGS) -o stealth_daemon memory_engine.cpp map_parser.cpp stealth_daemon.cpp

tester:
	$(CPP_COMPILER) $(CPP_FLAGS) -o engine_tester memory_engine.cpp map_parser.cpp main.cpp

clean:
	rm -f stealth_daemon engine_tester
