package postfix

import (
	"cmp"
	"encoding/binary"
	"errors"
	"fmt"
	"io"
	"math"
	"slices"
	"sort"
	"strings"
)

var (
	ErrUndefinedOperation     = errors.New("undefined operation")
	ErrIllegalOperation       = errors.New("illegal operation")
	ErrStackUnderflow         = errors.New("stack underflow")
	ErrFloatLiteralsUnderflow = errors.New("floats underflow")
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
	Operation_Add       Operation = 8
	Operation_Sub       Operation = 9
	Operation_Mul       Operation = 10
	Operation_MulAdd    Operation = 11
	Operation_Div       Operation = 12
	Operation_Mod       Operation = 13
	Operation_Neg       Operation = 14
	Operation_Abs       Operation = 15
	Operation_Inv       Operation = 16
	Operation_Pow       Operation = 17
	Operation_Sqrt      Operation = 18
	Operation_Exp       Operation = 19
	Operation_Ln        Operation = 20
	Operation_Sin       Operation = 21
	Operation_Cos       Operation = 22
	Operation_Tan       Operation = 23
	Operation_Asin      Operation = 24
	Operation_Acos      Operation = 25
	Operation_Atan2     Operation = 26
	Operation_AddVec    Operation = 27
	Operation_SubVec    Operation = 28
	Operation_MulVec    Operation = 29
	Operation_MulAddVec Operation = 30
	Operation_ScaleVec  Operation = 31
	Operation_NegVec    Operation = 32
	Operation_NormVec   Operation = 33
	Operation_MulMat    Operation = 34
	Operation_PolyVec   Operation = 35
	Operation_PolyMat   Operation = 36
	Operation_Lerp      Operation = 37
	Operation_LerpTable Operation = 38
)

type Expression struct {
	I []uint8
	D []float32
}

type PostfixHeader struct {
	ISize uint16
	DSize uint16
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
	case Operation_Lerp:
		return "Lerp"
	case Operation_LerpTable:
		return "LerpTable"
	default:
		return fmt.Sprintf("unknown[%d]", op)
	}
}

func (expr Expression) String() string {
	var sb strings.Builder
	sb.WriteString("{")
	space := false
	for _, v := range expr.I {
		if space {
			sb.WriteString(" ")
		}
		sb.WriteString("i:")
		sb.WriteString(fmt.Sprint(v))
		space = true
	}
	for _, v := range expr.D {
		if space {
			sb.WriteString(" ")
		}
		sb.WriteString("d:")
		sb.WriteString(fmt.Sprint(v))
		space = true
	}
	sb.WriteString("}")
	return sb.String()
}

func (expr *Expression) Write(w io.Writer) error {
	header := PostfixHeader{
		ISize: uint16(len(expr.I)),
		DSize: uint16(len(expr.D)),
	}
	padlen := 4 - header.ISize%4
	padding := make([]uint8, padlen)
	return cmp.Or(
		binary.Write(w, binary.LittleEndian, &header),
		binary.Write(w, binary.LittleEndian, &expr.I),
		binary.Write(w, binary.LittleEndian, &padding),
		binary.Write(w, binary.LittleEndian, &expr.D),
	)
}

func (expr *Expression) Read(r io.Reader) error {
	var header PostfixHeader
	err := binary.Read(r, binary.LittleEndian, &header)
	if err != nil {
		return err
	}
	expr.I = make([]uint8, header.ISize)
	expr.D = make([]float32, header.DSize)
	padlen := 4 - header.ISize%4
	padding := make([]uint8, padlen)
	return cmp.Or(
		binary.Read(r, binary.LittleEndian, &expr.I),
		binary.Read(r, binary.LittleEndian, &padding),
		binary.Read(r, binary.LittleEndian, &expr.D),
	)
}

func Equal(a, b *Expression) bool {
	return slices.Equal(a.I, b.I) && slices.Equal(a.D, b.D)
}

func Join(exprs ...*Expression) *Expression {
	result := &Expression{}
	for _, expr := range exprs {
		result.I = append(result.I, expr.I...)
		result.D = append(result.D, expr.D...)
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

func (b *Builder) push(literals []float64) {
	for _, v := range literals {
		b.expr.D = append(b.expr.D, float32(v))
	}
}

func (b *Builder) Push(literals ...float64) *Builder {
	b.expr.I = append(b.expr.I, uint8(Operation_Push), uint8(len(literals)))
	b.push(literals)
	return b
}

func (b *Builder) Pop(n int) *Builder {
	b.expr.I = append(b.expr.I, uint8(Operation_Pop), uint8(n))
	return b
}

func (b *Builder) Dup(n int) *Builder {
	b.expr.I = append(b.expr.I, uint8(Operation_Dup), uint8(n))
	return b
}

func (b *Builder) RotL(n int) *Builder {
	b.expr.I = append(b.expr.I, uint8(Operation_RotL), uint8(n))
	return b
}

func (b *Builder) RotR(n int) *Builder {
	b.expr.I = append(b.expr.I, uint8(Operation_RotR), uint8(n))
	return b
}

func (b *Builder) Rev(n int) *Builder {
	b.expr.I = append(b.expr.I, uint8(Operation_Rev), uint8(n))
	return b
}

func (b *Builder) Add() *Builder {
	b.expr.I = append(b.expr.I, uint8(Operation_Add))
	return b
}

func (b *Builder) Sub() *Builder {
	b.expr.I = append(b.expr.I, uint8(Operation_Sub))
	return b
}

func (b *Builder) Mul() *Builder {
	b.expr.I = append(b.expr.I, uint8(Operation_Mul))
	return b
}

func (b *Builder) MulAdd() *Builder {
	b.expr.I = append(b.expr.I, uint8(Operation_MulAdd))
	return b
}

func (b *Builder) Div() *Builder {
	b.expr.I = append(b.expr.I, uint8(Operation_Div))
	return b
}

func (b *Builder) Mod() *Builder {
	b.expr.I = append(b.expr.I, uint8(Operation_Mod))
	return b
}

func (b *Builder) Neg() *Builder {
	b.expr.I = append(b.expr.I, uint8(Operation_Neg))
	return b
}

func (b *Builder) Abs() *Builder {
	b.expr.I = append(b.expr.I, uint8(Operation_Abs))
	return b
}

func (b *Builder) Inv() *Builder {
	b.expr.I = append(b.expr.I, uint8(Operation_Inv))
	return b
}

func (b *Builder) Pow() *Builder {
	b.expr.I = append(b.expr.I, uint8(Operation_Pow))
	return b
}

func (b *Builder) Sqrt() *Builder {
	b.expr.I = append(b.expr.I, uint8(Operation_Sqrt))
	return b
}

func (b *Builder) Exp() *Builder {
	b.expr.I = append(b.expr.I, uint8(Operation_Exp))
	return b
}

func (b *Builder) Ln() *Builder {
	b.expr.I = append(b.expr.I, uint8(Operation_Ln))
	return b
}

func (b *Builder) Sin() *Builder {
	b.expr.I = append(b.expr.I, uint8(Operation_Sin))
	return b
}

func (b *Builder) Cos() *Builder {
	b.expr.I = append(b.expr.I, uint8(Operation_Cos))
	return b
}

func (b *Builder) Tan() *Builder {
	b.expr.I = append(b.expr.I, uint8(Operation_Tan))
	return b
}

func (b *Builder) Asin() *Builder {
	b.expr.I = append(b.expr.I, uint8(Operation_Asin))
	return b
}

func (b *Builder) Acos() *Builder {
	b.expr.I = append(b.expr.I, uint8(Operation_Acos))
	return b
}

func (b *Builder) Atan2() *Builder {
	b.expr.I = append(b.expr.I, uint8(Operation_Atan2))
	return b
}

func (b *Builder) PolyVec(coeffs int) *Builder {
	b.expr.I = append(b.expr.I, uint8(Operation_PolyVec), uint8(coeffs)<<1)
	return b
}

func (b *Builder) PushPolyVec(size int, literals []float64) *Builder {
	if size != len(literals) {
		panic("dimension mismatch")
	}
	b.expr.I = append(b.expr.I, uint8(Operation_PolyVec), uint8(size)<<1|1)
	b.push(literals)
	return b
}

func (b *Builder) PolyMat(rows, cols int) *Builder {
	b.expr.I = append(b.expr.I, uint8(Operation_PolyMat), uint8(rows), uint8(cols)<<1)
	return b
}

func (b *Builder) PushPolyMat(rows, cols int, literals []float64) *Builder {
	if rows*cols != len(literals) {
		panic("dimension mismatch")
	}
	b.expr.I = append(b.expr.I, uint8(Operation_PolyMat), uint8(rows), uint8(cols)<<1|1)
	b.push(literals)
	return b
}

func (b *Builder) AddVec(size int) *Builder {
	b.expr.I = append(b.expr.I, uint8(Operation_AddVec), uint8(size)<<1)
	return b
}

func (b *Builder) PushAddVec(size int, literals []float64) *Builder {
	if size != len(literals) {
		panic("dimension mismatch")
	}
	b.expr.I = append(b.expr.I, uint8(Operation_AddVec), uint8(size)<<1|1)
	b.push(literals)
	return b
}

func (b *Builder) SubVec(size int) *Builder {
	b.expr.I = append(b.expr.I, uint8(Operation_SubVec), uint8(size)<<1)
	return b
}

func (b *Builder) PushSubVec(size int, literals []float64) *Builder {
	if size != len(literals) {
		panic("dimension mismatch")
	}
	b.expr.I = append(b.expr.I, uint8(Operation_SubVec), uint8(size)<<1|1)
	b.push(literals)
	return b
}

func (b *Builder) MulVec(size int) *Builder {
	b.expr.I = append(b.expr.I, uint8(Operation_MulVec), uint8(size)<<1)
	return b
}

func (b *Builder) PushMulVec(size int, literals []float64) *Builder {
	if size != len(literals) {
		panic("dimension mismatch")
	}
	b.expr.I = append(b.expr.I, uint8(Operation_MulVec), uint8(size)<<1|1)
	b.push(literals)
	return b
}

func (b *Builder) MulAddVec(size int) *Builder {
	b.expr.I = append(b.expr.I, uint8(Operation_MulAddVec), uint8(size)<<2)
	return b
}

func (b *Builder) PushMulAddVec(size int, literals ...[]float64) *Builder {
	for _, l := range literals {
		if size != len(l) {
			panic("dimension mismatch")
		}
		b.push(l)
	}
	b.expr.I = append(b.expr.I, uint8(Operation_MulAddVec), uint8(size)<<2|uint8(len(literals)))
	return b
}

func (b *Builder) ScaleVec(size int) *Builder {
	b.expr.I = append(b.expr.I, uint8(Operation_ScaleVec), uint8(size)<<1)
	return b
}

func (b *Builder) PushScaleVec(size int, literals []float64) *Builder {
	if size != len(literals) {
		panic("dimension mismatch")
	}
	b.expr.I = append(b.expr.I, uint8(Operation_ScaleVec), uint8(size)<<1|1)
	b.push(literals)
	return b
}

func (b *Builder) NegVec(size int) *Builder {
	b.expr.I = append(b.expr.I, uint8(Operation_NegVec), uint8(size)<<1)
	return b
}

func (b *Builder) PushNegVec(size int, literals []float64) *Builder {
	if size != len(literals) {
		panic("dimension mismatch")
	}
	b.expr.I = append(b.expr.I, uint8(Operation_NegVec), uint8(size)<<1|1)
	b.push(literals)
	return b
}

func (b *Builder) NormVec(size int) *Builder {
	b.expr.I = append(b.expr.I, uint8(Operation_NormVec), uint8(size)<<1)
	return b
}

func (b *Builder) PushNormVec(size int, literals []float64) *Builder {
	if size != len(literals) {
		panic("dimension mismatch")
	}
	b.expr.I = append(b.expr.I, uint8(Operation_NormVec), uint8(size)<<1|1)
	b.push(literals)
	return b
}

func (b *Builder) MulMat(arows, brows, bcols int) *Builder {
	b.expr.I = append(b.expr.I, uint8(Operation_MulMat), uint8(arows), uint8(brows), uint8(bcols)<<1)
	return b
}

func (b *Builder) PushMulMat(arows, brows, bcols int, literals []float64) *Builder {
	if brows*bcols != len(literals) {
		panic("dimension mismatch")
	}
	b.push(literals)
	b.expr.I = append(b.expr.I, uint8(Operation_MulMat), uint8(arows), uint8(brows), uint8(bcols)<<1|1)
	return b
}

func (b *Builder) Transpose(rows, cols int) *Builder {
	b.expr.I = append(b.expr.I, uint8(Operation_Transpose), uint8(rows), uint8(cols))
	return b
}

func (b *Builder) Lerp(size int) *Builder {
	b.expr.I = append(b.expr.I, uint8(Operation_Lerp), uint8(size)<<2)
	return b
}

func (b *Builder) PushLerp(size int, literals ...[]float64) *Builder {
	for _, l := range literals {
		if size != len(l) {
			panic("dimension mismatch")
		}
		b.push(l)
	}
	b.expr.I = append(b.expr.I, uint8(Operation_Lerp), uint8(size)<<2|uint8(len(literals)))
	return b
}

func (b *Builder) LerpTable(rows, cols int) *Builder {
	b.expr.I = append(b.expr.I, uint8(Operation_LerpTable), uint8(rows), uint8(cols)<<1)
	return b
}

func (b *Builder) PushLerpTable(rows, cols int, literals []float64) *Builder {
	if rows*cols != len(literals) {
		panic("dimension mismatch")
	}
	b.expr.I = append(b.expr.I, uint8(Operation_LerpTable), uint8(rows), uint8(cols)<<1|1)
	b.push(literals)
	return b
}

func Eval(expr *Expression, stack []float64) ([]float64, error) {
	i_idx := 0
	f_idx := 0

	push := func(n int) error {
		if len(expr.D)-f_idx < n {
			return ErrFloatLiteralsUnderflow
		}
		values := expr.D[f_idx : f_idx+n]
		f_idx += n
		for _, value := range values {
			stack = append(stack, float64(value))
		}
		return nil
	}
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
	popi := func() int {
		v := expr.I[i_idx]
		i_idx++
		return int(v)
	}
	popiPush := func(bits, multiple int) (int, error) {
		n := popi()
		mask := (1 << bits) - 1
		count := n & mask
		i := n >> bits
		var err error
		if count > 0 {
			err = push(count * i * multiple)
		}
		return i, err
	}
	availi := func() int {
		return len(expr.I) - i_idx
	}

	for availi() > 0 {
		op := Operation(popi())
		switch op {
		case Operation_Undefined:
			return nil, ErrUndefinedOperation
		case Operation_Push:
			if availi() < 1 {
				return nil, ErrIllegalOperation
			}
			n := popi()

			if err := push(n); err != nil {
				return nil, err
			}
		case Operation_Pop:
			if availi() < 1 {
				return nil, ErrIllegalOperation
			}
			n := popi()

			if len(stack) < n {
				return nil, ErrStackUnderflow
			}
			stack = stack[:len(stack)-n]
		case Operation_Dup:
			if availi() < 1 {
				return nil, ErrIllegalOperation
			}
			n := popi()

			if len(stack) < n+1 {
				return nil, ErrStackUnderflow
			}
			stack = append(stack, stack[len(stack)-n-1])
		case Operation_RotL:
			if availi() < 1 {
				return nil, ErrIllegalOperation
			}
			n := popi()

			if n > 1 {
				if len(stack) < n {
					return nil, ErrStackUnderflow
				}
				values := popv(n)
				result := append(values[1:], values[0])
				stack = append(stack, result...)
			}
		case Operation_RotR:
			if availi() < 1 {
				return nil, ErrIllegalOperation
			}
			n := popi()

			if n > 1 {
				if len(stack) < n {
					return nil, ErrStackUnderflow
				}
				values := popv(n)
				result := append([]float64{values[len(values)-1]}, values[0:len(values)-1]...)
				stack = append(stack, result...)
			}
		case Operation_Rev:
			if availi() < 1 {
				return nil, ErrIllegalOperation
			}
			n := popi()

			if len(stack) < n {
				return nil, ErrStackUnderflow
			}
			values := stack[len(stack)-n:]
			for i, j := 0, n-1; i < j; i, j = i+1, j-1 {
				values[i], values[j] = values[j], values[i]
			}
		case Operation_Transpose:
			if availi() < 2 {
				return nil, ErrIllegalOperation
			}
			rows := popi()
			cols := popi()
			size := rows * cols

			if len(stack) < size {
				return nil, ErrStackUnderflow
			}
			m := popv(size)
			t := make([]float64, size)
			for i := range rows {
				for j := range cols {
					t[rows*j+i] = m[cols*i+j]
				}
			}
			stack = append(stack, t...)
		case Operation_Add:
			if len(stack) < 2 {
				return nil, ErrStackUnderflow
			}
			rhs := pop()
			lhs := pop()
			result := lhs + rhs
			stack = append(stack, result)
		case Operation_Sub:
			if len(stack) < 2 {
				return nil, ErrStackUnderflow
			}
			rhs := pop()
			lhs := pop()
			result := lhs - rhs
			stack = append(stack, result)
		case Operation_Mul:
			if len(stack) < 2 {
				return nil, ErrStackUnderflow
			}
			rhs := pop()
			lhs := pop()
			result := lhs * rhs
			stack = append(stack, result)
		case Operation_MulAdd:
			if len(stack) < 3 {
				return nil, ErrStackUnderflow
			}
			c := pop()
			b := pop()
			a := pop()
			result := a*b + c
			stack = append(stack, result)
		case Operation_Div:
			if len(stack) < 2 {
				return nil, ErrStackUnderflow
			}
			rhs := pop()
			lhs := pop()
			result := lhs / rhs
			stack = append(stack, result)
		case Operation_Mod:
			if len(stack) < 2 {
				return nil, ErrStackUnderflow
			}
			rhs := pop()
			lhs := pop()
			result := math.Remainder(lhs, rhs)
			stack = append(stack, result)
		case Operation_Neg:
			if len(stack) < 1 {
				return nil, ErrStackUnderflow
			}
			operand := pop()
			result := -operand
			stack = append(stack, result)
		case Operation_Abs:
			if len(stack) < 1 {
				return nil, ErrStackUnderflow
			}
			operand := pop()
			result := math.Abs(operand)
			stack = append(stack, result)
		case Operation_Inv:
			if len(stack) < 1 {
				return nil, ErrStackUnderflow
			}
			operand := pop()
			result := 1 / operand
			stack = append(stack, result)
		case Operation_Pow:
			if len(stack) < 2 {
				return nil, ErrStackUnderflow
			}
			exp := pop()
			base := pop()
			result := math.Pow(base, exp)
			stack = append(stack, result)
		case Operation_Sqrt:
			if len(stack) < 1 {
				return nil, ErrStackUnderflow
			}
			operand := pop()
			result := math.Sqrt(operand)
			stack = append(stack, result)
		case Operation_Exp:
			if len(stack) < 1 {
				return nil, ErrStackUnderflow
			}
			operand := pop()
			result := math.Exp(operand)
			stack = append(stack, result)
		case Operation_Ln:
			if len(stack) < 1 {
				return nil, ErrStackUnderflow
			}
			operand := pop()
			result := math.Log(operand)
			stack = append(stack, result)
		case Operation_Sin:
			if len(stack) < 1 {
				return nil, ErrStackUnderflow
			}
			operand := pop()
			result := math.Sin(operand)
			stack = append(stack, result)
		case Operation_Cos:
			if len(stack) < 1 {
				return nil, ErrStackUnderflow
			}
			operand := pop()
			result := math.Cos(operand)
			stack = append(stack, result)
		case Operation_Tan:
			if len(stack) < 1 {
				return nil, ErrStackUnderflow
			}
			operand := pop()
			result := math.Tan(operand)
			stack = append(stack, result)
		case Operation_Asin:
			if len(stack) < 1 {
				return nil, ErrStackUnderflow
			}
			operand := pop()
			result := math.Asin(operand)
			stack = append(stack, result)
		case Operation_Acos:
			if len(stack) < 1 {
				return nil, ErrStackUnderflow
			}
			operand := pop()
			result := math.Acos(operand)
			stack = append(stack, result)
		case Operation_Atan2:
			if len(stack) < 2 {
				return nil, ErrStackUnderflow
			}
			x := pop()
			y := pop()
			result := math.Atan2(y, x)
			stack = append(stack, result)
		case Operation_PolyVec:
			if availi() < 1 {
				return nil, ErrIllegalOperation
			}
			size, err := popiPush(1, 1)
			if err != nil {
				return nil, err
			}

			if len(stack) < size+1 {
				return nil, ErrStackUnderflow
			}
			coeff := popv(size)
			param := pop()
			paramPower := float64(1)
			var result float64
			for n := range size {
				result += coeff[n] * paramPower
				paramPower *= param
			}
			stack = append(stack, result)
		case Operation_PolyMat:
			if availi() < 2 {
				return nil, ErrIllegalOperation
			}
			rows := popi()
			cols, err := popiPush(1, rows)
			if err != nil {
				return nil, err
			}
			coeffs := rows * cols

			if len(stack) < coeffs+1 {
				return nil, ErrStackUnderflow
			}
			coeff := popv(coeffs)
			param := pop()
			paramPower := float64(1)
			result := make([]float64, cols)
			for i := range rows {
				for j := range cols {
					result[j] += coeff[cols*i+j] * paramPower
				}
				paramPower *= param
			}
			stack = append(stack, result...)
		case Operation_AddVec:
			if availi() < 1 {
				return nil, ErrIllegalOperation
			}
			size, err := popiPush(1, 1)
			if err != nil {
				return nil, err
			}

			if len(stack) < size*2 {
				return nil, ErrStackUnderflow
			}
			rhs := popv(size)
			lhs := popv(size)
			for i := range size {
				stack = append(stack, lhs[i]+rhs[i])
			}
		case Operation_SubVec:
			if availi() < 1 {
				return nil, ErrIllegalOperation
			}
			size, err := popiPush(1, 1)
			if err != nil {
				return nil, err
			}

			if len(stack) < size*2 {
				return nil, ErrStackUnderflow
			}
			rhs := popv(size)
			lhs := popv(size)
			for i := range size {
				stack = append(stack, lhs[i]-rhs[i])
			}
		case Operation_MulVec:
			if availi() < 1 {
				return nil, ErrIllegalOperation
			}
			size, err := popiPush(1, 1)
			if err != nil {
				return nil, err
			}

			if len(stack) < size*2 {
				return nil, ErrStackUnderflow
			}
			rhs := popv(size)
			lhs := popv(size)
			for i := range size {
				stack = append(stack, lhs[i]*rhs[i])
			}
		case Operation_MulAddVec:
			if availi() < 1 {
				return nil, ErrIllegalOperation
			}
			size, err := popiPush(2, 1)
			if err != nil {
				return nil, err
			}

			if len(stack) < size*3 {
				return nil, ErrStackUnderflow
			}
			c := popv(size)
			b := popv(size)
			a := popv(size)
			for i := range size {
				stack = append(stack, a[i]*b[i]+c[i])
			}
		case Operation_ScaleVec:
			if availi() < 1 {
				return nil, ErrIllegalOperation
			}
			size, err := popiPush(1, 1)
			if err != nil {
				return nil, err
			}

			if len(stack) < size+1 {
				return nil, ErrStackUnderflow
			}
			vec := popv(size)
			scalar := pop()
			for i := range size {
				stack = append(stack, scalar*vec[i])
			}
		case Operation_NegVec:
			if availi() < 1 {
				return nil, ErrIllegalOperation
			}
			size, err := popiPush(1, 1)
			if err != nil {
				return nil, err
			}

			if len(stack) < size {
				return nil, ErrStackUnderflow
			}
			vec := popv(size)
			for i := range size {
				stack = append(stack, -vec[i])
			}
		case Operation_NormVec:
			if availi() < 1 {
				return nil, ErrIllegalOperation
			}
			size, err := popiPush(1, 1)
			if err != nil {
				return nil, err
			}

			if len(stack) < size {
				return nil, ErrStackUnderflow
			}
			vec := popv(size)
			var result float64
			for i := range size {
				result += vec[i] * vec[i]
			}
			stack = append(stack, math.Sqrt(result))
		case Operation_MulMat:
			if availi() < 3 {
				return nil, ErrIllegalOperation
			}
			arows := popi()
			brows := popi()
			bcols, err := popiPush(1, brows)
			if err != nil {
				return nil, err
			}
			asize := arows * brows
			bsize := brows * bcols
			csize := arows * bcols
			if len(stack) < asize+bsize {
				return nil, ErrStackUnderflow
			}
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
		case Operation_Lerp:
			if availi() < 1 {
				return nil, ErrIllegalOperation
			}
			size, err := popiPush(2, 1)
			if err != nil {
				return nil, err
			}

			if len(stack) < size*2+1 {
				return nil, ErrStackUnderflow
			}
			v1 := popv(size)
			v0 := popv(size)
			t := pop()
			result := make([]float64, size)
			for i := range size {
				result[i] = (1-t)*v0[i] + t*v1[i]
			}
			stack = append(stack, result...)
		case Operation_LerpTable:
			if availi() < 2 {
				return nil, ErrIllegalOperation
			}
			rows := popi()
			cols, err := popiPush(1, rows)
			if err != nil {
				return nil, err
			}
			size := rows * cols

			if len(stack) < size+1 {
				return nil, ErrStackUnderflow
			}
			lut := popv(size)
			t := pop()
			ub := sort.Search(rows, func(i int) bool {
				return t < lut[i*cols]
			})
			if ub == 0 {
				stack = append(stack, lut[1:cols]...)
			} else if ub == rows {
				stack = append(stack, lut[len(lut)-cols+1:]...)
			} else {
				t0 := lut[(ub-1)*cols]
				t1 := lut[ub*cols]
				t = (t - t0) / (t1 - t0)
				v0 := lut[(ub-1)*cols+1 : ub*cols]
				v1 := lut[ub*cols+1 : (ub+1)*cols]
				result := make([]float64, cols-1)
				for i := range cols - 1 {
					result[i] = (1-t)*v0[i] + t*v1[i]
				}
				stack = append(stack, result...)
			}
		default:
			return nil, ErrUndefinedOperation
		}
	}
	return stack, nil
}
