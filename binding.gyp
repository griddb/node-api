{
  'variables': {
    # Need only for build on Windows
    'C_CLIENT_PATH': 'C:/Program Files/GridDB/C Client/4.5.0'
  },
  'targets': [
    {
      'target_name': 'griddb',
      'sources': [ 'src/Addon.cc', 'src/StoreFactory.cpp',
                   'src/Util.cpp', 'src/Store.cpp',
                   'src/ContainerInfo.cpp', 'src/Container.cpp',
                   'src/GSException.cpp',
                   'src/Query.cpp',
                   'src/RowSet.cpp', 'src/Field.cpp',
                   'src/AggregationResult.cpp',
                   'src/PartitionController.cpp',
                   'src/ExpirationInfo.cpp'],
      'include_dirs': ["<!@(node -p \"require('node-addon-api').include\")",
                       "include/"],
      'dependencies': ["<!(node -p \"require('node-addon-api').gyp\")"],
      'cflags!': [ '-fno-exceptions' ],
      'cflags_cc!': [ '-fno-exceptions' ],
      'xcode_settings': {
        'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
        'CLANG_CXX_LIBRARY': 'libc++',
        'MACOSX_DEPLOYMENT_TARGET': '10.7',
      },
      'msvs_settings': {
        'VCCLCompilerTool': { 'ExceptionHandling': 1 },
      },
      'conditions': [
        ['OS=="linux"', {'libraries': ['-lgridstore']}],
        ['OS=="mac"', {'libraries': ['-lgridstore']}],
        ['OS=="win"', {'libraries': ['<(C_CLIENT_PATH)/gridstore_c.lib']}],
       ],
      "defines": ["NAPI_DISABLE_CPP_EXCEPTIONS=1"]
    },
    {
      'target_name': 'copy_binary',
      'type': 'none',
      'dependencies': ['griddb'],
      'copies': [
        {
          'files': [ '<(PRODUCT_DIR)/griddb.node' ],
          'destination': '<(module_root_dir)'
        }
      ]
    },
    {
      'target_name': 'copy_c_client_library_in_windows',
      'type': 'none',
      'dependencies': ['copy_binary'],
      'copies': [
         {
           'files': [],
           'conditions': [
             ["OS=='win'", {
               'files': ['<(C_CLIENT_PATH)/gridstore_c.dll']
               }],
            ],
            'destination': '<(module_root_dir)'
         }
      ]
    }
  ]
}
