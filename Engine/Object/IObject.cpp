#include "gnspch.h"
#include "IObject.h"

#include <utility>


std::unordered_map<gns::Handle, gns::Object*> gns::Object::objectMap = {};
std::vector<gns::Handle> gns::Object::DeletionQueue = {};

void gns::Object::Dispose()
{
    DeletionQueue.push_back(m_handle);
}

gns::Object::~Object() = default;

gns::Object::Object()
{
    m_name = "Uninitialized Empty Object";
    LOG_WARNING("[Object]: you are Creating an object with a default empty Constructor! \n "
                "\t - Make sure you know what you are doing buh!");
}

gns::Object::Object(std::string name)
{
    m_handle = Handle::CreateFromString(name);
    m_name = name;
}

gns::Object::Object(Handle handle, std::string name)
{
    m_handle = handle;
    m_name = std::move(name);
}


void gns::Object::Reserve(std::size_t size)
{
    objectMap.reserve(size);
    DeletionQueue.reserve(size);
}

void gns::Object::DeleteMarkedObjects()
{
    for (auto handle : DeletionQueue)
    {
        delete objectMap.at(handle);
    }
    DeletionQueue.clear();
}

