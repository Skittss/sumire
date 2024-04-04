@if not exist build\msvc mkdir build\msvc
@cd build\msvc
cmake -S ../../ -B .
@cd ../../