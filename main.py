# tinyjvm.py
# A minimal stack-based VM + assembler for learning JVM concepts.
# Run: python3 tinyjvm.py

import sys
from typing import List, Dict, Tuple

# ----------------------------
# Opcode definitions
# ----------------------------
class Op:
    PUSH = "PUSH_CONST"
    LOAD = "LOAD"
    STORE = "STORE"
    ADD = "ADD"
    SUB = "SUB"
    MUL = "MUL"
    DIV = "DIV"
    POP = "POP"
    JMP = "JMP"
    JZ = "JZ"
    CALL = "CALL"
    RET = "RET"
    PRINT = "PRINT"
    HALT = "HALT"

ALL_OPS = {Op.PUSH, Op.LOAD, Op.STORE, Op.ADD, Op.SUB, Op.MUL, Op.DIV,
           Op.POP, Op.JMP, Op.JZ, Op.CALL, Op.RET, Op.PRINT, Op.HALT}

# ----------------------------
# Assembler: text -> instructions
# ----------------------------
def assemble(lines: List[str]) -> Tuple[List[Tuple], Dict[str,int]]:
    """
    Assemble simple assembly lines to instr list.
    Each instr is a tuple: (opcode, *operands)
    returns (instructions, label_to_address)
    """
    instrs = []
    labels = {}
    # first pass: collect labels
    pc = 0
    for raw in lines:
        line = raw.split("#",1)[0].strip()
        if not line:
            continue
        if line.endswith(":"):
            label = line[:-1].strip()
            labels[label] = pc
        else:
            pc += 1  # 1 instruction per line in this simple assembler

    # second pass: produce instruction tuples
    for raw in lines:
        line = raw.split("#",1)[0].strip()
        if not line or line.endswith(":"):
            continue
        parts = line.split()
        op = parts[0]
        if op not in ALL_OPS:
            raise ValueError(f"Unknown opcode: {op} (line: {line})")
        # parse operands (ints or labels)
        ops = []
        for tok in parts[1:]:
            if tok.lstrip("-").isdigit():
                ops.append(int(tok))
            else:
                # label reference (address)
                if tok not in labels:
                    raise ValueError(f"Unknown label reference: {tok}")
                ops.append(labels[tok])
        instrs.append((op, *ops))
    return instrs, labels

# ----------------------------
# VM implementation
# ----------------------------
class Frame:
    def __init__(self, ret_addr:int, nlocals:int):
        self.ret_addr = ret_addr
        self.locals = [0]*nlocals
        self.pc = 0  # pc inside the called function is absolute in this simple VM

class TinyVM:
    def __init__(self, instrs: List[Tuple]):
        self.instrs = instrs
        self.stack: List[int] = []
        self.callstack: List[Frame] = []
        self.pc = 0
        self.running = True

    def push(self, v:int):
        self.stack.append(v)

    def pop(self) -> int:
        if not self.stack:
            raise RuntimeError("stack underflow")
        return self.stack.pop()

    def run(self, entry=0):
        self.pc = entry
        # push an initial dummy frame so RET from main halts cleanly
        self.callstack.append(Frame(ret_addr=-1, nlocals=0))
        while self.running:
            if self.pc < 0 or self.pc >= len(self.instrs):
                raise RuntimeError(f"pc out of range: {self.pc}")
            instr = self.instrs[self.pc]
            op = instr[0]
            # print debug: uncomment if you want tracing
            # print(f"[pc={self.pc}] {instr} | stack={self.stack}")
            if op == Op.PUSH:
                self.push(instr[1])
                self.pc += 1
            elif op == Op.LOAD:
                idx = instr[1]
                val = self.current_frame().locals[idx]
                self.push(val)
                self.pc += 1
            elif op == Op.STORE:
                idx = instr[1]
                val = self.pop()
                self.current_frame().locals[idx] = val
                self.pc += 1
            elif op == Op.ADD:
                b = self.pop(); a = self.pop()
                self.push(a + b); self.pc += 1
            elif op == Op.SUB:
                b = self.pop(); a = self.pop()
                self.push(a - b); self.pc += 1
            elif op == Op.MUL:
                b = self.pop(); a = self.pop()
                self.push(a * b); self.pc += 1
            elif op == Op.DIV:
                b = self.pop(); a = self.pop()
                self.push(a // b); self.pc += 1
            elif op == Op.POP:
                self.pop(); self.pc += 1
            elif op == Op.JMP:
                self.pc = instr[1]
            elif op == Op.JZ:
                addr = instr[1]
                v = self.pop()
                if v == 0:
                    self.pc = addr
                else:
                    self.pc += 1
            elif op == Op.CALL:
                addr = instr[1]
                nlocals = instr[2]
                # save return address (next instruction)
                ret_addr = self.pc + 1
                # create new frame; set its pc to addr
                frame = Frame(ret_addr=ret_addr, nlocals=nlocals)
                frame.pc = addr
                self.callstack.append(frame)
                self.pc = addr
            elif op == Op.RET:
                # when returning, pop top of callstack
                frame = self.callstack.pop()
                if frame.ret_addr == -1:
                    # top-level return: halt
                    self.running = False
                    break
                self.pc = frame.ret_addr
            elif op == Op.PRINT:
                v = self.pop()
                print(v)
                self.pc += 1
            elif op == Op.HALT:
                self.running = False
                break
            else:
                raise RuntimeError(f"Unhandled opcode {op}")

    def current_frame(self) -> Frame:
        if not self.callstack:
            raise RuntimeError("no frame")
        return self.callstack[-1]

# ----------------------------
# Helpers: load program text
# ----------------------------
def load_asm_from_string(s: str) -> List[str]:
    return s.splitlines()

# ----------------------------
# Example: factorial program
# ----------------------------
FACTORIAL_ASM = """
start:
  PUSH_CONST 5
  CALL fact 1
  PRINT
  HALT

fact:
  STORE 0        # n -> locals[0]
  LOAD 0
  PUSH_CONST 0
  SUB
  JZ base_case   # if n == 0 jump
  LOAD 0
  PUSH_CONST 1
  SUB
  CALL fact 1
  LOAD 0
  MUL
  RET
base_case:
  PUSH_CONST 1
  RET
"""

# Another example: iterative sum
SUM_ASM = """
start:
  PUSH_CONST 10
  STORE 0      # n
  PUSH_CONST 0
  STORE 1      # i
  PUSH_CONST 0
  STORE 2      # acc
loop:
  LOAD 1
  LOAD 0
  SUB
  JZ done
  LOAD 2
  LOAD 1
  ADD
  STORE 2
  LOAD 1
  PUSH_CONST 1
  ADD
  STORE 1
  JMP loop
done:
  LOAD 2
  PRINT
  HALT
"""

# ----------------------------
# Simple runner
# ----------------------------
def run_asm(asm_text: str):
    lines = load_asm_from_string(asm_text)
    instrs, labels = assemble(lines)
    # convert instrs to list (they already are)
    vm = TinyVM(instrs)
    vm.run(entry=0)

# ----------------------------
# If launched as script, run examples
# ----------------------------
if __name__ == "__main__":
    print("Running factorial example (5!):")
    run_asm(FACTORIAL_ASM)
    print("\nRunning sum example (sum 0..9):")
    run_asm(SUM_ASM)
