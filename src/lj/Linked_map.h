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
    //! Insertion order iterating STL map.
    /*!
     \par
     This class is a wrapper for a std::map.  The additional functionality is
     provided to ensure that insertion order is preserved when iterating over
     the items.
     \par
     The functionality is done by implementating a bi-direction linked list
     ontop of the map.
     \tparam K The key type.
     \tparam V The value type.
     \author Jason Watson
     \version 1.0
     \date May 2, 2010
     */
    template<typename K, typename V>
    class Linked_map
    {
    public:
        typedef typename std::pair<K, V> value_type;                       //!< Type of stored values.
        typedef typename std::allocator<value_type> allocator_type;        //!< Type of the allocator.
        typedef typename allocator_type::size_type size_type;              //!< Type of the size.
        typedef typename allocator_type::difference_type difference_type;  //!< Type of the pointer difference.
        typedef value_type* pointer;                 //!< Type of the value pointer.
        typedef const value_type* const_pointer;     //!< Type of const value pointer.
        typedef value_type& reference;               //!< Type of the value reference.
        typedef const value_type& const_reference;   //!< Type of the const value reference.
        
        class Linked_map_const_iterator;
 
        //! Mostly STL style iterator for Linked_map.
        /*!
         \author Jason Watson
         \version 1.0
         \date May 2, 2010
         */
        class Linked_map_iterator : public std::iterator<std::forward_iterator_tag, typename Linked_map::value_type>
        {
            friend class Linked_map;
            friend class Linked_map_const_iterator;
        public:
            typedef typename Linked_map<K, V>::pointer pointer;                 //!< Type of the value pointer.
            typedef typename Linked_map<K, V>::const_pointer const_pointer;     //!< Type of const value pointer.
            typedef typename Linked_map<K, V>::reference reference;             //!< Type of the value reference.
            typedef typename Linked_map<K, V>::const_reference const_reference; //!< Type of the const value reference.

            //! Create a new linked map iterator.
            /*!
             \param v The value pointer.
             \param prev The previous node in the linked list.
             \param next The next node in the linked list.
             */
            Linked_map_iterator(pointer v, Linked_map_iterator* prev, Linked_map_iterator* next) : val_(v), prev_(prev), next_(next)
            {
            }
            
            //! Copy constructor.
            /*!
             \param o The original.
             */
            Linked_map_iterator(const Linked_map_iterator& o) : val_(o.val_), prev_(o.prev_), next_(o.next_)
            {
            }
            
            //! Reference cast operator.
            /*!
             \return Reference to the value type.
             */
            operator reference()
            {
                return *val_;
            }
            
            //! Constant reference cast operator.
            /*!
             \return Constant reference to the value type.
             */
            operator const_reference() const
            {
                return *val_;
            }
            
            //! Assignment operator.
            /*!
             \param o The original
             \return The assigned object.
             */
            Linked_map_iterator& operator=(const Linked_map_iterator& o)
            {
                val_ = o.val_;
                prev_ = o.prev_;
                next_ = o.next_;
                return *this;
            }
            
            //! Equals operator
            /*!
             \param o The other object.
             \return true if the previous and next nodes are equal,
             false otherwise.
             */
            bool operator==(const Linked_map_iterator& o) const
            {
                return (prev_ == o.prev_ &&
                        next_ == o.next_);
            }
            
            //! Not equals operator
            /*!
             \param o The other object.
             \return false if the previous or next nodes are equal,
             true otherwise.
             */
            bool operator!=(const Linked_map_iterator& o) const
            {
                return (prev_ != o.prev_ ||
                        next_ != o.next_);
            }
            
            //! Dereference operator.
            /*!
             \return Reference to the current value of this iterator.
             */
            reference operator*()
            {
                return *val_;
            }
            
            //! Constant dereference operator.
            /*!
             \return Constant reference to the current value of this iterator.
             */
            const_reference operator*() const
            {
                return *val_;
            }
            
            //! Member by pointer operator.
            /*!
             \return Pointer to the current value of this iterator.
             */
            pointer operator->()
            {
                return val_;
            }
            
            //! Constant member by pointer operator.
            /*!
             \return Constant pointer to the current value of this iterator.
             */
            const_pointer operator->() const
            {
                return val_;
            }
            
            //! Postfix increment operator.
            /*!
             \param foo Unused.
             \return A copy of the iterator.
             */
            Linked_map_iterator operator++(int foo)
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
            
            //! Prefix increment operator.
            /*!
             \return The iterator object.
             */
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
            pointer val_;
            Linked_map_iterator* prev_;
            Linked_map_iterator* next_;
        };

        //! Mostly STL style const iterator for Linked_map.
        /*!
         \author Jason Watson
         \version 1.0
         \date May 2, 2010
         */
        class Linked_map_const_iterator : public std::iterator<std::forward_iterator_tag, typename Linked_map::value_type>
        {
            friend class Linked_map;
        public:
            typedef typename Linked_map<K, V>::pointer pointer;                 //!< Type of the value pointer.
            typedef typename Linked_map<K, V>::const_pointer const_pointer;     //!< Type of const value pointer.
            typedef typename Linked_map<K, V>::reference reference;             //!< Type of the value reference.
            typedef typename Linked_map<K, V>::const_reference const_reference; //!< Type of the const value reference.
            
            //! Create a new linked map const iterator.
            /*!
             \param v The value pointer.
             \param prev The previous node in the linked list.
             \param next The next node in the linked list.
             */
            Linked_map_const_iterator(pointer v, Linked_map_iterator* prev, Linked_map_iterator* next) : val_(v), prev_(prev), next_(next)
            {
            }
            
            //! Conversion constructor.
            /*!
             \param o The original.
             */
            Linked_map_const_iterator(const Linked_map_iterator& o) : val_(o.val_), prev_(o.prev_), next_(o.next_)
            {
            }
            
            //! Copy constructor.
            /*!
             \param o The original.
             */
            Linked_map_const_iterator(const Linked_map_const_iterator& o) : val_(o.val_), prev_(o.prev_), next_(o.next_)
            {
            }
            
            //! Constant reference cast operator.
            /*!
             \return Constant reference to the value type.
             */
            operator const_reference() const
            {
                return *val_;
            }
            
            //! Assignment operator.
            /*!
             \param o The original.
             \return The assigned object.
             */
            Linked_map_const_iterator& operator=(const Linked_map_const_iterator& o)
            {
                val_ = o.val_;
                prev_ = o.prev_;
                next_ = o.next_;
                return *this;
            }
            
            //! Conversion assignment operator.
            /*!
             \param o The original.
             \return The assigned object.
             */
            Linked_map_const_iterator& operator=(const Linked_map_iterator& o)
            {
                val_ = o.val_;
                prev_ = o.prev_;
                next_ = o.next_;
                return *this;
            }
            
            //! Equals operator
            /*!
             \param o The other object.
             \return true if the previous and next nodes are equal,
             false otherwise.
             */
            bool operator==(const Linked_map_const_iterator& o) const
            {
                return (prev_ == o.prev_ &&
                        next_ == o.next_);
            }
            
            //! Not equals operator
            /*!
             \param o The other object.
             \return false if the previous or next nodes are equal,
             true otherwise.
             */
            bool operator!=(const Linked_map_const_iterator& o) const
            {
                return (prev_ != o.prev_ ||
                        next_ != o.next_);
            }
            
            //! Constant dereference operator.
            /*!
             \return Constant reference to the current value of this iterator.
             */
            const_reference operator*() const
            {
                return *val_;
            }
            
            //! Constant member by pointer operator.
            /*!
             \return Constant pointer to the current value of this iterator.
             */
            const_pointer operator->() const
            {
                return val_;
            }
            
            //! Postfix increment operator.
            /*!
             \param foo Unused.
             \return A copy of the iterator.
             */
            Linked_map_const_iterator operator++(int foo)
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
            
            //! Prefix increment operator.
            /*!
             \return The iterator object.
             */
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
            pointer val_;
            Linked_map_iterator* prev_;
            Linked_map_iterator* next_;
        };
        
        typedef Linked_map_iterator iterator;              //!< Type of standard forward iterator.
        typedef Linked_map_const_iterator const_iterator;  //!< Type of const forward iterator.
        
        //! Default constructor.
        Linked_map() : h_(0), t_(0), m_()
        {
            h_ = new iterator(0, 0, 0);
            t_ = new iterator(0, h_, 0);
            h_->next_ = t_;
        }
        
        //! Copy constructor.
        /*!
         \param o The original.
         */
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
        
        ~Linked_map()
        {
            iterator* ptr = h_;
            while (ptr)
            {
                iterator* tmp = ptr;
                ptr = ptr->next_;
                if (tmp->val_)
                {
                    delete tmp->val_;
                }
                
                delete tmp;
            }
        }
        
        //! Assignment operator.
        /*!
         \param o The original.
         \return The assigned map.
         */
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
        
        //! Iterator before the first inserted record.
        /*!
         \return iterator.
         */
        iterator begin()
        {
            return *h_->next_;
        }
        
        //! Iterator after the last inserted record.
        /*!
         \return Last iterator.
         */
        iterator end()
        {
            return *t_;
        }
        
        //! Const iterator before the first inserted record.
        /*!
         \return const iterator.
         */
        const_iterator begin() const
        {
            return *h_->next_;
        }
        
        //! Const iterator after the last inserted record.
        /*!
         \return const iterator.
         */
        const_iterator end() const
        {
            return *t_;
        }
        
        //! Find value.
        /*!
         \par
         returns end() if the key was not found.
         \param key The key to search for.
         \return iterator.
         */
        iterator find(K key) const
        {
            typename std::map<K, iterator*>::const_iterator iter = m_.find(key);
            if (m_.end() == iter)
            {
                return *t_;
            }
            return *iter->second;
        }
        
        //! Insert into the Linked_map.
        /*!
         \param v The value to insert.
         \return A pair containing the iterator of where the value was inserted,
         and a bool denoting if it was successful or not.
         */
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
        
        //! Remove all entries from the Linked_map.
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

        //! remove a specific key.
        /*!
         \param key The key to remove.
         */
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
        
        //! remove a specific iterator.
        /*!
         \param w The iterator to remove.
         */
        void erase(iterator& w)
        {
            erase(w.val_->first);
        }
        
        //! Number of items in the Linked_map.
        /*!
         \return The number of items in the linked map.
         */
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
