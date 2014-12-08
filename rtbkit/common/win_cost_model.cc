/* win_cost_model.h                                                    -*- C++ -*-
   Eric Robert, 13 May 2013
   Copyright (c) 2013 Datacratic.  All rights reserved.

*/

#include "rtbkit/common/win_cost_model.h"
#include "jml/arch/exception.h"
#include "jml/arch/format.h"
#include "jml/arch/spinlock.h"
#include "rtbkit/common/injection.h"
//#include <dlfcn.h>
#include <unordered_map>
#include <mutex>

namespace RTBKIT {

namespace {

// storage hash for model instances
std::unordered_map<std::string, WinCostModel::Model> models;

// lock to access it
typedef ML::Spinlock lock_type;
ML::Spinlock lock;

WinCostModel::Model const &
getModel(std::string const & name) {
  // this was made static because otherwise the compiler will complain about
  // returning a reference to a temporary object.
  // baffles me the reason why the previous implementation did not cause this error!
  // Also, I dont know why we are returning a ref here, all other places where this
  // same template is used. always returns by value???
  static WinCostModel::Model ret;
  ret = getLibrary(name,
		   "win_cost_model",
		   models,
		   lock,
		   "win cost model");

  return ret;
}

struct NoWinCostModel {

    static Amount evaluate(WinCostModel const & model,
                           Bid const & bid,
                           Amount const & price)
    {
        return price;
    }
};

struct AtInit {
    AtInit()
    {
        WinCostModel::registerModel("none", NoWinCostModel::evaluate);
    }
} atInit;
} // file scope

WinCostModel::
WinCostModel()
{
}

WinCostModel::
WinCostModel(std::string name, Json::Value data) :
    name(std::move(name)),
    data(std::move(data))
{
}

Amount
WinCostModel::
evaluate(Bid const & bid, Amount const & price) const
{
    if(name.empty()) {
        return NoWinCostModel::evaluate(*this, bid, price);
    }

    auto model = getModel(name);
    if(!model) {
        throw ML::Exception("win cost model '%s' not found", name.c_str());
    }

    return model(*this, bid, price);
}

Json::Value
WinCostModel::
toJson() const
{
    Json::Value result;
    if(!name.empty()) {
        result["name"] = name;
        if(!data.empty()) {
            result["data"] = data;
        }
    }
    return result;
}

WinCostModel
WinCostModel::
fromJson(Json::Value const & json)
{
    WinCostModel result;

    for(auto i = json.begin(), end = json.end(); i != end; ++i) {
        if(i.memberName() == "name")
            result.name = i->asString();
        else
        if(i.memberName() == "data")
            result.data = *i;
        else
            throw ML::Exception("unknown win cost model field " +
                                i.memberName());
    }

    return result;
}

void
WinCostModel::
serialize(ML::DB::Store_Writer & store) const
{
    int version = 1;
    store << version << name << data.toString();
}

void
WinCostModel::
reconstitute(ML::DB::Store_Reader & store)
{
    int version = 0; store >> version;
    if(version == 1) {
        std::string text;
        store >> name >> text;
        if(!text.empty()) {
            data = Json::parse(text);
        }
    }
    else {
        ML::Exception("reconstituting wrong version");
    }
}

void
WinCostModel::
registerModel(std::string const & name, Model model)
{
    std::lock_guard<lock_type> guard(lock);

    auto result = models.insert(make_pair(name, model));
    if(!result.second) {
        throw ML::Exception("already had a win cost model '%s' registered", name.c_str());
    }
}

void
WinCostModel::
createDescription(WinCostModelDescription & d) {
    d.addField("name", &WinCostModel::name, "");
    d.addField("data", &WinCostModel::data, "");
}

} // namespace RTBKIT
