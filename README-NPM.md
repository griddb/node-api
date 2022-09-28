GridDB Node API

## Overview

[GridDB](https://github.com/griddb/griddb) is Database for IoT with both NoSQL interface and SQL Interface.

GridDB Node API is developed using GridDB C Client and [node-addon-api](https://github.com/nodejs/node-addon-api).  

## Operating environment

Building of the library and execution of the sample programs have been checked in the following environment.

    OS: CentoS7.9(x64), Ubuntu 18.04(x64), Ubuntu 20.04(x64)
    Node.js: v16
    gcc/g++: >= 7
    GridDB C client: V5.0.0 CE
    GridDB server: V5.0.0 CE, CentOS 7.9(x64)

## Preparations

* Install [GridDB Server](https://github.com/griddb/griddb)
* Install [C Client](https://github.com/griddb/c_client).
    * (Ubuntu)
        ```console
            $ wget https://github.com/griddb/c_client/releases/download/v5.0.0/griddb-c-client_5.0.0_amd64.deb
            $ sudo dpkg -i griddb-c-client_5.0.0_amd64.deb
        ```
    * (CentOS)
        ```console
            $ wget https://github.com/griddb/c_client/releases/download/v5.0.0/griddb-c-client-5.0.0-linux.x86_64.rpm
            $ sudo rpm -ivh griddb-c-client-5.0.0-linux.x86_64.rpm
        ```
## QuickStart

### Install

    $ npm install griddb-node-api

### How to run sample

GridDB Server need to be started in advance.

    1. The command to run sample
        $ cp node_modules/griddb-node-api/sample/sample1.js .
        $ node sample1.js <GridDB notification address> <GridDB notification port>
            <GridDB cluster name> <GridDB user> <GridDB password>
          -->[ 'name01', false, 1, <Buffer 41 42 43 44 45 46 47 48 49 4a> ]

### Install gcc/g++:
1. CentOS 7(x64)
    ```console
    # Install gcc/g++ 7 on centos 7.9
    $ sudo yum install centos-release-scl
    $ sudo yum install devtoolset-7-gcc-c++

    # enable gcc/g++7 on centos 7.9
    $ scl enable devtoolset-7 bash
    ```
2. Ubuntu 18.04/20.04(x64)
    ```
    $ sudo apt-get install gcc g++
    ```