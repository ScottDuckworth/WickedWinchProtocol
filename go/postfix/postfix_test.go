package postfix_test

import (
	"bytes"
	"math"
	"testing"

	"github.com/ScottDuckworth/WickedWinchProtocol/go/postfix"
)

func equalSlices(got, want []float64) bool {
	if len(got) != len(want) {
		return false
	}
	for i := 0; i < len(got); i++ {
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
				{
					Op: []postfix.Operation{postfix.Operation_Push},
					I:  []uint8{1},
					F:  []float32{10},
				},
			},
			want: &postfix.Expression{
				Op: []postfix.Operation{postfix.Operation_Push},
				I:  []uint8{1},
				F:  []float32{10},
			},
		},
		{
			name: "two",
			exprs: []*postfix.Expression{
				{
					Op: []postfix.Operation{postfix.Operation_Push},
					I:  []uint8{1},
					F:  []float32{10},
				},
				{
					Op: []postfix.Operation{postfix.Operation_Pop},
					I:  []uint8{2},
					F:  []float32{20},
				},
			},
			want: &postfix.Expression{
				Op: []postfix.Operation{postfix.Operation_Push, postfix.Operation_Pop},
				I:  []uint8{1, 2},
				F:  []float32{10, 20},
			},
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
				Op: []postfix.Operation{postfix.Operation_Undefined},
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
				Op: []postfix.Operation{postfix.Operation_Push},
				F:  []float32{1},
			},
			wantErr: postfix.ErrIntLiteralsUnderflow,
		},
		{
			name: "push float literals underflow",
			expr: &postfix.Expression{
				Op: []postfix.Operation{postfix.Operation_Push},
				I:  []uint8{1},
			},
			wantErr: postfix.ErrFloatLiteralsUnderflow,
		},
		{
			name:      "pop 0",
			expr:      postfix.MakeBuilder().Pop(0).Build(),
			stack:     []float64{0, 1, 2},
			wantStack: []float64{0, 1, 2},
		},
		{
			name:      "pop 1",
			expr:      postfix.MakeBuilder().Pop(1).Build(),
			stack:     []float64{0, 1, 2},
			wantStack: []float64{0, 1},
		},
		{
			name:      "pop 2",
			expr:      postfix.MakeBuilder().Pop(2).Build(),
			stack:     []float64{0, 1, 2},
			wantStack: []float64{0},
		},
		{
			name: "pop int literals underflow",
			expr: &postfix.Expression{
				Op: []postfix.Operation{postfix.Operation_Pop},
			},
			wantErr: postfix.ErrIntLiteralsUnderflow,
		},
		{
			name: "pop stack underflow",
			expr: &postfix.Expression{
				Op: []postfix.Operation{postfix.Operation_Pop},
				I:  []uint8{1},
			},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:      "dup 0",
			expr:      postfix.MakeBuilder().Dup(0).Build(),
			stack:     []float64{0, 1, 2},
			wantStack: []float64{0, 1, 2, 2},
		},
		{
			name:      "dup 1",
			expr:      postfix.MakeBuilder().Dup(1).Build(),
			stack:     []float64{0, 1, 2},
			wantStack: []float64{0, 1, 2, 1},
		},
		{
			name:      "dup 2",
			expr:      postfix.MakeBuilder().Dup(2).Build(),
			stack:     []float64{0, 1, 2},
			wantStack: []float64{0, 1, 2, 0},
		},
		{
			name: "dup int literals underflow",
			expr: &postfix.Expression{
				Op: []postfix.Operation{postfix.Operation_Dup},
			},
			wantErr: postfix.ErrIntLiteralsUnderflow,
		},
		{
			name: "dup stack underflow",
			expr: &postfix.Expression{
				Op: []postfix.Operation{postfix.Operation_Dup},
				I:  []uint8{0},
			},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:      "rotl 0",
			expr:      postfix.MakeBuilder().RotL(0).Build(),
			stack:     []float64{0, 1, 2},
			wantStack: []float64{0, 1, 2},
		},
		{
			name:      "rotl 1",
			expr:      postfix.MakeBuilder().RotL(1).Build(),
			stack:     []float64{0, 1, 2},
			wantStack: []float64{0, 1, 2},
		},
		{
			name:      "rotl 2",
			expr:      postfix.MakeBuilder().RotL(2).Build(),
			stack:     []float64{0, 1, 2},
			wantStack: []float64{0, 2, 1},
		},
		{
			name:      "rotl 3",
			expr:      postfix.MakeBuilder().RotL(3).Build(),
			stack:     []float64{0, 1, 2},
			wantStack: []float64{1, 2, 0},
		},
		{
			name: "rotl int literals underflow",
			expr: &postfix.Expression{
				Op: []postfix.Operation{postfix.Operation_RotL},
			},
			wantErr: postfix.ErrIntLiteralsUnderflow,
		},
		{
			name: "rotl stack underflow",
			expr: &postfix.Expression{
				Op: []postfix.Operation{postfix.Operation_RotL},
				I:  []uint8{2},
			},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:      "rotr 0",
			expr:      postfix.MakeBuilder().RotR(0).Build(),
			stack:     []float64{0, 1, 2},
			wantStack: []float64{0, 1, 2},
		},
		{
			name:      "rotr 1",
			expr:      postfix.MakeBuilder().RotR(1).Build(),
			stack:     []float64{0, 1, 2},
			wantStack: []float64{0, 1, 2},
		},
		{
			name:      "rotr 2",
			expr:      postfix.MakeBuilder().RotR(2).Build(),
			stack:     []float64{0, 1, 2},
			wantStack: []float64{0, 2, 1},
		},
		{
			name:      "rotr 3",
			expr:      postfix.MakeBuilder().RotR(3).Build(),
			stack:     []float64{0, 1, 2},
			wantStack: []float64{2, 0, 1},
		},
		{
			name: "rotr int literals underflow",
			expr: &postfix.Expression{
				Op: []postfix.Operation{postfix.Operation_RotR},
			},
			wantErr: postfix.ErrIntLiteralsUnderflow,
		},
		{
			name: "rotr stack underflow",
			expr: &postfix.Expression{
				Op: []postfix.Operation{postfix.Operation_RotR},
				I:  []uint8{2},
			},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:      "rev 0",
			expr:      postfix.MakeBuilder().Rev(0).Build(),
			stack:     []float64{},
			wantStack: []float64{},
		},
		{
			name:      "rev 1",
			expr:      postfix.MakeBuilder().Rev(1).Build(),
			stack:     []float64{1},
			wantStack: []float64{1},
		},
		{
			name:      "rev 2",
			expr:      postfix.MakeBuilder().Rev(2).Build(),
			stack:     []float64{1, 2},
			wantStack: []float64{2, 1},
		},
		{
			name:      "rev 3",
			expr:      postfix.MakeBuilder().Rev(3).Build(),
			stack:     []float64{1, 2, 3},
			wantStack: []float64{3, 2, 1},
		},
		{
			name:      "rev 4",
			expr:      postfix.MakeBuilder().Rev(4).Build(),
			stack:     []float64{1, 2, 3, 4},
			wantStack: []float64{4, 3, 2, 1},
		},
		{
			name:    "rev underflow",
			expr:    postfix.MakeBuilder().Rev(3).Build(),
			stack:   []float64{1, 2},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:      "transpose 2x3",
			expr:      postfix.MakeBuilder().Transpose(2, 3).Build(),
			stack:     []float64{1, 2, 3, 4, 5, 6},
			wantStack: []float64{1, 4, 2, 5, 3, 6},
		},
		{
			name:    "transpose underflow",
			expr:    postfix.MakeBuilder().Transpose(2, 3).Build(),
			stack:   []float64{1, 2, 3, 4, 5},
			wantErr: postfix.ErrStackUnderflow,
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
			name:      "mod",
			expr:      postfix.MakeBuilder().Mod().Build(),
			stack:     []float64{0, 10, 3},
			wantStack: []float64{0, 1},
		},
		{
			name:    "mod underflow",
			expr:    postfix.MakeBuilder().Mod().Build(),
			stack:   []float64{1},
			wantErr: postfix.ErrStackUnderflow,
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
			expr:      postfix.MakeBuilder().PushMulAddVec(3, []float64{7, 8, 9}).Build(),
			stack:     []float64{1, 2, 3, 4, 5, 6},
			wantStack: []float64{1*4 + 7, 2*5 + 8, 3*6 + 9},
		},
		{
			name:      "push 2 multiply add vector 3 implicit",
			expr:      postfix.MakeBuilder().PushMulAddVec(3, []float64{4, 5, 6}, []float64{7, 8, 9}).Build(),
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
			name:      "norm vector 2",
			expr:      postfix.MakeBuilder().NormVec(2).Build(),
			stack:     []float64{3, 4},
			wantStack: []float64{5},
		},
		{
			name:    "norm vector underflow",
			expr:    postfix.MakeBuilder().NormVec(2).Build(),
			stack:   []float64{3},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:  "multiply matrix 2 3 4",
			expr:  postfix.MakeBuilder().MulMat(2, 3, 4).Build(),
			stack: []float64{1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12},
			wantStack: []float64{
				1*1 + 2*5 + 3*9, 1*2 + 2*6 + 3*10, 1*3 + 2*7 + 3*11, 1*4 + 2*8 + 3*12,
				4*1 + 5*5 + 6*9, 4*2 + 5*6 + 6*10, 4*3 + 5*7 + 6*11, 4*4 + 5*8 + 6*12,
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
			expr:  postfix.MakeBuilder().PushMulMat(2, 3, 4, []float64{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}).Build(),
			stack: []float64{1, 2, 3, 4, 5, 6},
			wantStack: []float64{
				1*1 + 2*5 + 3*9, 1*2 + 2*6 + 3*10, 1*3 + 2*7 + 3*11, 1*4 + 2*8 + 3*12,
				4*1 + 5*5 + 6*9, 4*2 + 5*6 + 6*10, 4*3 + 5*7 + 6*11, 4*4 + 5*8 + 6*12,
			},
		},
		{
			name:      "polynomial vector 1",
			expr:      postfix.MakeBuilder().PolyVec(1).Build(),
			stack:     []float64{2, 1},
			wantStack: []float64{1},
		},
		{
			name:      "polynomial vector 2",
			expr:      postfix.MakeBuilder().PolyVec(2).Build(),
			stack:     []float64{2, 1, 3},
			wantStack: []float64{7},
		},
		{
			name:      "polynomial vector 3",
			expr:      postfix.MakeBuilder().PolyVec(3).Build(),
			stack:     []float64{2, 1, 3, 5},
			wantStack: []float64{27},
		},
		{
			name:    "polynomial vector underflow",
			expr:    postfix.MakeBuilder().PolyVec(1).Build(),
			stack:   []float64{2},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:      "polynomial matrix 3x2",
			expr:      postfix.MakeBuilder().PolyMat(3, 2).Build(),
			stack:     []float64{2, 1, 2, 3, 4, 5, 6},
			wantStack: []float64{1 + 2*3 + 2*2*5, 2 + 2*4 + 2*2*6},
		},
		{
			name:    "polynomial matrix underflow",
			expr:    postfix.MakeBuilder().PolyMat(3, 2).Build(),
			stack:   []float64{2, 1, 2, 3, 4, 5},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:      "lerp 1d 0",
			expr:      postfix.MakeBuilder().Lerp(1).Build(),
			stack:     []float64{0, 1, 2},
			wantStack: []float64{1},
		},
		{
			name:      "lerp 1d 1",
			expr:      postfix.MakeBuilder().Lerp(1).Build(),
			stack:     []float64{1, 1, 2},
			wantStack: []float64{2},
		},
		{
			name:      "lerp 1d 0.5",
			expr:      postfix.MakeBuilder().Lerp(1).Build(),
			stack:     []float64{0.5, 1, 2},
			wantStack: []float64{1.5},
		},
		{
			name:      "lerp 3d 0.5",
			expr:      postfix.MakeBuilder().Lerp(3).Build(),
			stack:     []float64{0.5, 1, 2, 3, 4, 5, 6},
			wantStack: []float64{2.5, 3.5, 4.5},
		},
		{
			name:    "lerp underflow",
			expr:    postfix.MakeBuilder().Lerp(1).Build(),
			stack:   []float64{0.5, 1},
			wantErr: postfix.ErrStackUnderflow,
		},
		{
			name:      "push 1 lerp 3d 0.5",
			expr:      postfix.MakeBuilder().PushLerp(3, []float64{4, 5, 6}).Build(),
			stack:     []float64{0.5, 1, 2, 3},
			wantStack: []float64{2.5, 3.5, 4.5},
		},
		{
			name:      "push 2 lerp 3d 0.5",
			expr:      postfix.MakeBuilder().PushLerp(3, []float64{1, 2, 3}, []float64{4, 5, 6}).Build(),
			stack:     []float64{0.5},
			wantStack: []float64{2.5, 3.5, 4.5},
		},
		{
			name:      "lut before first",
			expr:      postfix.MakeBuilder().Lut(2, 3).Build(),
			stack:     []float64{0, 1, 10, 100, 2, 20, 200},
			wantStack: []float64{10, 100},
		},
		{
			name:      "lut after last",
			expr:      postfix.MakeBuilder().Lut(2, 3).Build(),
			stack:     []float64{3, 1, 10, 100, 2, 20, 200},
			wantStack: []float64{20, 200},
		},
		{
			name:      "lut lerp 1.5",
			expr:      postfix.MakeBuilder().Lut(3, 3).Build(),
			stack:     []float64{1.5, 1, 10, 100, 2, 20, 200, 4, 40, 400},
			wantStack: []float64{15, 150},
		},
		{
			name:      "lut lerp 3",
			expr:      postfix.MakeBuilder().Lut(3, 3).Build(),
			stack:     []float64{3, 1, 10, 100, 2, 20, 200, 4, 40, 400},
			wantStack: []float64{30, 300},
		},
		{
			name:    "lut underflow",
			expr:    postfix.MakeBuilder().Lut(1, 2).Build(),
			stack:   []float64{1, 0},
			wantErr: postfix.ErrStackUnderflow,
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
