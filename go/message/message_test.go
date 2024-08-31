package message_test

import (
	"bytes"
	"reflect"
	"testing"

	"github.com/ScottDuckworth/WickedWinchProtocol/go/message"
	"github.com/ScottDuckworth/WickedWinchProtocol/go/postfix"
)

func TestMessageHeader(t *testing.T) {
	w := message.MessageHeader{
		TargetId:    1,
		PayloadType: message.MessageType_NotifyBmpStatus,
		PayloadSize: 12,
	}

	var buf bytes.Buffer
	if err := w.Write(&buf); err != nil {
		t.Error(err)
	}

	if buf.Len() != 4 {
		t.Error("wrong size")
	}

	var r message.MessageHeader
	if err := r.Read(&buf); err != nil {
		t.Error(err)
	}

	if !reflect.DeepEqual(r, w) {
		t.Errorf("got %v, want %v", r, w)
	}
}

func TestPingRequest(t *testing.T) {
	w := message.PingRequest{
		PingId: 123,
	}

	var buf bytes.Buffer
	if err := w.Write(&buf); err != nil {
		t.Error(err)
	}

	if buf.Len() != 4 {
		t.Error("wrong size")
	}

	var r message.PingRequest
	if err := r.Read(&buf); err != nil {
		t.Error(err)
	}

	if !reflect.DeepEqual(r, w) {
		t.Errorf("got %v, want %v", r, w)
	}
}

func TestPingResponse(t *testing.T) {
	w := message.PingResponse{
		PingId:     123,
		DeviceTime: 456,
	}

	var buf bytes.Buffer
	if err := w.Write(&buf); err != nil {
		t.Error(err)
	}

	if buf.Len() != 8 {
		t.Error("wrong size")
	}

	var r message.PingResponse
	if err := r.Read(&buf); err != nil {
		t.Error(err)
	}

	if !reflect.DeepEqual(r, w) {
		t.Errorf("got %v, want %v", r, w)
	}
}

func TestTargetList(t *testing.T) {
	w := message.TargetList{
		TargetIds: []uint8{1, 2, 3},
	}

	var buf bytes.Buffer
	if err := w.Write(&buf); err != nil {
		t.Error(err)
	}

	if buf.Len() != 4 {
		t.Error("wrong size")
	}

	var r message.TargetList
	if err := r.Read(&buf); err != nil {
		t.Error(err)
	}

	if !reflect.DeepEqual(r, w) {
		t.Errorf("got %v, want %v", r, w)
	}
}

func TestBmpStatus(t *testing.T) {
	w := message.BmpStatus{
		DeviceTime: 123,
		Celsius:    5.1,
		Pascals:    109432,
	}

	var buf bytes.Buffer
	if err := w.Write(&buf); err != nil {
		t.Error(err)
	}

	if buf.Len() != 12 {
		t.Error("wrong size")
	}

	var r message.BmpStatus
	if err := r.Read(&buf); err != nil {
		t.Error(err)
	}

	if !reflect.DeepEqual(r, w) {
		t.Errorf("got %v, want %v", r, w)
	}
}

func TestWinchStatus(t *testing.T) {
	w := message.WinchStatus{
		DeviceTime: 123,
		Position:   4567,
		Flags:      message.WinchStatusFlag_PositionKnown | message.WinchStatusFlag_Limit1,
	}

	var buf bytes.Buffer
	if err := w.Write(&buf); err != nil {
		t.Error(err)
	}

	if buf.Len() != 9 {
		t.Error("wrong size")
	}

	var r message.WinchStatus
	if err := r.Read(&buf); err != nil {
		t.Error(err)
	}

	if !reflect.DeepEqual(r, w) {
		t.Errorf("got %v, want %v", r, w)
	}
}

func TestWinchConfig(t *testing.T) {
	w := message.WinchConfig{
		StepsPerRev:    800,
		TicksPerRev:    1000,
		DistancePerRev: 150,
	}

	var buf bytes.Buffer
	if err := w.Write(&buf); err != nil {
		t.Error(err)
	}

	if buf.Len() != 8 {
		t.Error("wrong size")
	}

	var r message.WinchConfig
	if err := r.Read(&buf); err != nil {
		t.Error(err)
	}

	if !reflect.DeepEqual(r, w) {
		t.Errorf("got %v, want %v", r, w)
	}
}

func TestWinchPath(t *testing.T) {
	w := message.WinchPath{
		Mode: message.WinchMode_LinearPosition,
		Path: &postfix.Path{
			Segments: []*postfix.PathSegment{
				{
					StartTime: 1,
					Expr:      postfix.MakeBuilder().Push(123).Build(),
				},
			},
		},
	}

	var buf bytes.Buffer
	if err := w.Write(&buf); err != nil {
		t.Error(err)
	}

	var r message.WinchPath
	if err := r.Read(&buf); err != nil {
		t.Error(err)
	}

	if !reflect.DeepEqual(r, w) {
		t.Errorf("got %v, want %v", r, w)
	}
}

func TestDmxConfig(t *testing.T) {
	w := message.DmxConfig{
		ChannelOffset: 10,
		ChannelMap:    []uint8{0, 1, 2},
	}

	var buf bytes.Buffer
	if err := w.Write(&buf); err != nil {
		t.Error(err)
	}

	if buf.Len() != 5 {
		t.Error("wrong size")
	}

	var r message.DmxConfig
	if err := r.Read(&buf); err != nil {
		t.Error(err)
	}

	if !reflect.DeepEqual(r, w) {
		t.Errorf("got %v, want %v", r, w)
	}
}

func TestDmxPath(t *testing.T) {
	w := message.DmxPath{
		Path: postfix.Path{
			Segments: []*postfix.PathSegment{
				{
					StartTime: 1,
					Expr:      postfix.MakeBuilder().Push(123).Build(),
				},
			},
		},
	}

	var buf bytes.Buffer
	if err := w.Write(&buf); err != nil {
		t.Error(err)
	}

	var r message.DmxPath
	if err := r.Read(&buf); err != nil {
		t.Error(err)
	}

	if !reflect.DeepEqual(r, w) {
		t.Errorf("got %v, want %v", r, w)
	}
}
