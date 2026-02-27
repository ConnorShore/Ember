#include "Ember.h"

#include <iostream>

class TestResource : public Ember::SharedResource
{
public:
	TestResource(int val) : m_Val(val) {}

private:
	int m_Val;
};

static void PrintMessage(Ember::SharedRef<TestResource> ref)
{
	std::cout << "Reference Count: " << ref->GetRefCount() << std::endl;
}

int main()
{
	EB_TRACE("This is a trace message");
	EB_INFO("This is an info message");
	EB_WARN("This is a warning message");
	EB_ERROR("This is an error message");
	EB_FATAL("This is a fatal message");

	
	Ember::SharedRef<TestResource> resource = Ember::SharedRef<TestResource>::Create(42);
	PrintMessage(resource);
	
	Ember::SharedRef<TestResource> resource2 = resource;
	PrintMessage(resource2);

	return 0;
}