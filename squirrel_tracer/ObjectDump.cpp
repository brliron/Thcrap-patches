#include <Squirrel tracer.h>

ObjectDump::ObjectDump()
	: hMap(nullptr), pointer(nullptr), size(0), json(nullptr)
{}

ObjectDump::ObjectDump(const ObjectDump& other)
	: hMap(nullptr), pointer(nullptr), size(0), json(nullptr)
{
	*this = other;
}

ObjectDump::ObjectDump(const void *mem_dump, size_t size, json_t *json)
	: hMap(nullptr), pointer(nullptr), size(0), json(nullptr)
{
	this->set(mem_dump, size, json);
}

ObjectDump& ObjectDump::operator=(const ObjectDump& other)
{
	this->free();
	DuplicateHandle(GetCurrentProcess(), other.hMap, GetCurrentProcess(), &this->hMap, 0, FALSE, DUPLICATE_SAME_ACCESS);
	this->pointer = nullptr;
	this->size = other.size;
	this->json = other.json;
	json_incref(this->json);
}

ObjectDump::~ObjectDump()
{
	this->free();
}

ObjectDump::operator bool()
{
	return this->hMap != nullptr;
}

void ObjectDump::set(const void *mem_dump, size_t size, json_t *json)
{
	this->free();
	this->hMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, size, nullptr);
	this->pointer = MapViewOfFile(this->hMap, FILE_MAP_WRITE, 0, 0, size); // This pointer may be remapped as FILE_MAP_READ later
	memcpy(this->pointer, mem_dump, size);
	this->size = size;
	this->json = json;
	json_incref(this->json);
}

void ObjectDump::free()
{
	this->unmap();
	if (this->hMap) {
		CloseHandle(this->hMap);
	}
	this->hMap = nullptr;
	this->size = 0;
	if (this->json) {
		json_decref(this->json);
	}
	this->json = nullptr;
}

void ObjectDump::map()
{
	if (this->pointer) {
		return;
	}
	this->pointer = MapViewOfFile(this->hMap, FILE_MAP_READ, 0, 0, size);
}

void ObjectDump::unmap()
{
	if (this->pointer) {
		UnmapViewOfFile(this->pointer);
	}
	this->pointer = nullptr;
}

bool ObjectDump::equal(const void* mem_dump, size_t size)
{
	if (this->hMap == nullptr || this->size != size) {
		return false;
	}
	if (!this->pointer) {
		this->map();
	}
	return memcmp(this->pointer, mem_dump, size) == 0;
}

bool ObjectDump::equal(const json_t *json)
{
	if (this->json == nullptr) {
		return false;
	}
	return json_equal(this->json, json) == 0;
}




ObjectDumpCollection::ObjectDumpCollection()
{}

ObjectDumpCollection::~ObjectDumpCollection()
{}

void ObjectDumpCollection::unmapAll()
{
	for (auto& it : *this) {
		it.second.unmap();
	}
}
