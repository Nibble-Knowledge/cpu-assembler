Building AS
===

Building AS4 is simple, just type make. If you type make here a statically linked and optimised version of the assembler will be built. Type "make debug" for unoptimised code that is profile and debugger friendly.

If you cd in src, typing make will default to the "debug" compilation. Type "make fast" to get the optimised version.

Of course, "make clean" will remove all generated code artefacts.