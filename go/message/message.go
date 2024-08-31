package message

import (
	"bytes"
	"encoding/binary"
	"io"

	"github.com/ScottDuckworth/WickedWinchProtocol/go/postfix"
)

type MessageType uint8

const (
	MessageType_None                    MessageType = 0
	MessageType_PingRequest             MessageType = 1  // Payload: PingRequest
	MessageType_PingResponse            MessageType = 2  // Payload: PingResponse
	MessageType_NotifyBmpStatus         MessageType = 3  // Payload: BmpStatus
	MessageType_NotifyWinchStatus       MessageType = 4  // Payload: WinchStatus
	MessageType_GetWinchTargetsRequest  MessageType = 5  // Payload: none
	MessageType_GetWinchTargetsResponse MessageType = 6  // Payload: TargetList
	MessageType_GetWinchConfigRequest   MessageType = 7  // Payload: none
	MessageType_GetWinchConfigResponse  MessageType = 8  // Payload: WinchConfig
	MessageType_SetWinchConfig          MessageType = 9  // Payload: WinchConfig
	MessageType_SetWinchPath            MessageType = 10 // Payload: WinchPath
	MessageType_GetDmxTargetsRequest    MessageType = 11 // Payload: none
	MessageType_GetDmxTargetsResponse   MessageType = 12 // Payload: TargetList
	MessageType_GetDmxConfigRequest     MessageType = 13 // Payload: none
	MessageType_GetDmxConfigResponse    MessageType = 14 // Payload: DmxConfig
	MessageType_SetDmxConfig            MessageType = 15 // Payload: DmxConfig
	MessageType_SetDmxPath              MessageType = 16 // Payload: DmxPath
)

type MessageHeader struct {
	TargetId    uint8
	PayloadType MessageType
	PayloadSize uint16
}

func (m *MessageHeader) Write(w io.Writer) error {
	return binary.Write(w, binary.LittleEndian, m)
}

func (m *MessageHeader) Read(r io.Reader) error {
	return binary.Read(r, binary.LittleEndian, m)
}

type PingRequest struct {
	PingId uint32
}

func (m *PingRequest) Write(w io.Writer) error {
	return binary.Write(w, binary.LittleEndian, m)
}

func (m *PingRequest) Read(r io.Reader) error {
	return binary.Read(r, binary.LittleEndian, m)
}

type PingResponse struct {
	PingId     uint32
	DeviceTime uint32
}

func (m *PingResponse) Write(w io.Writer) error {
	return binary.Write(w, binary.LittleEndian, m)
}

func (m *PingResponse) Read(r io.Reader) error {
	return binary.Read(r, binary.LittleEndian, m)
}

type targetListHeader struct {
	Size uint8
}

type TargetList struct {
	TargetIds []uint8
}

func (m *TargetList) Write(w io.Writer) error {
	header := targetListHeader{
		Size: uint8(len(m.TargetIds)),
	}
	err := binary.Write(w, binary.LittleEndian, &header)
	if err != nil {
		return err
	}

	return binary.Write(w, binary.LittleEndian, m.TargetIds)
}

func (m *TargetList) Read(r io.Reader) error {
	var header targetListHeader
	err := binary.Read(r, binary.LittleEndian, &header)
	if err != nil {
		return err
	}

	targets := make([]uint8, header.Size)
	err = binary.Read(r, binary.LittleEndian, &targets)
	if err != nil {
		return err
	}

	m.TargetIds = targets
	return nil
}

type BmpStatus struct {
	DeviceTime uint32
	Celsius    float32
	Pascals    float32
}

func (m *BmpStatus) Write(w io.Writer) error {
	return binary.Write(w, binary.LittleEndian, m)
}

func (m *BmpStatus) Read(r io.Reader) error {
	return binary.Read(r, binary.LittleEndian, m)
}

const (
	WinchStatusFlag_PositionKnown uint8 = 1 << 0
	WinchStatusFlag_Limit1        uint8 = 1 << 1
	WinchStatusFlag_Limit2        uint8 = 1 << 2
)

type WinchStatus struct {
	DeviceTime uint32
	// The linear position of the winch, if known.
	Position uint32
	// Bitwise-or of WinchStatusFlag.
	Flags uint8
}

func (m *WinchStatus) Write(w io.Writer) error {
	return binary.Write(w, binary.LittleEndian, m)
}

func (m *WinchStatus) Read(r io.Reader) error {
	return binary.Read(r, binary.LittleEndian, m)
}

type WinchConfig struct {
	// Number of stepper motor steps per revolution in the forward direction.
	StepsPerRev int16
	// Number of encoder ticks per revolution in the forward direction.
	TicksPerRev int16
	// Linear distance of the winch per revolution (circumference).
	DistancePerRev float32
}

func (m *WinchConfig) Write(w io.Writer) error {
	return binary.Write(w, binary.LittleEndian, m)
}

func (m *WinchConfig) Read(r io.Reader) error {
	return binary.Read(r, binary.LittleEndian, m)
}

type WinchMode uint8

const (
	// Winch is disengaged, free spooling.
	WinchMode_Disengage WinchMode = 0
	// Path returns extension velocity of the winch. Positive extends.
	WinchMode_LinearVelocity WinchMode = 1
	// Path returns extension position. Positive direction extends.
	WinchMode_LinearPosition WinchMode = 2
)

type winchPathHeader struct {
	// The WinchMode of this winch.
	Mode WinchMode
	// Dummy data for alignment.
	Padding uint8
	// The size of path_data.
	PathSize uint16
}

type WinchPath struct {
	// The WinchMode of this winch.
	Mode WinchMode
	Path *postfix.Path
}

func (m *WinchPath) Write(w io.Writer) error {
	var buffer bytes.Buffer
	err := m.Path.Write(&buffer)
	if err != nil {
		return err
	}

	header := winchPathHeader{
		Mode:     m.Mode,
		PathSize: uint16(buffer.Len()),
	}
	err = binary.Write(w, binary.LittleEndian, &header)
	if err != nil {
		return err
	}

	return binary.Write(w, binary.LittleEndian, buffer.Bytes())
}

func (m *WinchPath) Read(r io.Reader) error {
	var header winchPathHeader
	err := binary.Read(r, binary.LittleEndian, &header)
	if err != nil {
		return nil
	}

	pathData := make([]uint8, header.PathSize)
	err = binary.Read(r, binary.LittleEndian, &pathData)
	if err != nil {
		return nil
	}

	path := &postfix.Path{}
	err = path.Read(bytes.NewBuffer(pathData))
	if err != nil {
		return err
	}

	m.Mode = header.Mode
	m.Path = path
	return nil
}

type dmxConfigHeader struct {
	// Added to every value in channel_map to get the mapped DMX channel.
	ChannelOffset uint8
	// The number of DMX channels in channel_map.
	ChannelSize uint8
}

type DmxConfig struct {
	// Added to every value in ChannelMap to get the mapped DMX channel.
	ChannelOffset uint8
	ChannelMap    []uint8
}

func (m *DmxConfig) Write(w io.Writer) error {
	header := dmxConfigHeader{
		ChannelOffset: m.ChannelOffset,
		ChannelSize:   uint8(len(m.ChannelMap)),
	}
	err := binary.Write(w, binary.LittleEndian, &header)
	if err != nil {
		return err
	}

	return binary.Write(w, binary.LittleEndian, &m.ChannelMap)
}

func (m *DmxConfig) Read(r io.Reader) error {
	var header dmxConfigHeader
	err := binary.Read(r, binary.LittleEndian, &header)
	if err != nil {
		return err
	}

	cm := make([]uint8, header.ChannelSize)
	err = binary.Read(r, binary.LittleEndian, &cm)
	if err != nil {
		return err
	}

	m.ChannelOffset = header.ChannelOffset
	m.ChannelMap = cm
	return nil
}

type dmxPathHeader struct {
	// Padding for alignment.
	Padding uint16
	// The size of path_data.
	PathSize uint16
}

type DmxPath struct {
	Path postfix.Path
}

func (m *DmxPath) Write(w io.Writer) error {
	var buffer bytes.Buffer
	err := m.Path.Write(&buffer)
	if err != nil {
		return err
	}

	header := dmxPathHeader{
		PathSize: uint16(buffer.Len()),
	}
	err = binary.Write(w, binary.LittleEndian, &header)
	if err != nil {
		return err
	}

	return binary.Write(w, binary.LittleEndian, buffer.Bytes())
}

func (m *DmxPath) Read(r io.Reader) error {
	var header dmxPathHeader
	err := binary.Read(r, binary.LittleEndian, &header)
	if err != nil {
		return nil
	}

	pathData := make([]uint8, header.PathSize)
	err = binary.Read(r, binary.LittleEndian, &pathData)
	if err != nil {
		return nil
	}

	var path postfix.Path
	err = path.Read(bytes.NewBuffer(pathData))
	if err != nil {
		return err
	}

	m.Path = path
	return nil
}
