/* plugin_table.cc
   Flavio Moreira, 15 December 2014
   Copyright (c) 2014 Datacratic.  All rights reserved.
*/


#include <typeinfo>
#include <functional>
#include <exception>
#include <mutex>
#include <string>
#include <dlfcn.h>

#include "jml/arch/exception.h"
#include "rtbkit/common/plugin_table.h"

namespace RTBKIT {

// initing static pointer
//PluginTable* PluginTable::singleton = nullptr;

// get singleton instance
PluginTable&
PluginTable::instance()
{
  static PluginTable singleton;
  return singleton;
}

// destructor
PluginTable::~PluginTable()
{
  //  delete singleton;
  //singleton = nullptr;
}


// critical block write to the table
void
PluginTable::write(const std::string& name,
		   const std::string& typeName,
		   boost::any fctWrapper)
{
  // some safeguards...
  if (name.empty()) {
    throw ML::Exception("'name' parameter cannot be empty");
  }
  if (name.empty()) {
    throw ML::Exception("'typeName' parameter cannot be empty");
  }


  // assemble the key
  std::string key(name + "__" + typeName);
  
  // create plugin structure
  Plugin plug;
  plug.typeName = typeName;
  plug.functor = fctWrapper;

  auto element = std::pair<std::string, Plugin>(key, plug);

  // lock and write
  std::lock_guard<ML::Spinlock> guard(lock);
  table.insert(element);
}

// critical block reads from the table
boost::any&
PluginTable::read(const std::string& name,
		  const std::string& typeName,
		  const std::string& libSufix)
{
  // some safeguards...
  if (name.empty()) {
    throw ML::Exception("'name' parameter cannot be empty");
  }
  if (name.empty()) {
    throw ML::Exception("'typeName' parameter cannot be empty");
  }

  // assemble the key
  std::string key(name + "__" + typeName);

  for (int i=0; i<2; i++)
  {
    // check if it already exists
    {
      std::lock_guard<ML::Spinlock> guard(lock);
      auto iter = table.find(key);
      if(iter != table.end())
      {
	const std::string & tn = iter->second.typeName;

	if (typeName == tn)
	{
	  return iter->second.functor;
	}
      }
    }
    
    if (i == 0) // try to load it
    {
      // since it was not found we have to try to load the library
      std::string path = "lib" + name + "_" + libSufix + ".so";
      loadLib(path);
    } // we can add alternative forms of plugin load here

    ///////////
    // now hopefully the plugin is loaded
    // and we can load it in the next loop
  }  

  // else: getting the functor fails
  throw ML::Exception("couldn't get requested plugin");
}

// loads a dll
void
PluginTable::loadLib(const std::string& path)
{
  // some safeguards...
  if (path.empty()) {
    throw ML::Exception("'path' parameter cannot be empty");
  }

  // load lib
  void * handle = dlopen(path.c_str(), RTLD_NOW);
  
  if (!handle) {
    //std::cerr << dlerror() << std::endl;
    throw ML::Exception("couldn't load library from %s", path.c_str());
  }
}

  
}; // end namespace RTBKIT
