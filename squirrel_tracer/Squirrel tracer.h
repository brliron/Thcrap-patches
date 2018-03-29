/**
  * Touhou Community Reliant Automatic Patcher
  * Squirrel tracing plugin
  *
  * ----
  *
  * Main include file.
  */

#pragma once

#include <thcrap.h>
#include <stdio.h>

#ifdef Yield
# undef Yield
#endif
#include <new>

#define private public
#include "squirrel\include\squirrel.h"
#include "squirrel\squirrel\sqvm.h"
#include "squirrel\squirrel\sqstate.h"
#include "squirrel\squirrel\sqobject.h"
#include "squirrel\squirrel\sqarray.h"
#include "squirrel\squirrel\sqtable.h"
#include "squirrel\squirrel\sqclass.h"
#include "squirrel\squirrel\sqfuncproto.h"
#include "squirrel\squirrel\sqclosure.h"
#include "squirrel\squirrel\sqstring.h"
#include "squirrel\squirrel\squserdata.h"
#undef private

#ifdef __cplusplus

#include <map>
#include <stack>

class ObjectDump
{
private:
	HANDLE hMap;

	void *pointer;
	size_t size;
	json_t *json;
	size_t json_dump_size;

	void map();
	void release();

public:
	ObjectDump();
	ObjectDump(const ObjectDump& other);
	ObjectDump(const void *pointer, size_t size, json_t *json);
	~ObjectDump();
	ObjectDump& operator=(const ObjectDump& other);

	operator bool();
	void set(const void *pointer, size_t size, json_t *json);
	bool equal(const void *mem_dump, size_t size);
	bool equal(const json_t *json);
	void writeToFile(FILE *file);

	void unmap();
};

class ObjectDumpCollection
{
private:
	std::map<void*, ObjectDump> map;
	std::stack<ObjectDump*> mappedObjects;

public:
	ObjectDumpCollection();
	~ObjectDumpCollection();

	ObjectDump& operator[](void* key);
	void unmapAll();
};

class ClosureDB : public std::map<SQClosure*, std::string>
{
private:
	SQClosure * lastClosure; // Closure for the last instruction.
	std::string lastClosureFn; // File name for the last instruction. Used for caching.

public:
	std::string lastFileName; // File name of the last nut file loaded.

	ClosureDB();
	~ClosureDB();

	const std::string& get(SQClosure *closure);
};

enum ArgType;

class SquirrelTracer
{
private:
	CRITICAL_SECTION cs;
	FILE *file;
	ObjectDumpCollection objs_list;

	SQVM *vm;
	std::string fn;

	json_t *arg_to_json(ArgType type, uint32_t arg);
	json_t *add_obj(SQObject *o);
	template<typename T> json_t *add_refcounted(T *o);
	template<typename T> json_t *obj_to_json(T *o);

public:
	ClosureDB closureDB;

	SquirrelTracer();
	~SquirrelTracer();

	void enter(SQVM *vm);
	void leave();

	void add_instruction(SQInstruction *_i_);

	json_t *add_STK(int i) { return add_obj(&this->vm->_stack._vals[this->vm->_stackbase + i]); }
};

#endif

#ifdef __cplusplus
extern "C" {
#endif

int __stdcall thcrap_plugin_init();

#ifdef __cplusplus
}
#endif
