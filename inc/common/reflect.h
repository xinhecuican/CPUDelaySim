#ifndef REFLECT_H
#define REFLECT_H
#include <iostream>
#include <string>
#include <map>
#include <functional>
#include <memory>


class ObjectFactory {
private:
    std::map<std::string, std::function<void*()>> creators_;

public:
    // 注册任意类
    template<typename T>
    void registerClass(const std::string& className) {
        creators_[className] = []() -> void* {
            return new T();
        };
    }

    template<typename T>
    static T* createObject(const std::string& className) {
        auto it = getInstance().creators_.find(className);
        if (it != getInstance().creators_.end()) {
            return static_cast<T*>(it->second());
        }
        return nullptr;
    }

    static ObjectFactory& getInstance() {
        static ObjectFactory instance;
        return instance;
    }
};

template<typename T>
class RegisterHelper {
public:
    RegisterHelper(const std::string& name) {
        ObjectFactory::getInstance().registerClass<T>(name);
    }
};

#define REGISTER_CLASS(className) \
    static RegisterHelper<className> reg##className(#className);

#endif