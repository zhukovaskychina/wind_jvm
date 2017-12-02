/*
 * sun_misc_Unsafe.cpp
 *
 *  Created on: 2017年11月22日
 *      Author: zhengxiaolin
 */

#include "native/sun_misc_Unsafe.hpp"
#include <vector>
#include <algorithm>
#include <cassert>
#include "native/native.hpp"
#include "native/java_lang_String.hpp"
#include "utils/os.hpp"

static unordered_map<wstring, void*> methods = {
    {L"arrayBaseOffset:(" CLS ")I",				(void *)&JVM_ArrayBaseOffset},
    {L"arrayIndexScale:(" CLS ")I",				(void *)&JVM_ArrayIndexScale},
    {L"addressSize:()I",							(void *)&JVM_AddressSize},
    {L"objectFieldOffset:(" FLD ")J",			(void *)&JVM_ObjectFieldOffset},
    {L"getIntVolatile:(" OBJ "J)I",				(void *)&JVM_GetIntVolatile},
    {L"compareAndSwapInt:(" OBJ "JII)Z",			(void *)&JVM_CompareAndSwapInt},
    {L"allocateMemory:(J)J",						(void *)&JVM_AllocateMemory},
    {L"putLong:(JJ)V",							(void *)&JVM_PutLong},
    {L"getByte:(J)B",							(void *)&JVM_GetByte},
    {L"freeMemory:(J)V",							(void *)&JVM_FreeMemory},
};

void JVM_ArrayBaseOffset(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	ArrayOop *_array = (ArrayOop *)_stack.front();	_stack.pop_front();
	std::wcout << "[arrayBaseOffset] " << _array->get_buf_offset() << std::endl;		// delete
	_stack.push_back(new IntOop(_array->get_buf_offset()));
}
void JVM_ArrayIndexScale(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	ArrayOop *_array = (ArrayOop *)_stack.front();	_stack.pop_front();
	std::wcout << "[arrayScaleOffset] " << sizeof(intptr_t) << std::endl;		// delete
	_stack.push_back(new IntOop(sizeof(intptr_t)));
}
void JVM_AddressSize(list<Oop *> & _stack){
	_stack.push_back(new IntOop(sizeof(intptr_t)));
}
// see: http://hllvm.group.iteye.com/group/topic/37940
void JVM_ObjectFieldOffset(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	InstanceOop *field = (InstanceOop *)_stack.front();	_stack.pop_front();	// java/lang/reflect/Field obj.

	// 需要 new 一个对象... 实测...				// TODO: 还要支持 static 的！这时不用 new 对象了。
	Oop *oop;
	assert(field->get_field_value(FIELD L":name:Ljava/lang/String;", &oop));
	wstring name = java_lang_string::stringOop_to_wstring((InstanceOop *)oop);

	assert(field->get_field_value(FIELD L":type:Ljava/lang/Class;", &oop));
	MirrorOop *mirror = (MirrorOop *)oop;

	wstring descriptor = name + L":";

	// get the Type of this member variable.	// maybe: BasicType, InstanceType, ArrayType{ObjArrayType, BasicArrayType}.
	shared_ptr<Klass> mirrored_who = mirror->get_mirrored_who();
	if (mirrored_who) {	// not primitive type
		if (mirrored_who->get_type() == ClassType::InstanceClass) {
			descriptor += (L"L" + mirrored_who->get_name() + L";");
		} else if (mirrored_who->get_type() == ClassType::ObjArrayClass) {
			assert(false);		// TODO: 因为我并不知道怎么写，而且怕写错...
		} else if (mirrored_who->get_type() == ClassType::TypeArrayClass) {
			descriptor += mirrored_who->get_name();
		} else {
			assert(false);		// TODO: 同上...
		}
	} else {
		assert(mirror->get_extra() != L"");
		descriptor += mirror->get_extra();
	}

	// get the class which has the member variable.
	assert(field->get_field_value(FIELD L":clazz:Ljava/lang/Class;", &oop));
	MirrorOop *outer_klass_mirror = (MirrorOop *)oop;
	assert(outer_klass_mirror->get_mirrored_who()->get_type() == ClassType::InstanceClass);	// outer must be InstanceType.
	shared_ptr<InstanceKlass> outer_klass = std::static_pointer_cast<InstanceKlass>(outer_klass_mirror->get_mirrored_who());

	wstring BIG_signature = outer_klass->get_name() + L":" + descriptor;

	// try to new a obj
	int offset = outer_klass->new_instance()->get_all_field_offset(BIG_signature);			// TODO: GC!!

#ifdef DEBUG
	std::wcout << "(DEBUG) the field which names [ " << BIG_signature << " ], inside the [" << outer_klass->get_name() << "], has the offset [" << offset << "] of its FIELDS." << std::endl;
#endif

	_stack.push_back(new LongOop(offset));		// 这时候万一有了 GC，我的内存布局就全都变了...
}

void JVM_GetIntVolatile(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	InstanceOop *obj = (InstanceOop *)_stack.front();	_stack.pop_front();
	long offset = ((LongOop *)_stack.front())->value;	_stack.pop_front();

	// I don't know...
	assert(obj->get_klass()->get_type() == ClassType::InstanceClass);

#ifdef DEBUG
	std::wcout << "(DEBUG) [dangerous] will get an int from obj oop:[" << obj << "], which klass_name is: [" <<
			std::static_pointer_cast<InstanceKlass>(obj->get_klass())->get_name() << "], offset: [" << offset << "]: " << std::endl;
#endif
	Oop *target;
	if (std::static_pointer_cast<InstanceKlass>(obj->get_klass())->non_static_field_num() <= offset) {		// it's encoded static field offset.
		offset -= std::static_pointer_cast<InstanceKlass>(obj->get_klass())->non_static_field_num();	// decode
		target = std::static_pointer_cast<InstanceKlass>(obj->get_klass())->get_static_fields_addr()[offset];
	} else {		// it's in non-static field.
		target = obj->get_fields_addr()[offset];
	}
	// 非常危险...
	assert(target->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)target)->get_type() == Type::INT);

	int value = *((volatile int *)&((volatile IntOop *)target)->value);		// volatile
	_stack.push_back(new IntOop(value));
#ifdef DEBUG
	std::wcout << "(DEBUG) ---> int value is [" << ((IntOop *)target)->value << "] " << std::endl;
#endif

}

void JVM_CompareAndSwapInt(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	InstanceOop *obj = (InstanceOop *)_stack.front();	_stack.pop_front();
	long offset = ((LongOop *)_stack.front())->value;	_stack.pop_front();
	int expected = ((IntOop *)_stack.front())->value;	_stack.pop_front();
	int x = ((IntOop *)_stack.front())->value;	_stack.pop_front();

	Oop *target;
	if (std::static_pointer_cast<InstanceKlass>(obj->get_klass())->non_static_field_num() <= offset) {		// it's encoded static field offset.
		offset -= std::static_pointer_cast<InstanceKlass>(obj->get_klass())->non_static_field_num();	// decode
		target = std::static_pointer_cast<InstanceKlass>(obj->get_klass())->get_static_fields_addr()[offset];
	} else {		// it's in non-static field.
		target = obj->get_fields_addr()[offset];
	}
	assert(target->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)target)->get_type() == Type::INT);

	// CAS, from x86 assembly, and openjdk.
	_stack.push_back(new IntOop(cmpxchg(x, &((IntOop *)target)->value, expected) == expected));
#ifdef DEBUG
	std::wcout << "(DEBUG) compare obj + offset with [" << expected << "] and swap to be [" << x << "], success: [" << std::boolalpha << (bool)((IntOop *)_stack.back())->value << "]." << std::endl;
#endif
}

void JVM_AllocateMemory(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	long size = ((LongOop *)_stack.front())->value;	_stack.pop_front();

	if (size < 0) {
		assert(false);
	} else if (size == 0) {
		_stack.push_back(new LongOop((uintptr_t)nullptr));
	} else {
		void *addr = ::malloc(size);														// TODO: GC !!!!! 这个是堆外分配！！
		_stack.push_back(new LongOop((uintptr_t)addr));		// 地址放入。不过会转成 long。
	}
#ifdef DEBUG
	std::wcout << "(DEBUG) malloc size of [" << size << "] at address: [" << std::hex << ((LongOop *)_stack.back())->value << "]." << std::endl;
#endif
}

void JVM_PutLong(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	long addr = ((LongOop *)_stack.front())->value;	_stack.pop_front();
	long val = ((LongOop *)_stack.front())->value;	_stack.pop_front();

	*((long *)addr) = val;

#ifdef DEBUG
	std::wcout << "(DEBUG) put long val [" << val << "] at address: [" << std::hex << addr << "]." << std::endl;
#endif
}

void JVM_GetByte(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	long addr = ((LongOop *)_stack.front())->value;	_stack.pop_front();

	uint8_t val = *((uint8_t *)addr);

	_stack.push_back(new IntOop(val));

#ifdef DEBUG
	std::wcout << "(DEBUG) get byte val [" << std::dec << val << "] at address: [" << std::hex << addr << "]." << std::endl;
#endif
}

void JVM_FreeMemory(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	long addr = ((LongOop *)_stack.front())->value;	_stack.pop_front();

	::free((void *)addr);

#ifdef DEBUG
	std::wcout << "(DEBUG) free Memory of address: [" << std::hex << addr << "]." << std::endl;
#endif

}



// 返回 fnPtr.
void *sun_misc_unsafe_search_method(const wstring & signature)
{
	auto iter = methods.find(signature);
	if (iter != methods.end()) {
		return (*iter).second;
	}
	return nullptr;
}
