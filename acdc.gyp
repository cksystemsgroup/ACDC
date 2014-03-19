{
	'variables': {
		'default_cflags': [
			'-pthread',
			'-Wall',
			'-Werror',
			'-m64',
			'-mcx16',
		],
	},
    'target_defaults': {
	'configurations': {
		'Debug': {
			'cflags': ['<@(default_cflags)', '-g', '-O0', '-gdwarf-2'],
			'ldflags': ['-pthread'],
		},
		'Release': {
			'cflags': ['<@(default_cflags)', '-O3'],
			'ldflags': ['-pthread'],
		},
	},
    },
    'targets': [
        {
            'target_name': 'acdc',
            'type': 'executable',
            'defines': [],
            'include_dirs': [
                'src',
            ],
            'sources': [
                'src/acdc.c',
                'src/lifetime-class.c',
                'src/memory.c',
                'src/barrier.c',
                'src/lifetime-size-classes.c',
                'src/metadata-allocator.c',
                'src/distribution.c',
                'src/main.c',
                'src/proc_status.c',
            ],

        }

    ],
}
