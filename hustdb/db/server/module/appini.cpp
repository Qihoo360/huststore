#include "appini.h"
#include "db_lib.h"

appini_t * appini_t::m_appini = NULL;

appini_t::appini_t ( )
{
    m_appini = NULL;
}

appini_t::~ appini_t ( )
{
}

appini_t * appini_t::get_appini ( )
{
    return ( m_appini == NULL ) ? m_appini = new appini_t () : m_appini;
}

void appini_t::kill_me ( )
{
    if ( NULL != m_appini )
    {
        delete m_appini;
        m_appini = NULL;
    }
}

static inline
int ini_open2 (
                ini_t * ini
                )
{
    char *              p = ini->buf;
    char *              p2;
    char *              p3;
    char *              line;
    size_t              len;
    ini_section *    section = NULL;
    bool                is_err = false;

    while ( 1 )
    {

        p2 = strchr ( p, '\n' );

        if ( p2 )
        {
            * p2 = '\0';
            if ( p2 != ini->buf )
            {
                if ( '\r' == * ( p2 - 1 ) )
                {
                    * ( p2 - 1 ) = '\0';
                }
            }
        }

        line = p;

        do
        {

            if ( '\0' == * line )
                break;
            if ( '#' == line[ 0 ] || ';' == line[ 0 ] )
                break;

            if ( '[' == line[ 0 ] )
            {
                ++ line;

                len = strlen ( line );
                if ( len <= 1 )
                    break;

                if ( ']' == line[ len - 1 ] )
                    line[ len - 1 ] = '\0';

                if ( ini->sections_count >= COUNT_OF ( ini->sections ) )
                {
                    is_err = true;
                    break;
                }

                section = & ini->sections[ ini->sections_count ];

                section->name = line;
                ++ ini->sections_count;

            }
            else
            {

                if ( NULL == section )
                {
                    is_err = true;
                    break;
                }

                len = strlen ( line );
                if ( len <= 1 )
                {
                    is_err = true;
                    break;
                }

                p3 = strchr ( line, '=' );
                if ( NULL == p3 )
                {
                    is_err = true;
                    break;
                }

                * p3 = '\0';
                ++ p3;

                if ( '\0' == * line )
                    break;

                line = str_trim ( line, NULL );
                if ( NULL == line || '\0' == * line )
                    break;

                p3 = str_trim ( p3, NULL );
                if ( NULL == p3 )
                    p3 = ( char* ) "";

                if ( section->items_count >= COUNT_OF ( section->items ) )
                {
                    is_err = true;
                    break;
                }

                section->items[ section->items_count ].key = line;
                section->items[ section->items_count ].val = p3;

                ++ section->items_count;
            }

        }
        while ( 0 );

        if ( NULL == p2 )
        {
            break;
        }
        else
        {
            p = p2 + 1;
        }
    }

    if ( is_err )
    {
        return - 1;
    }

    return 0;
}

int appini_t::ini_open (
                         ini_t * ini,
                         const char * path
                         )
{
    FILE * fp;
    long len;
    int r;
    char ph[ MAX_PATH ];

    strncpy ( ph, path, sizeof ( ph ) );
    ph[ sizeof ( ph ) - 1 ] = '\0';

    memset ( ini, 0, sizeof ( ini_t ) );

    fp = fopen ( ph, "rb" );
    if ( NULL == fp )
    {
        return - 1;
    }

    fseek ( fp, 0, SEEK_END );

    len = ftell ( fp );
    if ( ( long ) - 1 == len || 0 == len )
    {
        fclose ( fp );
        return - 2;
    }

    fseek ( fp, 0, SEEK_SET );

    ini->len = ( size_t ) len;
    ini->buf = ( char * ) calloc ( 1, len + 1 );
    if ( NULL == ini->buf )
    {
        fclose ( fp );
        return - 3;
    }

    if ( ini->len != fread ( ini->buf, 1, ini->len, fp ) )
    {
        free ( ini->buf );
        ini->buf = NULL;
        fclose ( fp );
        return - 4;
    }

    fclose ( fp );

    r = ini_open2 ( ini );
    if ( 0 != r )
    {
        free ( ini->buf );
        ini->buf = NULL;
        return r;
    }

    return 0;
}

int appini_t::ini_open_memory (
                                ini_t * ini,
                                const char * content,
                                size_t content_len
                                )
{
    int r;

    memset ( ini, 0, sizeof ( ini_t ) );

    ini->len = content_len;
    ini->buf = ( char * ) calloc ( 1, content_len + 1 );
    if ( NULL == ini->buf )
    {
        return - 3;
    }

    fast_memcpy ( ini->buf, content, content_len );
    ini->buf[ content_len ] = '\0';

    r = ini_open2 ( ini );
    if ( 0 != r )
    {
        free ( ini->buf );
        ini->buf = NULL;
        return r;
    }

    return 0;
}

void appini_t::ini_close (
                           ini_t * ini
                           )
{
    if ( NULL == ini )
    {
        return;
    }

    if ( ini->buf )
    {
        free ( ini->buf );
        ini->buf = NULL;
    }

    ini->len = 0;
}

static inline
ini_section * ini_find_section (
                                 ini_t * ini,
                                 const char * section
                                 )
{
    size_t i;

    if ( NULL == section || '\0' == * section )
    {
        return NULL;
    }

    for ( i = 0; i < ini->sections_count; ++ i )
    {

        ini_section * p = & ini->sections[ i ];

        if ( 0 == stricmp ( p->name, section ) )
        {
            return p;
        }
    }

    return NULL;
}

static inline
const char * ini_find_in_section (
                                   ini_section * ini,
                                   const char * name
                                   )
{
    size_t i;

    if ( 0 == name || '\0' == * name )
    {
        return NULL;
    }

    for ( i = 0; i < ini->items_count; ++ i )
    {

        if ( 0 == stricmp ( ini->items[ i ].key, name ) )
            return ini->items[ i ].val;
    }

    return NULL;
}

size_t appini_t::ini_get_sections_count (
                                          ini_t * ini
                                          )
{
    if ( ini )
    {
        return ini->sections_count;
    }
    else
    {
        return 0;
    }
}

bool appini_t::ini_get_sections (
                                  ini_t * ini,
                                  const char ** sections,
                                  size_t * sections_count
                                  )
{
    size_t i;

    if ( NULL == ini || NULL == sections || NULL == sections_count || 0 == * sections_count )
    {
        return false;
    }

    for ( i = 0; i != ini->sections_count && i < * sections_count; ++ i )
    {
        sections[ i ] = ini->sections[ i ].name;
    }

    * sections_count = ini->sections_count;
    return true;
}

bool appini_t::ini_get_bool (
                              ini_t * ini,
                              const char * section,
                              const char * name,
                              bool def
                              )
{
    ini_section * p;
    const char * v;

    if ( NULL == ini || NULL == section || '\0' == * section || NULL == name || '\0' == * name )
    {
        return def;
    }

    p = ini_find_section ( ini, section );
    if ( NULL == p )
    {
        return def;
    }

    v = ini_find_in_section ( p, name );
    if ( NULL == v )
    {
        return def;
    }

    if ( 0 == stricmp ( "false", v ) || 0 == stricmp ( "no", v ) )
        return false;
    else if ( 0 == stricmp ( "true", v ) || 0 == stricmp ( "yes", v ) )
        return true;
    return def;
}

int appini_t::ini_get_int (
                            ini_t * ini,
                            const char * section,
                            const char * name,
                            int def
                            )
{
    ini_section * p;
    const char * v;

    if ( NULL == ini || NULL == section || '\0' == * section || NULL == name || '\0' == * name )
    {
        return def;
    }

    p = ini_find_section ( ini, section );
    if ( NULL == p )
    {
        return def;
    }

    v = ini_find_in_section ( p, name );
    if ( NULL == v )
    {
        return def;
    }

    if ( '\0' == * v || v[ 0 ] <= 0 || ( '-' != v[ 0 ] && ! isdigit ( v[ 0 ] ) ) )
        return def;

    return atoi ( v );
}

long long appini_t::ini_get_int64 (
                                    ini_t * ini,
                                    const char * section,
                                    const char * name,
                                    long long def
                                    )
{
    ini_section * p;
    const char * v;

    if ( NULL == ini || NULL == section || '\0' == * section || NULL == name || '\0' == * name )
    {
        return def;
    }

    p = ini_find_section ( ini, section );
    if ( NULL == p )
    {
        return def;
    }

    v = ini_find_in_section ( p, name );
    if ( NULL == v )
    {
        return def;
    }

    if ( '\0' == * v || v[ 0 ] <= 0 || ( '-' != v[ 0 ] && ! isdigit ( v[ 0 ] ) ) )
        return def;

#if defined( WIN32 ) || defined( WIN64 )
    return ( int64_t ) _atoi64 ( v );
#else
    return ( int64_t ) atoll ( v );
#endif
}

const char * appini_t::ini_get_string (
                                        ini_t * ini,
                                        const char * section,
                                        const char * name,
                                        const char * def
                                        )
{
    ini_section * p;
    const char * v;

    if ( NULL == ini || NULL == section || '\0' == * section || NULL == name || '\0' == * name )
    {
        return def;
    }

    p = ini_find_section ( ini, section );
    if ( NULL == p )
    {
        return def;
    }

    v = ini_find_in_section ( p, name );
    if ( NULL == v )
    {
        return def;
    }

    return v;
}

ini_t * appini_t::ini_create (
                               const char * path
                               )
{
    ini_t * ini;
    int     r;

    ini = ( ini_t * ) calloc ( 1, sizeof ( ini_t ) );
    if ( NULL == ini )
    {
        return NULL;
    }

    r = ini_open ( ini, path );
    if ( 0 != r )
    {
        free ( ini );
        return NULL;
    }

    return ini;
}

ini_t * appini_t::ini_create_memory (
                                      const char * content,
                                      size_t content_len
                                      )
{
    ini_t * ini;
    int     r;

    ini = ( ini_t * ) calloc ( 1, sizeof ( ini_t ) );
    if ( NULL == ini )
    {
        return NULL;
    }

    r = ini_open_memory ( ini, content, content_len );
    if ( 0 != r )
    {
        free ( ini );
        return NULL;
    }

    return ini;
}

void appini_t::ini_destroy (
                             ini_t * This
                             )
{
    if ( This )
    {
        ini_close ( This );
        free ( This );
    }
}
