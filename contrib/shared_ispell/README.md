Shared ISpell Dictionary
========================
This PostgreSQL extension provides a shared ispell dictionary, i.e.
a dictionary that's stored in shared segment. The traditional ispell
implementation means that each session initializes and stores the
dictionary on it's own, which means a lot of CPU/RAM is wasted.

This extension allocates an area in shared segment (you have to
choose the size in advance) and then loads the dictionary into it
when it's used for the first time.

If you need just snowball-type dictionaries, this extension is not
really interesting for you. But if you really need an ispell
dictionary, this may save you a lot of resources.


Install
-------
Installing the extension is quite simple, especially if you're on 9.1.
In that case all you need to do is this:

    $ make install

and then (after connecting to the database)

    db=# CREATE EXTENSION shared_ispell;

If you're on pre-9.1 version, you'll have to do the second part manually
by running the SQL script (shared_ispell--x.y.sql) in the database. If
needed, replace MODULE_PATHNAME by $libdir.


Config
------
No the functions are created, but you still need to load the shared
module. This needs to be done from postgresql.conf, as the module
needs to allocate space in the shared memory segment. So add this to
the config file (or update the current values)

    # libraries to load
    shared_preload_libraries = 'shared_ispell'

    # config of the shared memory
    shared_ispell.max_size = 32MB

Yes, there's a single GUC variable that defines the maximum size of
the shared segment. This is a hard limit, the shared segment is not
extensible and you need to set it so that all the dictionaries fit
into it and not much memory is wasted.

To find out how much memory you actually need, use a large value
(e.g. 200MB) and load all the dictionaries you want to use. Then use
the shared_ispell_mem_used() function to find out how much memory
was actually used (and set the max_size GUC variable accordingly).

Don't set it exactly to that value, leave there some free space,
so that you can reload the dictionaries without changing the GUC
max_size limit (which requires a restart of the DB). Ssomething
like 512kB should be just fine.

The shared segment can contain several dictionaries at the same time,
the amount of memory is the only limit. There's no limit on number
of dictionaries / words etc. Just the max_size GUC variable.


Using the dictionary
--------------------
Technically, the extension defines a 'shared_ispell' template that
you may use to define custom dictionaries. E.g. you may do this

    CREATE TEXT SEARCH DICTIONARY czech_shared (
        TEMPLATE = shared_ispell,
        DictFile = czech,
        AffFile = czech,
        StopWords = czech
    );

    CREATE TEXT SEARCH CONFIGURATION public.czech_shared
        ( COPY = pg_catalog.simple );

    ALTER TEXT SEARCH CONFIGURATION czech_shared
        ALTER MAPPING FOR asciiword, asciihword, hword_asciipart,
                        word, hword, hword_part
        WITH czech_shared;

and then do the usual stuff, e.g.

    db=# SELECT ts_lexize('czech_shared', 'automobile');

or whatever you want.


Available functions
-------------------
The extension provides five management functions, that allow you to
manage and get info about the preloaded dictionaries. The first two
functions

    shared_ispell_mem_used()
    shared_ispell_mem_available()

allow you to get info about the shared segment (used and free memory)
e.g. to properly size the segment (max_size). Then there are functions
return list of dictionaries / stop lists loaded in the shared segment

    shared_ispell_dicts()
    shared_ispell_stoplists()

e.g. like this

    db=# SELECT * FROM shared_ispell_dicts();

     dict_name | affix_name | words | affixes |  bytes   
    -----------+------------+-------+---------+----------
     bulgarian | bulgarian  | 79267 |      12 |  7622128
     czech     | czech      | 96351 |    2544 | 12715000
    (2 rows)


    db=# SELECT * FROM shared_ispell_stoplists();

     stop_name | words | bytes 
    -----------+-------+-------
     czech     |   259 |  4552
    (1 row)

The last function allows you to reset the dictionary (e.g. so that you
can reload the updated files from disk). The sessions that already use
the dictionaries will be forced to reinitialize them (the first one
will rebuild and copy them in the shared segment, the other ones will
use this prepared data).

    db=# SELECT shared_ispell_reset();

That's all for now ...

Changes from original version
-----------------------------
The original version of this module located in the Tomas Vondra's
[GitHub](https://github.com/tvondra/shared_ispell). That version does not handle
affixes that require full regular expressions (regex_t, implemented in regex.h).

This version of the module can handle that affixes with full regular
exressions. To handle it the module loads and stores affix files in each
sessions. The affix list is tiny and takes a little time and memory to parse.
Actually this is Tomas
[idea](http://www.postgresql.org/message-id/56A5F3D5.9030702@2ndquadrant.com),
but there is not related code in the GitHub.

Author
------
Tomas Vondra [GitHub](https://github.com/tvondra)