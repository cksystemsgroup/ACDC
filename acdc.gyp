{
    'target_defaults': {
	'configurations': {
		'Debug': {
			'cflags': ['-pthread'],
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
