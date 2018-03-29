#include <Squirrel tracer.h>

ObjectDump::ObjectDump()
	: hMap(nullptr), pointer(nullptr), size(0), json(nullptr), json_dump_size(0)
{}

ObjectDump::ObjectDump(const ObjectDump& other)
	: hMap(nullptr), pointer(nullptr), size(0), json(nullptr), json_dump_size(0)
{
	*this = other;
}

ObjectDump::ObjectDump(const void *mem_dump, size_t size, json_t *json)
	: hMap(nullptr), pointer(nullptr), size(0), json(nullptr), json_dump_size(0)
{
	this->set(mem_dump, size, json);
}

ObjectDump& ObjectDump::operator=(const ObjectDump& other)
{
	this->release();
	DuplicateHandle(GetCurrentProcess(), other.hMap, GetCurrentProcess(), &this->hMap, 0, FALSE, DUPLICATE_SAME_ACCESS);
	this->pointer = nullptr;
	this->size = other.size;
	this->json = other.json;
	json_incref(this->json);
	this->json_dump_size = 0;
	return *this;
}

ObjectDump::~ObjectDump()
{
	this->release();
}

ObjectDump::operator bool()
{
	return this->hMap != nullptr;
}

void ObjectDump::set(const void *mem_dump, size_t size, json_t *json)
{
	this->release();

	json_t *dumped_obj = json_object();
	json_object_set_new(dumped_obj, "type", json_string("object"));
	char string[] = "POINTER:0x00000000";
	sprintf(string, "POINTER:%p", mem_dump);
	json_object_set_new(dumped_obj, "address", json_string(string));
	json_object_set(dumped_obj, "content", json);

	this->json_dump_size = json_dumpb(dumped_obj, nullptr, 0, JSON_COMPACT);
	size_t alloc_size = size + this->json_dump_size;

	this->hMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, alloc_size, nullptr);
	if (this->hMap == nullptr) {
		log_mboxf("Error", MB_OK, "CreateFileMapping failed with error code %d", GetLastError());
	}
	this->pointer = MapViewOfFile(this->hMap, FILE_MAP_WRITE, 0, 0, alloc_size); // This pointer may be remapped as FILE_MAP_READ later
	if (this->pointer == nullptr) {
		if (GetLastError() == ERROR_NOT_ENOUGH_MEMORY) {
			log_mboxf("Error", MB_OK, "MapViewOfFile failed: ERROR_NOT_ENOUGH_MEMORY");
		}
		else {
			log_mboxf("Error", MB_OK, "MapViewOfFile failed with error code %d", GetLastError());
		}
	}

	memcpy(this->pointer, mem_dump, size);
	this->size = size;

	this->json = json;
	json_incref(this->json);

	this->json_dump_size = json_dumpb(dumped_obj, (char*)this->pointer + size, this->json_dump_size, JSON_COMPACT);
	json_decref(dumped_obj);
}

void ObjectDump::release()
{
	this->unmap();
	if (this->hMap) {
		CloseHandle(this->hMap);
	}
	this->hMap = nullptr;
	this->size = 0;
	this->json_dump_size = 0;
}

void ObjectDump::map()
{
	if (this->pointer) {
		return;
	}
	this->pointer = MapViewOfFile(this->hMap, FILE_MAP_READ, 0, 0, this->size + this->json_dump_size);
	if (this->pointer == nullptr) {
		if (GetLastError() == ERROR_NOT_ENOUGH_MEMORY) {
			log_mboxf("Error", MB_OK, "MapViewOfFile failed: ERROR_NOT_ENOUGH_MEMORY");
		}
		else {
			log_mboxf("Error", MB_OK, "MapViewOfFile failed with error code %d", GetLastError());
		}
	}
	if (!this->json) {
		json_error_t error;
		json_t *obj = json_loadb((char*)this->pointer + this->size, this->json_dump_size, JSON_COMPACT, &error);
		if (obj == nullptr) {
			log_mboxf("Error", MB_OK, "%s", error.text);
		}

		this->json = json_object_get(obj, "content");
		json_incref(this->json);
		json_decref(obj);
	}
}

void ObjectDump::unmap()
{
	if (this->pointer) {
		UnmapViewOfFile(this->pointer);
	}
	this->pointer = nullptr;
	if (this->json) {
		json_decref(this->json);
	}
	this->json = nullptr;
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
	return json_equal(this->json, json) != 0;
}

void ObjectDump::writeToFile(FILE *file)
{
	this->map();
	fwrite((char*)this->pointer + this->size, this->json_dump_size, 1, file);
	fwrite(",\n", 2, 1, file);
}



ObjectDumpCollection::ObjectDumpCollection()
{}

ObjectDumpCollection::~ObjectDumpCollection()
{}

ObjectDump& ObjectDumpCollection::operator[](void* key)
{
	ObjectDump& obj = map[key];
	this->mappedObjects.push(&obj);
	return obj;
}

void ObjectDumpCollection::unmapAll()
{
	while (!this->mappedObjects.empty()) {
		this->mappedObjects.top()->unmap();
		this->mappedObjects.pop();
	}
}
