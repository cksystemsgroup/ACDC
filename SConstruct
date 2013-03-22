global_env = Environment()
conf = Configure(global_env)

if not conf.CheckHeader('pthread.h'):
	print "You need 'pthread.h'. Not Found!"
	Exit(1)
else:
	conf.env.Append(LIBS=['pthread'])

if not conf.CheckHeader('math.h'):
	print "You need 'math.h'. Not Found!"
	Exit(1)
else:
	conf.env.Append(LIBS=['m'])

global_env = conf.Finish()

#Debug build
env = global_env.Clone()
env.Append(CPPFLAGS=['-g'])
VariantDir('build-dbg', 'src')
SConscript('build-dbg/SConscript', exports='env')

#Release using 128bit atomic ops. This is the default
env = global_env.Clone()
env.Append(CPPFLAGS=['-O3', '-Wall', '-Werror', '-mcx16', '-m64'])
VariantDir('build', 'src')
SConscript('build/SConscript', exports='env')

#Release using 64bit atomic ops. This limits ACDC to 64 threads
env = global_env.Clone()
env.Append(CPPFLAGS=['-O3', '-Wall', '-Werror'])
VariantDir('build-64threads', 'src')
SConscript('build-64threads/SConscript', exports='env')


