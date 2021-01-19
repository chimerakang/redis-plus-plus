//
// Created by sascha on 12.03.16.
//

#ifndef OOS_DELETE_ACTION_HPP
#define OOS_DELETE_ACTION_HPP

#ifdef _MSC_VER
#ifdef matador_object_EXPORTS
#define MATADOR_OBJECT_API __declspec(dllexport)
#define EXPIMP_OBJECT_TEMPLATE
#else
#define MATADOR_OBJECT_API __declspec(dllimport)
#define EXPIMP_OBJECT_TEMPLATE extern
#endif
#pragma warning(disable: 4251)
#else
#define MATADOR_OBJECT_API
#endif

#include "matador/object/action.hpp"
#include "matador/object/object_proxy.hpp"
#include "matador/object/prototype_node.hpp"

#include "matador/utils/basic_identifier.hpp"
#include "abstract_has_many.hpp"

namespace matador {

class object_serializer;

/// @cond MATADOR_DEV

/**
 * @internal
 * @class delete_action
 * @brief Action when deleting an serializable.
 *
 * This action is used when an objected
 * is deleted from the database.
 */
class MATADOR_OBJECT_API delete_action : public action
{
private:
  typedef void (*t_backup_func)(byte_buffer&, delete_action*, object_serializer &serializer);
  typedef void (*t_restore_func)(byte_buffer&, delete_action*, object_store*, object_serializer &serializer);
public:
  /**
   * Creates an delete_action.
   *
   * @param classname The serializable type name.
   * @param id The id of the deleted serializable.
   */
  template < class T >
  delete_action(object_proxy *proxy, T *obj)
    : classname_(proxy->node()->type())
    , id_(proxy->id())
    , pk_(identifier_resolver<T>::resolve(obj))
    , proxy_(proxy)
    , backup_func_(&backup_delete<T, object_serializer>)
    , restore_func_(&restore_delete<T, object_serializer>)
  {}

  ~delete_action() override;

  void accept(action_visitor *av) override;

  /**
   * Return the class name of the
   * serializable.
   *
   * @return The class name.
   */
  const char* classname() const;

  /**
   * The primary key of the serializable of the action.
   *
   * @return The primary key of the deleted serializable.
   */
  basic_identifier* pk() const;

  unsigned long id() const;

  object_proxy* proxy() const;

  void backup(byte_buffer &buffer) override;

  void restore(byte_buffer &buffer, object_store *store) override;

  void mark_deleted();

private:
  template < class T, class S >
  static void backup_delete(byte_buffer &buffer, delete_action *act, S &serializer)
  {
    T* obj = (T*)(act->proxy()->obj());
    act->backup_object(obj);
    serializer.serialize(obj, &buffer);
  }

  template < class T, class S >
  static void restore_delete(byte_buffer &buffer, delete_action *act, object_store *store, S &serializer)
  {
    // check if there is an serializable with id in
    // serializable store
    object_proxy *proxy = action::find_proxy(store, act->id());
    if (!proxy) {
      // create proxy
      proxy = act->proxy_;
//      proxy = new object_proxy(new T, act->id(), store);
      action::insert_proxy(store, proxy);
    } else {
      act->mark_deleted();
    }
//    object_serializer serializer;
    if (!proxy->obj()) {
      // create serializable with id and deserialize
      T *obj = act->init_object(new T);
      proxy->reset(obj);
      // data from buffer into serializable
      serializer.deserialize(obj, &buffer, store);
      // restore pk
      if (act->pk()) {
        proxy->pk(act->pk()->clone());
      }
      // insert serializable
//      store->insert<T>(proxy, false);
    } else {
      // data from buffer into serializable
      T *obj = act->init_object((T*)proxy->obj());
      serializer.deserialize(obj, &buffer, store);
    }
  }

  template < class T >
  void backup_object(T *, typename std::enable_if<!std::is_base_of<basic_has_many_to_many_item, T>::value>::type* = 0) {}

  template < class T >
  void backup_object(T *, typename std::enable_if<std::is_base_of<basic_has_many_to_many_item, T>::value>::type* = 0)
  {
//    owner_identifier_ = obj->owner();
  }

  template < class T >
  T* init_object(T *obj, typename std::enable_if<!std::is_base_of<basic_has_many_to_many_item, T>::value>::type* = 0)
  {
    return obj;
  }

  template < class T >
  T* init_object(T *obj, typename std::enable_if<std::is_base_of<basic_has_many_to_many_item, T>::value>::type* = 0)
  {
//    obj->owner(owner_identifier_);
    return obj;
  }

private:
  std::string classname_;
  unsigned long id_ = 0;
  basic_identifier *pk_ = nullptr;
  object_proxy *proxy_ = nullptr;
//  basic_identifier* owner_identifier_ = nullptr;

  t_backup_func backup_func_;
  t_restore_func restore_func_;

  bool deleted_ = false;
};

/// @endcond

}

#endif //OOS_DELETE_ACTION_HPP
