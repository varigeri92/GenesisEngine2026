#pragma once
#include "../Core/Handles.h"
#include <concepts>
#include <utility>


namespace gns
{
    struct Object;
    template <typename Object_T> 
    concept DerivedFromObject = std::derived_from<Object_T, gns::Object>;
    
    struct Object
    {
//Static:
    private:
        static std::unordered_map<gns::Handle, Object*> objectMap;
        static std::vector<gns::Handle> DeletionQueue;
    public:
        
        
        template <DerivedFromObject Object_T, typename... Args>
        static Object_T* Create(Args&& ... args)
        {
            Object_T* obj = new Object_T(std::forward<Args>(args)...);
            auto [it, inserted] = objectMap.try_emplace(obj->GetHandle(), obj);
            if (inserted)
                return obj;
            
            LOG_ERROR("Cannot Create Object!");
            delete obj;
            return nullptr;
        }
        
        template <DerivedFromObject Object_T>
        static Object_T* LoadFromFile(const std::string& path)
        {
            return nullptr;
        }
        
        template <DerivedFromObject Object_T>
        static Object_T* Find(const std::string& name)
        {
            return nullptr;
        }
        
        template <DerivedFromObject Object_T>
        static Object_T* Get(const gns::Handle handle)
        {
            return nullptr;
        }
        static void Reserve(std::size_t size);
        static void DeleteMarkedObjects();
//Member:
    private:
        gns::Handle m_handle;
        std::string m_name;
    public:
        virtual void Dispose();
        virtual  ~Object();
        Object();
        Object(std::string name);
        Object(Handle handle, std::string name);
        
        Handle GetHandle() const { return m_handle; }
        std::string_view GetName() const {return m_name;}
        void Rename(const std::string& name) {m_name = name;}
        
    };
}
