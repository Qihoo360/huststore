#ifndef _appini_h_
#define _appini_h_

#include "db_stdinc.h"

    struct ini_t;
typedef struct ini_t ini_t;

#define INI_SECTION_COUNT        32
#define INI_ITEM_COUNT           64
#define G_APPINI                 appini_t::get_appini ()

typedef struct
{
    const char * key;
    const char * val;
} ini_item;

typedef struct
{
    const char * name;
    ini_item items[ INI_ITEM_COUNT ];
    size_t items_count;
} ini_section;

struct ini_t
{
    char * buf;
    size_t len;
    ini_section sections[ INI_SECTION_COUNT ];
    size_t sections_count;
};

class appini_t
{
public:

    ~appini_t ( );

public:

    static appini_t * get_appini ( );
    static void kill_me ( );

public:

    ini_t * ini_create (
                         const char * path
                         );

    ini_t * ini_create_memory (
                                const char * content,
                                size_t content_len
                                );

    void ini_destroy (
                       ini_t * This
                       );

    int ini_open (
                   ini_t * ini,
                   const char * path
                   );

    int ini_open_memory (
                          ini_t * ini,
                          const char * content,
                          size_t content_len
                          );

    void ini_close (
                     ini_t * This
                     );

    size_t ini_get_sections_count (
                                    ini_t * ini
                                    );

    bool ini_get_sections (
                            ini_t * ini,
                            const char ** sections,
                            size_t * sections_count
                            );

    bool ini_get_bool (
                        ini_t * ini,
                        const char * section,
                        const char * name,
                        bool def
                        );

    int ini_get_int (
                      ini_t * ini,
                      const char * section,
                      const char * name,
                      int def
                      );

    long long ini_get_int64 (
                              ini_t * ini,
                              const char * section,
                              const char * name,
                              long long def
                              );

    const char * ini_get_string (
                                  ini_t * This,
                                  const char * section,
                                  const char * name,
                                  const char * def
                                  );

private:

    static appini_t * m_appini;

private:
    // disable
    appini_t ( );
    appini_t ( const appini_t & );
    const appini_t & operator= ( const appini_t & );
};

#endif
