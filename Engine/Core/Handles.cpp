#include "gnspch.h"
#include "Handles.h"
#include "../Utils/Random.h"

gns::Handle::Handle(uint64_t handle): m_handle(handle){}

gns::Handle gns::Handle::New()
{
	uint64_t rand = Random::Get<uint64_t>();
	if(rand == Handle::Invalid)
		rand = Random::Get<uint64_t>();

	return Handle(rand);
}

gns::Handle gns::Handle::Create(uint64_t handle)
{
	return Handle(handle);
}

