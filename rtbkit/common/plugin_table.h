#ifndef __rtb__plugin_table_h__
#define __rtb__plugin_table_h__

#include <string>
#include <unordered_map>
#include <boost/any.hpp>
#include "jml/arch/spinlock.h"

namespace RTBKIT {

struct PluginTable
{
public:
  static PluginTable& instance();

  // allow plugins to register themselves from their Init/AtInit
  // all legacy plugins use this.
  template <class T>
  void registerPlugin(const std::string& name, T& functor);

  // get a plugin factory this is a generic version, but requires that
  // the library suffix that contains this plugin to be provided
  template <class T>
  T& getPlugin(const std::string& name, const std::string& libSufix);

  // destructor
  ~PluginTable();

private:
 
  // data
  // -----
  // functor container
  // - the map key is a string pair containing name and typename
  // this is just to avoid name conflict
  // - the plugin holds the functor wrapped in an any type and also
  // the typeName to double check agains the request without have to
  // parse the the key.
  struct Plugin {
    std::string typeName;
    boost::any functor;
  };
  std::unordered_map<std::string, Plugin> table;

  // singleton pointer
  static PluginTable* singleton;

  // lock
  ML::Spinlock lock;

  // block default constructors
  PluginTable(){};
  PluginTable(PluginTable&){};

  // load library
  void loadLib(const std::string& path);

  // critical sessions functions
  // write to container
  void write(const std::string& key,
	     const std::string& typeName,
	     boost::any fctWrapper);

  // read from container
  boost::any& read(const std::string& name,
		   const std::string& typeName,
		   const std::string& libSufix);
 
};


// inject a new "factory" (functor) - called from the plugin dll
template <class T>
void
  PluginTable::registerPlugin(const std::string& name, T& functor)
{
  // get the type name to complete the plugin data
  std::string tName(typeid(functor).name());

  // wrapper the functor
  boost::any wrapper(functor);

  // add plugin to container
  write(name, tName, wrapper);
}

 
// get the functor from the name
template <class T>
T&
PluginTable::getPlugin(const std::string& name, const std::string& libSufix)
{
  // get the typename
  T mock;
  std::string tName(typeid(mock).name());

  // read plugi from container - cast and return
  return boost::any_cast<T&>(read(name, tName, libSufix));
}

 
}; // namespace RTBKIT

#endif // __rtb__plugin_table_h__
