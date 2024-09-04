aime2aime = shared_library(
    'aime2aime',
    name_prefix : '',
    include_directories : inc,
    implicit_include_directories : false,
    c_pch : '../precompiled.h',
    vs_module_defs : 'exports.def',
    link_with : [
        segapi.get_variable('api_lib')
    ],
    dependencies : [
    ],
    sources : [
        'aime.c',
        'aime.h',
        'aimeio.c',
        'config.c',
        'config.h',
        'util/dprintf.c',
        'util/dprintf.h',
        'util/dump.c',
        'util/dump.h',
    ],
)
