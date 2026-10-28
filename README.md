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

The operations are sequentially numbered and grouped by functional signature. Stack transformations are written in Forth notation `(before -> after)`, where items are ordered from deepest to top of stack (the rightmost item is the top of the stack).

| Operation Name | Opcode | Category | Description | Operand Bit Fields |
|:---|:---:|:---|:---|:---|
| `Undefined` | `0` (0x00) | Guard / Error | Undefined opcode (halts validation with illegal operation error) | *None* |
| `Push` | `1` (0x01) | Stack | Push literal float(s) onto stack: `( -> v...)` | **Byte 1**: `count` (8-bit uint $N$)<br>**Followed by**: $N \times 4$ bytes (IEEE 754 32-bit float literals) |
| `Drop` | `2` (0x02) | Stack | Discard top stack value: `(a -> )` | *None* |
| `Dup` | `3` (0x03) | Stack | Duplicate top stack value: `(a -> a a)` | *None* |
| `Swap` | `4` (0x04) | Stack | Swap top two stack values: `(a b -> b a)` | *None* |
| `Over` | `5` (0x05) | Stack | Copy second stack value to top: `(a b -> a b a)` | *None* |
| `Tuck` | `6` (0x06) | Stack | Insert copy of top value beneath second value: `(a b -> b a b)` | *None* |
| `Nip` | `7` (0x07) | Stack | Discard second value from top: `(a b -> b)` | *None* |
| `Rot` | `8` (0x08) | Stack | Rotate third value to top: `(a b c -> b c a)` | *None* |
| `Pick` | `9` (0x09) | Stack | Copy or move element or subvector from depth to top:<br>• Copy: `(... v ... -> ... v ... v)`<br>• Move: `(... v ... -> ... ... v)` | **1 byte**:<br>• `[4:0]`: `offset` (0..31 depth below top)<br>• `[5]`: `move` (0 = copy, 1 = move)<br>• `[7:6]`: `size - 1` (0..3 for vector size 1..4) |
| `Add` | `10` (0x0A) | Binary Scalar | Scalar addition: `(a b -> a + b)` | *None* |
| `Sub` | `11` (0x0B) | Binary Scalar | Scalar subtraction: `(a b -> a - b)` | *None* |
| `Mul` | `12` (0x0C) | Binary Scalar | Scalar multiplication: `(a b -> a * b)` | *None* |
| `Div` | `13` (0x0D) | Binary Scalar | Scalar division: `(a b -> a / b)` | *None* |
| `Pow` | `14` (0x0E) | Binary Scalar | Scalar power: `(a b -> a ^ b)` | *None* |
| `Atan2` | `15` (0x0F) | Binary Scalar | Four-quadrant inverse tangent: `(y x -> atan2(y, x))` | *None* |
| `Neg` | `16` (0x10) | Unary Scalar | Scalar negation: `(a -> -a)` | *None* |
| `Abs` | `17` (0x11) | Unary Scalar | Scalar absolute value: `(a -> \|a\|)` | *None* |
| `Inv` | `18` (0x12) | Unary Scalar | Scalar reciprocal: `(a -> 1 / a)` | *None* |
| `Sqrt` | `19` (0x13) | Unary Scalar | Square root: `(a -> sqrt(a))` | *None* |
| `Exp` | `20` (0x14) | Unary Scalar | Exponential: `(a -> exp(a))` | *None* |
| `Ln` | `21` (0x15) | Unary Scalar | Natural logarithm: `(a -> ln(a))` | *None* |
| `Sin` | `22` (0x16) | Unary Scalar | Sine: `(a -> sin(a))` | *None* |
| `Cos` | `23` (0x17) | Unary Scalar | Cosine: `(a -> cos(a))` | *None* |
| `Tan` | `24` (0x18) | Unary Scalar | Tangent: `(a -> tan(a))` | *None* |
| `Asin` | `25` (0x19) | Unary Scalar | Arc sine: `(a -> asin(a))` | *None* |
| `Acos` | `26` (0x1A) | Unary Scalar | Arc cosine: `(a -> acos(a))` | *None* |
| `Ipow` | `27` (0x1B) | Parameterized Scalar | Integer exponentiation: `(a -> a ^ exp)` | **1 byte**:<br>• `[7:0]`: `exp` (two's complement signed 8-bit int, -128..127) |
| `MulAdd` | `28` (0x1C) | Fused Scalar | Fused multiply-add: `(a b c -> a * b + c)` | *None* |
| `AddVec` | `29` (0x1D) | Binary Vector | Componentwise vector addition: `(u[N] v[N] -> (u + v)[N])` | **1 byte**: `[7:0]`: `size` ($N$, vector dimension) |
| `SubVec` | `30` (0x1E) | Binary Vector | Componentwise vector subtraction: `(u[N] v[N] -> (u - v)[N])` | **1 byte**: `[7:0]`: `size` ($N$, vector dimension) |
| `MulVec` | `31` (0x1F) | Binary Vector | Componentwise vector multiplication: `(u[N] v[N] -> (u * v)[N])` | **1 byte**: `[7:0]`: `size` ($N$, vector dimension) |
| `DivVec` | `32` (0x20) | Binary Vector | Componentwise vector division: `(u[N] v[N] -> (u / v)[N])` | **1 byte**: `[7:0]`: `size` ($N$, vector dimension) |
| `NegVec` | `33` (0x21) | Unary Vector | Componentwise vector negation: `(v[N] -> (-v)[N])` | **1 byte**: `[7:0]`: `size` ($N$, vector dimension) |
| `InvVec` | `34` (0x22) | Unary Vector | Componentwise vector reciprocal: `(v[N] -> (1 / v)[N])` | **1 byte**: `[7:0]`: `size` ($N$, vector dimension) |
| `NormSqVec` | `35` (0x23) | Vector Reduction | Squared Euclidean norm $\|v\|^2$: `(v[N] -> \|\|v\|\|^2)` | **1 byte**: `[7:0]`: `size` ($N$, vector dimension) |
| `SumVec` | `36` (0x24) | Vector Reduction | Sum of vector components: `(v[N] -> sum(v))` | **1 byte**: `[7:0]`: `size` ($N$, vector dimension) |
| `MulAddVec` | `37` (0x25) | Fused Vector | Componentwise fused multiply-add: `(a[N] b[N] c[N] -> (a * b + c)[N])` | **1 byte**: `[7:0]`: `size` ($N$, vector dimension) |
| `ScaleVec` | `38` (0x26) | Scaled Vector | Vector scalar multiplication: `(v[N] s -> (s * v)[N])` | **1 byte**: `[7:0]`: `size` ($N$, vector dimension) |
| `LerpVec` | `39` (0x27) | Vector Lerp | Linear interpolation between two vectors: `(v0[N] v1[N] t -> ((1 - t)*v0 + t*v1)[N])` | **1 byte**: `[7:0]`: `size` ($N$, vector dimension) |
| `MulMat` | `40` (0x28) | Matrix Multiply | Matrix multiplication $C = A \cdot B$:<br>• Stack B: `(A[arows,brows] B[brows,bcols] -> C[arows,bcols])`<br>• Immediate B: `(A[arows,brows] -> C[arows,bcols])` | **1 byte configuration**:<br>• `[1:0]`: `arows - 1` (0..3 for 1..4 rows in A)<br>• `[4:2]`: `brows - 1` (0..7 for 1..8 cols in A / rows in B)<br>• `[6:5]`: `bcols mode`:<br>&nbsp;&nbsp;– `0`: `bcols = brows` (square)<br>&nbsp;&nbsp;– `1`: `bcols = brows - 1` (projection down)<br>&nbsp;&nbsp;– `2`: `bcols = brows + 1` (projection up)<br>&nbsp;&nbsp;– `3`: `bcols = 1` (dot product / reduction)<br>• `[7]`: `immediate` (0 = matrix B on stack, 1 = immediate floats follow)<br>**If immediate**: followed by `brows * bcols * 4` bytes of IEEE 754 32-bit floats |
| `Polynomial` | `41` (0x29) | Poly / Matrix Poly | Horner's method evaluation for scalar $t$: `(t -> P(t)[cols])` | **1 byte dimension**:<br>• `[3:0]`: `rows - 1` (0..15 for 1..16 coefficients per column / degree + 1)<br>• `[7:4]`: `cols - 1` (0..15 for 1..16 output dimensions)<br>**Followed by**: `rows * cols * 4` bytes of IEEE 754 32-bit float coefficients (highest-order first) |
