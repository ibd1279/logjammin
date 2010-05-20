#pragma once
/*!
 \file Linked_map.h
 \brief LJ Linked_map header and implementation.
 \author Jason Watson
 Copyright (c) 2010, Jason Watson
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

#include <iostream>
#include <iterator>
#include <map>

namespace lj
{
    template<typename K, typename V>
    class Linked_map
    {
    public:
        typedef typename std::pair<K, V> value_type;
        typedef typename std::allocator<value_type> allocator_type;
        typedef typename allocator_type::size_type size_type;
        typedef typename allocator_type::difference_type difference_type;
        typedef value_type* pointer;
        typedef const value_type* const_pointer;
        typedef value_type& reference;
        typedef const value_type& const_reference;
        
        class Linked_map_const_iterator;
 
        class Linked_map_iterator : public std::iterator<std::forward_iterator_tag, typename Linked_map::value_type>
        {
            friend class Linked_map;
            friend class Linked_map_const_iterator;
        public:
            typedef typename Linked_map<K, V>::pointer pointer;
            typedef typename Linked_map<K, V>::const_pointer const_pointer;
            typedef typename Linked_map<K, V>::reference reference;
            typedef typename Linked_map<K, V>::const_reference const_reference;
            
            Linked_map_iterator(value_type* v, Linked_map_iterator* prev, Linked_map_iterator* next) : val_(v), prev_(prev), next_(next)
            {
            }
            
            Linked_map_iterator(const Linked_map_iterator& o) : val_(o.val_), prev_(o.prev_), next_(o.next_)
            {
            }
            
            operator reference()
            {
                return *val_;
            }
            
            operator const_reference() const
            {
                return *val_;
            }
            
            Linked_map_iterator& operator=(const Linked_map_iterator& o)
            {
                val_ = o.val_;
                prev_ = o.prev_;
                next_ = o.next_;
                return *this;
            }
            
            bool operator==(const Linked_map_iterator& o) const
            {
                return (prev_ == o.prev_ &&
                        next_ == o.next_);
            }
            
            bool operator!=(const Linked_map_iterator& o) const
            {
                return (prev_ != o.prev_ ||
                        next_ != o.next_);
            }
            
            reference operator*()
            {
                return *val_;
            }
            
            const_reference operator*() const
            {
                return *val_;
            }
            
            pointer operator->()
            {
                return val_;
            }
            
            const_pointer operator->() const
            {
                return val_;
            }
            
            Linked_map_iterator operator++(int)
            {
                Linked_map_iterator i(*this);
                if (next_)
                {
                    val_ = next_->val_;
                    prev_ = next_->prev_;
                    next_ = next_->next_;
                }
                return i;
            }
            Linked_map_iterator& operator++()
            {
                if (next_)
                {
                    val_ = next_->val_;
                    prev_ = next_->prev_;
                    next_ = next_->next_;
                }
                return *this;
            }
        private:
            value_type* val_;
            Linked_map_iterator* prev_;
            Linked_map_iterator* next_;
        };

        class Linked_map_const_iterator : public std::iterator<std::forward_iterator_tag, typename Linked_map::value_type>
        {
            friend class Linked_map;
        public:
            typedef typename Linked_map<K, V>::pointer pointer;
            typedef typename Linked_map<K, V>::const_pointer const_pointer;
            typedef typename Linked_map<K, V>::reference reference;
            typedef typename Linked_map<K, V>::const_reference const_reference;
            
            Linked_map_const_iterator(value_type* v, Linked_map_iterator* prev, Linked_map_iterator* next) : val_(v), prev_(prev), next_(next)
            {
            }
            
            Linked_map_const_iterator(const Linked_map_iterator& o) : val_(o.val_), prev_(o.prev_), next_(o.next_)
            {
            }

            Linked_map_const_iterator(const Linked_map_const_iterator& o) : val_(o.val_), prev_(o.prev_), next_(o.next_)
            {
            }
            
            operator reference()
            {
                return *val_;
            }
            
            operator const_reference() const
            {
                return *val_;
            }
            
            Linked_map_const_iterator& operator=(const Linked_map_const_iterator& o)
            {
                val_ = o.val_;
                prev_ = o.prev_;
                next_ = o.next_;
                return *this;
            }
            
            Linked_map_const_iterator& operator=(const Linked_map_iterator& o)
            {
                val_ = o.val_;
                prev_ = o.prev_;
                next_ = o.next_;
                return *this;
            }
            
            bool operator==(const Linked_map_const_iterator& o) const
            {
                return (prev_ == o.prev_ &&
                        next_ == o.next_);
            }
            
            bool operator!=(const Linked_map_const_iterator& o) const
            {
                return (prev_ != o.prev_ ||
                        next_ != o.next_);
            }
            
            const_reference operator*()
            {
                return *val_;
            }
            
            const_reference operator*() const
            {
                return *val_;
            }
            
            const_pointer operator->()
            {
                return val_;
            }
            
            const_pointer operator->() const
            {
                return val_;
            }
            
            Linked_map_const_iterator operator++(int)
            {
                Linked_map_const_iterator i(*this);
                if (next_)
                {
                    val_ = next_->val_;
                    prev_ = next_->prev_;
                    next_ = next_->next_;
                }
                return i;
            }
            Linked_map_const_iterator& operator++()
            {
                if (next_)
                {
                    val_ = next_->val_;
                    prev_ = next_->prev_;
                    next_ = next_->next_;
                }
                return *this;
            }
        private:
            value_type* val_;
            Linked_map_iterator* prev_;
            Linked_map_iterator* next_;
        };
        
        typedef Linked_map_iterator iterator;
        typedef Linked_map_const_iterator const_iterator;
        
        Linked_map() : h_(0), t_(0), m_()
        {
            h_ = new iterator(0, 0, 0);
            t_ = new iterator(0, h_, 0);
            h_->next_ = t_;
        }
        
        Linked_map(const Linked_map<K, V>& o) : h_(0), t_(0), m_()
        {
            h_ = new iterator(0, 0, 0);
            t_ = new iterator(0, h_, 0);
            h_->next_ = t_;
            
            for (iterator i = o.begin();
                 o.end() != i;
                 ++i)
            {
                insert(*i);
            }
        }
        
        Linked_map<K, V>& operator=(const Linked_map<K, V>& o)
        {
            clear();
            
            for (iterator i = o.begin();
                 o.end() != i;
                 ++i)
            {
                insert(*i);
            }
        }
        
        iterator begin()
        {
            return *h_->next_;
        }
        
        iterator end()
        {
            return *t_;
        }
        
        const_iterator begin() const
        {
            return *h_->next_;
        }
        
        const_iterator end() const
        {
            return *t_;
        }
        
        iterator find(K key) const
        {
            typename std::map<K, iterator*>::const_iterator iter = m_.find(key);
            if (m_.end() == iter)
            {
                return *t_;
            }
            return *iter->second;
        }
        
        typedef typename std::map<K, iterator*>::iterator internal_map_iterator;
        
        std::pair<iterator, bool> insert(const_reference v)
        {
            iterator* w = new iterator(new value_type(v),
                                       t_->prev_,
                                       t_);
            std::pair<typename std::map<K, iterator*>::iterator, bool> i = m_.insert(std::pair<K, iterator*>(v.first, w));
            if(!i.second)
            {
                delete w->val_;
                delete w;
                return std::pair<iterator, bool>(*i.first->second, false);
            }
            else
            {
                t_->prev_->next_ = w;
                t_->prev_ = w;
                return std::pair<iterator, bool>(*w, true);
            }
        }
                
        void clear()
        {
            iterator *current = h_;
            while(current->next_ != t_)
            {
                current = current->next_;
                delete current->val_;
                delete current;
            }
            m_.clear();
            h_->next_ = t_;
            t_->prev_ = h_;
            h_->prev_ = 0;
            t_->next_ = 0;
        }

        void erase(K key)
        {
            if (m_.end() != m_.find(key))
            {
                iterator* w = m_.find(key)->second;
                iterator* wp = w->prev_;
                iterator* wn = w->next_;
                if (wp)
                {
                    wp->next_ = wn;
                }
                if (wn)
                {
                    wn->prev_ = wp;
                }
                delete w->val_;
                delete w;
                m_.erase(m_.find(key));
            }
        }
        
        void erase(iterator& w)
        {
            erase(w.val_->first);
        }
        
        size_t size() const
        {
            return m_.size();
        }
        
    private:
        iterator* h_;
        iterator* t_;
        std::map<K, iterator*> m_;
    };
}; // namespace lj;
