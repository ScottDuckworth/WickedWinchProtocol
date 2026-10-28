# WickedWinchProtocol

WickedWinchProtocol defines the wire protocols and bytecode evaluation engine for the Wicked Winch robotic system.

## WickedPostfix Bytecode

`WickedPostfix` is a compact, deterministic, zero-allocation postfix bytecode evaluation machine designed for real-time trajectory generation, coordinate transformations, and polynomial evaluation on embedded motor controllers and central supervisors.

### Wire Format & Header

Every serialized `WickedPostfix` program begins with a 2-byte header:

```
+-----------------------------+
|  ProgSize (16-bit uint, LE) |
+-----------------------------+
|  Bytecode bytes (ProgSize)  |
+-----------------------------+
```

### Operation Set

The operations are sequentially numbered and grouped by functional signature. Stack transformations show stack states ordered from deepest to top of stack (`before` → `after`, where the rightmost item is the top of the stack).

| Operation Name | Opcode | Category | Stack Transformation | Description | Operand Bit Fields |
|:---|:---:|:---|:---|:---|:---|
| `Undefined` | `0` (0x00) | Guard / Error | — | Undefined opcode (halts validation with illegal operation error) | *None* |
| `Push` | `1` (0x01) | Stack | ` ` → `v...` | Push literal float(s) onto stack | **Byte 1**: `count` (8-bit uint $N$)<br>**Followed by**: $N \times 4$ bytes (IEEE 754 32-bit float literals) |
| `Drop` | `2` (0x02) | Stack | `a` → ` ` | Discard top stack value | *None* |
| `Dup` | `3` (0x03) | Stack | `a` → `a a` | Duplicate top stack value | *None* |
| `Swap` | `4` (0x04) | Stack | `a b` → `b a` | Swap top two stack values | *None* |
| `Over` | `5` (0x05) | Stack | `a b` → `a b a` | Copy second stack value to top | *None* |
| `Tuck` | `6` (0x06) | Stack | `a b` → `b a b` | Insert copy of top value beneath second value | *None* |
| `Nip` | `7` (0x07) | Stack | `a b` → `b` | Discard second value from top | *None* |
| `Rot` | `8` (0x08) | Stack | `a b c` → `b c a` | Rotate third value to top | *None* |
| `Pick` | `9` (0x09) | Stack | • Copy:<br>&nbsp;&nbsp;`... v ...` → `... v ... v`<br>• Move:<br>&nbsp;&nbsp;`... v ...` → `... ... v` | Copy or move element or subvector from depth to top | **1 byte**:<br>• `[4:0]`: `offset` (0..31 depth below top)<br>• `[5]`: `move` (0 = copy, 1 = move)<br>• `[7:6]`: `size - 1` (0..3 for vector size 1..4) |
| `Add` | `10` (0x0A) | Binary Scalar | `a b` → `a + b` | Scalar addition | *None* |
| `Sub` | `11` (0x0B) | Binary Scalar | `a b` → `a - b` | Scalar subtraction | *None* |
| `Mul` | `12` (0x0C) | Binary Scalar | `a b` → `a * b` | Scalar multiplication | *None* |
| `Div` | `13` (0x0D) | Binary Scalar | `a b` → `a / b` | Scalar division | *None* |
| `Pow` | `14` (0x0E) | Binary Scalar | `a b` → `a ^ b` | Scalar power | *None* |
| `Atan2` | `15` (0x0F) | Binary Scalar | `y x` → `atan2(y, x)` | Four-quadrant inverse tangent | *None* |
| `Neg` | `16` (0x10) | Unary Scalar | `a` → `-a` | Scalar negation | *None* |
| `Abs` | `17` (0x11) | Unary Scalar | `a` → `\|a\|` | Scalar absolute value | *None* |
| `Inv` | `18` (0x12) | Unary Scalar | `a` → `1 / a` | Scalar reciprocal | *None* |
| `Sqrt` | `19` (0x13) | Unary Scalar | `a` → `sqrt(a)` | Square root | *None* |
| `Exp` | `20` (0x14) | Unary Scalar | `a` → `exp(a)` | Exponential | *None* |
| `Ln` | `21` (0x15) | Unary Scalar | `a` → `ln(a)` | Natural logarithm | *None* |
| `Sin` | `22` (0x16) | Unary Scalar | `a` → `sin(a)` | Sine | *None* |
| `Cos` | `23` (0x17) | Unary Scalar | `a` → `cos(a)` | Cosine | *None* |
| `Tan` | `24` (0x18) | Unary Scalar | `a` → `tan(a)` | Tangent | *None* |
| `Asin` | `25` (0x19) | Unary Scalar | `a` → `asin(a)` | Arc sine | *None* |
| `Acos` | `26` (0x1A) | Unary Scalar | `a` → `acos(a)` | Arc cosine | *None* |
| `Ipow` | `27` (0x1B) | Parameterized Scalar | `a` → `a ^ exp` | Integer exponentiation | **1 byte**:<br>• `[7:0]`: `exp` (two's complement signed 8-bit int, -128..127) |
| `MulAdd` | `28` (0x1C) | Fused Scalar | `a b c` → `a * b + c` | Fused multiply-add | *None* |
| `AddVec` | `29` (0x1D) | Binary Vector | `u[N] v[N]` → `(u + v)[N]` | Componentwise vector addition | **1 byte**: `[7:0]`: `size` ($N$, vector dimension) |
| `SubVec` | `30` (0x1E) | Binary Vector | `u[N] v[N]` → `(u - v)[N]` | Componentwise vector subtraction | **1 byte**: `[7:0]`: `size` ($N$, vector dimension) |
| `MulVec` | `31` (0x1F) | Binary Vector | `u[N] v[N]` → `(u * v)[N]` | Componentwise vector multiplication | **1 byte**: `[7:0]`: `size` ($N$, vector dimension) |
| `DivVec` | `32` (0x20) | Binary Vector | `u[N] v[N]` → `(u / v)[N]` | Componentwise vector division | **1 byte**: `[7:0]`: `size` ($N$, vector dimension) |
| `NegVec` | `33` (0x21) | Unary Vector | `v[N]` → `(-v)[N]` | Componentwise vector negation | **1 byte**: `[7:0]`: `size` ($N$, vector dimension) |
| `InvVec` | `34` (0x22) | Unary Vector | `v[N]` → `(1 / v)[N]` | Componentwise vector reciprocal | **1 byte**: `[7:0]`: `size` ($N$, vector dimension) |
| `NormSqVec` | `35` (0x23) | Vector Reduction | `v[N]` → `\|\|v\|\|^2`<br><br>*Note: If $N = 0$, pushes 0.0 (` ` → `0.0`)* | Squared Euclidean norm $\|v\|^2$ | **1 byte**: `[7:0]`: `size` ($N$, vector dimension) |
| `SumVec` | `36` (0x24) | Vector Reduction | `v[N]` → `sum(v)`<br><br>*Note: If $N = 0$, pushes 0.0 (` ` → `0.0`)* | Sum of vector components | **1 byte**: `[7:0]`: `size` ($N$, vector dimension) |
| `MulAddVec` | `37` (0x25) | Fused Vector | `a[N] b[N] c[N]` → `(a * b + c)[N]` | Componentwise fused multiply-add | **1 byte**: `[7:0]`: `size` ($N$, vector dimension) |
| `ScaleVec` | `38` (0x26) | Scaled Vector | `v[N] s` → `(s * v)[N]` | Vector scalar multiplication | **1 byte**: `[7:0]`: `size` ($N$, vector dimension) |
| `LerpVec` | `39` (0x27) | Vector Lerp | `v0[N] v1[N] t` → `((1 - t)*v0 + t*v1)[N]` | Linear interpolation between two vectors | **1 byte**: `[7:0]`: `size` ($N$, vector dimension) |
| `MulMat` | `40` (0x28) | Matrix Multiply | • Stack B:<br>&nbsp;&nbsp;`A[arows,brows] B[brows,bcols]` → `C[arows,bcols]`<br>• Immediate B:<br>&nbsp;&nbsp;`A[arows,brows]` → `C[arows,bcols]`<br><br>*Note: May require `arows * bcols` temporary scratch space above the stack during execution.* | Matrix multiplication $C = A \cdot B$ | **1 byte configuration**:<br>• `[1:0]`: `arows - 1` (0..3 for 1..4 rows in A)<br>• `[4:2]`: `brows - 1` (0..7 for 1..8 cols in A / rows in B)<br>• `[6:5]`: `bcols mode`:<br>&nbsp;&nbsp;– `0`: `bcols = brows` (square)<br>&nbsp;&nbsp;– `1`: `bcols = brows - 1` (projection down)<br>&nbsp;&nbsp;– `2`: `bcols = brows + 1` (projection up)<br>&nbsp;&nbsp;– `3`: `bcols = 1` (dot product / reduction)<br>• `[7]`: `immediate` (0 = matrix B on stack, 1 = immediate floats follow)<br>**If immediate**: followed by `brows * bcols * 4` bytes of IEEE 754 32-bit floats |
| `Poly` | `41` (0x29) | Polynomial | `t` → `P(t)[cols]` | Polynomial evaluation for scalar $t$ | **1 byte dimension**:<br>• `[3:0]`: `rows - 1` (0..15 for 1..16 coefficients per column / degree + 1)<br>• `[7:4]`: `cols - 1` (0..15 for 1..16 output dimensions)<br>**Followed by**: `rows * cols * 4` bytes of IEEE 754 32-bit float coefficients (highest-order first) |
