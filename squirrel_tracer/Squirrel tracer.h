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

json_t *add_obj(FILE *file, SQObject *o);

class ObjectDump
{
private:
	HANDLE hMap;

	void *pointer;
	size_t size;
	json_t *json;

	void map();
	void free();

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

	void unmap();
};

class ObjectDumpCollection : public std::map<void*, ObjectDump>
{
public:
	ObjectDumpCollection();
	~ObjectDumpCollection();

	void unmapAll();
};

#endif

#ifdef __cplusplus
extern "C" {
#endif

int __stdcall thcrap_plugin_init();

#ifdef __cplusplus
}
#endif
