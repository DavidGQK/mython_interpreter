# Mython

`Mython` - Python-like interpreter.
Language is dynamically typed, supports classes and inheritance.

## How it works
> 1. Prepare code file with extension `*.my`
> 2. Start terminal in folder which contains `Mython.exe`
> 3. Enter following command `Mython <you_file_with_code.my> <output_filename.txt>`

## Syntax
Examples of all available features you can find in `test_it/test.my`

## Compilation and Installation
For compilation you need to have `Cmake`
> 1. Create folder for compilation e.g. `mython_cmake`
> 2. Copy whole github repository to that folder and rename it e.g. `mython`
> 3. Create another folder `build` in `mython_cmake` and go there
> 4. Start terminal in `build` folder
> 5. For compilation enter `cmake ../mython`
> 6. To turn tests off, add following key `"-DTESTING=OFF"` to the previous command
> 7. Enter following command for building `cmake --build . --verbose` 
> 8. Enter `ctest` for launching tests if you want
> 9. After building completed go to `Debug` folder where you can find `Mython.exe`