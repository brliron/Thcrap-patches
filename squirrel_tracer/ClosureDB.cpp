#include "Squirrel tracer.h"

ClosureDB::ClosureDB()
	: lastClosure(nullptr)
{}


ClosureDB::~ClosureDB()
{}

const std::string& ClosureDB::get(SQClosure *closure)
{
	// The last result is cached (that's the one we want 99% of the time).
	if (closure == this->lastClosure) {
		return this->lastClosureFn;
	}
	// Special case: the 1st closure isn't loaded from a file and don't have a file name.
	if (this->empty()) {
		return (*this)[closure] = "/";
	}
	this->lastClosure = closure;

	// Try to find the closure
	auto& it = this->find(closure);
	if (it != this->end()) {
		this->lastClosureFn = it->second;
		return it->second;
	}

	// Create a new closure.
	// This is probably the result of a Closure instruction, so we'll use the file name from the previous instruction.
	(*this)[closure] = this->lastClosureFn;
	return this->lastClosureFn;
}
