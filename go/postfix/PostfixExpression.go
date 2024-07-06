package postfix

//go:generate protoc --go_out=./ --go_opt=module=github.com/ScottDuckworth/WickedWinchProtocol proto/PathExpression.proto

import (
	"errors"
	"math"
	"sort"

	pathpb "github.com/ScottDuckworth/WickedWinchProtocol/proto"
)

var (
	ErrUndefinedOperation     = errors.New("undefined operation")
	ErrStackUnderflow         = errors.New("stack underflow")
	ErrIntLiteralsUnderflow   = errors.New("ints underflow")
	ErrFloatLiteralsUnderflow = errors.New("floats underflow")
)

func Join(exprs ...*pathpb.PostfixExpression) *pathpb.PostfixExpression {
	result := &pathpb.PostfixExpression{}
	for _, expr := range exprs {
		result.Op = append(result.Op, expr.Op...)
		result.I = append(result.I, expr.I...)
		result.F = append(result.F, expr.F...)
	}
	return result
}

type Builder struct {
	expr pathpb.PostfixExpression
}

func MakeBuilder() *Builder {
	return &Builder{}
}

func (b *Builder) Build() *pathpb.PostfixExpression {
	return &b.expr
}

func (b *Builder) push(literals []float64) {
	for _, v := range literals {
		b.expr.F = append(b.expr.F, float32(v))
	}
}

func (b *Builder) Push(literals ...float64) *Builder {
	b.expr.Op = append(b.expr.Op, pathpb.Operation_Push)
	b.expr.I = append(b.expr.I, int32(len(literals)))
	b.push(literals)
	return b
}

func (b *Builder) Pop(n int) *Builder {
	b.expr.Op = append(b.expr.Op, pathpb.Operation_Pop)
	b.expr.I = append(b.expr.I, int32(n))
	return b
}

func (b *Builder) Dup(n int) *Builder {
	b.expr.Op = append(b.expr.Op, pathpb.Operation_Dup)
	b.expr.I = append(b.expr.I, int32(n))
	return b
}

func (b *Builder) RotL(n int) *Builder {
	b.expr.Op = append(b.expr.Op, pathpb.Operation_RotL)
	b.expr.I = append(b.expr.I, int32(n))
	return b
}

func (b *Builder) RotR(n int) *Builder {
	b.expr.Op = append(b.expr.Op, pathpb.Operation_RotR)
	b.expr.I = append(b.expr.I, int32(n))
	return b
}

func (b *Builder) Rev(n int) *Builder {
	b.expr.Op = append(b.expr.Op, pathpb.Operation_Rev)
	b.expr.I = append(b.expr.I, int32(n))
	return b
}

func (b *Builder) Add() *Builder {
	b.expr.Op = append(b.expr.Op, pathpb.Operation_Add)
	return b
}

func (b *Builder) Sub() *Builder {
	b.expr.Op = append(b.expr.Op, pathpb.Operation_Sub)
	return b
}

func (b *Builder) Mul() *Builder {
	b.expr.Op = append(b.expr.Op, pathpb.Operation_Mul)
	return b
}

func (b *Builder) MulAdd() *Builder {
	b.expr.Op = append(b.expr.Op, pathpb.Operation_MulAdd)
	return b
}

func (b *Builder) Div() *Builder {
	b.expr.Op = append(b.expr.Op, pathpb.Operation_Div)
	return b
}

func (b *Builder) Mod() *Builder {
	b.expr.Op = append(b.expr.Op, pathpb.Operation_Mod)
	return b
}

func (b *Builder) Neg() *Builder {
	b.expr.Op = append(b.expr.Op, pathpb.Operation_Neg)
	return b
}

func (b *Builder) Abs() *Builder {
	b.expr.Op = append(b.expr.Op, pathpb.Operation_Abs)
	return b
}

func (b *Builder) Inv() *Builder {
	b.expr.Op = append(b.expr.Op, pathpb.Operation_Inv)
	return b
}

func (b *Builder) Pow() *Builder {
	b.expr.Op = append(b.expr.Op, pathpb.Operation_Pow)
	return b
}

func (b *Builder) Sqrt() *Builder {
	b.expr.Op = append(b.expr.Op, pathpb.Operation_Sqrt)
	return b
}

func (b *Builder) Exp() *Builder {
	b.expr.Op = append(b.expr.Op, pathpb.Operation_Exp)
	return b
}

func (b *Builder) Ln() *Builder {
	b.expr.Op = append(b.expr.Op, pathpb.Operation_Ln)
	return b
}

func (b *Builder) Sin() *Builder {
	b.expr.Op = append(b.expr.Op, pathpb.Operation_Sin)
	return b
}

func (b *Builder) Cos() *Builder {
	b.expr.Op = append(b.expr.Op, pathpb.Operation_Cos)
	return b
}

func (b *Builder) Tan() *Builder {
	b.expr.Op = append(b.expr.Op, pathpb.Operation_Tan)
	return b
}

func (b *Builder) Asin() *Builder {
	b.expr.Op = append(b.expr.Op, pathpb.Operation_Asin)
	return b
}

func (b *Builder) Acos() *Builder {
	b.expr.Op = append(b.expr.Op, pathpb.Operation_Acos)
	return b
}

func (b *Builder) Atan2() *Builder {
	b.expr.Op = append(b.expr.Op, pathpb.Operation_Atan2)
	return b
}

func (b *Builder) PolyVec(coeffs int) *Builder {
	b.expr.Op = append(b.expr.Op, pathpb.Operation_PolyVec)
	b.expr.I = append(b.expr.I, int32(coeffs)<<1)
	return b
}

func (b *Builder) PushPolyVec(size int, literals []float64) *Builder {
	if size != len(literals) {
		panic("dimension mismatch")
	}
	b.expr.Op = append(b.expr.Op, pathpb.Operation_PolyVec)
	b.expr.I = append(b.expr.I, int32(size)<<1|1)
	b.push(literals)
	return b
}

func (b *Builder) PolyMat(rows, cols int) *Builder {
	b.expr.Op = append(b.expr.Op, pathpb.Operation_PolyMat)
	b.expr.I = append(b.expr.I, int32(rows), int32(cols)<<1)
	return b
}

func (b *Builder) PushPolyMat(rows, cols int, literals []float64) *Builder {
	if rows*cols != len(literals) {
		panic("dimension mismatch")
	}
	b.expr.Op = append(b.expr.Op, pathpb.Operation_PolyMat)
	b.expr.I = append(b.expr.I, int32(rows), int32(cols)<<1|1)
	b.push(literals)
	return b
}

func (b *Builder) AddVec(size int) *Builder {
	b.expr.Op = append(b.expr.Op, pathpb.Operation_AddVec)
	b.expr.I = append(b.expr.I, int32(size)<<1)
	return b
}

func (b *Builder) PushAddVec(size int, literals []float64) *Builder {
	if size != len(literals) {
		panic("dimension mismatch")
	}
	b.expr.Op = append(b.expr.Op, pathpb.Operation_AddVec)
	b.expr.I = append(b.expr.I, int32(size)<<1|1)
	b.push(literals)
	return b
}

func (b *Builder) SubVec(size int) *Builder {
	b.expr.Op = append(b.expr.Op, pathpb.Operation_SubVec)
	b.expr.I = append(b.expr.I, int32(size)<<1)
	return b
}

func (b *Builder) PushSubVec(size int, literals []float64) *Builder {
	if size != len(literals) {
		panic("dimension mismatch")
	}
	b.expr.Op = append(b.expr.Op, pathpb.Operation_SubVec)
	b.expr.I = append(b.expr.I, int32(size)<<1|1)
	b.push(literals)
	return b
}

func (b *Builder) MulVec(size int) *Builder {
	b.expr.Op = append(b.expr.Op, pathpb.Operation_MulVec)
	b.expr.I = append(b.expr.I, int32(size)<<1)
	return b
}

func (b *Builder) PushMulVec(size int, literals []float64) *Builder {
	if size != len(literals) {
		panic("dimension mismatch")
	}
	b.expr.Op = append(b.expr.Op, pathpb.Operation_MulVec)
	b.expr.I = append(b.expr.I, int32(size)<<1|1)
	b.push(literals)
	return b
}

func (b *Builder) MulAddVec(size int) *Builder {
	b.expr.Op = append(b.expr.Op, pathpb.Operation_MulAddVec)
	b.expr.I = append(b.expr.I, int32(size)<<2)
	return b
}

func (b *Builder) PushMulAddVec(size int, literals ...[]float64) *Builder {
	for _, l := range literals {
		if size != len(l) {
			panic("dimension mismatch")
		}
		b.push(l)
	}
	b.expr.Op = append(b.expr.Op, pathpb.Operation_MulAddVec)
	b.expr.I = append(b.expr.I, int32(size)<<2|int32(len(literals)))
	return b
}

func (b *Builder) ScaleVec(size int) *Builder {
	b.expr.Op = append(b.expr.Op, pathpb.Operation_ScaleVec)
	b.expr.I = append(b.expr.I, int32(size)<<1)
	return b
}

func (b *Builder) PushScaleVec(size int, literals []float64) *Builder {
	if size != len(literals) {
		panic("dimension mismatch")
	}
	b.expr.Op = append(b.expr.Op, pathpb.Operation_ScaleVec)
	b.expr.I = append(b.expr.I, int32(size)<<1|1)
	b.push(literals)
	return b
}

func (b *Builder) NegVec(size int) *Builder {
	b.expr.Op = append(b.expr.Op, pathpb.Operation_NegVec)
	b.expr.I = append(b.expr.I, int32(size)<<1)
	return b
}

func (b *Builder) PushNegVec(size int, literals []float64) *Builder {
	if size != len(literals) {
		panic("dimension mismatch")
	}
	b.expr.Op = append(b.expr.Op, pathpb.Operation_NegVec)
	b.expr.I = append(b.expr.I, int32(size)<<1|1)
	b.push(literals)
	return b
}

func (b *Builder) NormVec(size int) *Builder {
	b.expr.Op = append(b.expr.Op, pathpb.Operation_NormVec)
	b.expr.I = append(b.expr.I, int32(size)<<1)
	return b
}

func (b *Builder) PushNormVec(size int, literals []float64) *Builder {
	if size != len(literals) {
		panic("dimension mismatch")
	}
	b.expr.Op = append(b.expr.Op, pathpb.Operation_NormVec)
	b.expr.I = append(b.expr.I, int32(size)<<1|1)
	b.push(literals)
	return b
}

func (b *Builder) MulMat(arows, brows, bcols int) *Builder {
	b.expr.Op = append(b.expr.Op, pathpb.Operation_MulMat)
	b.expr.I = append(b.expr.I, int32(arows), int32(brows), int32(bcols)<<1)
	return b
}

func (b *Builder) PushMulMat(arows, brows, bcols int, literals []float64) *Builder {
	if brows*bcols != len(literals) {
		panic("dimension mismatch")
	}
	b.expr.Op = append(b.expr.Op, pathpb.Operation_MulMat)
	b.expr.I = append(b.expr.I, int32(arows), int32(brows), int32(bcols)<<1|1)
	return b
}

func (b *Builder) Transpose(rows, cols int) *Builder {
	b.expr.Op = append(b.expr.Op, pathpb.Operation_Transpose)
	b.expr.I = append(b.expr.I, int32(rows), int32(cols))
	return b
}

func (b *Builder) Lerp(size int) *Builder {
	b.expr.Op = append(b.expr.Op, pathpb.Operation_Lerp)
	b.expr.I = append(b.expr.I, int32(size)<<2)
	return b
}

func (b *Builder) PushLerp(size int, literals ...[]float64) *Builder {
	for _, l := range literals {
		if size != len(l) {
			panic("dimension mismatch")
		}
		b.push(l)
	}
	b.expr.Op = append(b.expr.Op, pathpb.Operation_Lerp)
	b.expr.I = append(b.expr.I, int32(size)<<2|int32(len(literals)))
	return b
}

func (b *Builder) Lut(rows, cols int) *Builder {
	b.expr.Op = append(b.expr.Op, pathpb.Operation_Lut)
	b.expr.I = append(b.expr.I, int32(rows), int32(cols)<<1)
	return b
}

func (b *Builder) PushLut(rows, cols int, literals []float64) *Builder {
	if rows*cols != len(literals) {
		panic("dimension mismatch")
	}
	b.expr.Op = append(b.expr.Op, pathpb.Operation_Lut)
	b.expr.I = append(b.expr.I, int32(rows), int32(cols)<<1|1)
	b.push(literals)
	return b
}

func Eval(expr *pathpb.PostfixExpression, stack []float64) ([]float64, error) {
	ints := expr.GetI()
	floats := expr.GetF()

	push := func(n int) error {
		if len(floats) < n {
			return ErrFloatLiteralsUnderflow
		}
		values := floats[:n]
		floats = floats[n:]
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
		v := ints[0]
		ints = ints[1:]
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

	for _, op := range expr.GetOp() {
		switch op {
		case pathpb.Operation_Undefined:
			return nil, ErrUndefinedOperation
		case pathpb.Operation_Push:
			if len(ints) < 1 {
				return nil, ErrIntLiteralsUnderflow
			}
			n := popi()

			if err := push(n); err != nil {
				return nil, err
			}
		case pathpb.Operation_Pop:
			if len(ints) < 1 {
				return nil, ErrIntLiteralsUnderflow
			}
			n := popi()

			if len(stack) < n {
				return nil, ErrStackUnderflow
			}
			stack = stack[:len(stack)-n]
		case pathpb.Operation_Dup:
			if len(ints) < 1 {
				return nil, ErrIntLiteralsUnderflow
			}
			n := popi()

			if len(stack) < n+1 {
				return nil, ErrStackUnderflow
			}
			stack = append(stack, stack[len(stack)-n-1])
		case pathpb.Operation_RotL:
			if len(ints) < 1 {
				return nil, ErrIntLiteralsUnderflow
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
		case pathpb.Operation_RotR:
			if len(ints) < 1 {
				return nil, ErrIntLiteralsUnderflow
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
		case pathpb.Operation_Rev:
			if len(ints) < 1 {
				return nil, ErrIntLiteralsUnderflow
			}
			n := popi()

			if len(stack) < n {
				return nil, ErrStackUnderflow
			}
			values := stack[len(stack)-n:]
			for i, j := 0, n-1; i < j; i, j = i+1, j-1 {
				values[i], values[j] = values[j], values[i]
			}
		case pathpb.Operation_Transpose:
			if len(ints) < 2 {
				return nil, ErrIntLiteralsUnderflow
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
		case pathpb.Operation_Add:
			if len(stack) < 2 {
				return nil, ErrStackUnderflow
			}
			rhs := pop()
			lhs := pop()
			result := lhs + rhs
			stack = append(stack, result)
		case pathpb.Operation_Sub:
			if len(stack) < 2 {
				return nil, ErrStackUnderflow
			}
			rhs := pop()
			lhs := pop()
			result := lhs - rhs
			stack = append(stack, result)
		case pathpb.Operation_Mul:
			if len(stack) < 2 {
				return nil, ErrStackUnderflow
			}
			rhs := pop()
			lhs := pop()
			result := lhs * rhs
			stack = append(stack, result)
		case pathpb.Operation_MulAdd:
			if len(stack) < 3 {
				return nil, ErrStackUnderflow
			}
			c := pop()
			b := pop()
			a := pop()
			result := a*b + c
			stack = append(stack, result)
		case pathpb.Operation_Div:
			if len(stack) < 2 {
				return nil, ErrStackUnderflow
			}
			rhs := pop()
			lhs := pop()
			result := lhs / rhs
			stack = append(stack, result)
		case pathpb.Operation_Mod:
			if len(stack) < 2 {
				return nil, ErrStackUnderflow
			}
			rhs := pop()
			lhs := pop()
			result := math.Remainder(lhs, rhs)
			stack = append(stack, result)
		case pathpb.Operation_Neg:
			if len(stack) < 1 {
				return nil, ErrStackUnderflow
			}
			operand := pop()
			result := -operand
			stack = append(stack, result)
		case pathpb.Operation_Abs:
			if len(stack) < 1 {
				return nil, ErrStackUnderflow
			}
			operand := pop()
			result := math.Abs(operand)
			stack = append(stack, result)
		case pathpb.Operation_Inv:
			if len(stack) < 1 {
				return nil, ErrStackUnderflow
			}
			operand := pop()
			result := 1 / operand
			stack = append(stack, result)
		case pathpb.Operation_Pow:
			if len(stack) < 2 {
				return nil, ErrStackUnderflow
			}
			exp := pop()
			base := pop()
			result := math.Pow(base, exp)
			stack = append(stack, result)
		case pathpb.Operation_Sqrt:
			if len(stack) < 1 {
				return nil, ErrStackUnderflow
			}
			operand := pop()
			result := math.Sqrt(operand)
			stack = append(stack, result)
		case pathpb.Operation_Exp:
			if len(stack) < 1 {
				return nil, ErrStackUnderflow
			}
			operand := pop()
			result := math.Exp(operand)
			stack = append(stack, result)
		case pathpb.Operation_Ln:
			if len(stack) < 1 {
				return nil, ErrStackUnderflow
			}
			operand := pop()
			result := math.Log(operand)
			stack = append(stack, result)
		case pathpb.Operation_Sin:
			if len(stack) < 1 {
				return nil, ErrStackUnderflow
			}
			operand := pop()
			result := math.Sin(operand)
			stack = append(stack, result)
		case pathpb.Operation_Cos:
			if len(stack) < 1 {
				return nil, ErrStackUnderflow
			}
			operand := pop()
			result := math.Cos(operand)
			stack = append(stack, result)
		case pathpb.Operation_Tan:
			if len(stack) < 1 {
				return nil, ErrStackUnderflow
			}
			operand := pop()
			result := math.Tan(operand)
			stack = append(stack, result)
		case pathpb.Operation_Asin:
			if len(stack) < 1 {
				return nil, ErrStackUnderflow
			}
			operand := pop()
			result := math.Asin(operand)
			stack = append(stack, result)
		case pathpb.Operation_Acos:
			if len(stack) < 1 {
				return nil, ErrStackUnderflow
			}
			operand := pop()
			result := math.Acos(operand)
			stack = append(stack, result)
		case pathpb.Operation_Atan2:
			if len(stack) < 2 {
				return nil, ErrStackUnderflow
			}
			x := pop()
			y := pop()
			result := math.Atan2(y, x)
			stack = append(stack, result)
		case pathpb.Operation_PolyVec:
			if len(ints) < 1 {
				return nil, ErrIntLiteralsUnderflow
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
		case pathpb.Operation_PolyMat:
			if len(ints) < 2 {
				return nil, ErrIntLiteralsUnderflow
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
		case pathpb.Operation_AddVec:
			if len(ints) < 1 {
				return nil, ErrIntLiteralsUnderflow
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
		case pathpb.Operation_SubVec:
			if len(ints) < 1 {
				return nil, ErrIntLiteralsUnderflow
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
		case pathpb.Operation_MulVec:
			if len(ints) < 1 {
				return nil, ErrIntLiteralsUnderflow
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
		case pathpb.Operation_MulAddVec:
			if len(ints) < 1 {
				return nil, ErrIntLiteralsUnderflow
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
		case pathpb.Operation_ScaleVec:
			if len(ints) < 1 {
				return nil, ErrIntLiteralsUnderflow
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
		case pathpb.Operation_NegVec:
			if len(ints) < 1 {
				return nil, ErrIntLiteralsUnderflow
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
		case pathpb.Operation_NormVec:
			if len(ints) < 1 {
				return nil, ErrIntLiteralsUnderflow
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
		case pathpb.Operation_MulMat:
			if len(ints) < 3 {
				return nil, ErrIntLiteralsUnderflow
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
		case pathpb.Operation_Lerp:
			if len(ints) < 1 {
				return nil, ErrIntLiteralsUnderflow
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
		case pathpb.Operation_Lut:
			if len(ints) < 2 {
				return nil, ErrIntLiteralsUnderflow
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
