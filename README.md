GridDB Node API

## Overview

GridDB Node API is developed using GridDB C Client and [node-addon-api](https://github.com/nodejs/node-addon-api).  

## Operating environment

Building of the library and execution of the sample programs have been checked in the following environment.

    OS: CentOS 7.9(x64)/Ubuntu 20.04(x64)/Windows 10/MacOS 10.15
    Node.js: v16
    GridDB C client: V5.0.0 CE
    GridDB server: V5.0.0 CE, CentOS 7.9(x64)

Note: We can use Node.js v18 for Ubuntu 20.04(x64)/Windows 10/MacOS 10.15(/CentOS 8).

## Preparations

Install [GridDB Server](https://github.com/griddb/griddb) and [C Client](https://github.com/griddb/c_client). 

## QuickStart (CentOS 7/Ubuntu 20.04(x64)/MacOS 10.15)

### Build
    1. Set LIBRARY_PATH.

        export LIBRARY_PATH=$LIBRARY_PATH:<C client library file directory path>

    2. The command to run build project

        $ npm install

    3. Set the NODE_PATH variable for griddb Node.js module files.

        $ export NODE_PATH=<installed directory path>

    4. Write require("griddb-node-api") in Node.js.

### How to run sample

GridDB Server need to be started in advance.

    1. Set LD_LIBRARY_PATH.

        export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:<C client library file directory path>

    2. The command to run sample

        $ node sample/sample1.js <GridDB notification address> <GridDB notification port>
            <GridDB cluster name> <GridDB user> <GridDB password>
          -->[ 'name01', false, 1, <Buffer 41 42 43 44 45 46 47 48 49 4a> ]

## QuickStart (Windows 10)

### Build and run in cmd
    1. Correct the path to C Client "C_CLIENT_PATH" in binding.gyp. Default is 'C:/Program Files/GridDB/C Client/5.0.0'

    2. The command to run build project

        $ npm install

    3. Set the NODE_PATH variable for griddb Node.js module files.

        $ set NODE_PATH=<installed directory path>

    3. Write require("griddb-node-api") in Node.js.

### How to run sample

GridDB Server need to be started in advance.

    1. The command to run sample

        $ node sample/sample1.js <GridDB notification address> <GridDB notification port>
            <GridDB cluster name> <GridDB user> <GridDB password>
          -->[ 'name01', false, 1, <Buffer 41 42 43 44 45 46 47 48 49 4a> ]

## Document

- [Node API Reference](https://griddb.github.io/node-api/NodeAPIReference.htm)

## Community

  * Issues  
    Use the GitHub issue function if you have any requests, questions, or bug reports. 
  * PullRequest  
    Use the GitHub pull request function if you want to contribute code.
    You'll need to agree GridDB Contributor License Agreement(CLA_rev1.1.pdf).
    By using the GitHub pull request function, you shall be deemed to have agreed to GridDB Contributor License Agreement.

## License
  
  GridDB Node API source license is Apache License, version 2.0.
