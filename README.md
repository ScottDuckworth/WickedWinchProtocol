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
| `Push` | `1` (0x01) | Stack | ` ` → `v` | Push scalar float literal onto stack | **Followed by**: 4 bytes (IEEE 754 32-bit float literal) |
| `Drop` | `2` (0x02) | Stack | `a` → ` ` | Discard top stack value | *None* |
| `Dup` | `3` (0x03) | Stack | `a` → `a a` | Duplicate top stack value | *None* |
| `Swap` | `4` (0x04) | Stack | `a b` → `b a` | Swap top two stack values | *None* |
| `Over` | `5` (0x05) | Stack | `a b` → `a b a` | Copy second stack value to top | *None* |
| `Tuck` | `6` (0x06) | Stack | `a b` → `b a b` | Insert copy of top value beneath second value | *None* |
| `Nip` | `7` (0x07) | Stack | `a b` → `b` | Discard second value from top | *None* |
| `Rot` | `8` (0x08) | Stack | `a b c` → `b c a` | Rotate third value to top | *None* |
| `Copy` | `9` (0x09) | Stack | `... v[N] ...` → `... v[N] ... v[N]` | Copy element or subvector from depth to top | **1 byte**:<br>• `[4:0]`: `offset` (0..31 depth below top)<br>• `[7:5]`: `size - 1` (0..7 for vector size 1..8) |
| `Move` | `10` (0x0A) | Stack | `... v[N] ...` → `... ... v[N]` | Move / rotate element or subvector from depth to top | **1 byte**:<br>• `[4:0]`: `offset` (0..31 depth below top)<br>• `[7:5]`: `size - 1` (0..7 for vector size 1..8) |
| `Add` | `11` (0x0B) | Binary Scalar | `a b` → `a + b` | Scalar addition | *None* |
| `Sub` | `12` (0x0C) | Binary Scalar | `a b` → `a - b` | Scalar subtraction | *None* |
| `Mul` | `13` (0x0D) | Binary Scalar | `a b` → `a * b` | Scalar multiplication | *None* |
| `Div` | `14` (0x0E) | Binary Scalar | `a b` → `a / b` | Scalar division | *None* |
| `Pow` | `15` (0x0F) | Binary Scalar | `a b` → `a ^ b` | Scalar power | *None* |
| `Atan2` | `16` (0x10) | Binary Scalar | `y x` → `atan2(y, x)` | Four-quadrant inverse tangent | *None* |
| `Neg` | `17` (0x11) | Unary Scalar | `a` → `-a` | Scalar negation | *None* |
| `Abs` | `18` (0x12) | Unary Scalar | `a` → `\|a\|` | Scalar absolute value | *None* |
| `Inv` | `19` (0x13) | Unary Scalar | `a` → `1 / a` | Scalar reciprocal | *None* |
| `Sqrt` | `20` (0x14) | Unary Scalar | `a` → `sqrt(a)` | Square root | *None* |
| `Exp` | `21` (0x15) | Unary Scalar | `a` → `exp(a)` | Exponential | *None* |
| `Ln` | `22` (0x16) | Unary Scalar | `a` → `ln(a)` | Natural logarithm | *None* |
| `Sin` | `23` (0x17) | Unary Scalar | `a` → `sin(a)` | Sine | *None* |
| `Cos` | `24` (0x18) | Unary Scalar | `a` → `cos(a)` | Cosine | *None* |
| `Tan` | `25` (0x19) | Unary Scalar | `a` → `tan(a)` | Tangent | *None* |
| `Asin` | `26` (0x1A) | Unary Scalar | `a` → `asin(a)` | Arc sine | *None* |
| `Acos` | `27` (0x1B) | Unary Scalar | `a` → `acos(a)` | Arc cosine | *None* |
| `Ipow` | `28` (0x1C) | Parameterized Scalar | `a` → `a ^ exp` | Integer exponentiation | **1 byte**:<br>• `[7:0]`: `exp` (two's complement signed 8-bit int, -128..127) |
| `MulAdd` | `29` (0x1D) | Fused Scalar | `a b c` → `a * b + c` | Fused multiply-add | *None* |
| `PushVec` | `30` (0x1E) | Vector Stack | ` ` → `v[N]` | Push vector float literals onto stack | **1 byte**: `[7:0]`: `size - 1` (0..254 for vector dimension $N = 1..255$)<br>**Followed by**: `size * 4` bytes (IEEE 754 32-bit float literals) |
| `AddVec` | `31` (0x1F) | Binary Vector | `u[N] v[N]` → `(u + v)[N]` | Componentwise vector addition | **1 byte**: `[7:0]`: `size - 1` (0..254 for vector dimension $N = 1..255$) |
| `SubVec` | `32` (0x20) | Binary Vector | `u[N] v[N]` → `(u - v)[N]` | Componentwise vector subtraction | **1 byte**: `[7:0]`: `size - 1` (0..254 for vector dimension $N = 1..255$) |
| `MulVec` | `33` (0x21) | Binary Vector | `u[N] v[N]` → `(u * v)[N]` | Componentwise vector multiplication | **1 byte**: `[7:0]`: `size - 1` (0..254 for vector dimension $N = 1..255$) |
| `DivVec` | `34` (0x22) | Binary Vector | `u[N] v[N]` → `(u / v)[N]` | Componentwise vector division | **1 byte**: `[7:0]`: `size - 1` (0..254 for vector dimension $N = 1..255$) |
| `NegVec` | `35` (0x23) | Unary Vector | `v[N]` → `(-v)[N]` | Componentwise vector negation | **1 byte**: `[7:0]`: `size - 1` (0..254 for vector dimension $N = 1..255$) |
| `InvVec` | `36` (0x24) | Unary Vector | `v[N]` → `(1 / v)[N]` | Componentwise vector reciprocal | **1 byte**: `[7:0]`: `size - 1` (0..254 for vector dimension $N = 1..255$) |
| `NormSqVec` | `37` (0x25) | Vector Reduction | `v[N]` → `\|\|v\|\|^2` | Squared Euclidean norm $\|v\|^2$ | **1 byte**: `[7:0]`: `size - 1` (0..254 for vector dimension $N = 1..255$) |
| `SumVec` | `38` (0x26) | Vector Reduction | `v[N]` → `sum(v)` | Sum of vector components | **1 byte**: `[7:0]`: `size - 1` (0..254 for vector dimension $N = 1..255$) |
| `MulAddVec` | `39` (0x27) | Fused Vector | `a[N] b[N] c[N]` → `(a * b + c)[N]` | Componentwise fused multiply-add | **1 byte**: `[7:0]`: `size - 1` (0..254 for vector dimension $N = 1..255$) |
| `ScaleVec` | `40` (0x28) | Scaled Vector | `v[N] s` → `(s * v)[N]` | Vector scalar multiplication | **1 byte**: `[7:0]`: `size - 1` (0..254 for vector dimension $N = 1..255$) |
| `LerpVec` | `41` (0x29) | Vector Lerp | `t` → `((1 - t)*v0 + t*v1)[N]` | Linear interpolation between two immediate vectors for scalar $t$ | **1 byte dimension**:<br>• `[7:0]`: `size - 1` (0..254 for vector dimension $N = 1..255$)<br>**Followed by**: `2 * size * 4` bytes of IEEE 754 32-bit floats (interleaved component pairs: $v_{0,0}, v_{1,0}, v_{0,1}, v_{1,1}, \dots, v_{0,N-1}, v_{1,N-1}$) |
| `MulMat` | `42` (0x2A) | Vector-Matrix Multiply | `v[rows]` → `(v * M)[cols]` | Vector-matrix multiplication $w = v \cdot M$ with immediate matrix $M$ | **1 byte dimension**:<br>• `[1:0]`: `rows - 1` (0..3 for 1..4 input dimensions / rows in $M$)<br>• `[3:2]`: Reserved (must be 0)<br>• `[7:4]`: `cols - 1` (0..15 for 1..16 output dimensions / cols in $M$)<br>**Followed by**: `rows * cols * 4` bytes of IEEE 754 32-bit floats ($M$ in column-major order: column by column) |
| `Poly` | `43` (0x2B) | Polynomial | `t` → `P(t)[cols]` | Polynomial evaluation for scalar $t$ with immediate coefficients | **1 byte dimension**:<br>• `[3:0]`: `rows - 1` (0..15 for 1..16 coefficients per column / degree + 1)<br>• `[7:4]`: `cols - 1` (0..15 for 1..16 output dimensions)<br>**Followed by**: `rows * cols * 4` bytes of IEEE 754 32-bit float coefficients (degree by degree from highest order down to constant term: for each degree, column 0 to cols-1) |
