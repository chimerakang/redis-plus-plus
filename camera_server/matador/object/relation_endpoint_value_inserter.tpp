#include "matador/object/relation_endpoint_value_inserter.hpp"

namespace matador {
namespace detail {

template<class Value>
template<class Owner>
void relation_endpoint_value_inserter<Value>::insert(const object_ptr <Owner> &owner, const std::string &field,
                                                     has_many_item_holder <Value> holder)
{
  field_ = field;
  holder_ = std::move(holder);

  matador::access::serialize(*this, *owner);

  field_.clear();
}

template<class Value>
void relation_endpoint_value_inserter<Value>::serialize(const char *id,
                                                        object_pointer <Value, object_holder_type::BELONGS_TO> &x,
                                                        cascade_type cascade)
{
  if (field_ != id) {
    return;
  }
  x.reset(holder_.value().proxy_, cascade, false);
}

template<class Value>
void relation_endpoint_value_inserter<Value>::serialize(const char *id,
                                                        object_pointer <Value, object_holder_type::HAS_ONE> &x,
                                                        cascade_type cascade)
{
  if (field_ != id) {
    return;
  }
  x.reset(holder_.value().proxy_, cascade, false);
}

}
}