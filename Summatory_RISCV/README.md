
# Sumatoria_RISCV

RISC-V Assembly program that uses a stack to store and sum the numbers from 1 to 40.

---

## Description

This program performs two main operations:

1. Pushes the numbers from 1 to 40 into the stack.
2. Pops those numbers from the stack and adds them together.

The final result is stored in register `a0`.

---

## Expected Result

```text
1 + 2 + 3 + ... + 40 = 820
```

```asm
a0 = 820
```

```asm
a0 = 0x00000334
```

---

## How It Works

### Push Loop

The `push_loop` stores each number from 1 to 40 in the stack.

```asm
addi sp, sp, -4
sw t0, 0(sp)
```

### Pop Loop

The `pop_loop` retrieves each value from the stack and adds it to `a0`.

```asm
lw t2, 0(sp)
add a0, a0, t2
addi sp, sp, 4
```

---

## Registers Used

| Register | Purpose |
|---|---|
| `t0` | Counter |
| `t1` | Limit value |
| `t2` | Value loaded from the stack |
| `a0` | Final sum |
| `sp` | Stack pointer |

---

## Technologies

- RISC-V Assembly
- Stack memory
- RISC-V simulator

