# llvm2cpg

Forked from https://github.com/ShiftLeftSecurity/llvm2cpg (2023-10-16)

`llvm2cpg` is a tool that converts LLVM Bitcode into Code Property Graph (CPG).

The CPG can be further analyzed via [Joern](https://joern.io) or [Ocular](http://ocular.shiftleft.io/).

To get started, make sure to look into an [~introductory tutorial~](https://docs.joern.io/llvm2cpg/hello-llvm)[【WebArchive】](http://web.archive.org/web/20220625054134/https://docs.joern.io/llvm2cpg/hello-llvm/).

After that, follow along and learn how to [~extract LLVM Bitcode from real-world projects~](https://docs.joern.io/llvm2cpg/getting-bitcode)[【WebArchive】](http://web.archive.org/web/20220625054134/https://docs.joern.io/llvm2cpg/getting-bitcode).

If you get questions - feel free to open [an issue](https://github.com/ShiftLeftSecurity/llvm2cpg/issues/new) or come over to the [Joern chat](https://gitter.im/joern-code-analyzer/community).

## Build

### Run CI manually

1. install ansible

2. run ansible playbook
```bash
ansible-playbook ci/ubuntu-playbook.yaml

# or skip the failed task
ansible-playbook -v ci/ubuntu-playbook.yaml --skip-tags "failed"

# the package will be store in /tmp/packages
# eg. /tmp/packages/llvm2cpg-0.8.0-LLVM-9.0-ubuntu-20.04.zip
```

## Build using cmake directly

```bash
cp -r /path/to/llvm2cpg /opt/
mkdir /opt/build.llvm2cpg.debug.dir
cd  /opt/build.llvm2cpg.debug.dir
cmake -DPATH_TO_LLVM=/opt/llvm-9.0.0 -DPATH_TO_JOERN=/opt/joern-cli -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS= /opt/llvm2cpg
make all
# or build parallel
# make all -j30

```

## Run demo

1. Generate cpg

```bash
cd /opt/build.llvm2cpg.debug.dir
./tools/llvm2cpg/llvm2cpg /path/to/demo.ll # generate cpg.bin.zip

```
2. Load to neo4j

```bash
# load cpg.bin.zip into joern
../joern-cli/joern
loadCpg("cpg.bin.zip")
save


# export to neo4jcsv
../joern-cli/joern-export ./workspace/cpg.bin.zip/cpg.bin --repr all --format neo4jcsv
# the neo4jcsv will be store in out/


# load neo4jcsv to neo4j
mkdir neo4j_data
docker run -it --rm --name neo4j-bsca -p 7474:7474 -p 7687:7687 -v ${PWD}/neo4j_data:/data -v ${PWD}/out:/var/lib/neo4j/import neo4j:4.4 

# in another terminal
docker exec -it neo4j-bsca /bin/bash

cd /var/lib/neo4j
bin/cypher-shell -u neo4j -p neo4j
# change password to <password> then exit cyber-shell

find /var/lib/neo4j/import -name 'nodes_*_cypher.csv' -exec bin/cypher-shell -u neo4j -p "<password>" --file {} \;
find /var/lib/neo4j/import -name 'edges_*_cypher.csv' -exec bin/cypher-shell -u neo4j -p "<password>" --file {} \;

```