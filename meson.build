project('kafkacat', 'c', version: '1.4.0', subproject_dir: 'meson')

static = get_option('static')
yajl = dependency('yajl', version: '>=2', required: get_option('json'), static: static, fallback: ['yajl', 'yajl_dep'])
rdkafka = dependency('rdkafka', version: '>=0.9', static: static, fallback: ['rdkafka', 'rdkafka_dep'])

conf = configuration_data()
conf.set10('HAVE_YAJL', yajl.found())
conf.set10('ENABLE_JSON', yajl.found() and get_option('json').enabled())
conf.set10('ENABLE_KAFKACONSUMER', true)
conf.set_quoted('KAFKACAT_VERSION', meson.project_version())
configure_file(configuration: conf, output: 'config.h')
link_args = []
if static
	link_args += ['-static']
endif

executable('kafkacat',
	sources: ['format.c', 'json.c', 'kafkacat.c', 'kafkacat.h', 'rdport.h', 'tools.c'],
	dependencies: [yajl, rdkafka],
	link_args: link_args,
)
