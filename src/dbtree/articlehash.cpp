// ライセンス: GPL2

//#define _DEBUG
#include "jddebug.h"

#include "articlehash.h"
#include "articlebase.h"

#include <algorithm>
#include <stdlib.h> // atoi


using namespace DBTREE;


enum
{
    HASH_TBLSIZE = 1024
};


ArticleHash::ArticleHash()
    : m_min_hash( HASH_TBLSIZE + 1 )
{
}


ArticleHash::~ArticleHash()
{
#ifdef _DEBUG
    if( size() ){

        std::cout << "ArticleHash::~ArticleHash\n";
        const size_t tblsize = m_table.size();
        for( size_t hash = 0; hash < tblsize; ++hash ){
            if( m_table[ hash ].size())  std::cout << hash << " : size = " << m_table[ hash ].size() << std::endl;
        }
    }
#endif
}


int ArticleHash::get_hash( const std::string& id ) const
{
    const size_t hash = atoi( id.c_str() ) & ( HASH_TBLSIZE -1 );

#ifdef _DEBUG
    std::cout << id << " -> " << hash << std::endl;
#endif

    return hash;
}


ArticleBase* ArticleHash::insert( std::unique_ptr<ArticleBase> article )
{
    if( ! m_table.size() ){
        m_table.resize( HASH_TBLSIZE );
    }

    const size_t hash = get_hash( article->get_id() );

    if( hash < m_min_hash ) m_min_hash = hash;
    ++m_size;
    m_table[ hash ].push_back( std::move( article ) );
    return m_table[ hash ].back().get();
}


ArticleBase* ArticleHash::find( const std::string& datbase, const std::string& id )
{
    if( ! m_table.size() ) return nullptr;

    const size_t hash = get_hash( id );

    auto& block = m_table[ hash ];
    auto it = std::find_if( block.begin(), block.end(),
                            [&datbase, &id]( auto& a ) { return a->equal( datbase, id ); } );
    if( it != block.end() ) return it->get();

    return nullptr;
}


ArticleHashIterator ArticleHash::begin()
{
    m_it_hash = m_min_hash;
    m_it_pos = 0;
    m_it_size = 0;

    return ArticleHashIterator{ this };
}


ArticleBase* ArticleHash::it_get()
{
    if( m_it_hash >= m_table.size() ) return nullptr;

    return m_table[ m_it_hash ][ m_it_pos ].get();
}


void ArticleHash::it_inc()
{
#ifdef _DEBUG
    std::cout << "ArticleHash::it_inc hash = " << m_it_hash << " pos = " << m_it_pos;
#endif

    ++m_it_size;
    if( m_it_size < size() ){

        ++m_it_pos;
        if( m_it_pos == m_table[ m_it_hash ].size() ){

            m_it_pos = 0;
            while( ! m_table[ ++m_it_hash ].size() );
        }
    }

#ifdef _DEBUG
    std::cout << " -> hash = " << m_it_hash << " tablesize = " << m_table[ m_it_hash ].size()
              << " pos = " << m_it_pos << " size = " << m_it_size << " / " << size() << std::endl;
#endif
}


/////////////////////////////////////////////////////


ArticleHashIterator::ArticleHashIterator( ArticleHash* hashtable )
    : m_hashtable( hashtable )
{}


ArticleBase* ArticleHashIterator::operator * ()
{
    return m_hashtable->it_get();
}


ArticleBase* ArticleHashIterator::operator ++ ()
{
    m_hashtable->it_inc();
    return m_hashtable->it_get();
}


bool ArticleHashIterator::operator != ( const size_t size )
{
    return ( m_hashtable->it_size() != size );
}
