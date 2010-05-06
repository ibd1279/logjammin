#pragma once
/*
 *  LinkedMap.h
 *  logjammin
 *
 *  Created by Jason Watson on 5/3/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
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
 
        class Linked_map_iterator : public std::iterator<std::forward_iterator_tag, typename Linked_map::value_type>
        {
            friend class Linked_map;
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
                if (!next_) {
                    val_ = 0;
                    prev_ = 0;
                    next_ = 0;
                }
                else
                {
                    val_ = next_->val_;
                    prev_ = next_->prev_;
                    next_ = next_->next_;
                }
                return i;
            }
            Linked_map_iterator& operator++()
            {
                if (!next_) {
                    val_ = 0;
                    prev_ = 0;
                    next_ = 0;
                }
                else
                {
                    val_ = next_->val_;
                    prev_ = next_->prev_;
                    next_ = next_->next_;
                }
                return *this;
            }
        private:
            value_type* val_;
            Linked_map_iterator prev_;
            Linked_map_iterator next_;
        };
        
        typedef Linked_map_iterator iterator;
        
        Linked_map() : h_(0), t_(0), m_()
        {
            h_ = new iterator(0, 0, 0);
            t_ = new iterator(0, h_, 0);
            h_->next_ = t_;
        }
        
        iterator& begin()
        {
            return *h_->next_;
        }
        
        iterator& end()
        {
            return *t_;
        }
        
        iterator find(K key)
        {
            if (m_.end() == m_.find(key))
            {
                return *t_;
            }
            return *m_.find(key)->second;
        }
        
        bool insert(value_type& v)
        {
            iterator* w = new iterator(new value_type(v),
                                       t_->prev_,
                                       t_);
            if(!m_.insert(std::pair<K, iterator*>(v)).second)
            {
                delete w->val_;
                delete w;
                return false;
            }
            else
            {
                t_->prev_->next_ = w;
                t_->prev_ = w;
                return true;
            }
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
                delete w->key_;
                delete w->item_;
                delete w;
                m_.erase(m_.find(key));
            }
        }
        
    private:
        iterator* h_;
        iterator* t_;
        std::map<K, iterator*> m_;
    };
}; // namespace lj;
