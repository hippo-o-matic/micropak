project('micropak', 'cpp',
        version : '0.4',
        default_options: ['cpp_std=c++17'],
        license: 'MIT')

conf_data = configuration_data()

conf_data.set('MICROPAK_BUFFER_SIZE', get_option('max_buffer_size'))
conf_data.set('MICROPAK_USE_GZIP', get_option('use_gzip'))

configure_file(output : 'config.h',
               configuration : conf_data)

include = [include_directories('.')]
deps = []

if get_option('use_gzip')
    deps += dependency('zlib') 
endif

add_project_link_arguments(['-ldl','-lstdc++fs'], language : 'cpp')

micropak = shared_library('micropak', 'micropak.cpp',
                            include_directories: '.',
                            dependencies : deps)

micropak_dep = declare_dependency(include_directories: include, link_with: micropak)

if get_option('build_sample')
    executable('demopack', 'demo.cpp', link_with : micropak)
endif