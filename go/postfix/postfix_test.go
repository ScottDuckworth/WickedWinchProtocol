package postfix_test

import (
	"bytes"
	"encoding/binary"
	"errors"
	"math"
	"testing"

	"github.com/ScottDuckworth/WickedWinchProtocol/go/postfix"
)

func TestPostfixHeaderSize(t *testing.T) {
	var header postfix.PostfixHeader
	var buf bytes.Buffer
	binary.Write(&buf, binary.LittleEndian, &header)
	if got, want := buf.Len(), 2; got != want {
		t.Errorf("PostfixHeader size: got %v, want %v", got, want)
	}
}

func equalSlices(got, want []float64) bool {
	if len(got) != len(want) {
		return false
	}
	for i := range got {
		if math.Abs(got[i]-want[i]) > 0.001 {
			return false
		}
	}
	return true
}

func TestJoin(t *testing.T) {
	for _, test := range []struct {
		name  string
		exprs []*postfix.Expression
		want  *postfix.Expression
	}{
		{
			name:  "empty",
			exprs: nil,
			want:  &postfix.Expression{},
		},
		{
			name: "one",
			exprs: []*postfix.Expression{
				postfix.MakeBuilder().Push(10).Build(),
			},
			want: postfix.MakeBuilder().Push(10).Build(),
		},
		{
			name: "two",
			exprs: []*postfix.Expression{
				postfix.MakeBuilder().Push(10).Build(),
				postfix.MakeBuilder().Drop().Build(),
			},
			want: postfix.MakeBuilder().Push(10).Drop().Build(),
		},
	} {
		t.Run(test.name, func(t *testing.T) {
			got := postfix.Join(test.exprs...)
			if !postfix.Equal(got, test.want) {
				t.Errorf("got %v, want %v", got, test.want)
			}
		})
	}
}

func TestEvalPostfixExpression(t *testing.T) {
	for _, test := range []struct {
		name      string
		expr      *postfix.Expression
		stack     []float64
		wantStack []float64
		wantErr   error
	}{
		{
			name: "undefined",
			expr: &postfix.Expression{
				Prog: []uint8{uint8(postfix.Operation_Undefined)},
			},
			wantErr: postfix.ErrUndefinedOperation,
		},
		{
			name:      "push 0",
			expr:      postfix.MakeBuilder().Push().Build(),
			stack:     []float64{0},
			wantStack: []float64{0},
		},
		{
			name:      "push 1",
			expr:      postfix.MakeBuilder().Push(1).Build(),
			stack:     []float64{0},
			wantStack: []float64{0, 1},
		},
		{
			name:      "push 2",
			expr:      postfix.MakeBuilder().Push(1, 2).Build(),
			stack:     []float64{0},
			wantStack: []float64{0, 1, 2},
		},
		{
			name: "push int literals underflow",
			expr: &postfix.Expression{
				Prog: []uint8{uint8(postfix.Operation_Push)},
			},
			wantErr: postfix.ErrIllegalOperation,
		},
		{
			name: "push float literals underflow",
			expr: &postfix.Expression{
				Prog: []uint8{uint8(postfix.Operation_Push), 1},
			},
			wantErr: postfix.ErrIllegalOperation,
		},
		{
			name:      "drop",
			expr:      postfix.MakeBuilder().Drop().Build(),
			stack:     []float64{0, 1, 2},
			wantStack: []float64{0, 1},
		},
		{
			name:    "drop stack underflow",
			expr:    postfix.MakeBuilder().Drop().Build(),
			stack:   []float64{},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:      "dup",
			expr:      postfix.MakeBuilder().Dup().Build(),
			stack:     []float64{0, 1, 2},
			wantStack: []float64{0, 1, 2, 2},
		},
		{
			name:    "dup stack underflow",
			expr:    postfix.MakeBuilder().Dup().Build(),
			stack:   []float64{},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:      "swap",
			expr:      postfix.MakeBuilder().Swap().Build(),
			stack:     []float64{0, 1, 2},
			wantStack: []float64{0, 2, 1},
		},
		{
			name:    "swap stack underflow",
			expr:    postfix.MakeBuilder().Swap().Build(),
			stack:   []float64{0},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:      "over",
			expr:      postfix.MakeBuilder().Over().Build(),
			stack:     []float64{0, 1},
			wantStack: []float64{0, 1, 0},
		},
		{
			name:    "over stack underflow",
			expr:    postfix.MakeBuilder().Over().Build(),
			stack:   []float64{0},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:      "tuck",
			expr:      postfix.MakeBuilder().Tuck().Build(),
			stack:     []float64{0, 1},
			wantStack: []float64{1, 0, 1},
		},
		{
			name:    "tuck stack underflow",
			expr:    postfix.MakeBuilder().Tuck().Build(),
			stack:   []float64{0},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:      "nip",
			expr:      postfix.MakeBuilder().Nip().Build(),
			stack:     []float64{0, 1, 2},
			wantStack: []float64{0, 2},
		},
		{
			name:    "nip stack underflow",
			expr:    postfix.MakeBuilder().Nip().Build(),
			stack:   []float64{0},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:      "rot",
			expr:      postfix.MakeBuilder().Rot().Build(),
			stack:     []float64{0, 1, 2},
			wantStack: []float64{1, 2, 0},
		},
		{
			name:    "rot stack underflow",
			expr:    postfix.MakeBuilder().Rot().Build(),
			stack:   []float64{0, 1},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:      "pick copy scalar",
			expr:      postfix.MakeBuilder().Pick(1, 1, false).Build(),
			stack:     []float64{10, 20, 30},
			wantStack: []float64{10, 20, 30, 20},
		},
		{
			name:      "pick copy vector",
			expr:      postfix.MakeBuilder().Pick(1, 2, false).Build(),
			stack:     []float64{10, 20, 30, 40},
			wantStack: []float64{10, 20, 30, 40, 20, 30},
		},
		{
			name:      "pick move scalar",
			expr:      postfix.MakeBuilder().Pick(2, 1, true).Build(),
			stack:     []float64{10, 20, 30, 40},
			wantStack: []float64{10, 30, 40, 20},
		},
		{
			name:      "pick move vector",
			expr:      postfix.MakeBuilder().Pick(1, 2, true).Build(),
			stack:     []float64{10, 20, 30, 40},
			wantStack: []float64{10, 40, 20, 30},
		},
		{
			name:    "pick stack underflow",
			expr:    postfix.MakeBuilder().Pick(2, 2, false).Build(),
			stack:   []float64{10, 20, 30},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name: "pick int literals underflow",
			expr: &postfix.Expression{
				Prog: []uint8{uint8(postfix.Operation_Pick)},
			},
			wantErr: postfix.ErrIllegalOperation,
		},
		{
			name:      "add",
			expr:      postfix.MakeBuilder().Add().Build(),
			stack:     []float64{0, 1, 2},
			wantStack: []float64{0, 3},
		},
		{
			name:    "add underflow",
			expr:    postfix.MakeBuilder().Add().Build(),
			stack:   []float64{1},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:      "subtract",
			expr:      postfix.MakeBuilder().Sub().Build(),
			stack:     []float64{0, 1, 2},
			wantStack: []float64{0, -1},
		},
		{
			name:    "subtract underflow",
			expr:    postfix.MakeBuilder().Sub().Build(),
			stack:   []float64{1},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:      "multiply",
			expr:      postfix.MakeBuilder().Mul().Build(),
			stack:     []float64{0, 10, 2},
			wantStack: []float64{0, 20},
		},
		{
			name:    "multiply underflow",
			expr:    postfix.MakeBuilder().Mul().Build(),
			stack:   []float64{1},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:      "multiply add",
			expr:      postfix.MakeBuilder().MulAdd().Build(),
			stack:     []float64{10, 2, 1},
			wantStack: []float64{21},
		},
		{
			name:    "multiply add underflow",
			expr:    postfix.MakeBuilder().MulAdd().Build(),
			stack:   []float64{1, 2},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:      "divide",
			expr:      postfix.MakeBuilder().Div().Build(),
			stack:     []float64{0, 10, 2},
			wantStack: []float64{0, 5},
		},
		{
			name:    "divide underflow",
			expr:    postfix.MakeBuilder().Div().Build(),
			stack:   []float64{1},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:      "ipow positive",
			expr:      postfix.MakeBuilder().Ipow(3).Build(),
			stack:     []float64{0, 2},
			wantStack: []float64{0, 8},
		},
		{
			name:      "ipow zero",
			expr:      postfix.MakeBuilder().Ipow(0).Build(),
			stack:     []float64{0, 5},
			wantStack: []float64{0, 1},
		},
		{
			name:      "ipow negative",
			expr:      postfix.MakeBuilder().Ipow(-2).Build(),
			stack:     []float64{0, 2},
			wantStack: []float64{0, 0.25},
		},
		{
			name:    "ipow underflow",
			expr:    postfix.MakeBuilder().Ipow(2).Build(),
			stack:   []float64{},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name: "ipow truncated",
			expr: &postfix.Expression{
				Prog: []uint8{uint8(postfix.Operation_Ipow)},
			},
			stack:   []float64{2},
			wantErr: postfix.ErrIllegalOperation,
		},
		{
			name:      "negate",
			expr:      postfix.MakeBuilder().Neg().Build(),
			stack:     []float64{0, 1},
			wantStack: []float64{0, -1},
		},
		{
			name:    "negate underflow",
			expr:    postfix.MakeBuilder().Neg().Build(),
			stack:   []float64{},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:      "abs positive",
			expr:      postfix.MakeBuilder().Abs().Build(),
			stack:     []float64{0, 1},
			wantStack: []float64{0, 1},
		},
		{
			name:      "abs negative",
			expr:      postfix.MakeBuilder().Abs().Build(),
			stack:     []float64{0, -1},
			wantStack: []float64{0, 1},
		},
		{
			name:    "negate underflow",
			expr:    postfix.MakeBuilder().Abs().Build(),
			stack:   []float64{},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:      "inverse",
			expr:      postfix.MakeBuilder().Inv().Build(),
			stack:     []float64{0, 2},
			wantStack: []float64{0, 0.5},
		},
		{
			name:    "inverse underflow",
			expr:    postfix.MakeBuilder().Inv().Build(),
			stack:   []float64{},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:      "power",
			expr:      postfix.MakeBuilder().Pow().Build(),
			stack:     []float64{0, 10, 2},
			wantStack: []float64{0, 100},
		},
		{
			name:    "power underflow",
			expr:    postfix.MakeBuilder().Pow().Build(),
			stack:   []float64{1},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:      "sqrt",
			expr:      postfix.MakeBuilder().Sqrt().Build(),
			stack:     []float64{0, 4},
			wantStack: []float64{0, 2},
		},
		{
			name:    "sqrt underflow",
			expr:    postfix.MakeBuilder().Sqrt().Build(),
			stack:   []float64{},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:      "exp",
			expr:      postfix.MakeBuilder().Exp().Build(),
			stack:     []float64{0, math.Log(2)},
			wantStack: []float64{0, 2},
		},
		{
			name:    "exp underflow",
			expr:    postfix.MakeBuilder().Exp().Build(),
			stack:   []float64{},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:      "log",
			expr:      postfix.MakeBuilder().Ln().Build(),
			stack:     []float64{0, math.Exp(2)},
			wantStack: []float64{0, 2},
		},
		{
			name:    "log underflow",
			expr:    postfix.MakeBuilder().Ln().Build(),
			stack:   []float64{},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:      "sin",
			expr:      postfix.MakeBuilder().Sin().Build(),
			stack:     []float64{0, math.Pi / 2},
			wantStack: []float64{0, 1},
		},
		{
			name:    "sin underflow",
			expr:    postfix.MakeBuilder().Sin().Build(),
			stack:   []float64{},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:      "cos",
			expr:      postfix.MakeBuilder().Cos().Build(),
			stack:     []float64{0, math.Pi},
			wantStack: []float64{0, -1},
		},
		{
			name:    "cos underflow",
			expr:    postfix.MakeBuilder().Cos().Build(),
			stack:   []float64{},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:      "tan",
			expr:      postfix.MakeBuilder().Tan().Build(),
			stack:     []float64{0, math.Pi / 4},
			wantStack: []float64{0, 1},
		},
		{
			name:    "tan underflow",
			expr:    postfix.MakeBuilder().Tan().Build(),
			stack:   []float64{},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:      "asin",
			expr:      postfix.MakeBuilder().Asin().Build(),
			stack:     []float64{0, 0.5},
			wantStack: []float64{0, math.Pi / 6},
		},
		{
			name:    "asin underflow",
			expr:    postfix.MakeBuilder().Asin().Build(),
			stack:   []float64{},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:      "acos",
			expr:      postfix.MakeBuilder().Acos().Build(),
			stack:     []float64{0, 0.5},
			wantStack: []float64{0, math.Pi * 2 / 6},
		},
		{
			name:    "acos underflow",
			expr:    postfix.MakeBuilder().Acos().Build(),
			stack:   []float64{},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:      "atan2",
			expr:      postfix.MakeBuilder().Atan2().Build(),
			stack:     []float64{1, 0},
			wantStack: []float64{math.Pi / 2},
		},
		{
			name:    "atan2 underflow",
			expr:    postfix.MakeBuilder().Atan2().Build(),
			stack:   []float64{1},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:      "add vector 3",
			expr:      postfix.MakeBuilder().AddVec(3).Build(),
			stack:     []float64{1, 2, 3, 4, 5, 6},
			wantStack: []float64{1 + 4, 2 + 5, 3 + 6},
		},
		{
			name:    "add vector underflow",
			expr:    postfix.MakeBuilder().AddVec(3).Build(),
			stack:   []float64{1, 2, 3, 4, 5},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:      "subtract vector 3",
			expr:      postfix.MakeBuilder().SubVec(3).Build(),
			stack:     []float64{1, 2, 3, 4, 5, 6},
			wantStack: []float64{1 - 4, 2 - 5, 3 - 6},
		},
		{
			name:    "subtract vector underflow",
			expr:    postfix.MakeBuilder().SubVec(3).Build(),
			stack:   []float64{1, 2, 3, 4, 5},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:      "multiply vector 3",
			expr:      postfix.MakeBuilder().MulVec(3).Build(),
			stack:     []float64{1, 2, 3, 4, 5, 6},
			wantStack: []float64{1 * 4, 2 * 5, 3 * 6},
		},
		{
			name:    "multiply vector underflow",
			expr:    postfix.MakeBuilder().MulVec(3).Build(),
			stack:   []float64{1, 2, 3, 4, 5},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:      "multiply add vector 3",
			expr:      postfix.MakeBuilder().MulAddVec(3).Build(),
			stack:     []float64{1, 2, 3, 4, 5, 6, 7, 8, 9},
			wantStack: []float64{1*4 + 7, 2*5 + 8, 3*6 + 9},
		},
		{
			name:    "multiply add vector underflow",
			expr:    postfix.MakeBuilder().MulAddVec(3).Build(),
			stack:   []float64{1, 2, 3, 4, 5, 6, 7, 8},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:      "push 1 multiply add vector 3 implicit",
			expr:      postfix.MakeBuilder().Push(7, 8, 9).MulAddVec(3).Build(),
			stack:     []float64{1, 2, 3, 4, 5, 6},
			wantStack: []float64{1*4 + 7, 2*5 + 8, 3*6 + 9},
		},
		{
			name:      "push 2 multiply add vector 3 implicit",
			expr:      postfix.MakeBuilder().Push(4, 5, 6, 7, 8, 9).MulAddVec(3).Build(),
			stack:     []float64{1, 2, 3},
			wantStack: []float64{1*4 + 7, 2*5 + 8, 3*6 + 9},
		},
		{
			name:      "scale vector 3",
			expr:      postfix.MakeBuilder().ScaleVec(3).Build(),
			stack:     []float64{2, 1, 2, 3},
			wantStack: []float64{2, 4, 6},
		},
		{
			name:    "scale vector underflow",
			expr:    postfix.MakeBuilder().ScaleVec(3).Build(),
			stack:   []float64{2, 1, 2},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:      "divide vector 3",
			expr:      postfix.MakeBuilder().DivVec(3).Build(),
			stack:     []float64{6, 8, -10, 2, 4, 5},
			wantStack: []float64{3, 2, -2},
		},
		{
			name:    "divide vector underflow",
			expr:      postfix.MakeBuilder().DivVec(3).Build(),
			stack:   []float64{6, 8, -10, 2, 4},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:      "negate vector 3",
			expr:      postfix.MakeBuilder().NegVec(3).Build(),
			stack:     []float64{1, 2, -3},
			wantStack: []float64{-1, -2, 3},
		},
		{
			name:    "negate vector underflow",
			expr:    postfix.MakeBuilder().NegVec(3).Build(),
			stack:   []float64{1, 2},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:      "invert vector 3",
			expr:      postfix.MakeBuilder().InvVec(3).Build(),
			stack:     []float64{2, 4, -0.5},
			wantStack: []float64{0.5, 0.25, -2},
		},
		{
			name:    "invert vector underflow",
			expr:      postfix.MakeBuilder().InvVec(3).Build(),
			stack:   []float64{2, 4},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:      "norm squared vector 2",
			expr:      postfix.MakeBuilder().NormSqVec(2).Build(),
			stack:     []float64{3, 4},
			wantStack: []float64{25},
		},
		{
			name:      "norm squared and sqrt vector 2",
			expr:      postfix.MakeBuilder().NormSqVec(2).Sqrt().Build(),
			stack:     []float64{3, 4},
			wantStack: []float64{5},
		},
		{
			name:    "norm squared vector underflow",
			expr:    postfix.MakeBuilder().NormSqVec(2).Build(),
			stack:   []float64{3},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:      "sum vector 3",
			expr:      postfix.MakeBuilder().SumVec(3).Build(),
			stack:     []float64{1, 2.5, -0.5},
			wantStack: []float64{3.0},
		},
		{
			name:    "sum vector underflow",
			expr:    postfix.MakeBuilder().SumVec(3).Build(),
			stack:   []float64{1, 2},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name: "multiply matrix 2 3 4",
			expr: postfix.MakeBuilder().MulMat(2, 3, 4).Build(),
			stack: []float64{1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12},
			wantStack: []float64{
				1*1 + 2*5 + 3*9, 1*2 + 2*6 + 3*10, 1*3 + 2*7 + 3*11, 1*4 + 2*8 + 3*12,
				4*1 + 5*5 + 6*9, 4*2 + 5*6 + 6*10, 4*3 + 5*7 + 6*11, 4*4 + 5*8 + 6*12,
			},
		},
		{
			name: "multiply matrix square 1 3 3",
			expr: postfix.MakeBuilder().MulMat(1, 3, 3).Build(),
			stack: []float64{1, 2, 3, 1, 0, 0, 0, 1, 0, 0, 0, 1},
			wantStack: []float64{1, 2, 3},
		},
		{
			name: "multiply matrix projection down 1 3 2",
			expr: postfix.MakeBuilder().MulMat(1, 3, 2).Build(),
			stack: []float64{1, 2, 3, 1, 2, 3, 4, 5, 6},
			wantStack: []float64{1*1 + 2*3 + 3*5, 1*2 + 2*4 + 3*6},
		},
		{
			name: "multiply matrix dot product 1 3 1",
			expr: postfix.MakeBuilder().MulMat(1, 3, 1).Build(),
			stack: []float64{1, 2, 3, 4, 5, 6},
			wantStack: []float64{1*4 + 2*5 + 3*6},
		},
		{
			name: "multiply matrix immediate 1 3 3",
			expr: postfix.MakeBuilder().MulMatImmediate(1, 3, 3, 1, 2, 3, 4, 5, 6, 7, 8, 9).Build(),
			stack: []float64{2, 3, 4},
			wantStack: []float64{
				2*1 + 3*4 + 4*7,
				2*2 + 3*5 + 4*8,
				2*3 + 3*6 + 4*9,
			},
		},
		{
			name:    "multiply matrix underflow",
			expr:    postfix.MakeBuilder().MulMat(2, 3, 4).Build(),
			stack:   []float64{1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:  "push multiply matrix 2 3 4",
			expr:  postfix.MakeBuilder().Push(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12).MulMat(2, 3, 4).Build(),
			stack: []float64{1, 2, 3, 4, 5, 6},
			wantStack: []float64{
				1*1 + 2*5 + 3*9, 1*2 + 2*6 + 3*10, 1*3 + 2*7 + 3*11, 1*4 + 2*8 + 3*12,
				4*1 + 5*5 + 6*9, 4*2 + 5*6 + 6*10, 4*3 + 5*7 + 6*11, 4*4 + 5*8 + 6*12,
			},
		},
		{
			name:      "polynomial vector 1",
			expr:      postfix.MakeBuilder().Polynomial(1, 1, 1).Build(),
			stack:     []float64{2},
			wantStack: []float64{1},
		},
		{
			name:      "polynomial vector 2",
			expr:      postfix.MakeBuilder().Polynomial(2, 1, 1, 3).Build(),
			stack:     []float64{2},
			wantStack: []float64{7},
		},
		{
			name:      "polynomial vector 3",
			expr:      postfix.MakeBuilder().Polynomial(3, 1, 1, 3, 5).Build(),
			stack:     []float64{2},
			wantStack: []float64{27},
		},
		{
			name:    "polynomial vector underflow",
			expr:    postfix.MakeBuilder().Polynomial(1, 1, 1).Build(),
			stack:   []float64{},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:      "polynomial matrix 3x2",
			expr:      postfix.MakeBuilder().Polynomial(3, 2, 1, 2, 3, 4, 5, 6).Build(),
			stack:     []float64{2},
			wantStack: []float64{1 + 2*3 + 2*2*5, 2 + 2*4 + 2*2*6},
		},
		{
			name:    "polynomial matrix underflow",
			expr:    postfix.MakeBuilder().Polynomial(3, 2, 1, 2, 3, 4, 5, 6).Build(),
			stack:   []float64{},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:      "lerp 1d 0",
			expr:      postfix.MakeBuilder().LerpVec(1).Build(),
			stack:     []float64{0, 1, 2},
			wantStack: []float64{1},
		},
		{
			name:      "lerp 1d 1",
			expr:      postfix.MakeBuilder().LerpVec(1).Build(),
			stack:     []float64{1, 1, 2},
			wantStack: []float64{2},
		},
		{
			name:      "lerp 1d 0.5",
			expr:      postfix.MakeBuilder().LerpVec(1).Build(),
			stack:     []float64{0.5, 1, 2},
			wantStack: []float64{1.5},
		},
		{
			name:      "lerp 3d 0.5",
			expr:      postfix.MakeBuilder().LerpVec(3).Build(),
			stack:     []float64{0.5, 1, 2, 3, 4, 5, 6},
			wantStack: []float64{2.5, 3.5, 4.5},
		},
		{
			name:    "lerp underflow",
			expr:    postfix.MakeBuilder().LerpVec(1).Build(),
			stack:   []float64{0.5, 1},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:      "push 1 lerp 3d 0.5",
			expr:      postfix.MakeBuilder().Push(4, 5, 6).LerpVec(3).Build(),
			stack:     []float64{0.5, 1, 2, 3},
			wantStack: []float64{2.5, 3.5, 4.5},
		},
		{
			name:      "push 2 lerp 3d 0.5",
			expr:      postfix.MakeBuilder().Push(1, 2, 3, 4, 5, 6).LerpVec(3).Build(),
			stack:     []float64{0.5},
			wantStack: []float64{2.5, 3.5, 4.5},
		},
	} {
		t.Run(test.name, func(t *testing.T) {
			gotStack, gotErr := postfix.Eval(test.expr, test.stack)
			if !equalSlices(gotStack, test.wantStack) {
				t.Errorf("got %v, want %v", gotStack, test.wantStack)
			}
			if gotErr != test.wantErr {
				t.Errorf("got %v, want %v", gotErr, test.wantErr)
			}

			var buf bytes.Buffer
			if err := test.expr.Write(&buf); err != nil {
				t.Errorf("write error: %v", err)
			}
			var expr postfix.Expression
			if err := expr.Read(&buf); err != nil {
				t.Errorf("read error: %v", err)
			}
			if got, want := &expr, test.expr; !postfix.Equal(got, want) {
				t.Errorf("after serialization: got %v, want %v", got, want)
			}
		})
	}
}
func TestValidateAndEvaluate(t *testing.T) {
	// Push 1, 2 to an initial stack of [42]. Initial length = 1.
	// Expected final stack is [42, 1, 2], final length = 3, minCapacity = 3.
	expr := postfix.MakeBuilder().Push(1, 2).Build()

	minCapacity, finalStackLen, _, err := postfix.Validate(expr, 1)
	if err != nil {
		t.Fatalf("unexpected validation error: %v", err)
	}
	if minCapacity != 3 {
		t.Errorf("got minCapacity %d, want 3", minCapacity)
	}
	if finalStackLen != 3 {
		t.Errorf("got finalStackLen %d, want 3", finalStackLen)
	}

	minCapExpr, finalLenExpr, _, err := expr.Validate(1)
	if err != nil {
		t.Fatalf("unexpected validation error: %v", err)
	}
	if minCapExpr != 3 || finalLenExpr != 3 {
		t.Errorf("got minCapacity %d, finalStackLen %d; want 3, 3", minCapExpr, finalLenExpr)
	}

	// Caller checking expected final stack length
	checkExpectedFinalLen := func(expected int) error {
		_, finalLen, _, err := expr.Validate(1)
		if err != nil {
			return err
		}
		if finalLen != expected {
			return postfix.ErrUnexpectedStackSize
		}
		return nil
	}

	// Validation with mismatched expected final stack length
	if err := checkExpectedFinalLen(2); !errors.Is(err, postfix.ErrUnexpectedStackSize) {
		t.Errorf("got %v, want %v", err, postfix.ErrUnexpectedStackSize)
	}
	if err := checkExpectedFinalLen(4); !errors.Is(err, postfix.ErrUnexpectedStackSize) {
		t.Errorf("got %v, want %v", err, postfix.ErrUnexpectedStackSize)
	}
	if err := checkExpectedFinalLen(3); err != nil {
		t.Errorf("unexpected error: %v", err)
	}

	// Direct evaluation
	got := expr.Evaluate([]float64{42})
	want := []float64{42, 1, 2}
	if !equalSlices(got, want) {
		t.Errorf("got %v, want %v", got, want)
	}

	// Package-level Evaluate
	gotPkg := postfix.Evaluate(expr, []float64{42})
	if !equalSlices(gotPkg, want) {
		t.Errorf("got %v, want %v", gotPkg, want)
	}
}

func TestValidateStackOverflow(t *testing.T) {
	expr := postfix.MakeBuilder().Dup().Build()
	_, _, _, err := expr.Validate(math.MaxUint16)
	if !errors.Is(err, postfix.ErrStackOverflow) {
		t.Errorf("got %v, want %v", err, postfix.ErrStackOverflow)
	}
}

func TestOperationString(t *testing.T) {
	if got := postfix.Operation_Ipow.String(); got != "Ipow" {
		t.Errorf("got %q, want %q", got, "Ipow")
	}
}
