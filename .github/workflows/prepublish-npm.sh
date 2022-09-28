#!/bin/sh -xe

pull_run_docker() {
    # Pull and run image centos docker
    docker pull centos:centos${CENTOS_VERSION}
    docker run --network="host" --name ${DOCKER_CONTAINER_NAME_CENTOS} -ti -d -v `pwd`:/node-api -w/node-api centos:centos${CENTOS_VERSION}
}

# Install dependency, support for griddb node-api
install_dependency() {
    # Install C API latest version on https://build.opensuse.org/ site
    docker exec ${DOCKER_CONTAINER_NAME_CENTOS} /bin/bash -xec "set -eux \
        && yum update -y \
        && yum install -y curl python3 python3-devel centos-release-scl \
        && yum install -y devtoolset-7 --nogpgcheck \
        && yum install -y devtoolset-7-gcc-c++ --nogpgcheck \
        && cd /etc/yum.repos.d/ \
        && curl https://download.opensuse.org/repositories/home:/knonomura/CentOS_7/home:knonomura.repo -o /etc/yum.repos.d/home:knonomura.repo \
        && yum install -y griddb-c-client \
        && yum clean all"

    # Install node version 16.x
    docker exec ${DOCKER_CONTAINER_NAME_CENTOS} /bin/bash -xec "set -eux \
        && curl -sL https://rpm.nodesource.com/setup_16.x | bash - \
        && yum install -y nodejs --nogpgcheck \
        && npm --version \
        && yum clean all"
}

build_node_api() {
    # Build GridDB node-api
    docker exec ${DOCKER_CONTAINER_NAME_CENTOS} /bin/bash -xec "source /opt/rh/devtoolset-7/enable && npm install && gcc --version && g++ --version"
}

run_sample_centos7() {
    # Run sample on CentOS 7
    docker exec ${DOCKER_CONTAINER_NAME_CENTOS} /bin/bash -xec "export NODE_PATH=/node-api && \
        node sample/sample1.js ${GRIDDB_NOTIFICATION_ADDRESS} ${GRIDDB_NOTIFICATION_PORT} ${GRIDDB_CLUSTER_NAME} ${GRIDDB_USERNAME} ${GRIDDB_PASSWORD}"
}

# Run sample Node-api on OS ubuntu 20.04
run_sample_ubuntu() {
    # Install C API
    echo 'deb http://download.opensuse.org/repositories/home:/knonomura/xUbuntu_18.04/ /' | sudo tee /etc/apt/sources.list.d/home:knonomura.list
    curl -fsSL https://download.opensuse.org/repositories/home:knonomura/xUbuntu_18.04/Release.key | gpg --dearmor | sudo tee /etc/apt/trusted.gpg.d/home_knonomura.gpg > /dev/null
    sudo apt update -y
    sudo apt install griddb-c-client -y
    # Run sample
    export NODE_PATH=`pwd`
    node sample/sample1.js ${GRIDDB_NOTIFICATION_ADDRESS} ${GRIDDB_NOTIFICATION_PORT} ${GRIDDB_CLUSTER_NAME} ${GRIDDB_USERNAME} ${GRIDDB_PASSWORD}
}