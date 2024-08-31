package postfix

import (
	"bytes"
	"cmp"
	"encoding/binary"
	"fmt"
	"io"
	"strings"
)

type PathFlag uint8

const (
	Path_Overflow PathFlag = 1 << 0
)

type PathSegment struct {
	StartTime uint32
	Expr      *Expression
}

type Path struct {
	Flags    uint8
	Segments []*PathSegment
}

type PathHeader struct {
	SegmentSize uint16
	Flags       uint8
	Padding     uint8
}

type PathSegmentHeader struct {
	StartTime uint32
	Offset    uint16
	Size      uint16
}

func (segment PathSegment) String() string {
	return fmt.Sprintf("{start:%v expr:%v}", segment.StartTime, segment.Expr)
}

func (path Path) String() string {
	var sb strings.Builder
	for _, s := range path.Segments {
		sb.WriteString(" segment:")
		sb.WriteString(s.String())
	}
	return fmt.Sprintf("{flags:%v%s}", path.Flags, sb.String())
}

func (path *Path) Write(w io.Writer) error {
	flags := path.Flags
	if len(path.Segments) > 0 {
		if path.Segments[len(path.Segments)-1].StartTime > path.Segments[0].StartTime {
			flags |= uint8(Path_Overflow)
		}
	}
	header := PathHeader{
		SegmentSize: uint16(len(path.Segments)),
		Flags:       flags,
	}
	headerSize := 4 + 8*len(path.Segments)

	var payload bytes.Buffer
	segmentHeader := make([]PathSegmentHeader, len(path.Segments))
	for i, segment := range path.Segments {
		startSize := payload.Len()
		err := segment.Expr.Write(&payload)
		if err != nil {
			return err
		}
		segmentHeader[i].StartTime = segment.StartTime
		segmentHeader[i].Offset = uint16(startSize + headerSize)
		segmentHeader[i].Size = uint16(payload.Len() - startSize)
	}

	return cmp.Or(
		binary.Write(w, binary.LittleEndian, &header),
		binary.Write(w, binary.LittleEndian, &segmentHeader),
		binary.Write(w, binary.LittleEndian, payload.Bytes()),
	)
}

func (path *Path) Read(r io.Reader) error {
	var header PathHeader
	err := binary.Read(r, binary.LittleEndian, &header)
	if err != nil {
		return err
	}

	if header.SegmentSize > 0 {
		segmentHeader := make([]PathSegmentHeader, header.SegmentSize)
		err = binary.Read(r, binary.LittleEndian, &segmentHeader)
		if err != nil {
			return err
		}

		headerSize := 4 + 8*uint16(header.SegmentSize)
		var payloadSize uint16
		for _, sh := range segmentHeader {
			payloadSize = max(payloadSize, sh.Offset+sh.Size)
		}
		payloadSize -= headerSize

		payload := make([]byte, payloadSize)
		err = binary.Read(r, binary.LittleEndian, &payload)
		if err != nil {
			return err
		}

		segments := make([]*PathSegment, header.SegmentSize)
		for i, sh := range segmentHeader {
			segments[i] = &PathSegment{}
			segments[i].StartTime = sh.StartTime
			begin := sh.Offset
			end := sh.Offset + sh.Size
			if begin < headerSize {
				return fmt.Errorf("segment offset out of range: %v", begin)
			}
			if end > headerSize+payloadSize {
				return fmt.Errorf("segment offset + size out of range: %v", end)
			}
			begin -= headerSize
			end -= headerSize
			data := payload[begin:end]
			segments[i].Expr = &Expression{}
			err = segments[i].Expr.Read(bytes.NewReader(data))
			if err != nil {
				return err
			}
		}

		path.Segments = segments
	}

	path.Flags = header.Flags
	return nil
}

func PathEqual(a, b *Path) bool {
	if a.Flags != b.Flags {
		return false
	}
	if len(a.Segments) != len(b.Segments) {
		return false
	}
	for i := range a.Segments {
		if a.Segments[i].StartTime != b.Segments[i].StartTime {
			return false
		}
		if !Equal(a.Segments[i].Expr, b.Segments[i].Expr) {
			return false
		}
	}
	return true
}
