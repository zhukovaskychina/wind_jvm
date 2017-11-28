#ifndef __CLASS_PARSER_H__
#define __CLASS_PARSER_H__

#include <iostream>
#include <unordered_map>
#include <vector>
#include <arpa/inet.h>

//#define DEBUG

// used in parser

#define MAGIC_NUMBER 	0xCAFEBABE		// magic number

// access flags	C -> use for the whole Class/Interface		M -> use for Method		F -> use for Field		I -> use for Inner class
#define ACC_PUBLIC		0x0001			// can be visited by all (CFMI)
#define ACC_PRIVATE		0x0002			// cannot be visited by all (FMI)
#define ACC_PROTECTED	0x0004			// cannot be visited by this (FMI)
#define ACC_STATIC		0x0008			// static (FMI)
#define ACC_FINAL		0x0010			// cannot have a child class (CFMI)
#define ACC_SUPER		0x0020			// when the `invokespecial` instruction used, the father class's method should be treated specially (C)
#define ACC_SYNCHRONIZED	0x0020			// method was synchornized by **monitor** (M)
#define ACC_VOLATILE		0x0040			// volatile (fragile) (F)
#define ACC_BRIDGE		0x0040			// method was generated by compiler (M)
#define ACC_TRANSIENT	0x0080			// cannot be serialized (F)
#define ACC_VARARGS		0x0080			// method has va_args (M)
#define ACC_NATIVE		0x0100			// method is native (M)
#define ACC_INTERFACE	0x0200			// this is an interface, otherwise a class (CI)
#define ACC_ABSTRACT		0x0400			// cannot be instantiation (CMI)
#define ACC_STRICT		0x0800			// method uses **strictfp**, strictfp float format (M)
#define ACC_SYNTHETIC	0x1000			// the code is not generalized by java (by compiler but not the coder) (CFMI)
#define ACC_ANNOTATION	0x2000			// this is an annotation like @Override. at the same time the `ACC_INTERFACE` should also be settled (CI)
#define ACC_ENUM			0x4000			// this is an enum like enum {...} (CFI)
#define ACC_MANDATED		0x8000

// constant pool
#define CONSTANT_Class				7
#define CONSTANT_Fieldref			9
#define CONSTANT_Methodref			10
#define CONSTANT_InterfaceMethodref	11
#define CONSTANT_String				8
#define CONSTANT_Integer				3
#define CONSTANT_Float				4
#define CONSTANT_Long				5
#define CONSTANT_Double				6
#define CONSTANT_NameAndType			12
#define CONSTANT_Utf8				1
#define CONSTANT_MethodHandle		15
#define CONSTANT_MethodType			16
#define CONSTANT_InvokeDynamic		18

// constant pool: MethodHandle
#define REF_getField					1
#define REF_getStatic				2
#define REF_putField					3
#define REF_putStatic				4
#define REF_invokeVirtual			5
#define REF_invokeStatic				6
#define REF_invokeSpecial			7
#define REF_newInvokeSpecial			8
#define REF_invokeInterface			9

/*===------------  basic -----------------*/

#define u1	std::uint8_t
#define u2	std::uint16_t
#define u4	std::uint32_t

#define FLOAT_INFINITY			0x7f800000
#define FLOAT_NEGATIVE_INFINITY	0xff800000
#define FLOAT_NAN				0x7f880000
			// define by myself
#define DOUBLE_INFINITY			0x7ff0000000000000L
#define DOUBLE_NEGATIVE_INFINITY	0xfff0000000000000L
#define DOUBLE_NAN				0x7ff8000000000000L


#define BIT_NUM	8
			// one byte has 8 bits
/*===------------ aux functions --------------===*/

// attention!!! these peek() functions is in normal order!! but when you read(), the order is reversed!! in little endian!!

struct CodeStub;

u1 peek1(std::ifstream & f);	// peek u1
u2 peek2(std::ifstream & f);
u4 peek4(std::ifstream & f);
u1 read1(std::ifstream & f);
u2 read2(std::ifstream & f);
u4 read4(std::ifstream & f);

/*===----------- hexdump stub ---------------===*/
// to save the ClassFile hex code stub... for Reflection...
struct CodeStub {
	std::vector<u1> stub;	// TODO: 改成 vector！！可以适应不定长度的变长策略......
	void inject(u1 code) {
		stub.push_back(code);
	}
	void inject(u2 code) {	// 已经存入变量中的小端序。(变量内部存储的顺序)
		u2 real_hex = htons(code);		// 还原回 hexdump 的大端序。
		inject((u1)((real_hex >> 8) & 0xFF));	// !! 先写入高字节。按顺序写入。
		inject((u1)(real_hex & 0xFF));
	}
	void inject(u4 code) {
		u4 real_hex = htonl(code);		// 还原回 hexdump 的大端序。
		inject((u1)((real_hex >> 24) & 0xFF));
		inject((u1)((real_hex >> 16) & 0xFF));
		inject((u1)((real_hex >> 8) & 0xFF));
		inject((u1)(real_hex & 0xFF));
	}
	void inject(u1 *bytes, int num) {
		for (int i = 0; i < num; i ++) {
			inject(bytes[i]);
		}
	}
	void inject(u2 *bytes, int num) {
		for (int i = 0; i < num; i ++) {
			inject(bytes[i]);
		}
	}

	CodeStub & operator+= (const CodeStub & rhs) {
		this->stub.insert(this->stub.end(), rhs.stub.begin(), rhs.stub.end());
		return *this;
	}

	CodeStub operator+ (const CodeStub & rhs) {
		CodeStub tmp;
		tmp += *this;
		tmp += rhs;
		return tmp;
	}
};

/*===----------- constant pool --------------===*/

struct cp_info {
	u1 tag = 0;
};

struct CONSTANT_CS_info : public cp_info{			// Class, String
	u2 index;
	friend std::ifstream & operator >> (std::ifstream & f, CONSTANT_CS_info & i);
};

struct CONSTANT_FMI_info : public cp_info {		// Field, Methodref, InterfaceMethodref
	u2 class_index;
	u2 name_and_type_index;
	friend std::ifstream & operator >> (std::ifstream & f, CONSTANT_FMI_info & i);
};

struct CONSTANT_Integer_info : public cp_info {		// Integer
	u4 bytes;
	friend std::ifstream & operator >> (std::ifstream & f, CONSTANT_Integer_info & i);
	int get_value();
};

struct CONSTANT_Float_info : public cp_info {		// Float
	u4 bytes;
	friend std::ifstream & operator >> (std::ifstream & f, CONSTANT_Float_info & i);
	float get_value();
};

struct CONSTANT_Long_info : public cp_info {		// Long
	u4 high_bytes;
	u4 low_bytes;
	friend std::ifstream & operator >> (std::ifstream & f, CONSTANT_Long_info & i);
	long get_value();
};

struct CONSTANT_Double_info : public cp_info {		// Double
	u4 high_bytes;
	u4 low_bytes;
	friend std::ifstream & operator >> (std::ifstream & f, CONSTANT_Double_info & i);
	double get_value();
};

struct CONSTANT_NameAndType_info : public cp_info {// Name, Type
	u2 name_index;
	u2 descriptor_index;
	friend std::ifstream & operator >> (std::ifstream & f, CONSTANT_NameAndType_info & i);
};

struct CONSTANT_Utf8_info : public cp_info {		// string literal
	u2 length;
	u1* bytes = nullptr;
	std::wstring str = L"";
	friend std::ifstream & operator >> (std::ifstream & f, CONSTANT_Utf8_info & i);
	bool test_bit_1(int position, int bit_pos);
	bool is_first_type(int position);	// $ 4.4.7
	bool is_second_type(int position);
	bool is_third_type(int position);
	bool is_forth_type(int position);
	u2 cal_first_type(int position);
	u2 cal_second_type(int position);
	u2 cal_third_type(int position);
	u2 cal_forth_type(int position);
	std::wstring convert_to_Unicode();
	~CONSTANT_Utf8_info();
};

struct CONSTANT_MethodHandle_info : public cp_info {	// method handler
	u1 reference_kind;
	u2 reference_index;
	friend std::ifstream & operator >> (std::ifstream & f, CONSTANT_MethodHandle_info & i);
};

struct CONSTANT_MethodType_info : public cp_info {	// method type
	u2 descriptor_index;
	friend std::ifstream & operator >> (std::ifstream & f, CONSTANT_MethodType_info & i);
};

struct CONSTANT_InvokeDynamic_info : public cp_info {
	u2 bootstrap_method_attr_index;
	u2 name_and_type_index;
	friend std::ifstream & operator >> (std::ifstream & f, CONSTANT_InvokeDynamic_info & i);
};

/*===-----------  DEBUG CONSTANT POOL -----------===*/

void print_constant_pool(cp_info **bufs, int length);

/*===------------ Field && Method --------------===*/

struct attribute_info {		// show be moved up because of incompleted type. but seperate by .h will solve the problem.
	u2 attribute_name_index;
	u4 attribute_length;
	friend std::ifstream & operator >> (std::ifstream & f, attribute_info & i);
};

struct field_info {
	u2 access_flags;
	u2 name_index;
	u2 descriptor_index;
	u2 attributes_count;
	attribute_info **attributes = nullptr;		// [attributes_count]
//	friend std::ifstream & operator >> (std::ifstream & f, field_info & i);
	void fill(std::ifstream & f, cp_info **constant_pool);
	~field_info();
};

struct method_info {
	u2 access_flags;
	u2 name_index;
	u2 descriptor_index;
	u2 attributes_count;
	attribute_info **attributes = nullptr;		// [attributes_count]
//	friend std::ifstream & operator >> (std::ifstream & f, method_info & i);
	void fill(std::ifstream & f, cp_info **constant_pool);
	~method_info();
};

/*===-----------  DEBUG Fields -----------===*/

void print_fields(field_info *bufs, int length, cp_info **constant_pool);

/*===-----------  DEBUG Methods -----------===*/

void print_methods(method_info *bufs, int length, cp_info **constant_pool);

/*===-----------  DEBUG Attributes -----------===*/

extern std::unordered_map<u1, std::pair<std::string, int>> bccode_map;

int get_args_size (method_info *bufs, std::wstring & method_name, int i);

void print_attributes(attribute_info *ptr, cp_info **constant_pool);

/*===----------- Attributes ---------------===*/

// aux hashmap
extern std::unordered_map<std::wstring, int> attribute_table;

// aux function
int peek_attribute(u2 attribute_name_index, cp_info **constant_pool);	// look ahead ifstream to see which attribute the next is.

struct ConstantValue_attribute : public attribute_info {
	u2 constantvalue_index;	
	friend std::ifstream & operator >> (std::ifstream & f, ConstantValue_attribute & i);
};

struct Code_attribute : public attribute_info {
	u2 max_stack;
	u2 max_locals;
	u4 code_length;
	u1 *code;										// [code_length]
	u2 exception_table_length; 
	struct exception_table_t { 
		u2 start_pc;
		u2 end_pc;
		u2 handler_pc;
		u2 catch_type;
		friend std::ifstream & operator >> (std::ifstream & f, exception_table_t & i);
	};
	exception_table_t *exception_table = nullptr;		// [exception_table_length]
	u2 attributes_count;
	attribute_info **attributes = nullptr;				// [attributes_count]
	
	void fill(std::ifstream & f, cp_info **constant_pool);
	~Code_attribute();
};

// StackMapTable Attributes
#define ITEM_Top					0
#define ITEM_Integer				1
#define ITEM_Float				2
#define ITEM_Double				3
#define ITEM_Long				4
#define ITEM_Null				5
#define ITEM_UninitializedThis	6
#define ITEM_Object				7
#define ITEM_Uninitialized		8

struct StackMapTable_attribute : public attribute_info {
	// variable_info
	struct verification_type_info {
		u1 tag;
		friend std::ifstream & operator >> (std::ifstream & f, StackMapTable_attribute::verification_type_info & i);
	};
	struct Top_variable_info : public verification_type_info {
		// nothing
		friend std::ifstream & operator >> (std::ifstream & f, StackMapTable_attribute::Top_variable_info & i);
	};
	struct Integer_variable_info : public verification_type_info {
		// nothing
		friend std::ifstream & operator >> (std::ifstream & f, StackMapTable_attribute::Integer_variable_info & i);
	};
	struct Float_variable_info : public verification_type_info {
		// nothing
		friend std::ifstream & operator >> (std::ifstream & f, StackMapTable_attribute::Float_variable_info & i);
	};
	struct Double_variable_info : public verification_type_info {
		// nothing
		friend std::ifstream & operator >> (std::ifstream & f, StackMapTable_attribute::Double_variable_info & i);
	};
	struct Long_variable_info : public verification_type_info {
		// nothing
		friend std::ifstream & operator >> (std::ifstream & f, StackMapTable_attribute::Long_variable_info & i);
	};
	struct Null_variable_info : public verification_type_info {
		// nothing
		friend std::ifstream & operator >> (std::ifstream & f, StackMapTable_attribute::Null_variable_info & i);
	};
	struct UninitializedThis_variable_info : public verification_type_info {
		// nothing
		friend std::ifstream & operator >> (std::ifstream & f, StackMapTable_attribute::UninitializedThis_variable_info & i);
	};
	struct Object_variable_info : public verification_type_info {
		u2 cpool_index;
		friend std::ifstream & operator >> (std::ifstream & f, StackMapTable_attribute::Object_variable_info & i);
	};
	struct Uninitialized_variable_info : public verification_type_info {
		u2 offset;
		friend std::ifstream & operator >> (std::ifstream & f, StackMapTable_attribute::Uninitialized_variable_info & i);
	};
	
	// aux function
	static verification_type_info* create_verification_type(std::ifstream & f);
	
	// stack_map_frame
	struct stack_map_frame {
		u1 frame_type;	
		friend std::ifstream & operator >> (std::ifstream & f, StackMapTable_attribute::stack_map_frame & i);
	};
	struct same_frame : public stack_map_frame {		// frame_type: 0-63
		// none
		friend std::ifstream & operator >> (std::ifstream & f, StackMapTable_attribute::same_frame & i);
	};
	struct same_locals_1_stack_item_frame : public stack_map_frame  {	// frame_type: 64-127
		verification_type_info *stack[1];		// [1]
		friend std::ifstream & operator >> (std::ifstream & f, StackMapTable_attribute::same_locals_1_stack_item_frame & i);
		~same_locals_1_stack_item_frame();
	};
	struct same_locals_1_stack_item_frame_extended : public stack_map_frame  {	// frame_type: 247
		u2 offset_delta;
		verification_type_info *stack[1];
		friend std::ifstream & operator >> (std::ifstream & f, StackMapTable_attribute::same_locals_1_stack_item_frame_extended & i);
		~same_locals_1_stack_item_frame_extended();
	};
	struct chop_frame : public stack_map_frame  {		// frame_type: 248-250
		u2 offset_delta;
		friend std::ifstream & operator >> (std::ifstream & f, StackMapTable_attribute::chop_frame & i);
	};
	struct same_frame_extended : public stack_map_frame  {	// frame_type: 251
		u2 offset_delta;
		friend std::ifstream & operator >> (std::ifstream & f, StackMapTable_attribute::same_frame_extended & i);
	};
	struct append_frame : public stack_map_frame  {			// frame_type: 252-254
		u2 offset_delta;
		verification_type_info **locals = nullptr;	// [frame_type - 251]
		friend std::ifstream & operator >> (std::ifstream & f, StackMapTable_attribute::append_frame & i);
		~append_frame();
	};
	struct full_frame : public stack_map_frame  {			// frame_type: 255
		u2 offset_delta;
		u2 number_of_locals;
		verification_type_info **locals = nullptr;	// [number_of_locals]
		u2 number_of_stack_items;
		verification_type_info **stack = nullptr;	// [number_of_stack_items]
		friend std::ifstream & operator >> (std::ifstream & f, StackMapTable_attribute::full_frame & i);
		~full_frame();
	};
	
	// aux function
	static stack_map_frame* peek_stackmaptable_frame(std::ifstream & f);
	
	// per se
	u2 number_of_entries;
	stack_map_frame **entries = nullptr;		// [number_of_entries];
	friend std::ifstream & operator >> (std::ifstream & f, StackMapTable_attribute & i);
	~StackMapTable_attribute();
};

struct Exceptions_attribute : public attribute_info {
	u2 number_of_exceptions;
	u2 *exception_index_table = nullptr;		// [number_of_exceptions]
	friend std::ifstream & operator >> (std::ifstream & f, Exceptions_attribute & i);
	~Exceptions_attribute();
};

struct InnerClasses_attribute : public attribute_info {
	u2 number_of_classes;
	struct classes_t { 
		u2 inner_class_info_index;
		u2 outer_class_info_index;
		u2 inner_name_index;
		u2 inner_class_access_flags;
		friend std::ifstream & operator >> (std::ifstream & f, InnerClasses_attribute::classes_t & i);
	} *classes = nullptr;				// [number_of_classes]
	friend std::ifstream & operator >> (std::ifstream & f, InnerClasses_attribute & i);
	~InnerClasses_attribute();
};

struct EnclosingMethod_attribute : public attribute_info {
	u2 class_index;
	u2 method_index;
	friend std::ifstream & operator >> (std::ifstream & f, EnclosingMethod_attribute & i);
};

struct Synthetic_attribute : public attribute_info {
	friend std::ifstream & operator >> (std::ifstream & f, Synthetic_attribute & i);
};

struct Signature_attribute : public attribute_info {
     u2 signature_index;
	friend std::ifstream & operator >> (std::ifstream & f, Signature_attribute & i);
};

struct SourceFile_attribute : public attribute_info {
	u2 sourcefile_index;
	friend std::ifstream & operator >> (std::ifstream & f, SourceFile_attribute & i);
};

struct SourceDebugExtension_attribute : public attribute_info {
     u1 *debug_extension = nullptr;		// [attribute_length];
	friend std::ifstream & operator >> (std::ifstream & f, SourceDebugExtension_attribute & i);
	~SourceDebugExtension_attribute();
};

struct LineNumberTable_attribute : public attribute_info {
	u2 line_number_table_length; 
	struct line_number_table_t { 
		u2 start_pc;
		u2 line_number;
		friend std::ifstream & operator >> (std::ifstream & f, LineNumberTable_attribute::line_number_table_t & i);
	} *line_number_table = nullptr;		// [line_number_table_length]
	friend std::ifstream & operator >> (std::ifstream & f, LineNumberTable_attribute & i);
	~LineNumberTable_attribute();
};

struct LocalVariableTable_attribute : public attribute_info {
	u2 local_variable_table_length; 
	struct local_variable_table_t { 
		u2 start_pc;
		u2 length;
		u2 name_index;
		u2 descriptor_index;
		u2 index;
		friend std::ifstream & operator >> (std::ifstream & f, LocalVariableTable_attribute::local_variable_table_t & i);
	} *local_variable_table = nullptr;	// [local_variable_table_length]
	friend std::ifstream & operator >> (std::ifstream & f, LocalVariableTable_attribute & i);
	~LocalVariableTable_attribute();
};

struct LocalVariableTypeTable_attribute : public attribute_info {
	u2 local_variable_type_table_length; 
	struct local_variable_type_table_t { 
		u2 start_pc;
		u2 length;
		u2 name_index;
		u2 signature_index;
		u2 index;
		friend std::ifstream & operator >> (std::ifstream & f, LocalVariableTypeTable_attribute::local_variable_type_table_t & i);
	} *local_variable_type_table = nullptr;// [local_variable_type_table_length]
	friend std::ifstream & operator >> (std::ifstream & f, LocalVariableTypeTable_attribute & i);
	~LocalVariableTypeTable_attribute();
};

struct Deprecated_attribute : public attribute_info {
	friend std::ifstream & operator >> (std::ifstream & f, Deprecated_attribute & i);
};

//struct element_value {		// changed
//	u1 tag;
//	union value_t{
//		u2 const_value_index;
//		struct enum_const_value_t {
//			u2 type_name_index; 
//			u2 const_name_index;
//			friend std::ifstream & operator >> (std::ifstream & f, element_value::value_t::enum_const_value_t & i);
//		} enum_const_value;
//		u2 class_info_index; 
//		annotation *annotation_value;	// [1] 
//		struct array_value_t { 
//			u2 num_values;
//			element_value *values = nullptr;		// [num_values]
//			friend std::ifstream & operator >> (std::ifstream & f, element_value::value_t::array_value_t & i);
//			~array_value_t();
//		} array_value;
//	} value;
//	
//	friend std::ifstream & operator >> (std::ifstream & f, element_value & i);
//};

struct value_t {
	CodeStub stub;
};

struct const_value_t : public value_t {
	u2 const_value_index;
	friend std::ifstream & operator >> (std::ifstream & f, const_value_t & i);
};

struct enum_const_value_t : public value_t {
	u2 type_name_index;
	u2 const_name_index;
	friend std::ifstream & operator >> (std::ifstream & f, enum_const_value_t & i);
};

struct class_info_t : public value_t {
	u2 class_info_index;
	friend std::ifstream & operator >> (std::ifstream & f, class_info_t & i);
};

struct element_value {
	u1 tag;
	value_t *value = nullptr;	// [1]
	friend std::ifstream & operator >> (std::ifstream & f, element_value & i);
	~element_value();
	CodeStub stub;
};

struct annotation : public value_t {
	u2 type_index;
	u2 num_element_value_pairs;
	struct element_value_pairs_t { 
		u2 element_name_index;
		element_value value;
		friend std::ifstream & operator >> (std::ifstream & f, annotation::element_value_pairs_t & i);
		CodeStub stub;
     } *element_value_pairs = nullptr;		// [num_element_value_pairs]

	friend std::ifstream & operator >> (std::ifstream & f, annotation & i);
	~annotation();
	CodeStub stub;
};

struct array_value_t : public value_t { 
	u2 num_values;
	element_value *values = nullptr;		// [num_values]
	friend std::ifstream & operator >> (std::ifstream & f, array_value_t & i);
	~array_value_t();
};

struct type_annotation {
	// target_type
	struct target_info_t {
		CodeStub stub;
	};
	struct type_parameter_target : target_info_t {
		u1 type_parameter_index;
		friend std::ifstream & operator >> (std::ifstream & f, type_annotation::type_parameter_target & i);
	};
	struct supertype_target : target_info_t {
		u2 supertype_index;
		friend std::ifstream & operator >> (std::ifstream & f, type_annotation::supertype_target & i);
	};
	struct type_parameter_bound_target : target_info_t {
		u1 type_parameter_index;
		u1 bound_index;
		friend std::ifstream & operator >> (std::ifstream & f, type_annotation::type_parameter_bound_target & i);
	};
	struct empty_target : target_info_t {
		friend std::ifstream & operator >> (std::ifstream & f, type_annotation::empty_target & i);
	};
	struct formal_parameter_target : target_info_t {
		u1 formal_parameter_index;
		friend std::ifstream & operator >> (std::ifstream & f, type_annotation::formal_parameter_target & i);
	};
	struct throws_target : target_info_t {
		u2 throws_type_index;
		friend std::ifstream & operator >> (std::ifstream & f, type_annotation::throws_target & i);
	};
	struct localvar_target : target_info_t {
		u2 table_length;
		struct table_t {   
			u2 start_pc;
			u2 length;
			u2 index;
			friend std::ifstream & operator >> (std::ifstream & f, type_annotation::localvar_target::table_t & i);
			CodeStub stub;
		} *table = nullptr;				// [table_length];
		friend std::ifstream & operator >> (std::ifstream & f, type_annotation::localvar_target & i);
		~localvar_target();
	};
	struct catch_target : target_info_t {
		u2 exception_table_index;
		friend std::ifstream & operator >> (std::ifstream & f, type_annotation::catch_target & i);
	};
	struct offset_target : target_info_t {
		u2 offset;
		friend std::ifstream & operator >> (std::ifstream & f, type_annotation::offset_target & i);
	};
	struct type_argument_target : target_info_t {
		u2 offset;
		u1 type_argument_index;
		friend std::ifstream & operator >> (std::ifstream & f, type_annotation::type_argument_target & i);
	};
	// type_path
	struct type_path {
		u1 path_length;
		struct path_t {   
			u1 type_path_kind;
			u1 type_argument_index;
			friend std::ifstream & operator >> (std::ifstream & f, type_annotation::type_path::path_t & i);
			CodeStub stub;
		} *path = nullptr;				// [path_length];
		friend std::ifstream & operator >> (std::ifstream & f, type_annotation::type_path & i);
		~type_path();
		CodeStub stub;
	};
	
	// basic
	u1 target_type;
	target_info_t *target_info = nullptr;	// [1]
	type_path target_path;
	annotation *anno = nullptr;				// [1]
	
	friend std::ifstream & operator >> (std::ifstream & f, type_annotation & i);
	~type_annotation();
	CodeStub stub;
};

// aux data-structure
struct parameter_annotations_t {	// extract from Runtime_XXX_Annotations_attributes
	u2 num_annotations;
	annotation *annotations = nullptr;	// [num_annotations]
	friend std::ifstream & operator >> (std::ifstream & f, parameter_annotations_t & i);
	~parameter_annotations_t();
	CodeStub stub;
};

struct RuntimeVisibleAnnotations_attribute : public attribute_info {
	parameter_annotations_t parameter_annotations;
	friend std::ifstream & operator >> (std::ifstream & f, RuntimeVisibleAnnotations_attribute & i);
};

struct RuntimeInvisibleAnnotations_attribute : public attribute_info {
	parameter_annotations_t parameter_annotations;
	friend std::ifstream & operator >> (std::ifstream & f, RuntimeInvisibleAnnotations_attribute & i);
};

struct RuntimeVisibleParameterAnnotations_attribute : public attribute_info {
	u1 num_parameters;
	parameter_annotations_t *parameter_annotations = nullptr;		// [num_parameters];
	friend std::ifstream & operator >> (std::ifstream & f, RuntimeVisibleParameterAnnotations_attribute & i);
	~RuntimeVisibleParameterAnnotations_attribute();
};

struct RuntimeInvisibleParameterAnnotations_attribute : public attribute_info {
	u1 num_parameters;
	parameter_annotations_t *parameter_annotations = nullptr;		// [num_parameters];
	friend std::ifstream & operator >> (std::ifstream & f, RuntimeInvisibleParameterAnnotations_attribute & i);
	~RuntimeInvisibleParameterAnnotations_attribute();
};

struct RuntimeVisibleTypeAnnotations_attribute : public attribute_info {
	u2 num_annotations;
	type_annotation *annotations = nullptr;					// [num_annotations];
	friend std::ifstream & operator >> (std::ifstream & f, RuntimeVisibleTypeAnnotations_attribute & i);
	~RuntimeVisibleTypeAnnotations_attribute();
};

struct RuntimeInvisibleTypeAnnotations_attribute : public attribute_info {
	u2 num_annotations;
	type_annotation *annotations = nullptr;					// [num_annotations];
	friend std::ifstream & operator >> (std::ifstream & f, RuntimeInvisibleTypeAnnotations_attribute & i);
	~RuntimeInvisibleTypeAnnotations_attribute();
};

struct AnnotationDefault_attribute : public attribute_info {
	element_value default_value;
	friend std::ifstream & operator >> (std::ifstream & f, AnnotationDefault_attribute & i);
};

struct BootstrapMethods_attribute : public attribute_info {
	u2 num_bootstrap_methods;
	struct bootstrap_methods_t {   
		u2 bootstrap_method_ref;
		u2 num_bootstrap_arguments;
		u2 *bootstrap_arguments = nullptr;					// [num_bootstrap_arguments];
		friend std::ifstream & operator >> (std::ifstream & f, BootstrapMethods_attribute::bootstrap_methods_t & i);
		~bootstrap_methods_t();
	} *bootstrap_methods = nullptr;							// [num_bootstrap_methods];
	friend std::ifstream & operator >> (std::ifstream & f, BootstrapMethods_attribute & i);
	~BootstrapMethods_attribute();
};

struct MethodParameters_attribute : public attribute_info {
	u1 parameters_count;
	struct parameters_t {   
		u2 name_index;
		u2 access_flags;
		friend std::ifstream & operator >> (std::ifstream & f, MethodParameters_attribute::parameters_t & i);
	} *parameters = nullptr;									// [parameters_count];
	friend std::ifstream & operator >> (std::ifstream & f, MethodParameters_attribute & i);
	~MethodParameters_attribute();
};

attribute_info* new_attribute(std::ifstream & f, cp_info **constant_pool);

std::string parse_inner_element_value(element_value *inner_ev);
std::string recursive_parse_annotation (annotation *target);

/*===----------- .class ----------------===*/

struct ClassLoader;

struct ClassFile {
	u4 magic;
	u2 minor_version;
	u2 major_version;
	u2 constant_pool_count;
	cp_info **constant_pool = nullptr;		// [constant_pool_count-1]	// for cp_info's polymorphic, can't use array of cp_info but instead using array of cp_info*
	u2 access_flags;
	u2 this_class;
	u2 super_class;
	u2 interfaces_count;
	u2 *interfaces = nullptr;				// [interfaces_count]
	u2 fields_count;
	field_info *fields = nullptr;			// [fields_count]
	u2 methods_count;
	method_info *methods = nullptr;			// [methods_count];
	u2 attributes_count;
	attribute_info **attributes = nullptr;	// [attributes_count];

	void parse_header(std::ifstream & f);
	void parse_constant_pool(std::ifstream & f);
	void parse_class_msgs(std::ifstream & f);
	void parse_interfaces(std::ifstream & f);
	void parse_fields(std::ifstream & f);
	void parse_methods(std::ifstream & f);
	void parse_attributes(std::ifstream & f);
	
	friend std::ifstream & operator >> (std::ifstream & f, ClassFile & cf);
	
	ClassFile() {}
	ClassFile(ClassFile && cf);
	~ClassFile();
private:
	ClassFile(const ClassFile & cf);	// banned copy
};

#endif
