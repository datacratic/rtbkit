#include <unordered_map>
#include <mutex>
#include <functional>
#include <string>
#include <dlfcn.h>
#include "jml/arch/spinlock.h"
#include "jml/arch/exception.h"


template <class T>
inline T getLibrary(std::string const &name,
	     std::string const &lib_sufix, 
	     std::unordered_map<std::string, T> container,
	     ML::Spinlock &lock,
	     std::string const &errMsg = "") {
    // see if it's already existing
    {
        std::lock_guard<ML::Spinlock> guard(lock);
        auto i = container.find(name);
        if (i != container.end()) return i->second;
    }

    // else, try to load the bidder library
    std::string path = "lib" + name + "_" + lib_sufix + ".so";
    void * handle = dlopen(path.c_str(), RTLD_NOW);
    if (!handle) {
        std::cerr << dlerror() << std::endl;
        throw ML::Exception("couldn't load  %s library %s", lib_sufix.c_str(),  path.c_str());
    }

    // if it went well, it should be registered now
    std::lock_guard<ML::Spinlock> guard(lock);
    auto i = container.find(name);
    if (i != container.end()) return i->second;

    throw ML::Exception("couldn't find %s named %s", errMsg.c_str(), name.c_str());
}

