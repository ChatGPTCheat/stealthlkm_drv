CPP_COMPILER := /opt/android-ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android33-clang++
CPP_FLAGS := -std=c++17 -O3 -Wall -pthread -I../kernel/ -static-libstdc++

all: daemon tester

daemon:
	$(CPP_COMPILER) $(CPP_FLAGS) -o stealth_daemon memory_engine.cpp map_parser.cpp pid_utils.cpp stealth_daemon.cpp

tester:
	$(CPP_COMPILER) $(CPP_FLAGS) -o engine_tester memory_engine.cpp map_parser.cpp pid_utils.cpp main.cpp

test:
	$(CPP_COMPILER) $(CPP_FLAGS) pid_utils.cpp test_pid_resolution.cpp -o test_pid_resolution || g++ -std=c++17 -O3 -Wall pid_utils.cpp test_pid_resolution.cpp -o test_pid_resolution
	./test_pid_resolution

clean:
	rm -f stealth_daemon engine_tester test_pid_resolution
