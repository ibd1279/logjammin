#pragma once
#include <string>
#include <sstream>
#include "openprop.h"
#include "Tokyo.h"

//! Base Class for Object/Relational Mapping.
/*!
 Provides a base for implementing more complicated model DB objects. Methods in
 this class should be overridden to perform additional commands necessary for
 maintaining index integrity.
 \author Jason Watson
 \version 1.0
 \date July 3, 2009
 */
template<class V>
class ModelDB : public tokyo::DB<unsigned long long, std::string> {
public:
    //! Create a new model DB object.
    /*!
     \param db_open_func Method for opening the database.
     \param mode Mode to open the database in.
     */
    ModelDB(void (*db_open_func)(TCBDB *, int), int mode) : tokyo::DB<unsigned long long, std::string>(db_open_func, mode) {
    }
    
    //! Store a record in the database.
    /*!
     Replaces the existing record if it alrady exists.
     \param model the object to store.
     \exception TokyoException When the database cannot be written.
     */
    virtual void put(V *model) = 0;
    
    //! Get a record from the database.
    /*!
     \param key The primary key of the object to get.
     \param model The object to populate with data.
     \exception TokyoException When the database cannot be read.
     */
    virtual void at(const unsigned long long key, V *model) {
        // Get data and turn it into a stream.
        std::string v = _at(key);
        std::istringstream data(v);
        
        // Convert the stream into a map.
        OpenProp::File *record = OpenProp::File::Load(data);
        if(!record)
            throw std::string("Unable to parse DB record");
        
        // Populate the current instance with the map.
        model->populate(record);
        delete record;
        
        set_pkey(model, key);
    }
    
    //! Get all the records from the database.
    /*!
     \param results List to place the results into.
     */
    virtual void all(std::list<V *> &results) {
        BDBCUR *cur = begin();
        
        do {
            std::string v = cursor_value(cur);
            std::istringstream data(v);
            
            // Convert the stream into a map.
            OpenProp::File *record = OpenProp::File::Load(data);
            if(!record)
                throw std::string("Unable to parse DB record");
            
            // Populate the current instance with the map.
            V *model = new V();
            model->populate(record);
            delete record;
            
            set_pkey(model, cursor_key(cur));
            results.push_back(model);
        } while(tcbdbcurnext(cur));
        tcbdbcurdel(cur);
    }
    
    //! Remove a record from the database.
    /*!
     \param model The model object to remove.
     \exception TokyoException when the database cannot be written.
     */
    virtual void remove(V *model) = 0;
    
protected:
    //! Method for setting the primary key.
    /*!
     The primary key cannot be directly set by inherited classes. This method
     gets around that limitation.
     \param model The model to set the primary key on.
     \param key The key value.
     */
    void set_pkey(V* model, unsigned long long key) {
        model->pkey(key);
    }
};

//! Baseclass for data storage.
/*!
 \author Jason Watson
 \version 1.0
 \date July 3, 2009
 */
template<class V>
class Model {
    friend class ModelDB<V>;
public:
    //! Save the current object into the database.
    virtual void save() {
        V *model = dynamic_cast<V *>(this);
        if(model == NULL)
            throw std::string("Invalid combination of Model and ModelDB.");
        
        ModelDB<V> *model_db = dao();
        model_db->put(model);
        delete model_db;
    }
    
    //! Remove the current object from the database.
    virtual void purge() {
        V *model = dynamic_cast<V *>(this);
        if(model == NULL)
            throw std::string("Invalid combination of Model and ModelDB.");
        
        ModelDB<V> *model_db = dao();
        model_db->remove(model);
        delete model_db;
    }
    
    //! Get the primary key for the current object.
    /*!
     The primary key should be positive for records that exist in the database.
     Zero for records that do not exist in the database.
     \return The primary key for the object.
     */
    virtual unsigned long long pkey() const { return _pkey; }

    //! Get the serialized version of this instance.
    /*!
     Instances should override this method to serialize the fields of the object.
     The output format should be in OpenProp format.
     \return The OpenProp string representing this object.
     */
    virtual const std::string serialize() const = 0;
    
    //! Convert a database record into an instance object.
    /*!
     \param props A structured object to load values from.
     \sa load().
     */
    virtual void populate(OpenProp::File *props) = 0;

protected:
    //! Set the primary key for the current object.
    /*!
     This method should only be called when populating, purging, or saving.
     \param key The primary key.
     */
    virtual void pkey(const unsigned long long key) { _pkey = key; }

    //! Get the DAO
    virtual ModelDB<V> *dao() const = 0;

    //! Create and open the DB object.
    Model() {
        _pkey = 0;
    }
    
    //! Close the database object.
    virtual ~Model() {
    }
    
    //! Helper method to escape strings for OpenProp format.
    /*!
     \param val The value to escape.
     \return The escaped version.
     \sa populate() and serialize().
     */
    static std::string escape(const std::string &val) {
        std::string::const_iterator iter = val.begin();
        std::string r;
        for(; iter != val.end(); ++iter) {
            char c = *iter;
            if(c == '\\' || c == '"')
                r.push_back('\\');
            else if(c == '\n')
                r.append("\\n\\");
            r.push_back(*iter);
        }
        return r;
    }

    //! Current primary key.
    unsigned long long _pkey;
};
