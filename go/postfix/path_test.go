package postfix_test

import (
	"bytes"
	"encoding/binary"
	"testing"

	"github.com/ScottDuckworth/WickedWinchProtocol/go/postfix"
)

func TestPathHeaderSize(t *testing.T) {
	var header postfix.PathHeader
	var buf bytes.Buffer
	binary.Write(&buf, binary.LittleEndian, &header)
	if got, want := buf.Len(), 4; got != want {
		t.Errorf("PathHeader size: got %v, want %v", got, want)
	}
}

func TestPathSegmentHeaderSize(t *testing.T) {
	var header postfix.PathSegmentHeader
	var buf bytes.Buffer
	binary.Write(&buf, binary.LittleEndian, &header)
	if got, want := buf.Len(), 8; got != want {
		t.Errorf("PathHeader size: got %v, want %v", got, want)
	}
}

func TestPathReadWrite(t *testing.T) {
	for _, test := range []struct {
		name string
		path postfix.Path
	}{
		{
			name: "empty",
		},
		{
			name: "one",
			path: postfix.Path{
				Target: 123,
				Flags:  3,
				Segments: []postfix.PathSegment{
					{
						StartTime: 1000,
						Expr:      postfix.MakeBuilder().Push(1, 2, 3).Build(),
					},
				},
			},
		},
		{
			name: "two",
			path: postfix.Path{
				Target: 321,
				Flags:  3,
				Segments: []postfix.PathSegment{
					{
						StartTime: 1000,
						Expr:      postfix.MakeBuilder().Pop(1).Push(3, 2, 1).Build(),
					},
				},
			},
		},
	} {
		t.Run(test.name, func(t *testing.T) {
			var buf bytes.Buffer
			if err := test.path.Write(&buf); err != nil {
				t.Errorf("write error: %v", err)
			}

			t.Logf("write: %v", test.path)
			t.Logf("bytes: %v", buf.Bytes())

			var got postfix.Path
			if err := got.Read(&buf); err != nil {
				t.Errorf("read error: %v", err)
			}

			t.Logf("read:  %v", got)

			if !postfix.PathEqual(&got, &test.path) {
				t.Errorf("got %v, want %v", got, test.path)
			}
		})
	}
}
