#include "matador/object/relation_field_endpoint.hpp"
#include "matador/object/object_store.hpp"

namespace matador {
namespace detail {

template<class T>
void basic_relation_endpoint::set_has_many_item_proxy(has_many_item_holder<T> &holder, const object_holder &obj)
{
  set_has_many_item_proxy(holder, obj.proxy_);
}

template<class T>
void basic_relation_endpoint::set_has_many_item_proxy(has_many_item_holder<T> &holder, object_proxy *proxy)
{
  holder.has_many_to_many_item_poxy_ = proxy;
}

template < class Value >
void basic_relation_endpoint::insert_value_into_foreign(const has_many_item_holder<Value> &holder, object_proxy *owner)
{
  auto sptr = foreign_endpoint.lock();
  if (sptr) {
    static_cast<relation_endpoint<Value>*>(sptr.get())->insert_value(holder, owner);
  }
}

template < class Value >
void basic_relation_endpoint::remove_value_from_foreign(const has_many_item_holder<Value> &holder, object_proxy *owner)
{
  auto sptr = foreign_endpoint.lock();
  if (sptr) {
    static_cast<relation_endpoint<Value>*>(sptr.get())->remove_value(holder, owner);
  }
}

template < class Value, class Owner, basic_relation_endpoint::relation_type Type >
void from_one_endpoint<
  Value, Owner, Type,typename std::enable_if<!matador::is_builtin<Value>::value>::type
>::insert_holder(object_store &/*store*/, has_many_item_holder<Value> &holder, object_proxy *owner)
{
  this->set_has_many_item_proxy(holder, owner);
}

template < class Value, class Owner, basic_relation_endpoint::relation_type Type >
void from_one_endpoint<
  Value, Owner, Type, typename std::enable_if<!matador::is_builtin<Value>::value>::type
>::remove_holder(object_store &/*store*/, has_many_item_holder<Value> &holder, object_proxy *) // owner
{
  this->set_has_many_item_proxy(holder, nullptr);
}

template < class Value, class Owner, basic_relation_endpoint::relation_type Type >
void from_one_endpoint<
  Value, Owner, Type, typename std::enable_if<!matador::is_builtin<Value>::value>::type
>::insert_value(object_proxy *value, object_proxy *owner)
{
  insert_value(has_many_item_holder<Value>(value, nullptr), owner);
}

template < class Value, class Owner, basic_relation_endpoint::relation_type Type >
void from_one_endpoint<
  Value, Owner, Type, typename std::enable_if<!matador::is_builtin<Value>::value>::type
>::remove_value(object_proxy *value, object_proxy *owner)
{
  remove_value(has_many_item_holder<Value>(value, nullptr), owner);
}

template < class Value, class Owner, basic_relation_endpoint::relation_type Type>
void from_one_endpoint<
  Value, Owner, Type, typename std::enable_if<!matador::is_builtin<Value>::value>::type
>::insert_value(const basic_has_many_item_holder &holder, object_proxy *owner)
{
  object_ptr<Owner> ownptr(owner);
  const auto &value_holder = static_cast<const has_many_item_holder<Value>&>(holder);
  inserter.insert(ownptr, this->field, value_holder);
//  this->increment_reference_count(value_holder.value());
}

template < class Value, class Owner, basic_relation_endpoint::relation_type Type>
void from_one_endpoint<
  Value, Owner, Type, typename std::enable_if<!matador::is_builtin<Value>::value>::type
>::remove_value(const basic_has_many_item_holder &holder, object_proxy *owner)
{
  object_ptr<Owner> ownptr(owner);
  const auto &value_holder = static_cast<const has_many_item_holder<Value>&>(holder);
  remover.remove(ownptr, this->field, value_holder);
//  this->decrement_reference_count(value_holder.value());
}


template < class Value, class Owner, basic_relation_endpoint::relation_type Type >
void from_one_endpoint<
  Value, Owner, Type,typename std::enable_if<matador::is_builtin<Value>::value>::type
>::insert_holder(object_store &/*store*/, has_many_item_holder<Value> &holder, object_proxy *owner)
{
  this->set_has_many_item_proxy(holder, owner);
}

template < class Value, class Owner, basic_relation_endpoint::relation_type Type >
void from_one_endpoint<
  Value, Owner, Type, typename std::enable_if<matador::is_builtin<Value>::value>::type
>::remove_holder(object_store &/*store*/, has_many_item_holder<Value> &holder, object_proxy *) // owner
{
  this->set_has_many_item_proxy(holder, nullptr);
}

template < class Value, class Owner, basic_relation_endpoint::relation_type Type >
void from_one_endpoint<
  Value, Owner, Type, typename std::enable_if<matador::is_builtin<Value>::value>::type
>::insert_value(object_proxy *value, object_proxy *owner)
{
  insert_value(has_many_item_holder<Value>(value, nullptr), owner);
}

template < class Value, class Owner, basic_relation_endpoint::relation_type Type >
void from_one_endpoint<
  Value, Owner, Type, typename std::enable_if<matador::is_builtin<Value>::value>::type
>::remove_value(object_proxy *value, object_proxy *owner)
{
  remove_value(has_many_item_holder<Value>(value, nullptr), owner);
}

template < class Value, class Owner, basic_relation_endpoint::relation_type Type>
void from_one_endpoint<
  Value, Owner, Type, typename std::enable_if<matador::is_builtin<Value>::value>::type
>::insert_value(const basic_has_many_item_holder &holder, object_proxy *owner)
{
  object_ptr<Owner> ownptr(owner);
  inserter.insert(ownptr, this->field, static_cast<const has_many_item_holder<Value>&>(holder));
}

template < class Value, class Owner, basic_relation_endpoint::relation_type Type>
void from_one_endpoint<
  Value, Owner, Type, typename std::enable_if<matador::is_builtin<Value>::value>::type
>::remove_value(const basic_has_many_item_holder &holder, object_proxy *owner)
{
  object_ptr<Owner> ownptr(owner);
  remover.remove(ownptr, this->field, static_cast<const has_many_item_holder<Value>&>(holder));
}

template < class Value, class Owner >
void belongs_to_many_endpoint<
  Value, Owner, typename std::enable_if<!matador::is_builtin<Value>::value>::type
>::insert_holder(object_store &, has_many_item_holder<Value> &holder, object_proxy *owner)
{
  this->set_has_many_item_proxy(holder, owner);
}

template < class Value, class Owner >
void belongs_to_many_endpoint<
  Value, Owner, typename std::enable_if<!matador::is_builtin<Value>::value>::type
>::remove_holder(object_store &, has_many_item_holder<Value> &holder, object_proxy *) // owner
{
  this->set_has_many_item_proxy(holder, nullptr);
}

template < class Value, class Owner >
void belongs_to_many_endpoint<Value, Owner, typename std::enable_if<!matador::is_builtin<Value>::value>::type>::insert_value(object_proxy *value, object_proxy *owner)
{
  object_ptr<Owner> valptr(value);
  inserter.insert(valptr, this->field, has_many_item_holder<Value>(owner, nullptr));
  owner->ostore()->mark_modified(valptr);
}

template < class Value, class Owner >
void belongs_to_many_endpoint<Value, Owner, typename std::enable_if<!matador::is_builtin<Value>::value>::type>::remove_value(object_proxy *value, object_proxy *owner)
{
  object_ptr<Owner> valptr(value);
  remover.remove(valptr, this->field, has_many_item_holder<Value>(owner, nullptr));
  owner->ostore()->mark_modified(valptr);
}

template < class Value, class Owner >
void belongs_to_many_endpoint<Value, Owner, typename std::enable_if<!matador::is_builtin<Value>::value>::type>::insert_value(const basic_has_many_item_holder &holder, object_proxy *owner)
{
  inserter.insert(static_cast<const has_many_item_holder<Owner>&>(holder).value(), this->field, has_many_item_holder<Value>(owner, nullptr));
  owner->ostore()->mark_modified(static_cast<const has_many_item_holder<Owner>&>(holder).value());
}

template < class Value, class Owner >
void belongs_to_many_endpoint<Value, Owner, typename std::enable_if<!matador::is_builtin<Value>::value>::type>::remove_value(const basic_has_many_item_holder &holder, object_proxy *owner)
{
  remover.remove(static_cast<const has_many_item_holder<Owner> &>(holder).value(), this->field,
  has_many_item_holder<Value>(owner, nullptr));
  owner->ostore()->mark_modified(static_cast<const has_many_item_holder<Owner> &>(holder).value());
}

template<class Value, class Owner>
void belongs_to_many_endpoint<Value, Owner, typename std::enable_if<!matador::is_builtin<Value>::value>::type>::print(std::ostream &out) const
{
  out << "belongs_to_many_endpoint<" << typeid(Value).name() << "," << typeid(Owner).name() << "> relation " << this->node->type() << "::" << this->field << " (" << this->type_name << ")";
}

template < class Value, class Owner >
void many_to_one_endpoint<
  Value, Owner, typename std::enable_if<!std::is_base_of<basic_has_many_to_many_item, Value>::value>::type
>::insert_holder(object_store &/*store*/, has_many_item_holder<Value> &holder, object_proxy *owner)
{
  this->mark_holder_as_inserted(holder);
  this->set_has_many_item_proxy(holder, owner);
  this->increment_reference_count(holder.value());
}

template < class Value, class Owner >
void many_to_one_endpoint<Value, Owner,
  typename std::enable_if<!std::is_base_of<basic_has_many_to_many_item, Value>::value>::type
>::remove_holder(object_store &, has_many_item_holder<Value> &holder, object_proxy *) // owner
{
  this->set_has_many_item_proxy(holder, nullptr);
  this->decrement_reference_count(holder.value());
}

template < class Value, class Owner >
void many_to_one_endpoint<Value, Owner,
  typename std::enable_if<!std::is_base_of<basic_has_many_to_many_item, Value>::value>::type
>::insert_value(object_proxy *value, object_proxy *owner)
{
  insert_value(value_type(value, nullptr), owner);
}

template < class Value, class Owner >
void many_to_one_endpoint<Value, Owner,
  typename std::enable_if<!std::is_base_of<basic_has_many_to_many_item, Value>::value>::type
>::remove_value(object_proxy *value, object_proxy *owner)
{
  remove_value(value_type(value, nullptr), owner);
}

template < class Value, class Owner >
void many_to_one_endpoint<Value, Owner,
  typename std::enable_if<!std::is_base_of<basic_has_many_to_many_item, Value>::value>::type
>::insert_value(const basic_has_many_item_holder &holder, object_proxy *owner)
{
  const auto &value_holder = static_cast<const has_many_item_holder<Value>&>(holder);

  object_ptr<Owner> ownptr(owner);
  this->mark_holder_as_inserted(const_cast<has_many_item_holder<Value>&>(value_holder));
  inserter.insert(ownptr, this->field, value_holder);
//  this->increment_reference_count(value_holder.value());
}

template < class Value, class Owner >
void many_to_one_endpoint<Value, Owner,
  typename std::enable_if<!std::is_base_of<basic_has_many_to_many_item, Value>::value>::type
>::remove_value(const basic_has_many_item_holder &holder, object_proxy *owner)
{
  const auto &value_holder = static_cast<const has_many_item_holder<Value>&>(holder);

  object_ptr<Owner> ownptr(owner);
  remover.remove(ownptr, this->field, value_holder);
//  this->decrement_reference_count(value_holder.value());
}

}
}