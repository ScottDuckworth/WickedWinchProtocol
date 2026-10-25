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
	Operation_Undefined Operation = 0
	Operation_Push      Operation = 1
	Operation_Pop       Operation = 2
	Operation_Dup       Operation = 3
	Operation_RotL      Operation = 4
	Operation_RotR      Operation = 5
	Operation_Rev       Operation = 6
	Operation_Transpose Operation = 7
	Operation_Store     Operation = 8
	Operation_Load      Operation = 9
	Operation_Add       Operation = 10
	Operation_Sub       Operation = 11
	Operation_Mul       Operation = 12
	Operation_MulAdd    Operation = 13
	Operation_Div       Operation = 14
	Operation_Mod       Operation = 15
	Operation_Neg       Operation = 16
	Operation_Abs       Operation = 17
	Operation_Inv       Operation = 18
	Operation_Pow       Operation = 19
	Operation_Sqrt      Operation = 20
	Operation_Exp       Operation = 21
	Operation_Ln        Operation = 22
	Operation_Sin       Operation = 23
	Operation_Cos       Operation = 24
	Operation_Tan       Operation = 25
	Operation_Asin      Operation = 26
	Operation_Acos      Operation = 27
	Operation_Atan2     Operation = 28
	Operation_AddVec    Operation = 29
	Operation_SubVec    Operation = 30
	Operation_MulVec    Operation = 31
	Operation_MulAddVec Operation = 32
	Operation_ScaleVec  Operation = 33
	Operation_NegVec    Operation = 34
	Operation_NormVec   Operation = 35
	Operation_MulMat    Operation = 36
	Operation_PolyVec   Operation = 37
	Operation_PolyMat   Operation = 38
	Operation_LerpVec   Operation = 39
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
	case Operation_Pop:
		return "Pop"
	case Operation_Dup:
		return "Dup"
	case Operation_RotL:
		return "RotL"
	case Operation_RotR:
		return "RotR"
	case Operation_Rev:
		return "Rev"
	case Operation_Transpose:
		return "Transpose"
	case Operation_Store:
		return "Store"
	case Operation_Load:
		return "Load"
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
	case Operation_NegVec:
		return "NegVec"
	case Operation_NormVec:
		return "NormVec"
	case Operation_MulMat:
		return "MulMat"
	case Operation_PolyVec:
		return "PolyVec"
	case Operation_PolyMat:
		return "PolyMat"
	case Operation_LerpVec:
		return "LerpVec"
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

func (b *Builder) Pop(n int) *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_Pop), uint8(n))
	return b
}

func (b *Builder) Dup(n int) *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_Dup), uint8(n))
	return b
}

func (b *Builder) RotL(n int) *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_RotL), uint8(n))
	return b
}

func (b *Builder) RotR(n int) *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_RotR), uint8(n))
	return b
}

func (b *Builder) Rev(n int) *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_Rev), uint8(n))
	return b
}

func (b *Builder) Transpose(rows, cols int) *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_Transpose), uint8(rows), uint8(cols))
	return b
}

func (b *Builder) Store(slot, size int, pop bool) *Builder {
	popBit := 0
	if pop {
		popBit = 1
	}
	arg := (slot & 0x03) | (popBit << 4) | (((size - 1) & 0x03) << 6)
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_Store), uint8(arg))
	return b
}

func (b *Builder) Load(slot, size int) *Builder {
	arg := (slot & 0x03) | (((size - 1) & 0x03) << 6)
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_Load), uint8(arg))
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

func (b *Builder) PolyVec(coeffs ...float64) *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_PolyVec), uint8(len(coeffs)))
	for i := len(coeffs) - 1; i >= 0; i-- {
		b.expr.Prog = binary.LittleEndian.AppendUint32(b.expr.Prog, math.Float32bits(float32(coeffs[i])))
	}
	return b
}

func (b *Builder) PolyMat(rows, cols int, coeffs ...float64) *Builder {
	if len(coeffs) != rows*cols {
		panic("invalid number of coefficients for PolyMat")
	}
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_PolyMat), uint8(rows), uint8(cols))
	for i := rows - 1; i >= 0; i-- {
		rowOffset := i * cols
		for j := range cols {
			b.expr.Prog = binary.LittleEndian.AppendUint32(b.expr.Prog, math.Float32bits(float32(coeffs[rowOffset+j])))
		}
	}
	return b
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

func (b *Builder) NegVec(size int) *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_NegVec), uint8(size))
	return b
}

func (b *Builder) NormVec(size int) *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_NormVec), uint8(size))
	return b
}

func (b *Builder) MulMat(arows, brows, bcols int) *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_MulMat), uint8(arows), uint8(brows), uint8(bcols))
	return b
}

func (b *Builder) LerpVec(size int) *Builder {
	b.expr.Prog = append(b.expr.Prog, uint8(Operation_LerpVec), uint8(size))
	return b
}

func (expr *Expression) Validate(initialStackLen int) (minCapacity, finalStackLen int, err error) {
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
			return 0, 0, ErrUndefinedOperation
		case Operation_Push:
			if avail() < 1 {
				return 0, 0, ErrIllegalOperation
			}
			n := fetchByte()
			if avail() < n*4 {
				return 0, 0, ErrIllegalOperation
			}
			pc += n * 4
			stackLen += n
			updateMax(stackLen)
		case Operation_Pop:
			if avail() < 1 {
				return 0, 0, ErrIllegalOperation
			}
			n := fetchByte()
			if stackLen < n {
				return 0, 0, ErrStackUnderflow
			}
			stackLen -= n
		case Operation_Dup:
			if avail() < 1 {
				return 0, 0, ErrIllegalOperation
			}
			n := fetchByte()
			if stackLen < n+1 {
				return 0, 0, ErrStackUnderflow
			}
			stackLen += 1
			updateMax(stackLen)
		case Operation_RotL, Operation_RotR, Operation_Rev:
			if avail() < 1 {
				return 0, 0, ErrIllegalOperation
			}
			n := fetchByte()
			if stackLen < n {
				return 0, 0, ErrStackUnderflow
			}
		case Operation_Transpose:
			if avail() < 2 {
				return 0, 0, ErrIllegalOperation
			}
			rows := fetchByte()
			cols := fetchByte()
			size := rows * cols
			if stackLen < size {
				return 0, 0, ErrStackUnderflow
			}
			if rows > 1 && cols > 1 && rows != cols {
				updateMax(stackLen + size)
			}
		case Operation_Store:
			if avail() < 1 {
				return 0, 0, ErrIllegalOperation
			}
			arg := fetchByte()
			slot := arg & 0x03
			pop := (arg & 0x10) != 0
			size := ((arg >> 6) & 0x03) + 1
			if slot+size > 4 {
				return 0, 0, ErrIllegalOperation
			}
			if stackLen < size {
				return 0, 0, ErrStackUnderflow
			}
			if pop {
				stackLen -= size
			}
		case Operation_Load:
			if avail() < 1 {
				return 0, 0, ErrIllegalOperation
			}
			arg := fetchByte()
			slot := arg & 0x03
			size := ((arg >> 6) & 0x03) + 1
			if slot+size > 4 {
				return 0, 0, ErrIllegalOperation
			}
			stackLen += size
			updateMax(stackLen)
		case Operation_Add, Operation_Sub, Operation_Mul, Operation_Div, Operation_Mod, Operation_Pow, Operation_Atan2:
			if stackLen < 2 {
				return 0, 0, ErrStackUnderflow
			}
			stackLen -= 1
		case Operation_MulAdd:
			if stackLen < 3 {
				return 0, 0, ErrStackUnderflow
			}
			stackLen -= 2
		case Operation_Neg, Operation_Abs, Operation_Inv, Operation_Sqrt, Operation_Exp, Operation_Ln, Operation_Sin, Operation_Cos, Operation_Tan, Operation_Asin, Operation_Acos:
			if stackLen < 1 {
				return 0, 0, ErrStackUnderflow
			}
		case Operation_AddVec, Operation_SubVec, Operation_MulVec:
			if avail() < 1 {
				return 0, 0, ErrIllegalOperation
			}
			size := fetchByte()
			if stackLen < size*2 {
				return 0, 0, ErrStackUnderflow
			}
			stackLen -= size
		case Operation_MulAddVec:
			if avail() < 1 {
				return 0, 0, ErrIllegalOperation
			}
			size := fetchByte()
			if stackLen < size*3 {
				return 0, 0, ErrStackUnderflow
			}
			stackLen -= size * 2
		case Operation_ScaleVec:
			if avail() < 1 {
				return 0, 0, ErrIllegalOperation
			}
			size := fetchByte()
			if stackLen < size+1 {
				return 0, 0, ErrStackUnderflow
			}
			stackLen -= 1
		case Operation_NegVec:
			if avail() < 1 {
				return 0, 0, ErrIllegalOperation
			}
			size := fetchByte()
			if stackLen < size {
				return 0, 0, ErrStackUnderflow
			}
		case Operation_NormVec:
			if avail() < 1 {
				return 0, 0, ErrIllegalOperation
			}
			size := fetchByte()
			if stackLen < size {
				return 0, 0, ErrStackUnderflow
			}
			if size == 0 {
				stackLen += 1
				updateMax(stackLen)
			} else {
				stackLen = stackLen - size + 1
			}
		case Operation_MulMat:
			if avail() < 3 {
				return 0, 0, ErrIllegalOperation
			}
			arows := fetchByte()
			brows := fetchByte()
			bcols := fetchByte()
			asize := arows * brows
			bsize := brows * bcols
			csize := arows * bcols
			if stackLen < asize+bsize {
				return 0, 0, ErrStackUnderflow
			}
			updateMax(stackLen + csize)
			stackLen = stackLen - (asize + bsize) + csize
		case Operation_PolyVec:
			if avail() < 1 {
				return 0, 0, ErrIllegalOperation
			}
			size := fetchByte()
			if avail() < size*4 {
				return 0, 0, ErrIllegalOperation
			}
			pc += size * 4
			if stackLen < 1 {
				return 0, 0, ErrStackUnderflow
			}
		case Operation_PolyMat:
			if avail() < 2 {
				return 0, 0, ErrIllegalOperation
			}
			rows := fetchByte()
			cols := fetchByte()
			coeffSize := rows * cols
			if avail() < coeffSize*4 {
				return 0, 0, ErrIllegalOperation
			}
			pc += coeffSize * 4
			if stackLen < 1 {
				return 0, 0, ErrStackUnderflow
			}
			stackLen = stackLen - 1 + cols
			updateMax(stackLen)
		case Operation_LerpVec:
			if avail() < 1 {
				return 0, 0, ErrIllegalOperation
			}
			size := fetchByte()
			if stackLen < size*2+1 {
				return 0, 0, ErrStackUnderflow
			}
			stackLen -= size + 1
		default:
			return 0, 0, ErrUndefinedOperation
		}
	}

	if maxStackLen > math.MaxUint16 {
		return 0, 0, ErrStackOverflow
	}

	return maxStackLen, stackLen, nil
}

func Validate(expr *Expression, initialStackLen int) (minCapacity, finalStackLen int, err error) {
	return expr.Validate(initialStackLen)
}

func (expr *Expression) Evaluate(stack []float64) []float64 {
	pc := 0
	scratch := [4]float64{}

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
		case Operation_Pop:
			n := fetchByte()
			stack = stack[:len(stack)-n]
		case Operation_Dup:
			n := fetchByte()
			stack = append(stack, stack[len(stack)-n-1])
		case Operation_RotL:
			n := fetchByte()
			if n > 1 {
				values := popv(n)
				result := append(values[1:], values[0])
				stack = append(stack, result...)
			}
		case Operation_RotR:
			n := fetchByte()
			if n > 1 {
				values := popv(n)
				result := append([]float64{values[len(values)-1]}, values[0:len(values)-1]...)
				stack = append(stack, result...)
			}
		case Operation_Rev:
			n := fetchByte()
			if n > 1 {
				values := stack[len(stack)-n:]
				for i, j := 0, n-1; i < j; i, j = i+1, j-1 {
					values[i], values[j] = values[j], values[i]
				}
			}
		case Operation_Transpose:
			rows := fetchByte()
			cols := fetchByte()
			size := rows * cols
			m := popv(size)
			t := make([]float64, size)
			for i := range rows {
				for j := range cols {
					t[rows*j+i] = m[cols*i+j]
				}
			}
			stack = append(stack, t...)
		case Operation_Store:
			arg := fetchByte()
			slot := arg & 0x03
			pop := (arg & 0x10) != 0
			size := ((arg >> 6) & 0x03) + 1
			var vals []float64
			if pop {
				vals = popv(size)
			} else {
				vals = stack[len(stack)-size:]
			}
			copy(scratch[slot:slot+size], vals)
		case Operation_Load:
			arg := fetchByte()
			slot := arg & 0x03
			size := ((arg >> 6) & 0x03) + 1
			stack = append(stack, scratch[slot:slot+size]...)
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
		case Operation_NegVec:
			size := fetchByte()
			vec := popv(size)
			for i := range size {
				stack = append(stack, -vec[i])
			}
		case Operation_NormVec:
			size := fetchByte()
			vec := popv(size)
			var result float64
			for i := range size {
				result += vec[i] * vec[i]
			}
			stack = append(stack, math.Sqrt(result))
		case Operation_MulMat:
			arows := fetchByte()
			brows := fetchByte()
			bcols := fetchByte()
			asize := arows * brows
			bsize := brows * bcols
			csize := arows * bcols
			b := popv(bsize)
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
		case Operation_PolyVec:
			size := fetchByte()
			t := pop()
			if size == 0 {
				stack = append(stack, 0)
				continue
			}
			result := fetchFloat()
			for range size - 1 {
				result = result*t + fetchFloat()
			}
			stack = append(stack, result)
		case Operation_PolyMat:
			rows := fetchByte()
			cols := fetchByte()
			t := pop()
			if rows == 0 {
				for range cols {
					stack = append(stack, 0)
				}
				continue
			}
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
	if _, _, err := expr.Validate(len(stack)); err != nil {
		return nil, err
	}
	return expr.Evaluate(stack), nil
}

func Eval(expr *Expression, stack []float64) ([]float64, error) {
	return expr.Eval(stack)
}
