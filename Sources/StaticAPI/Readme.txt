These API source files should be used when linking specific Serious Engine projects together under Windows platforms.
Projects that need to be properly statically linked have an inline source file (.inl) of the same name as the project folder under "Sources/".

These files are supposed to be included *only once* in the entire project (dynamic library, shared object or an executable file), preferably in some main source file in order to define all the necessary functionality and also link the library itself.
They can be included in a file such as "StdH.cpp", "Main.cpp", "<project name>.cpp" or similar.

To generate special lists of objects from specific modules, such as entity classes, use scripts under "Scripts/". The resulting files are supposed to be placed under "Lists/".
