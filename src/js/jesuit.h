#pragma once
/*!
 \file js/jesuit.h
 \brief Integration helper file for v8 and logjam
 \author Jason Watson
 Copyright (c) 2011, Jason Watson
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
 this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

 * Neither the name of the LogJammin nor the names of its contributors
 may be used to endorse or promote products derived from this software
 without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 */

#include "v8.h"

namespace js
{
    //! v8 JavaScript integration class.
    /*!
     \par
     Integration class for v8. Provides helpers for turning a C++
     class into something that can be called by v8. It does depend
     on certain static fields being exposed on the C++ class;
     Specifically, \c static \c js::Jesuit::Cache \c JESUIT_CACHE and
     \c static \c js::Jesuit::Accessor \c JESUIT_ACCESSORS[] .
     \par Example
     \code
     struct Foo {
         // Necessary public static fields.
         static js::Jesuit<Foo>::Cache JESUIT_CACHE;
         static js::Jesuit<Foo>::Accessor JESUIT_ACCESSORS[];
         int bar_;
         Foo(int bar) : bar_(bar) { }
         v8::Handle<v8::Value> bar(v8::Local<v8::String> prop,
                const v8::AccessorInfo& info) {
             return v8::Integer::New(bar_);
         }
     }
     js::Jesuit<Foo>::Cache Foo::JESUIT_CACHE;
     js::Jesuit<Foo>::Accessor Foo::JESUIT_ACCESSORS[] = {
         JESUIT_ACCESSOR(Foo, "bar"), // for bar()
         // JESUIT_ACCESSOR_PAIR(Foo, "bar"), // for get_bar() and set_bar()
         JESUIT_END
     };
     \endcode
     \todo Doesn't handle index based handlers yet.
     \tparam T The v8 class wrapper.
     \author Jason Watson (jwatson@slashopt.net)
     \date March 18, 2012.
     */
    template <class T>
    class Jesuit
    {
    public:
        //! function Member function pointer def.
        /*!
         \par
         This is very similar to the v8 definition for functions
         setters.  The main difference is that this is specifically
         typed to be member functions, not static functions.
         */
        typedef v8::Handle<v8::Value> (T::*mfp_func)(
                const v8::Arguments& args);

        //! Getter Member function pointer def.
        /*!
         \par
         This is very similar to the v8 definition for getters and
         setters.  The main difference is that this is specifically
         typed to be member functions, not static functions.
         */
        typedef v8::Handle<v8::Value> (T::*mfp_get)(v8::Local<v8::String> prop,
                const v8::AccessorInfo& info);

        //! Setter Member function pointer def.
        /*!
         \par
         This is very similar to the v8 definition for getters and
         setters.  The main difference is that this is specifically
         typed to be member functions, not static functions.
         */
        typedef v8::Handle<v8::Value> (T::*mfp_set)(v8::Local<v8::String> prop,
                v8::Local<v8::Value> value,
                const v8::AccessorInfo& info);

        //! Query Member function pointer def.
        /*!
         \par
         This is very similar to the v8 definition for named query
         methods.  The main difference is that this is specifically
         typed to be member functions, not static functions. This
         method also simplifies the return type. Jesuit handles the
         conversion to a v8 type.
         */
        typedef int32_t (T::*mfp_qry)(v8::Local<v8::String> prop,
                const v8::AccessorInfo& info);

        //! Delete Member function pointer def.
        /*!
         \par
         This is very similar to the v8 definition for named delete
         methods.  The main difference is that this is specifically
         typed to be member functions, not static functions. This
         method also simplifies the return type. Jesuit handles the
         conversion to a v8 type.
         */
        typedef bool (T::*mfp_del)(v8::Local<v8::String> prop,
                const v8::AccessorInfo& info);

        //! Enumerator Member function pointer def.
        /*!
         \par
         This is very similar to the v8 definition for getters and
         setters.  The main difference is that this is specifically
         typed to be member functions, not static functions.
         */
        typedef v8::Handle<v8::Array> (T::*mfp_enum)(
                const v8::AccessorInfo& info);

        //! Cache type used to ensure all objects share a template.
        typedef v8::Persistent<v8::ObjectTemplate> Cache;

        //! Accessor Registration Type.
        struct Accessors
        {
            const char* property; //!< Name of the accessor.
            mfp_func method; //!< function method pointer.
            mfp_get getter; //!< getter method pointer.
            mfp_set setter; //!< setter member pointer.
            mfp_qry query; //!< query a property.
            mfp_del deleter; //!< delete a property.
            mfp_enum enumerator; //!< iterate over a property.
        };

        //! Wrap a pointer for V8.
        /*!
         \par
         Takes a pointer of type T and turns it into a v8 object.
         \param obj The object pointer to wrap.
         \return The v8 object in a handle.
         */
        static v8::Handle<v8::Object> wrap(T* obj)
        {
            v8::HandleScope handle_scope;

            if(T::JESUIT_CACHE.IsEmpty())
            {
                v8::Handle<v8::ObjectTemplate> raw = make_template();
                T::JESUIT_CACHE = v8::Persistent<v8::ObjectTemplate>::New(raw);
            }
            v8::Handle<v8::ObjectTemplate> templ = T::JESUIT_CACHE;

            v8::Handle<v8::Object> result = templ->NewInstance();
            v8::Handle<v8::External> obj_ptr = v8::External::New(obj);
            result->SetInternalField(0, obj_ptr);

            return handle_scope.Close(result);
        }

        //! Unwrap a v8 object into a C++ pointer.
        static T* unwrap(v8::Handle<v8::Object> result)
        {
            v8::Handle<v8::External> obj_ptr = v8::Handle<v8::External>::Cast(
                    result->GetInternalField(0));
            void* ptr = obj_ptr->Value();
            return static_cast<T*>(ptr);
        }

    private:
        static v8::Handle<v8::ObjectTemplate> make_template()
        {
            v8::HandleScope handle_scope;

            // setup the template to store an instance pointer.
            v8::Handle<v8::ObjectTemplate> result =
                    v8::ObjectTemplate::New();
            result->SetInternalFieldCount(1);

            // Register all the accessors for the class.
            // TODO Should this support index accessors?
            for (Accessors* reg = T::JESUIT_ACCESSORS;
                    reg->getter != NULL || reg->method != NULL;
                    ++reg)
            {
                if (reg->property != NULL && reg->getter != NULL)
                {
                    result->SetAccessor(v8::String::NewSymbol(reg->property),
                            getter_thunk,
                            setter_thunk,
                            v8::External::New(reg));
                }
                else if (reg->property != NULL && reg->method != NULL)
                {
                    result->Set(v8::String::NewSymbol(reg->property),
                            v8::FunctionTemplate::New(method_thunk,
                                    v8::External::New(reg)));
                }
                else
                {
                    result->SetNamedPropertyHandler(
                            getter_thunk,
                            named_setter_thunk,
                            query_thunk,
                            delete_thunk,
                            enum_thunk,
                            v8::External::New(reg));
                }
            }

            // allow the result to escape from the handle scope.
            return handle_scope.Close(result);
        }

        //! Convert a static method call into an instance method call.
        static v8::Handle<v8::Value> method_thunk(
                const v8::Arguments& args)
        {
            v8::Handle<v8::External> reg_wrapper =
                    v8::Handle<v8::External>::Cast(args.Data());
            Accessors* reg = static_cast<Accessors*>(
                    reg_wrapper->Value());

            v8::Handle<v8::Value> hidden_pointer =
                    args.Holder()->GetInternalField(0);
            v8::Handle<v8::External> obj_wrapper =
                    v8::Handle<v8::External>::Cast(hidden_pointer);
            T* obj = static_cast<T*>(obj_wrapper->Value());

            return (obj->*(reg->method))(args);
        }

        //! Convert a static method call into an instance method call.
        static v8::Handle<v8::Value> getter_thunk(v8::Local<v8::String> prop,
                const v8::AccessorInfo& info)
        {
            v8::Handle<v8::External> reg_wrapper =
                    v8::Handle<v8::External>::Cast(info.Data());
            Accessors* reg = static_cast<Accessors*>(
                    reg_wrapper->Value());

            v8::Handle<v8::Value> hidden_pointer =
                    info.Holder()->GetInternalField(0);
            v8::Handle<v8::External> obj_wrapper =
                    v8::Handle<v8::External>::Cast(hidden_pointer);
            T* obj = static_cast<T*>(obj_wrapper->Value());

            return (obj->*(reg->getter))(prop, info);
        }

        //! Convert a static method call into an instance method call.
        static void setter_thunk(
                v8::Local<v8::String> prop,
                v8::Local<v8::Value> value,
                const v8::AccessorInfo& info)
        {
            v8::Handle<v8::External> reg_wrapper =
                    v8::Handle<v8::External>::Cast(info.Data());
            Accessors* reg = static_cast<Accessors*>(
                    reg_wrapper->Value());

            v8::Handle<v8::Value> hidden_pointer =
                    info.Holder()->GetInternalField(0);
            v8::Handle<v8::External> obj_wrapper =
                    v8::Handle<v8::External>::Cast(hidden_pointer);
            T* obj = static_cast<T*>(obj_wrapper->Value());

            (obj->*(reg->setter))(prop, value, info);
        }

        //! Convert a static method call into an instance method call.
        static v8::Handle<v8::Value> named_setter_thunk(
                v8::Local<v8::String> prop,
                v8::Local<v8::Value> value,
                const v8::AccessorInfo& info)
        {
            v8::Handle<v8::External> reg_wrapper =
                    v8::Handle<v8::External>::Cast(info.Data());
            Accessors* reg = static_cast<Accessors*>(
                    reg_wrapper->Value());

            v8::Handle<v8::Value> hidden_pointer =
                    info.Holder()->GetInternalField(0);
            v8::Handle<v8::External> obj_wrapper =
                    v8::Handle<v8::External>::Cast(hidden_pointer);
            T* obj = static_cast<T*>(obj_wrapper->Value());

            return (obj->*(reg->setter))(prop, value, info);
        }

        //! Convert a static method call into an instance method call.
        static v8::Handle<v8::Integer> query_thunk(v8::Local<v8::String> prop,
                const v8::AccessorInfo& info)
        {
            v8::Handle<v8::External> reg_wrapper =
                    v8::Handle<v8::External>::Cast(info.Data());
            Accessors* reg = static_cast<Accessors*>(
                    reg_wrapper->Value());

            v8::Handle<v8::Value> hidden_pointer =
                    info.Holder()->GetInternalField(0);
            v8::Handle<v8::External> obj_wrapper =
                    v8::Handle<v8::External>::Cast(hidden_pointer);
            T* obj = static_cast<T*>(obj_wrapper->Value());

            return v8::Integer::New((obj->*(reg->query))(prop, info));
        }

        //! Convert a static method call into an instance method call.
        static v8::Handle<v8::Boolean> delete_thunk(v8::Local<v8::String> prop,
                const v8::AccessorInfo& info)
        {
            v8::Handle<v8::External> reg_wrapper =
                    v8::Handle<v8::External>::Cast(info.Data());
            Accessors* reg = static_cast<Accessors*>(
                    reg_wrapper->Value());

            v8::Handle<v8::Value> hidden_pointer =
                    info.Holder()->GetInternalField(0);
            v8::Handle<v8::External> obj_wrapper =
                    v8::Handle<v8::External>::Cast(hidden_pointer);
            T* obj = static_cast<T*>(obj_wrapper->Value());

            return v8::Boolean::New((obj->*(reg->deleter))(prop, info));
        }

        //! Convert a static method call into an instance method call.
        static v8::Handle<v8::Array> enum_thunk(const v8::AccessorInfo& info)
        {
            v8::Handle<v8::External> reg_wrapper =
                    v8::Handle<v8::External>::Cast(info.Data());
            Accessors* reg = static_cast<Accessors*>(
                    reg_wrapper->Value());

            v8::Handle<v8::Value> hidden_pointer =
                    info.Holder()->GetInternalField(0);
            v8::Handle<v8::External> obj_wrapper =
                    v8::Handle<v8::External>::Cast(hidden_pointer);
            T* obj = static_cast<T*>(obj_wrapper->Value());

            return (obj->*(reg->enumerator))(info);
        }
    };
}; // namespace js
#define JESUIT_METHOD(Class, Prop) {#Prop, &Class::Prop, 0, 0, 0, 0, 0}
#define JESUIT_ACCESSOR(Class, Prop) {#Prop, 0, &Class::Prop, 0, 0, 0, 0}
#define JESUIT_ACCESSOR_PAIR(Class, Prop) {#Prop, 0, &Class::get_##Prop, &Class::set_##Prop, 0, 0, 0}
#define JESUIT_NAME_HANDLER(Class, Prefix) {0, 0, &Class::Prefix##_get, &Class::Prefix##_set, &Class::Prefix##_query, &Class::Prefix##_delete, &Class::Prefix##_enum}
#define JESUIT_END {0, 0, 0, 0, 0, 0, 0}

