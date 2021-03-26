{
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
      'conditions': [
        ['OS=="linux"', {'libraries': ['-lgridstore']}],
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
    }
  ]
}
