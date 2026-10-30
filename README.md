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

Stack transformations show stack states ordered from deepest to top of stack (`before` → `after`, where the rightmost item is the top of the stack).

| Operation Name | Stack Transformation | Description | Operand Bit Fields |
|:---|:---|:---|:---|
| `Undefined` | — | Undefined opcode (halts validation with illegal operation error) | *None* |
| `Push` | ` ` → `v` | Push scalar float literal onto stack | **Followed by**: 4 bytes (IEEE 754 32-bit float literal) |
| `Drop` | `a` → ` ` | Discard top stack value | *None* |
| `Dup` | `a` → `a a` | Duplicate top stack value | *None* |
| `Swap` | `a b` → `b a` | Swap top two stack values | *None* |
| `Over` | `a b` → `a b a` | Copy second stack value to top | *None* |
| `Tuck` | `a b` → `b a b` | Insert copy of top value beneath second value | *None* |
| `Nip` | `a b` → `b` | Discard second value from top | *None* |
| `Rot` | `a b c` → `b c a` | Rotate third value to top | *None* |
| `Copy` | `... v[N] ...` → `... v[N] ... v[N]` | Copy element or subvector from depth to top | **1 byte**:<br>• `[4:0]`: `offset` (0..31 depth below top)<br>• `[7:5]`: `size - 1` (0..7 for vector size 1..8) |
| `Move` | `... v[N] ...` → `... ... v[N]` | Move / rotate element or subvector from depth to top | **1 byte**:<br>• `[4:0]`: `offset` (0..31 depth below top)<br>• `[7:5]`: `size - 1` (0..7 for vector size 1..8) |
| `Add` | `a b` → `a + b` | Scalar addition | *None* |
| `Sub` | `a b` → `a - b` | Scalar subtraction | *None* |
| `Mul` | `a b` → `a * b` | Scalar multiplication | *None* |
| `Div` | `a b` → `a / b` | Scalar division | *None* |
| `Pow` | `a b` → `a ^ b` | Scalar power | *None* |
| `Atan2` | `y x` → `atan2(y, x)` | Four-quadrant inverse tangent | *None* |
| `Neg` | `a` → `-a` | Scalar negation | *None* |
| `Abs` | `a` → `\|a\|` | Scalar absolute value | *None* |
| `Inv` | `a` → `1 / a` | Scalar reciprocal | *None* |
| `Sqrt` | `a` → `sqrt(a)` | Square root | *None* |
| `Exp` | `a` → `exp(a)` | Exponential | *None* |
| `Ln` | `a` → `ln(a)` | Natural logarithm | *None* |
| `Sin` | `a` → `sin(a)` | Sine | *None* |
| `Cos` | `a` → `cos(a)` | Cosine | *None* |
| `Tan` | `a` → `tan(a)` | Tangent | *None* |
| `Asin` | `a` → `asin(a)` | Arc sine | *None* |
| `Acos` | `a` → `acos(a)` | Arc cosine | *None* |
| `Ipow` | `a` → `a ^ exp` | Integer exponentiation | **1 byte**:<br>• `[7:0]`: `exp` (two's complement signed 8-bit int, -128..127) |
| `MulAdd` | `a b c` → `a * b + c` | Fused multiply-add | *None* |
| `PushVec` | ` ` → `v[N]` | Push vector float literals onto stack | **1 byte**: `[7:0]`: `size - 1` (0..254 for vector dimension $N = 1..255$)<br>**Followed by**: `size * 4` bytes (IEEE 754 32-bit float literals) |
| `AddVec` | `u[N] v[N]` → `(u + v)[N]` | Componentwise vector addition | **1 byte**: `[7:0]`: `size - 1` (0..254 for vector dimension $N = 1..255$) |
| `SubVec` | `u[N] v[N]` → `(u - v)[N]` | Componentwise vector subtraction | **1 byte**: `[7:0]`: `size - 1` (0..254 for vector dimension $N = 1..255$) |
| `MulVec` | `u[N] v[N]` → `(u * v)[N]` | Componentwise vector multiplication | **1 byte**: `[7:0]`: `size - 1` (0..254 for vector dimension $N = 1..255$) |
| `DivVec` | `u[N] v[N]` → `(u / v)[N]` | Componentwise vector division | **1 byte**: `[7:0]`: `size - 1` (0..254 for vector dimension $N = 1..255$) |
| `NegVec` | `v[N]` → `(-v)[N]` | Componentwise vector negation | **1 byte**: `[7:0]`: `size - 1` (0..254 for vector dimension $N = 1..255$) |
| `InvVec` | `v[N]` → `(1 / v)[N]` | Componentwise vector reciprocal | **1 byte**: `[7:0]`: `size - 1` (0..254 for vector dimension $N = 1..255$) |
| `NormSqVec` | `v[N]` → `\|\|v\|\|^2` | Squared Euclidean norm $\|v\|^2$ | **1 byte**: `[7:0]`: `size - 1` (0..254 for vector dimension $N = 1..255$) |
| `SumVec` | `v[N]` → `sum(v)` | Sum of vector components | **1 byte**: `[7:0]`: `size - 1` (0..254 for vector dimension $N = 1..255$) |
| `MulAddVec` | `a[N] b[N] c[N]` → `(a * b + c)[N]` | Componentwise fused multiply-add | **1 byte**: `[7:0]`: `size - 1` (0..254 for vector dimension $N = 1..255$) |
| `ScaleVec` | `v[N] s` → `(s * v)[N]` | Vector scalar multiplication | **1 byte**: `[7:0]`: `size - 1` (0..254 for vector dimension $N = 1..255$) |
| `LerpVec` | `t` → `((1 - t)*v0 + t*v1)[N]` | Linear interpolation between two immediate vectors for scalar $t$ | **1 byte dimension**:<br>• `[7:0]`: `size - 1` (0..254 for vector dimension $N = 1..255$)<br>**Followed by**: `2 * size * 4` bytes of IEEE 754 32-bit floats (interleaved component pairs: $v_{0,0}, v_{1,0}, v_{0,1}, v_{1,1}, \dots, v_{0,N-1}, v_{1,N-1}$) |
| `MulMat` | `v[rows]` → `(v * M)[cols]` | Vector-matrix multiplication $w = v \cdot M$ with immediate matrix $M$ | **1 byte dimension**:<br>• `[1:0]`: `rows - 1` (0..3 for 1..4 input dimensions / rows in $M$)<br>• `[3:2]`: Reserved (must be 0)<br>• `[7:4]`: `cols - 1` (0..15 for 1..16 output dimensions / cols in $M$)<br>**Followed by**: `rows * cols * 4` bytes of IEEE 754 32-bit floats ($M$ in column-major order: column by column) |
| `Poly` | `t` → `P(t)[cols]` | Polynomial evaluation for scalar $t$ with immediate coefficients | **1 byte dimension**:<br>• `[3:0]`: `rows - 1` (0..15 for 1..16 coefficients per column / degree + 1)<br>• `[7:4]`: `cols - 1` (0..15 for 1..16 output dimensions)<br>**Followed by**: `rows * cols * 4` bytes of IEEE 754 32-bit float coefficients (degree by degree from highest order down to constant term: for each degree, column 0 to cols-1) |
