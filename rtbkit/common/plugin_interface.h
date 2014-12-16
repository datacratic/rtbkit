#ifndef __rtb__plugin_interface_h__
#define __rtb__plugin_interface_h__

#import "rtbkit/common/plugin_table.h"
#import <string>

namespace RTBKIT {

template<class T>
struct PluginInterface
{
  static void registerPlugin(const std::string& name,
			     typename T::Factory functor);
  static typename T::Factory& getPlugin(const std::string& name);
};

template<class T>
void PluginInterface<T>::registerPlugin(const std::string& name,
					       typename T::Factory functor)
{
  PluginTable::instance().registerPlugin<typename T::Factory>(name, functor);
}

template<class T>
typename T::Factory& PluginInterface<T>::getPlugin(const std::string& name)
{
  return PluginTable::instance().getPlugin<typename T::Factory>(name, T::libNameSufix());
}


};

#endif // __rtb__plugin_interface_h__
