# Dependencies
## CMakeLists.txt
- `PROJECT_NAME` variable set to the name of the project
- `BULK_PATH` variable set to the path of your `Bulk/include` folder
- `EXEC` variable set to the name of the file containing the `main` function
- `CMAKE_CXX_COMPILER` variable set to the path of the `g++` compiler
- `CMAKE_C_COMPILER` variable set to the paths of the `gcc` compiler

# Compiling
In the current directory, run `cmake .` to generate the `Makefile`. Then, run `cmake --build ./` in order to compile the main file.

# Running
After compiling the files, the executables can be found in the same directory. They can then be run in the terminal using `./[project_name]`