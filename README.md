Assembler Project

Overview
---------
This project is an assembler developed as part of the "System Programming Laboratory" (Open University of Israel) course. It translates pseudo-assembly code into several machine-readable formats, primarily producing an .ob file that encodes instructions in an encoded-4 base. The assembler follows a two-pass approach, along with a preprocessing stage, to ensure that all labels, instructions, and directives are correctly translated.

Features
---------
- **Preprocessing stage**: Handles macros and prepares the assembly code for the main passes.
- **First pass**: Identifies and records labels, addresses, and directives while detecting syntax errors.
- **Second pass**: Generates the machine code and output files based on the information gathered in the first pass.
- Outputs several files, including:
  - `.am`: Source code following macro processing.
  - `.ob`: Encoded instructions.
  - `.ent`: Entry labels.
  - `.ext`: External labels used.

Directory Structure
--------------------
- `assembler.c`, `assembler.h`: Main program files containing the entry point and core functions.
- `first_pass.c`, `first_pass.h`: Functions related to the first pass of the assembler.
- `second_pass.c`, `second_pass.h`: Functions related to the second pass of the assembler.
- `pre_process.c`, `pre_process.h`: Functions for handling macros and preparing the input.
- `general_lib.c`, `general_lib.h`: General helper functions used throughout the assembler.
- `makefile`: Build automation to compile the project.

Build Instructions
-------------------
To build the assembler, use the `make` utility. Run the following command from the project directory:

    make

This will compile all source files and produce the executable named `assembler`.

Usage
------
Once compiled, you can use the assembler to translate an assembly file (named <source_file.as>) as follows:

    ./assembler <source_file>

You can enter several files at once, by separating their names with a space - ` `:

    ./assembler <source_file_1> <source_file_2>

If your file name or path contain spaces, make sure to include quotation marks for proper operation:

    ./assembler "folder/sub folder/<source_file_1>" <source_file_2>

Replace `<source_file>` with the name of your pseudo-assembly file (excluding the `.as` extension).

Errors 
--------
The assembler will report errors during the assembly process. Make sure to check the output for any messages indicating syntax errors, memory allocation issues, or incorrect file paths. 
