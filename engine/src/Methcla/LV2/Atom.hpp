// Copyright 2012-2013 Samplecount S.L.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef METHCLA_LV2_ATOM_HPP_INCLUDED
#define METHCLA_LV2_ATOM_HPP_INCLUDED

#include <cassert>
#include <string>

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"
#include "lv2/lv2plug.in/ns/ext/atom/util.h"

namespace LV2 {

    class Property
    {
    public:
        Property(LV2_URID key, LV2_URID context=0)
            : m_key(key)
            , m_context(context)
        { }

        LV2_URID key() const { return m_key; }
        LV2_URID context() const { return m_context; }

    private:
        LV2_URID m_key, m_context;
    };

    class Forge : public LV2_Atom_Forge
    {
    public:
        Forge(const LV2_Atom_Forge& forge)
            : LV2_Atom_Forge(forge)
        { }
        Forge(LV2_URID_Map* uridMap)
        {
            lv2_atom_forge_init(this, uridMap);
        }

        void set_buffer(uint8_t* data, uint32_t size)
        {
            lv2_atom_forge_set_buffer(this, data, size);
        }

        void atom(uint32_t size, uint32_t type)
        {
            lv2_atom_forge_atom(this, size, type);
        }

        void write(const void* data, uint32_t size)
        {
            lv2_atom_forge_write(this, const_cast<void*>(data), size);
        }

        Forge& operator<<(int32_t x)
        {
            lv2_atom_forge_int(this, x);
            return *this;
        }

        Forge& operator<<(int64_t x)
        {
            lv2_atom_forge_long(this, x);
            return *this;
        }

        Forge& operator<<(float x)
        {
            lv2_atom_forge_float(this, x);
            return *this;
        }

        Forge& operator<<(double x)
        {
            lv2_atom_forge_double(this, x);
            return *this;
        }

        Forge& operator<<(bool x)
        {
            lv2_atom_forge_bool(this, x);
            return *this;
        }

        Forge& operator<<(LV2_URID x)
        {
            lv2_atom_forge_urid(this, x);
            return *this;
        }

        Forge& operator<<(const char* x)
        {
            lv2_atom_forge_string(this, x, strlen(x));
            return *this;
        }

        Forge& operator<<(const std::string& x)
        {
            lv2_atom_forge_string(this, x.c_str(), x.size());
            return *this;
        }

        Forge& operator<<(const class Property& x)
        {
            lv2_atom_forge_property_head(this, x.key(), x.context());
            return *this;
        }
    };

    class Frame : public LV2_Atom_Forge_Frame
    {
    public:
        Frame(Forge& forge)
            : m_forge(forge)
        { }
        virtual ~Frame()
        {
            lv2_atom_forge_pop(&m_forge, this);
        }

        operator Forge& () { return m_forge; }

    protected:
        Forge& m_forge;
    };

    class TupleFrame : public Frame
    {
    public:
        TupleFrame(Forge& forge)
            : Frame(forge)
        {
            lv2_atom_forge_tuple(&m_forge, this);
        }
    };

    class ObjectFrame : public Frame
    {
    public:
        ObjectFrame(Forge& forge, LV2_URID id, LV2_URID otype)
            : Frame(forge)
        {
            lv2_atom_forge_blank(&m_forge, this, id, otype);
        }
    };

    //
// class ObjectIterator
//   : public boost::iterator_facade<
//         ObjectIterator
//       , Property const
//       , boost::input_iterator_tag
//     >
// {
//  public:
//     ObjectIterator()
//       : m_node(0) {}

//     explicit ObjectIterator(node_base* p)
//       : m_node(p) {}

//  private:
//     friend class boost::iterator_core_access;

//     void increment() { m_node = m_node->next(); }

//     bool equal(const_node_iterator const& other) const
//     {
//         return this->m_node == other.m_node;
//     }

//     node_base const& dereference() const { return *m_node; }

//     node_base const* m_node;
// };

//class Property
//{
//public:
	//Property(const LV2_Atom_Property_Body* prop)
		//: m_impl(prop)
	//{ }

//private:
	//const LV2_Atom_Property* m_impl;
//};

//class Object
//{
//public:
	//Object(const LV2_Atom_Object* obj)
		//: m_impl(obj)
	//{ }

	//class const_iterator
	//{
	//public:
		//explicit const_iterator(const LV2_Atom_Object_Body* obj)
			//: m_iter(it)
		//{ }

		//operator bool () const { return !lv2_atom_}
	//};

	//const_iterator begin() const { return ObjectIterator(m_impl); }
//private:
	//const LV2_Atom_Object_Body* m_impl;
//};

};

#endif // METHCLA_LV2_ATOM_HPP_INCLUDED
