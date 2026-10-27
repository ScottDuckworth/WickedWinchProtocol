package postfix

import (
	"cmp"
	"encoding/binary"
	"errors"
	"fmt"
	"io"
	"math"
	"slices"
	"strings"
)

var (
	ErrUndefinedOperation     = errors.New("undefined operation")
	ErrIllegalOperation       = errors.New("illegal operation")
	ErrStackUnderflow         = errors.New("stack underflow")
	ErrStackOverflow          = errors.New("stack overflow")
	ErrFloatLiteralsUnderflow = errors.New("floats underflow")
	ErrUnexpectedStackSize    = errors.New("unexpected stack size")
)

type Operation uint8

const (
	Operation_Undefined  Operation = 0
	Operation_Push       Operation = 1
	Operation_Drop       Operation = 2
	Operation_Dup        Operation = 3
	Operation_Swap       Operation = 4
	Operation_Over       Operation = 5
	Operation_Tuck       Operation = 6
	Operation_Nip        Operation = 7
	Operation_Rot        Operation = 8
	Operation_Pick       Operation = 9
	Operation_Add        Operation = 10
	Operation_Sub        Operation = 11
	Operation_Mul        Operation = 12
	Operation_MulAdd     Operation = 13
	Operation_Div        Operation = 14
	Operation_Mod        Operation = 15
	Operation_Neg        Operation = 16
	Operation_Abs        Operation = 17
	Operation_Inv        Operation = 18
	Operation_Pow        Operation = 19
	Operation_Sqrt       Operation = 20
	Operation_Exp        Operation = 21
	Operation_Ln         Operation = 22
	Operation_Sin        Operation = 23
	Operation_Cos        Operation = 24
	Operation_Tan        Operation = 25
	Operation_Asin       Operation = 26
	Operation_Acos       Operation = 27
	Operation_Atan2      Operation = 28
	Operation_AddVec     Operation = 29
	Operation_SubVec     Operation = 30
	Operation_MulVec     Operation = 31
	Operation_MulAddVec  Operation = 32
	Operation_ScaleVec   Operation = 33
	Operation_DivVec     Operation = 34
	Operation_NegVec     Operation = 35
	Operation_InvVec     Operation = 36
	Operation_NormSqVec  Operation = 37
	Operation_SumVec     Operation = 38
	Operation_LerpVec    Operation = 39
	Operation_MulMat     Operation = 40
	Operation_Polynomial Operation = 41
)

type Expression struct {
	Prog []uint8
}

type PostfixHeader struct {
	ProgSize uint16
}

func (op Operation) String() string {
	switch op {
	case Operation_Undefined:
		return "Undefined"
	case Operation_Push:
		return "Push"
	case Operation_Drop:
		return "Drop"
	case Operation_Dup:
		return "Dup"
	case Operation_Swap:
		return "Swap"
	case Operation_Over:
		return "Over"
	case Operation_Tuck:
		return "Tuck"
	case Operation_Nip:
		return "Nip"
	case Operation_Rot:
		return "Rot"
	case Operation_Pick:
		return "Pick"
	case Operation_Add:
		return "Add"
	case Operation_Sub:
		return "Sub"
	case Operation_Mul:
		return "Mul"
	case Operation_MulAdd:
		return "MulAdd"
	case Operation_Div:
		return "Div"
	case Operation_Mod:
		return "Mod"
	case Operation_Neg:
		return "Neg"
	case Operation_Abs:
		return "Abs"
	case Operation_Inv:
		return "Inv"
	case Operation_Pow:
		return "Pow"
	case Operation_Sqrt:
		return "Sqrt"
	case Operation_Exp:
		return "Exp"
	case Operation_Ln:
		return "Ln"
	case Operation_Sin:
		return "Sin"
	case Operation_Cos:
		return "Cos"
	case Operation_Tan:
		return "Tan"
	case Operation_Asin:
		return "Asin"
	case Operation_Acos:
		return "Acos"
	case Operation_Atan2:
		return "Atan2"
	case Operation_AddVec:
		return "AddVec"
	case Operation_SubVec:
		return "SubVec"
	case Operation_MulVec:
		return "MulVec"
	case Operation_MulAddVec:
		return "MulAddVec"
	case Operation_ScaleVec:
		return "ScaleVec"
	case Operation_DivVec:
		return "DivVec"
	case Operation_NegVec:
		return "NegVec"
	case Operation_InvVec:
		return "InvVec"
	case Operation_NormSqVec:
		return "NormSqVec"
	case Operation_SumVec:
		return "SumVec"
	case Operation_LerpVec:
		return "LerpVec"
	case Operation_MulMat:
		return "MulMat"
	case Operation_Polynomial:
		return "Polynomial"
	default:
		return fmt.Sprintf("unknown[%d]", op)
	}
}

func (expr Expression) String() string {
	var sb strings.Builder
	sb.WriteString("{")
	for i, v := range expr.Prog {
		if i > 0 {
			sb.WriteString(" ")
		}
		sb.WriteString(fmt.Sprint(v))
	}
	sb.WriteString("}")
	return sb.String()
}

func (expr *Expression) Write(w io.Writer) error {
	header := PostfixHeader{
		ProgSize: uint16(len(expr.Prog)),
	}
	return cmp.Or(
		binary.Write(w, binary.LittleEndian, &header),
		binary.Write(w, binary.LittleEndian, &expr.Prog),
	)
}

func (expr *Expression) Read(r io.Reader) error {
	var header PostfixHeader
	err := binary.Read(r, binary.LittleEndian, &header)
	if err != nil {
		return err
	}
	expr.Prog = make([]uint8, header.ProgSize)
	return binary.Read(r, binary.LittleEndian, &expr.Prog)
}

func Equal(a, b *Expression) bool {
	return slices.Equal(a.Prog, b.Prog)
}

func Join(exprs ...*Expression) *Expression {
	result := &Expression{}
	for _, expr := range exprs {
		result.Prog = append(result.Prog, expr.Prog...)
	}
	return result
}

type Builder struct {
	expr Expression
}

func MakeBuilder() *Builder {
	return &Builder{}
}

func (b *Builder) Build() *Expression {
	return &b.expr
}

func (b *Builder) Push(literals ...float64) *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_Push), uint8(len(literals)))
	for _, v := range literals {
		b.expr.Prog = binary.LittleEndian.AppendUint32(b.expr.Prog, math.Float32bits(float32(v)))
	}
	return b
}

func (b *Builder) Drop() *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_Drop))
	return b
}

func (b *Builder) Dup() *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_Dup))
	return b
}

func (b *Builder) Swap() *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_Swap))
	return b
}

func (b *Builder) Over() *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_Over))
	return b
}

func (b *Builder) Tuck() *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_Tuck))
	return b
}

func (b *Builder) Nip() *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_Nip))
	return b
}

func (b *Builder) Rot() *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_Rot))
	return b
}

func (b *Builder) Pick(offset int, opts ...any) *Builder {
	size := 1
	move := false
	for _, opt := range opts {
		switch v := opt.(type) {
		case int:
			size = v
		case bool:
			move = v
		}
	}
	moveBit := 0
	if move {
		moveBit = 1
	}
	arg := (offset & 0x1F) | (moveBit << 5) | (((size - 1) & 0x03) << 6)
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_Pick), uint8(arg))
	return b
}

func (b *Builder) Add() *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_Add))
	return b
}

func (b *Builder) Sub() *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_Sub))
	return b
}

func (b *Builder) Mul() *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_Mul))
	return b
}

func (b *Builder) MulAdd() *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_MulAdd))
	return b
}

func (b *Builder) Div() *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_Div))
	return b
}

func (b *Builder) Mod() *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_Mod))
	return b
}

func (b *Builder) Neg() *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_Neg))
	return b
}

func (b *Builder) Abs() *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_Abs))
	return b
}

func (b *Builder) Inv() *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_Inv))
	return b
}

func (b *Builder) Pow() *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_Pow))
	return b
}

func (b *Builder) Sqrt() *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_Sqrt))
	return b
}

func (b *Builder) Exp() *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_Exp))
	return b
}

func (b *Builder) Ln() *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_Ln))
	return b
}

func (b *Builder) Sin() *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_Sin))
	return b
}

func (b *Builder) Cos() *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_Cos))
	return b
}

func (b *Builder) Tan() *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_Tan))
	return b
}

func (b *Builder) Asin() *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_Asin))
	return b
}

func (b *Builder) Acos() *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_Acos))
	return b
}

func (b *Builder) Atan2() *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_Atan2))
	return b
}

// Polynomial evaluates a polynomial or matrix of polynomials using Horner's method.
// Operand constraints:
//   - rows: 1 to 16 (number of polynomial coefficients per column, i.e. degree + 1)
//   - cols: 1 to 16 (number of dimensions / columns)
//   - len(coeffs) == rows * cols
func (b *Builder) Polynomial(rows, cols int, coeffs ...float64) *Builder {
	if rows < 1 || rows > 16 || cols < 1 || cols > 16 {
		panic(fmt.Sprintf("Polynomial rows (%d) and cols (%d) must be in range [1, 16]", rows, cols))
	}
	if len(coeffs) != rows*cols {
		panic("invalid number of coefficients for Polynomial")
	}
	arg := uint8((rows-1)&0x0F) | (uint8((cols-1)&0x0F) << 4)
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_Polynomial), arg)
	for i := rows - 1; i >= 0; i-- {
		rowOffset := i * cols
		for j := range cols {
			b.expr.Prog = binary.LittleEndian.AppendUint32(b.expr.Prog, math.Float32bits(float32(coeffs[rowOffset+j])))
		}
	}
	return b
}

// PolyVec is a convenience method that evaluates a 1D polynomial via Operation_Polynomial.
func (b *Builder) PolyVec(coeffs ...float64) *Builder {
	return b.Polynomial(len(coeffs), 1, coeffs...)
}

// PolyMat is an alias for Polynomial.
func (b *Builder) PolyMat(rows, cols int, coeffs ...float64) *Builder {
	return b.Polynomial(rows, cols, coeffs...)
}

func packMulMat(arows, brows, bcols int, immediate bool) uint8 {
	if arows < 1 || arows > 4 {
		panic(fmt.Sprintf("MulMat arows (%d) must be in range [1, 4]", arows))
	}
	if brows < 1 || brows > 8 {
		panic(fmt.Sprintf("MulMat brows (%d) must be in range [1, 8]", brows))
	}
	var mode uint8
	switch {
	case bcols == brows:
		mode = 0
	case bcols == brows-1:
		mode = 1
	case bcols == brows+1:
		mode = 2
	case bcols == 1:
		mode = 3
	default:
		panic(fmt.Sprintf("Unsupported MulMat bcols (%d) for brows (%d): must be brows, brows-1, brows+1, or 1", bcols, brows))
	}
	arg := uint8((arows-1)&0x03) | (uint8((brows-1)&0x07) << 2) | (mode << 5)
	if immediate {
		arg |= 0x80
	}
	return arg
}

func unpackMulMat(arg uint8) (arows, brows, bcols int, immediate bool) {
	arows = int(arg&0x03) + 1
	brows = int((arg>>2)&0x07) + 1
	mode := (arg >> 5) & 0x03
	immediate = (arg & 0x80) != 0
	switch mode {
	case 0:
		bcols = brows
	case 1:
		bcols = brows - 1
	case 2:
		bcols = brows + 1
	case 3:
		bcols = 1
	}
	return
}

func (b *Builder) AddVec(size int) *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_AddVec), uint8(size))
	return b
}

func (b *Builder) SubVec(size int) *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_SubVec), uint8(size))
	return b
}

func (b *Builder) MulVec(size int) *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_MulVec), uint8(size))
	return b
}

func (b *Builder) MulAddVec(size int) *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_MulAddVec), uint8(size))
	return b
}

func (b *Builder) ScaleVec(size int) *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_ScaleVec), uint8(size))
	return b
}

func (b *Builder) DivVec(size int) *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_DivVec), uint8(size))
	return b
}

func (b *Builder) NegVec(size int) *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_NegVec), uint8(size))
	return b
}

func (b *Builder) InvVec(size int) *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_InvVec), uint8(size))
	return b
}

func (b *Builder) NormSqVec(size int) *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_NormSqVec), uint8(size))
	return b
}

func (b *Builder) SumVec(size int) *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_SumVec), uint8(size))
	return b
}

func (b *Builder) LerpVec(size int) *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_LerpVec), uint8(size))
	return b
}

// MulMat encodes a matrix multiplication C = A * B, where A is (arows x brows) and B is (brows x bcols).
// Operand constraints:
//   - arows: 1 to 4
//   - brows: 1 to 8
//   - bcols: must be brows (square), brows - 1 (projection down), brows + 1 (projection up), or 1 (reduction/dot product)
//   - Matrix B is popped from the evaluation stack.
func (b *Builder) MulMat(arows, brows, bcols int) *Builder {
	arg := packMulMat(arows, brows, bcols, false)
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_MulMat), arg)
	return b
}

// MulMatImmediate encodes a matrix multiplication C = A * B with immediate matrix B, where A is on the stack (arows x brows)
// and B is supplied as immediate float literals in the program stream (brows x bcols).
// Operand constraints:
//   - arows: 1 to 4
//   - brows: 1 to 8
//   - bcols: must be brows (square), brows - 1 (projection down), brows + 1 (projection up), or 1 (reduction/dot product)
//   - len(bMatrix) == brows * bcols
func (b *Builder) MulMatImmediate(arows, brows, bcols int, bMatrix ...float64) *Builder {
	if len(bMatrix) != brows*bcols {
		panic("invalid number of matrix elements for MulMatImmediate")
	}
	arg := packMulMat(arows, brows, bcols, true)
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_MulMat), arg)
	for _, f := range bMatrix {
		b.expr.Prog = binary.LittleEndian.AppendUint32(b.expr.Prog, math.Float32bits(float32(f)))
	}
	return b
}

func (expr *Expression) Validate(initialStackLen int) (minCapacity, finalStackLen int, ops []Operation, err error) {
	pc := 0
	stackLen := initialStackLen
	maxStackLen := initialStackLen

	updateMax := func(depth int) {
		if depth > maxStackLen {
			maxStackLen = depth
		}
	}
	avail := func() int {
		return len(expr.Prog) - pc
	}
	fetchByte := func() int {
		v := expr.Prog[pc]
		pc++
		return int(v)
	}

	for avail() > 0 {
		op := Operation(fetchByte())
		switch op {
		case Operation_Undefined:
			return 0, 0, nil, ErrUndefinedOperation
		case Operation_Push:
			if avail() < 1 {
				return 0, 0, nil, ErrIllegalOperation
			}
			n := fetchByte()
			if avail() < n*4 {
				return 0, 0, nil, ErrIllegalOperation
			}
			pc += n * 4
			stackLen += n
			updateMax(stackLen)
		case Operation_Drop:
			if stackLen < 1 {
				return 0, 0, nil, ErrStackUnderflow
			}
			stackLen -= 1
		case Operation_Dup:
			if stackLen < 1 {
				return 0, 0, nil, ErrStackUnderflow
			}
			stackLen += 1
			updateMax(stackLen)
		case Operation_Swap:
			if stackLen < 2 {
				return 0, 0, nil, ErrStackUnderflow
			}
		case Operation_Over:
			if stackLen < 2 {
				return 0, 0, nil, ErrStackUnderflow
			}
			stackLen += 1
			updateMax(stackLen)
		case Operation_Tuck:
			if stackLen < 2 {
				return 0, 0, nil, ErrStackUnderflow
			}
			stackLen += 1
			updateMax(stackLen)
		case Operation_Nip:
			if stackLen < 2 {
				return 0, 0, nil, ErrStackUnderflow
			}
			stackLen -= 1
		case Operation_Rot:
			if stackLen < 3 {
				return 0, 0, nil, ErrStackUnderflow
			}
		case Operation_Pick:
			if avail() < 1 {
				return 0, 0, nil, ErrIllegalOperation
			}
			arg := fetchByte()
			offset := arg & 0x1F
			move := (arg & 0x20) != 0
			size := ((arg >> 6) & 0x03) + 1
			if stackLen < offset+size {
				return 0, 0, nil, ErrStackUnderflow
			}
			if !move {
				stackLen += size
				updateMax(stackLen)
			}
		case Operation_Add, Operation_Sub, Operation_Mul, Operation_Div, Operation_Mod, Operation_Pow, Operation_Atan2:
			if stackLen < 2 {
				return 0, 0, nil, ErrStackUnderflow
			}
			stackLen -= 1
		case Operation_MulAdd:
			if stackLen < 3 {
				return 0, 0, nil, ErrStackUnderflow
			}
			stackLen -= 2
		case Operation_Neg, Operation_Abs, Operation_Inv, Operation_Sqrt, Operation_Exp, Operation_Ln, Operation_Sin, Operation_Cos, Operation_Tan, Operation_Asin, Operation_Acos:
			if stackLen < 1 {
				return 0, 0, nil, ErrStackUnderflow
			}
		case Operation_AddVec, Operation_SubVec, Operation_MulVec, Operation_DivVec:
			if avail() < 1 {
				return 0, 0, nil, ErrIllegalOperation
			}
			size := fetchByte()
			if stackLen < size*2 {
				return 0, 0, nil, ErrStackUnderflow
			}
			stackLen -= size
		case Operation_MulAddVec:
			if avail() < 1 {
				return 0, 0, nil, ErrIllegalOperation
			}
			size := fetchByte()
			if stackLen < size*3 {
				return 0, 0, nil, ErrStackUnderflow
			}
			stackLen -= size * 2
		case Operation_ScaleVec:
			if avail() < 1 {
				return 0, 0, nil, ErrIllegalOperation
			}
			size := fetchByte()
			if stackLen < size+1 {
				return 0, 0, nil, ErrStackUnderflow
			}
			stackLen -= 1
		case Operation_NegVec, Operation_InvVec:
			if avail() < 1 {
				return 0, 0, nil, ErrIllegalOperation
			}
			size := fetchByte()
			if stackLen < size {
				return 0, 0, nil, ErrStackUnderflow
			}
		case Operation_NormSqVec, Operation_SumVec:
			if avail() < 1 {
				return 0, 0, nil, ErrIllegalOperation
			}
			size := fetchByte()
			if stackLen < size {
				return 0, 0, nil, ErrStackUnderflow
			}
			if size == 0 {
				stackLen += 1
				updateMax(stackLen)
			} else {
				stackLen = stackLen - size + 1
			}
		case Operation_MulMat:
			if avail() < 1 {
				return 0, 0, nil, ErrIllegalOperation
			}
			arg := fetchByte()
			arows, brows, bcols, immediate := unpackMulMat(uint8(arg))
			if bcols <= 0 {
				return 0, 0, nil, ErrIllegalOperation
			}
			asize := arows * brows
			bsize := brows * bcols
			csize := arows * bcols
			if immediate {
				if avail() < bsize*4 {
					return 0, 0, nil, ErrIllegalOperation
				}
				pc += bsize * 4
				if stackLen < asize {
					return 0, 0, nil, ErrStackUnderflow
				}
				updateMax(stackLen + csize)
				stackLen = stackLen - asize + csize
			} else {
				if stackLen < asize+bsize {
					return 0, 0, nil, ErrStackUnderflow
				}
				updateMax(stackLen + csize)
				stackLen = stackLen - (asize + bsize) + csize
			}
		case Operation_Polynomial:
			if avail() < 1 {
				return 0, 0, nil, ErrIllegalOperation
			}
			arg := fetchByte()
			rows := int(arg&0x0F) + 1
			cols := int((arg>>4)&0x0F) + 1
			coeffSize := rows * cols
			if avail() < coeffSize*4 {
				return 0, 0, nil, ErrIllegalOperation
			}
			pc += coeffSize * 4
			if stackLen < 1 {
				return 0, 0, nil, ErrStackUnderflow
			}
			stackLen = stackLen - 1 + cols
			updateMax(stackLen)
		case Operation_LerpVec:
			if avail() < 1 {
				return 0, 0, nil, ErrIllegalOperation
			}
			size := fetchByte()
			if stackLen < size*2+1 {
				return 0, 0, nil, ErrStackUnderflow
			}
			stackLen -= size + 1
		default:
			return 0, 0, nil, ErrUndefinedOperation
		}
		ops = append(ops, op)
	}

	if maxStackLen > math.MaxUint16 {
		return 0, 0, nil, ErrStackOverflow
	}

	return maxStackLen, stackLen, ops, nil
}

func Validate(expr *Expression, initialStackLen int) (minCapacity, finalStackLen int, ops []Operation, err error) {
	return expr.Validate(initialStackLen)
}

func (expr *Expression) Evaluate(stack []float64) []float64 {
	pc := 0

	pop := func() float64 {
		i := len(stack) - 1
		v := stack[i]
		stack = stack[:i]
		return v
	}
	popv := func(n int) []float64 {
		i := len(stack) - n
		v := stack[i:]
		stack = stack[:i]
		return v
	}
	fetchByte := func() int {
		v := expr.Prog[pc]
		pc++
		return int(v)
	}
	fetchFloat := func() float64 {
		bits := binary.LittleEndian.Uint32(expr.Prog[pc : pc+4])
		pc += 4
		return float64(math.Float32frombits(bits))
	}

	for pc < len(expr.Prog) {
		op := Operation(fetchByte())
		switch op {
		case Operation_Push:
			n := fetchByte()
			for range n {
				stack = append(stack, fetchFloat())
			}
		case Operation_Drop:
			stack = stack[:len(stack)-1]
		case Operation_Dup:
			stack = append(stack, stack[len(stack)-1])
		case Operation_Swap:
			i := len(stack) - 1
			stack[i], stack[i-1] = stack[i-1], stack[i]
		case Operation_Over:
			stack = append(stack, stack[len(stack)-2])
		case Operation_Tuck:
			top := stack[len(stack)-1]
			stack[len(stack)-1] = stack[len(stack)-2]
			stack[len(stack)-2] = top
			stack = append(stack, top)
		case Operation_Nip:
			stack[len(stack)-2] = stack[len(stack)-1]
			stack = stack[:len(stack)-1]
		case Operation_Rot:
			i := len(stack) - 1
			b := stack[i-2]
			stack[i-2] = stack[i-1]
			stack[i-1] = stack[i]
			stack[i] = b
		case Operation_Pick:
			arg := fetchByte()
			offset := arg & 0x1F
			move := (arg & 0x20) != 0
			size := ((arg >> 6) & 0x03) + 1
			if move {
				if offset > 0 {
					start := len(stack) - offset - size
					mid := start + size
					var tmp [4]float64
					copy(tmp[:size], stack[start:mid])
					copy(stack[start:], stack[mid:])
					copy(stack[len(stack)-size:], tmp[:size])
				}
			} else {
				start := len(stack) - offset - size
				stack = append(stack, stack[start:start+size]...)
			}
		case Operation_Add:
			rhs := pop()
			lhs := pop()
			stack = append(stack, lhs+rhs)
		case Operation_Sub:
			rhs := pop()
			lhs := pop()
			stack = append(stack, lhs-rhs)
		case Operation_Mul:
			rhs := pop()
			lhs := pop()
			stack = append(stack, lhs*rhs)
		case Operation_MulAdd:
			c := pop()
			b := pop()
			a := pop()
			stack = append(stack, a*b+c)
		case Operation_Div:
			rhs := pop()
			lhs := pop()
			stack = append(stack, lhs/rhs)
		case Operation_Mod:
			rhs := pop()
			lhs := pop()
			stack = append(stack, math.Mod(lhs, rhs))
		case Operation_Neg:
			v := pop()
			stack = append(stack, -v)
		case Operation_Abs:
			v := pop()
			stack = append(stack, math.Abs(v))
		case Operation_Inv:
			v := pop()
			stack = append(stack, 1/v)
		case Operation_Pow:
			rhs := pop()
			lhs := pop()
			stack = append(stack, math.Pow(lhs, rhs))
		case Operation_Sqrt:
			v := pop()
			stack = append(stack, math.Sqrt(v))
		case Operation_Exp:
			v := pop()
			stack = append(stack, math.Exp(v))
		case Operation_Ln:
			v := pop()
			stack = append(stack, math.Log(v))
		case Operation_Sin:
			v := pop()
			stack = append(stack, math.Sin(v))
		case Operation_Cos:
			v := pop()
			stack = append(stack, math.Cos(v))
		case Operation_Tan:
			v := pop()
			stack = append(stack, math.Tan(v))
		case Operation_Asin:
			v := pop()
			stack = append(stack, math.Asin(v))
		case Operation_Acos:
			v := pop()
			stack = append(stack, math.Acos(v))
		case Operation_Atan2:
			x := pop()
			y := pop()
			stack = append(stack, math.Atan2(y, x))
		case Operation_AddVec:
			size := fetchByte()
			rhs := popv(size)
			lhs := popv(size)
			for i := range size {
				stack = append(stack, lhs[i]+rhs[i])
			}
		case Operation_SubVec:
			size := fetchByte()
			rhs := popv(size)
			lhs := popv(size)
			for i := range size {
				stack = append(stack, lhs[i]-rhs[i])
			}
		case Operation_MulVec:
			size := fetchByte()
			rhs := popv(size)
			lhs := popv(size)
			for i := range size {
				stack = append(stack, lhs[i]*rhs[i])
			}
		case Operation_MulAddVec:
			size := fetchByte()
			c := popv(size)
			b := popv(size)
			a := popv(size)
			for i := range size {
				stack = append(stack, a[i]*b[i]+c[i])
			}
		case Operation_ScaleVec:
			size := fetchByte()
			vec := popv(size)
			scalar := pop()
			for i := range size {
				stack = append(stack, scalar*vec[i])
			}
		case Operation_DivVec:
			size := fetchByte()
			rhs := popv(size)
			lhs := popv(size)
			for i := range size {
				stack = append(stack, lhs[i]/rhs[i])
			}
		case Operation_NegVec:
			size := fetchByte()
			vec := popv(size)
			for i := range size {
				stack = append(stack, -vec[i])
			}
		case Operation_InvVec:
			size := fetchByte()
			vec := popv(size)
			for i := range size {
				stack = append(stack, 1.0/vec[i])
			}
		case Operation_NormSqVec:
			size := fetchByte()
			vec := popv(size)
			var result float64
			for i := range size {
				result += vec[i] * vec[i]
			}
			stack = append(stack, result)
		case Operation_SumVec:
			size := fetchByte()
			vec := popv(size)
			var result float64
			for i := range size {
				result += vec[i]
			}
			stack = append(stack, result)
		case Operation_MulMat:
			arg := fetchByte()
			arows, brows, bcols, immediate := unpackMulMat(uint8(arg))
			asize := arows * brows
			bsize := brows * bcols
			csize := arows * bcols
			var b []float64
			if immediate {
				b = make([]float64, bsize)
				for i := range bsize {
					b[i] = fetchFloat()
				}
			} else {
				b = popv(bsize)
			}
			a := popv(asize)
			c := make([]float64, csize)
			for i := range arows {
				for j := range bcols {
					var r float64
					for k := range brows {
						r += a[brows*i+k] * b[bcols*k+j]
					}
					c[bcols*i+j] = r
				}
			}
			stack = append(stack, c...)
		case Operation_Polynomial:
			arg := fetchByte()
			rows := int(arg&0x0F) + 1
			cols := int((arg>>4)&0x0F) + 1
			t := pop()
			if cols == 1 {
				result := fetchFloat()
				for range rows - 1 {
					result = result*t + fetchFloat()
				}
				stack = append(stack, result)
			} else {
				result := make([]float64, cols)
				for j := range cols {
					result[j] = fetchFloat()
				}
				for range rows - 1 {
					for j := range cols {
						result[j] = result[j]*t + fetchFloat()
					}
				}
				stack = append(stack, result...)
			}
		case Operation_LerpVec:
			size := fetchByte()
			v1 := popv(size)
			v0 := popv(size)
			t := pop()
			for i := range size {
				stack = append(stack, (1-t)*v0[i]+t*v1[i])
			}
		}
	}
	return stack
}

func Evaluate(expr *Expression, stack []float64) []float64 {
	return expr.Evaluate(stack)
}

func (expr *Expression) Eval(stack []float64) ([]float64, error) {
	if _, _, _, err := expr.Validate(len(stack)); err != nil {
		return nil, err
	}
	return expr.Evaluate(stack), nil
}

func Eval(expr *Expression, stack []float64) ([]float64, error) {
	return expr.Eval(stack)
}
