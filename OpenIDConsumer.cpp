#include "OpenIDConsumer.h"
#include "Model.h"

namespace {
    const char ASSOC_DB[] = "/var/db/logjammin/assoc.tcb";
    const char ASSOC_INDX_HANDLE[] = "/var/db/logjammin/assoc_handle.tcb";
    const char ASSOC_INDX_PROVIDER[] = "/var/db/logjammin/assoc_provider.tcb";
    
    struct AssociationWithPkey : public openid_1_1::AssociatedRelayProvider::Association, Model<AssociationWithPkey> {
        static void at(unsigned long long key, AssociationWithPkey *model);
        
        AssociationWithPkey() { }
        AssociationWithPkey(openid_1_1::AssociatedRelayProvider::Association *);
        AssociationWithPkey(unsigned long long key) { AssociationWithPkey::at(key, this); };
        
        const std::string serialize() const {
            std::ostringstream data;
            data << "assoc_type=\"" << escape(assoc_type) << "\";\n";
            data << "assoc_handle=\"" << escape(assoc_handle) << "\";\n";
            data << "provider=\"" << escape(provider) << "\";\n";
            data << "session_type=\"" << escape(session_type) << "\";\n";
            data << "dh_server_public=\"" << escape(dh_server_public) << "\";\n";
            data << "secret=\"" << escape(secret) << "\";\n";
            data << "expires_at=\"" << expires_at << "\";\n";
            return data.str();
        }
        void populate(OpenProp::File *props) {
            assoc_type.assign(props->getValue("assoc_type"));
            assoc_handle.assign(props->getValue("assoc_handle"));
            provider.assign(props->getValue("provider"));
            session_type.assign(props->getValue("session_type"));
            dh_server_public.assign(props->getValue("dh_server_public"));
            secret.assign(props->getValue("secret"));
            expires_at = atol(props->getValue("expires_at"));
        }
        virtual ModelDB<AssociationWithPkey> *dao() const;
    };
    
    class AssocDB : public ModelDB<AssociationWithPkey> {
        static void open_db_file(TCBDB *db, int mode) {
            tcbdbsetcmpfunc(db, tccmpint64, NULL);
            tcbdbtune(db, -1, -1, -1, -1, -1, BDBTLARGE | BDBTBZIP);
            tcbdbopen(db, ASSOC_DB, mode);
        }
        static void open_indx_handle_file(TCBDB *db, int mode) {
            tcbdbsetcmpfunc(db, tccmpint64, NULL);
            tcbdbtune(db, -1, -1, -1, -1, -1, BDBTLARGE | BDBTBZIP);
            tcbdbopen(db, ASSOC_INDX_HANDLE, mode);
        }
        static void open_indx_provider_file(TCBDB *db, int mode) {
            tcbdbsetcmpfunc(db, tccmpint64, NULL);
            tcbdbtune(db, -1, -1, -1, -1, -1, BDBTLARGE | BDBTBZIP);
            tcbdbopen(db, ASSOC_INDX_PROVIDER, mode);
        }
    public:
        tokyo::Index<unsigned long long, std::string> index_handle, index_provider;
        
        AssocDB() :
        ModelDB<AssociationWithPkey>(&open_db_file, BDBOREADER | BDBOWRITER | BDBOCREAT),
        index_handle(&open_indx_handle_file, BDBOREADER | BDBOWRITER | BDBOCREAT),
        index_provider(&open_indx_provider_file, BDBOREADER | BDBOWRITER | BDBOCREAT)
        {
        }
        
        virtual void put(AssociationWithPkey *model) {
            try {
                begin_transaction();
                index_handle.begin_transaction();
                index_provider.begin_transaction();
                
                // Clean up the indicies
                if(model->pkey() != 0) {
                    AssociationWithPkey assoc(model->pkey());
                    index_handle.remove(assoc.assoc_handle, model->pkey());
                    index_provider.remove(assoc.provider, model->pkey());
                }
                
                // Destroy previous entries for these indicies
                std::set<unsigned long long> tmp = index_handle.is(model->assoc_handle);
                for(std::set<unsigned long long>::const_iterator iter = tmp.begin();
                    iter != tmp.end();
                    ++iter) {
                    index_handle.remove(model->assoc_handle, *iter);
                }
                
                tmp = index_provider.is(model->provider);
                for(std::set<unsigned long long>::const_iterator iter = tmp.begin();
                    iter != tmp.end();
                    ++iter) {
                    index_provider.remove(model->provider, *iter);
                }
                
                // Get the primary key for new objects.
                unsigned long long key = model->pkey();
                if(key == 0) {
                    try {
                        key = max() + 1;
                    } catch(tokyo::Exception &ex) {
                        key = 1;
                    }
                }
                
                // Store the records.
                this->_put(key, model->serialize());
                index_handle.put(model->assoc_handle, key);
                index_provider.put(model->provider, key);
                
                index_provider.commit_transaction();
                index_handle.commit_transaction();
                commit_transaction();
                
                set_pkey(model, key);
            } catch (tokyo::Exception &ex) {
                index_provider.abort_transaction();
                index_handle.abort_transaction();
                abort_transaction();
                throw ex;
            } catch (std::string &ex) {
                index_provider.abort_transaction();
                index_handle.abort_transaction();
                abort_transaction();
                throw ex;
            }
        }
        
        virtual void remove(AssociationWithPkey *model) {
            if(model->pkey() != 0) {
                try {
                    begin_transaction();
                    index_handle.begin_transaction();
                    index_provider.begin_transaction();
                    
                    AssociationWithPkey assoc(model->pkey());
                    this->_remove(model->pkey());
                    index_handle.remove(assoc.assoc_handle, model->pkey());
                    index_provider.remove(assoc.provider, model->pkey());
                    
                    index_provider.commit_transaction();
                    index_handle.commit_transaction();
                    commit_transaction();
                    
                    set_pkey(model, 0);
                } catch (tokyo::Exception &ex) {
                    index_provider.abort_transaction();
                    index_handle.abort_transaction();
                    abort_transaction();
                    throw ex;
                } catch (std::string &ex) {
                    index_provider.abort_transaction();
                    index_handle.abort_transaction();
                    abort_transaction();
                    throw ex;
                }
            }
        }            
    };
    
    void AssociationWithPkey::at(unsigned long long key, AssociationWithPkey *model) {
        AssocDB dao;
        dao.at(key, model);
    }
    
    ModelDB<AssociationWithPkey> *AssociationWithPkey::dao() const {
        return new AssocDB();
    }
    
    AssociationWithPkey::AssociationWithPkey(openid_1_1::AssociatedRelayProvider::Association *src) {
        assoc_type.assign(src->assoc_type);
        assoc_handle.assign(src->assoc_handle);
        provider.assign(src->provider);
        session_type.assign(src->session_type);
        dh_server_public.assign(src->dh_server_public);
        secret.assign(src->secret);
        expires_at = src->expires_at;
    }

};

LogJamminConsumer::LogJamminConsumer(const std::string &identifier) : AssociatedRelayProvider(identifier) {
}

void LogJamminConsumer::invalidate_assoc_handle(const std::string &assoc_handle) {
    std::cerr << "invalidating handle " << assoc_handle << std::endl;
    AssocDB dao;
    std::set<unsigned long long> tmp(dao.index_handle.is(assoc_handle));
    for(std::set<unsigned long long>::const_iterator iter = tmp.begin();
        iter != tmp.end();
        ++iter) {
        AssociationWithPkey assoc(*iter);
        dao.remove(&assoc);
    }
}

std::string *LogJamminConsumer::lookup_assoc_handle(const std::string &provider) {
    std::cerr << "looking up handle by provider " << provider << std::endl;

    AssocDB dao;
    std::set<unsigned long long> tmp(dao.index_provider.is(provider));
    if(!tmp.size())
        return NULL;
    AssociationWithPkey assoc(*(tmp.begin()));

    std::cerr << "    Handle is " << assoc.assoc_handle << std::endl;
    return new std::string(assoc.assoc_handle);
}

LogJamminConsumer::Association *LogJamminConsumer::lookup_association(const std::string &assoc_handle) {
    std::cerr << "looking up association by handle " << assoc_handle << std::endl;

    AssocDB dao;
    std::set<unsigned long long> tmp(dao.index_handle.is(assoc_handle));
    if(!tmp.size())
        return NULL;

    AssociationWithPkey *ptr = new AssociationWithPkey(*(tmp.begin()));
    std::cerr << ptr->serialize() << std::endl;
    return ptr;
}

void LogJamminConsumer::store_assoc_handle(const Association *association) {
    AssocDB dao;
    AssociationWithPkey assoc((openid_1_1::AssociatedRelayProvider::Association *)association);
    std::cerr << "Storing Assocation " << assoc.serialize() << std::endl;
    dao.put(&assoc);
}

