# TinyJVM — A Minimal JVM Implementation in C

This repository contains a minimal Java Virtual Machine (JVM) that I implemented in C as part of a systems programming project. The goal of TinyJVM is to provide a low-level, educational look at how Java `.class` files are loaded, parsed, and executed at the bytecode level.

## Features

- **Class File Loader**  
  - Parses Java `.class` file structure (constant pool, fields, methods, attributes).  
  - Implemented in `read_class.c` / `read_class.h`.

- **Bytecode Interpreter**  
  - Supports a subset of core JVM bytecodes (stack operations, arithmetic ops, control flow).  
  - Main interpreter loop is implemented in `jvm.c`.

- **Heap & Memory Model**  
  - Simple heap allocator for objects and arrays.  
  - Reference handling and dynamic object allocation in `heap.c`.

- **Execution Engine**  
  - Loads a class, finds `main`, and interprets bytecode instructions.  
  - Implements operand stack, local variables, and constant pool resolution.

## 📁 Project Structure

