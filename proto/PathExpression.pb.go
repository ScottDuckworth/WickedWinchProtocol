// Code generated by protoc-gen-go. DO NOT EDIT.
// versions:
// 	protoc-gen-go v1.25.0-devel
// 	protoc        v3.14.0
// source: proto/PathExpression.proto

package proto

import (
	protoreflect "google.golang.org/protobuf/reflect/protoreflect"
	protoimpl "google.golang.org/protobuf/runtime/protoimpl"
	reflect "reflect"
	sync "sync"
)

const (
	// Verify that this generated code is sufficiently up-to-date.
	_ = protoimpl.EnforceVersion(20 - protoimpl.MinVersion)
	// Verify that runtime/protoimpl is sufficiently up-to-date.
	_ = protoimpl.EnforceVersion(protoimpl.MaxVersion - 20)
)

type Operation int32

const (
	Operation_Undefined Operation = 0
	// n = popi()
	// for 1 .. n
	//   push(popf())
	Operation_Push Operation = 1
	// n = popi()
	// popv(n)
	Operation_Pop Operation = 2
	// n = popi()
	// push(stack[len(stack)-1-n])
	Operation_Dup Operation = 3
	// n = popi()
	// r = popv(n-1)
	// l = pop()
	// push(r...)
	// push(l)
	Operation_RotL Operation = 4
	// n = popi()
	// r = pop()
	// l = popv(n-1)
	// push(r)
	// push(l...)
	Operation_RotR Operation = 5
	// n = popi()
	// push(reverse(popv(n)))
	Operation_Rev Operation = 6
	// rows = popi()
	// cols = popi()
	// m = popv(rows * cols)
	// push(transpose(m))
	Operation_Transpose Operation = 7
	// rhs = pop()
	// lhs = pop()
	// push(lhs + rhs)
	Operation_Add Operation = 8
	// rhs = pop()
	// lhs = pop()
	// push(lhs - rhs)
	Operation_Sub Operation = 9
	// rhs = pop()
	// lhs = pop()
	// push(lhs * rhs)
	Operation_Mul Operation = 10
	// c = pop()
	// b = pop()
	// a = pop()
	// push(a + b * c)
	Operation_MulAdd Operation = 11
	// rhs = pop()
	// lhs = pop()
	// push(lhs / rhs)
	Operation_Div Operation = 12
	// rhs = pop()
	// lhs = pop()
	// push(lhs % rhs)
	Operation_Mod Operation = 13
	// push(-pop())
	Operation_Neg Operation = 14
	// push(abs(pop()))
	Operation_Abs Operation = 15
	// push(1 / pop())
	Operation_Inv Operation = 16
	// exp = pop()
	// base = pop()
	// push(pow(base, exp))
	Operation_Pow Operation = 17
	// push(sqrt(pop()))
	Operation_Sqrt Operation = 18
	// push(exp(pop()))
	Operation_Exp Operation = 19
	// push(ln(pop()))
	Operation_Ln Operation = 20
	// push(sin(pop()))
	Operation_Sin Operation = 21
	// push(cos(pop()))
	Operation_Cos Operation = 22
	// push(tan(pop()))
	Operation_Tan Operation = 23
	// push(asin(pop()))
	Operation_Asin Operation = 24
	// push(acos(pop()))
	Operation_Acos Operation = 25
	// x = pop()
	// y = pop()
	// push(atan2(y, x))
	Operation_Atan2 Operation = 26
	// size = popi() >> 1 // bit 0 indicates implicit push(size)
	// rhs = popv(size)
	// lhs = popv(size)
	// push(lhs + rhs)
	Operation_AddVec Operation = 27
	// size = popi() >> 1 // bit 0 indicates implicit push(size)
	// rhs = popv(size)
	// lhs = popv(size)
	// push(lhs - rhs)
	Operation_SubVec Operation = 28
	// size = popi() >> 1 // bit 0 indicates implicit push(size)
	// rhs = popv(size)
	// lhs = popv(size)
	// push(lhs * rhs)
	Operation_MulVec Operation = 29
	// size = popi() >> 1 // bit 0 indicates implicit push(size)
	// coeff = popv(size)
	// scale = pop()
	// push(scale * coeff)
	Operation_ScaleVec Operation = 30
	// size = popi() >> 1 // bit 0 indicates implicit push(size)
	// coeff = popv(size)
	// push(norm(coeff))
	Operation_NormVec Operation = 31
	// size = popi() >> 1 // bit 0 indicates implicit push(size)
	// coeff = popv(size)
	// param = pop()
	// push(sum(coeff[n] * param^n for n in 0 .. size-1))
	Operation_PolyVec Operation = 32
	// rows = popi()
	// cols = popi() >> 1 // bit 0 indicates implicit push(rows * cols)
	// coeff = popv(rows * cols)
	// param = pop()
	// for i := range rows
	//   for j := range cols
	//     result[j] += coeff[cols*i+j] * param^i
	// push(result)
	Operation_PolyMat Operation = 33
	// size = popi()
	// v1 = popv(size)
	// v0 = popv(size)
	// t = pop()
	// push(lerp(t, v0, v1))
	Operation_Lerp Operation = 34
	Operation_Lut  Operation = 35
)

// Enum value maps for Operation.
var (
	Operation_name = map[int32]string{
		0:  "Undefined",
		1:  "Push",
		2:  "Pop",
		3:  "Dup",
		4:  "RotL",
		5:  "RotR",
		6:  "Rev",
		7:  "Transpose",
		8:  "Add",
		9:  "Sub",
		10: "Mul",
		11: "MulAdd",
		12: "Div",
		13: "Mod",
		14: "Neg",
		15: "Abs",
		16: "Inv",
		17: "Pow",
		18: "Sqrt",
		19: "Exp",
		20: "Ln",
		21: "Sin",
		22: "Cos",
		23: "Tan",
		24: "Asin",
		25: "Acos",
		26: "Atan2",
		27: "AddVec",
		28: "SubVec",
		29: "MulVec",
		30: "ScaleVec",
		31: "NormVec",
		32: "PolyVec",
		33: "PolyMat",
		34: "Lerp",
		35: "Lut",
	}
	Operation_value = map[string]int32{
		"Undefined": 0,
		"Push":      1,
		"Pop":       2,
		"Dup":       3,
		"RotL":      4,
		"RotR":      5,
		"Rev":       6,
		"Transpose": 7,
		"Add":       8,
		"Sub":       9,
		"Mul":       10,
		"MulAdd":    11,
		"Div":       12,
		"Mod":       13,
		"Neg":       14,
		"Abs":       15,
		"Inv":       16,
		"Pow":       17,
		"Sqrt":      18,
		"Exp":       19,
		"Ln":        20,
		"Sin":       21,
		"Cos":       22,
		"Tan":       23,
		"Asin":      24,
		"Acos":      25,
		"Atan2":     26,
		"AddVec":    27,
		"SubVec":    28,
		"MulVec":    29,
		"ScaleVec":  30,
		"NormVec":   31,
		"PolyVec":   32,
		"PolyMat":   33,
		"Lerp":      34,
		"Lut":       35,
	}
)

func (x Operation) Enum() *Operation {
	p := new(Operation)
	*p = x
	return p
}

func (x Operation) String() string {
	return protoimpl.X.EnumStringOf(x.Descriptor(), protoreflect.EnumNumber(x))
}

func (Operation) Descriptor() protoreflect.EnumDescriptor {
	return file_proto_PathExpression_proto_enumTypes[0].Descriptor()
}

func (Operation) Type() protoreflect.EnumType {
	return &file_proto_PathExpression_proto_enumTypes[0]
}

func (x Operation) Number() protoreflect.EnumNumber {
	return protoreflect.EnumNumber(x)
}

// Deprecated: Use Operation.Descriptor instead.
func (Operation) EnumDescriptor() ([]byte, []int) {
	return file_proto_PathExpression_proto_rawDescGZIP(), []int{0}
}

type Path struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	Segments []*PathSegment `protobuf:"bytes,1,rep,name=segments,proto3" json:"segments,omitempty"`
}

func (x *Path) Reset() {
	*x = Path{}
	if protoimpl.UnsafeEnabled {
		mi := &file_proto_PathExpression_proto_msgTypes[0]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *Path) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*Path) ProtoMessage() {}

func (x *Path) ProtoReflect() protoreflect.Message {
	mi := &file_proto_PathExpression_proto_msgTypes[0]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use Path.ProtoReflect.Descriptor instead.
func (*Path) Descriptor() ([]byte, []int) {
	return file_proto_PathExpression_proto_rawDescGZIP(), []int{0}
}

func (x *Path) GetSegments() []*PathSegment {
	if x != nil {
		return x.Segments
	}
	return nil
}

type PathSegment struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	StartTime uint32             `protobuf:"fixed32,1,opt,name=start_time,json=startTime,proto3" json:"start_time,omitempty"`
	Expr      *PostfixExpression `protobuf:"bytes,2,opt,name=expr,proto3" json:"expr,omitempty"`
}

func (x *PathSegment) Reset() {
	*x = PathSegment{}
	if protoimpl.UnsafeEnabled {
		mi := &file_proto_PathExpression_proto_msgTypes[1]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *PathSegment) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*PathSegment) ProtoMessage() {}

func (x *PathSegment) ProtoReflect() protoreflect.Message {
	mi := &file_proto_PathExpression_proto_msgTypes[1]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use PathSegment.ProtoReflect.Descriptor instead.
func (*PathSegment) Descriptor() ([]byte, []int) {
	return file_proto_PathExpression_proto_rawDescGZIP(), []int{1}
}

func (x *PathSegment) GetStartTime() uint32 {
	if x != nil {
		return x.StartTime
	}
	return 0
}

func (x *PathSegment) GetExpr() *PostfixExpression {
	if x != nil {
		return x.Expr
	}
	return nil
}

type PostfixExpression struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	Op []Operation `protobuf:"varint,1,rep,packed,name=op,proto3,enum=proto.Operation" json:"op,omitempty"`
	I  []int32     `protobuf:"varint,2,rep,packed,name=i,proto3" json:"i,omitempty"`
	F  []float32   `protobuf:"fixed32,3,rep,packed,name=f,proto3" json:"f,omitempty"`
}

func (x *PostfixExpression) Reset() {
	*x = PostfixExpression{}
	if protoimpl.UnsafeEnabled {
		mi := &file_proto_PathExpression_proto_msgTypes[2]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *PostfixExpression) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*PostfixExpression) ProtoMessage() {}

func (x *PostfixExpression) ProtoReflect() protoreflect.Message {
	mi := &file_proto_PathExpression_proto_msgTypes[2]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use PostfixExpression.ProtoReflect.Descriptor instead.
func (*PostfixExpression) Descriptor() ([]byte, []int) {
	return file_proto_PathExpression_proto_rawDescGZIP(), []int{2}
}

func (x *PostfixExpression) GetOp() []Operation {
	if x != nil {
		return x.Op
	}
	return nil
}

func (x *PostfixExpression) GetI() []int32 {
	if x != nil {
		return x.I
	}
	return nil
}

func (x *PostfixExpression) GetF() []float32 {
	if x != nil {
		return x.F
	}
	return nil
}

var File_proto_PathExpression_proto protoreflect.FileDescriptor

var file_proto_PathExpression_proto_rawDesc = []byte{
	0x0a, 0x1a, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x2f, 0x50, 0x61, 0x74, 0x68, 0x45, 0x78, 0x70, 0x72,
	0x65, 0x73, 0x73, 0x69, 0x6f, 0x6e, 0x2e, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x12, 0x05, 0x70, 0x72,
	0x6f, 0x74, 0x6f, 0x22, 0x36, 0x0a, 0x04, 0x50, 0x61, 0x74, 0x68, 0x12, 0x2e, 0x0a, 0x08, 0x73,
	0x65, 0x67, 0x6d, 0x65, 0x6e, 0x74, 0x73, 0x18, 0x01, 0x20, 0x03, 0x28, 0x0b, 0x32, 0x12, 0x2e,
	0x70, 0x72, 0x6f, 0x74, 0x6f, 0x2e, 0x50, 0x61, 0x74, 0x68, 0x53, 0x65, 0x67, 0x6d, 0x65, 0x6e,
	0x74, 0x52, 0x08, 0x73, 0x65, 0x67, 0x6d, 0x65, 0x6e, 0x74, 0x73, 0x22, 0x5a, 0x0a, 0x0b, 0x50,
	0x61, 0x74, 0x68, 0x53, 0x65, 0x67, 0x6d, 0x65, 0x6e, 0x74, 0x12, 0x1d, 0x0a, 0x0a, 0x73, 0x74,
	0x61, 0x72, 0x74, 0x5f, 0x74, 0x69, 0x6d, 0x65, 0x18, 0x01, 0x20, 0x01, 0x28, 0x07, 0x52, 0x09,
	0x73, 0x74, 0x61, 0x72, 0x74, 0x54, 0x69, 0x6d, 0x65, 0x12, 0x2c, 0x0a, 0x04, 0x65, 0x78, 0x70,
	0x72, 0x18, 0x02, 0x20, 0x01, 0x28, 0x0b, 0x32, 0x18, 0x2e, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x2e,
	0x50, 0x6f, 0x73, 0x74, 0x66, 0x69, 0x78, 0x45, 0x78, 0x70, 0x72, 0x65, 0x73, 0x73, 0x69, 0x6f,
	0x6e, 0x52, 0x04, 0x65, 0x78, 0x70, 0x72, 0x22, 0x51, 0x0a, 0x11, 0x50, 0x6f, 0x73, 0x74, 0x66,
	0x69, 0x78, 0x45, 0x78, 0x70, 0x72, 0x65, 0x73, 0x73, 0x69, 0x6f, 0x6e, 0x12, 0x20, 0x0a, 0x02,
	0x6f, 0x70, 0x18, 0x01, 0x20, 0x03, 0x28, 0x0e, 0x32, 0x10, 0x2e, 0x70, 0x72, 0x6f, 0x74, 0x6f,
	0x2e, 0x4f, 0x70, 0x65, 0x72, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x52, 0x02, 0x6f, 0x70, 0x12, 0x0c,
	0x0a, 0x01, 0x69, 0x18, 0x02, 0x20, 0x03, 0x28, 0x05, 0x52, 0x01, 0x69, 0x12, 0x0c, 0x0a, 0x01,
	0x66, 0x18, 0x03, 0x20, 0x03, 0x28, 0x02, 0x52, 0x01, 0x66, 0x2a, 0x80, 0x03, 0x0a, 0x09, 0x4f,
	0x70, 0x65, 0x72, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x12, 0x0d, 0x0a, 0x09, 0x55, 0x6e, 0x64, 0x65,
	0x66, 0x69, 0x6e, 0x65, 0x64, 0x10, 0x00, 0x12, 0x08, 0x0a, 0x04, 0x50, 0x75, 0x73, 0x68, 0x10,
	0x01, 0x12, 0x07, 0x0a, 0x03, 0x50, 0x6f, 0x70, 0x10, 0x02, 0x12, 0x07, 0x0a, 0x03, 0x44, 0x75,
	0x70, 0x10, 0x03, 0x12, 0x08, 0x0a, 0x04, 0x52, 0x6f, 0x74, 0x4c, 0x10, 0x04, 0x12, 0x08, 0x0a,
	0x04, 0x52, 0x6f, 0x74, 0x52, 0x10, 0x05, 0x12, 0x07, 0x0a, 0x03, 0x52, 0x65, 0x76, 0x10, 0x06,
	0x12, 0x0d, 0x0a, 0x09, 0x54, 0x72, 0x61, 0x6e, 0x73, 0x70, 0x6f, 0x73, 0x65, 0x10, 0x07, 0x12,
	0x07, 0x0a, 0x03, 0x41, 0x64, 0x64, 0x10, 0x08, 0x12, 0x07, 0x0a, 0x03, 0x53, 0x75, 0x62, 0x10,
	0x09, 0x12, 0x07, 0x0a, 0x03, 0x4d, 0x75, 0x6c, 0x10, 0x0a, 0x12, 0x0a, 0x0a, 0x06, 0x4d, 0x75,
	0x6c, 0x41, 0x64, 0x64, 0x10, 0x0b, 0x12, 0x07, 0x0a, 0x03, 0x44, 0x69, 0x76, 0x10, 0x0c, 0x12,
	0x07, 0x0a, 0x03, 0x4d, 0x6f, 0x64, 0x10, 0x0d, 0x12, 0x07, 0x0a, 0x03, 0x4e, 0x65, 0x67, 0x10,
	0x0e, 0x12, 0x07, 0x0a, 0x03, 0x41, 0x62, 0x73, 0x10, 0x0f, 0x12, 0x07, 0x0a, 0x03, 0x49, 0x6e,
	0x76, 0x10, 0x10, 0x12, 0x07, 0x0a, 0x03, 0x50, 0x6f, 0x77, 0x10, 0x11, 0x12, 0x08, 0x0a, 0x04,
	0x53, 0x71, 0x72, 0x74, 0x10, 0x12, 0x12, 0x07, 0x0a, 0x03, 0x45, 0x78, 0x70, 0x10, 0x13, 0x12,
	0x06, 0x0a, 0x02, 0x4c, 0x6e, 0x10, 0x14, 0x12, 0x07, 0x0a, 0x03, 0x53, 0x69, 0x6e, 0x10, 0x15,
	0x12, 0x07, 0x0a, 0x03, 0x43, 0x6f, 0x73, 0x10, 0x16, 0x12, 0x07, 0x0a, 0x03, 0x54, 0x61, 0x6e,
	0x10, 0x17, 0x12, 0x08, 0x0a, 0x04, 0x41, 0x73, 0x69, 0x6e, 0x10, 0x18, 0x12, 0x08, 0x0a, 0x04,
	0x41, 0x63, 0x6f, 0x73, 0x10, 0x19, 0x12, 0x09, 0x0a, 0x05, 0x41, 0x74, 0x61, 0x6e, 0x32, 0x10,
	0x1a, 0x12, 0x0a, 0x0a, 0x06, 0x41, 0x64, 0x64, 0x56, 0x65, 0x63, 0x10, 0x1b, 0x12, 0x0a, 0x0a,
	0x06, 0x53, 0x75, 0x62, 0x56, 0x65, 0x63, 0x10, 0x1c, 0x12, 0x0a, 0x0a, 0x06, 0x4d, 0x75, 0x6c,
	0x56, 0x65, 0x63, 0x10, 0x1d, 0x12, 0x0c, 0x0a, 0x08, 0x53, 0x63, 0x61, 0x6c, 0x65, 0x56, 0x65,
	0x63, 0x10, 0x1e, 0x12, 0x0b, 0x0a, 0x07, 0x4e, 0x6f, 0x72, 0x6d, 0x56, 0x65, 0x63, 0x10, 0x1f,
	0x12, 0x0b, 0x0a, 0x07, 0x50, 0x6f, 0x6c, 0x79, 0x56, 0x65, 0x63, 0x10, 0x20, 0x12, 0x0b, 0x0a,
	0x07, 0x50, 0x6f, 0x6c, 0x79, 0x4d, 0x61, 0x74, 0x10, 0x21, 0x12, 0x08, 0x0a, 0x04, 0x4c, 0x65,
	0x72, 0x70, 0x10, 0x22, 0x12, 0x07, 0x0a, 0x03, 0x4c, 0x75, 0x74, 0x10, 0x23, 0x42, 0x35, 0x5a,
	0x33, 0x67, 0x69, 0x74, 0x68, 0x75, 0x62, 0x2e, 0x63, 0x6f, 0x6d, 0x2f, 0x53, 0x63, 0x6f, 0x74,
	0x74, 0x44, 0x75, 0x63, 0x6b, 0x77, 0x6f, 0x72, 0x74, 0x68, 0x2f, 0x57, 0x69, 0x63, 0x6b, 0x65,
	0x64, 0x57, 0x69, 0x6e, 0x63, 0x68, 0x50, 0x72, 0x6f, 0x74, 0x6f, 0x63, 0x6f, 0x6c, 0x2f, 0x70,
	0x72, 0x6f, 0x74, 0x6f, 0x62, 0x06, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x33,
}

var (
	file_proto_PathExpression_proto_rawDescOnce sync.Once
	file_proto_PathExpression_proto_rawDescData = file_proto_PathExpression_proto_rawDesc
)

func file_proto_PathExpression_proto_rawDescGZIP() []byte {
	file_proto_PathExpression_proto_rawDescOnce.Do(func() {
		file_proto_PathExpression_proto_rawDescData = protoimpl.X.CompressGZIP(file_proto_PathExpression_proto_rawDescData)
	})
	return file_proto_PathExpression_proto_rawDescData
}

var file_proto_PathExpression_proto_enumTypes = make([]protoimpl.EnumInfo, 1)
var file_proto_PathExpression_proto_msgTypes = make([]protoimpl.MessageInfo, 3)
var file_proto_PathExpression_proto_goTypes = []interface{}{
	(Operation)(0),            // 0: proto.Operation
	(*Path)(nil),              // 1: proto.Path
	(*PathSegment)(nil),       // 2: proto.PathSegment
	(*PostfixExpression)(nil), // 3: proto.PostfixExpression
}
var file_proto_PathExpression_proto_depIdxs = []int32{
	2, // 0: proto.Path.segments:type_name -> proto.PathSegment
	3, // 1: proto.PathSegment.expr:type_name -> proto.PostfixExpression
	0, // 2: proto.PostfixExpression.op:type_name -> proto.Operation
	3, // [3:3] is the sub-list for method output_type
	3, // [3:3] is the sub-list for method input_type
	3, // [3:3] is the sub-list for extension type_name
	3, // [3:3] is the sub-list for extension extendee
	0, // [0:3] is the sub-list for field type_name
}

func init() { file_proto_PathExpression_proto_init() }
func file_proto_PathExpression_proto_init() {
	if File_proto_PathExpression_proto != nil {
		return
	}
	if !protoimpl.UnsafeEnabled {
		file_proto_PathExpression_proto_msgTypes[0].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*Path); i {
			case 0:
				return &v.state
			case 1:
				return &v.sizeCache
			case 2:
				return &v.unknownFields
			default:
				return nil
			}
		}
		file_proto_PathExpression_proto_msgTypes[1].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*PathSegment); i {
			case 0:
				return &v.state
			case 1:
				return &v.sizeCache
			case 2:
				return &v.unknownFields
			default:
				return nil
			}
		}
		file_proto_PathExpression_proto_msgTypes[2].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*PostfixExpression); i {
			case 0:
				return &v.state
			case 1:
				return &v.sizeCache
			case 2:
				return &v.unknownFields
			default:
				return nil
			}
		}
	}
	type x struct{}
	out := protoimpl.TypeBuilder{
		File: protoimpl.DescBuilder{
			GoPackagePath: reflect.TypeOf(x{}).PkgPath(),
			RawDescriptor: file_proto_PathExpression_proto_rawDesc,
			NumEnums:      1,
			NumMessages:   3,
			NumExtensions: 0,
			NumServices:   0,
		},
		GoTypes:           file_proto_PathExpression_proto_goTypes,
		DependencyIndexes: file_proto_PathExpression_proto_depIdxs,
		EnumInfos:         file_proto_PathExpression_proto_enumTypes,
		MessageInfos:      file_proto_PathExpression_proto_msgTypes,
	}.Build()
	File_proto_PathExpression_proto = out.File
	file_proto_PathExpression_proto_rawDesc = nil
	file_proto_PathExpression_proto_goTypes = nil
	file_proto_PathExpression_proto_depIdxs = nil
}
