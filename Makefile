all: debug release

debug:
	rm -rf build-dbg
	mkdir build-dbg
	cd build-dbg && cmake .. -DCMAKE_BUILD_TYPE=Debug && make -j$(nproc)

release:
	rm -rf build
	mkdir build
	cd build && cmake .. -DCMAKE_BUILD_TYPE=Release && make -j$(nproc)
