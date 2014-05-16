Falcon
======

A Fast, Reliable, Distributed Build System.

## Purpose

Falcon is a build system with a focus on speed and scalability.

This project is in its early stages. We are currently mainly focused on building
a proof-of-concept rather than a robust cross-platform build system.

## Concepts

### Simple configuration file

The configuration file - the "makefile" - is designed to be human-readable but
not convenient to write by hand. There are no variables, the file only contains
the dependency graph of the project, with a list of rules that take several
inputs to build several outputs. We consider that it is not Falcon's job to
provide a powerful way to define the dependency graph. We expect this file to be
generated by other tools such as CMake.

### Use Watchman to permit fast incremental builds

Falcon is a daemon that uses watchman to be notified of changes that occur on
the files that belong to the project. The configuration file is only loaded once
on startup and each time it changes. This makes incremental builds faster
because at the time you trigger a build, falcon already has the graph in memory
and knows what to do.

### Consistency and correctness

Falcon aims at being consistent and correct. For example, if you change the
configuration file, Falcon should remove any stale target that no longer exists.

### Smart local caching

Falcon understands git and provides a caching mechanism so that you don't need
to re-build anything when you switch to a branch you already built. Falcon
ensures that the last version of your project is in cache, for every git branch.

### Distributed caching

Falcon will be Distributed. If you are working on a very large project with
hundreds of thousands of files, you will be able to build changes that you pull
very quickly. Falcon will be able to retrieve already built targets by querying
the cache of another Falcon daemon running in a continuous build server. Why
build something that someone already built for you?

### Thrift API

Falcon uses Thrift to provide an API that will allow developers to develop
applications around Falcon. Clients are able to query Falcon for the state of
the project's graph, trigger builds, manage the cache, etc. Among other things,
this will permit a deep integration to IDEs and editors.

## Current state

Right now falcon is in a working (but experimental) state. There are still a few
important missing parts:

- We plan on building a CMake generator for the graph configuration file;
- The reload of the graph configuration file is not fully working yet;
- Distributed caching is not implemented.

## System Requirements

Falcon is known to compile on Ubuntu 13.10.

You need to following libraries:

- thrift-compiler, python-thrift;
- libgit2;
- libboost-program-options-dev;
- libgoogle-glog-dev;
- ijson for python (pip install ijson).

You will also need to build and install Watchman
(https://github.com/facebook/watchman).

## Build

We use CMake to build falcon.

```
mkdir build
cd build
cmake ..
make
sudo make install
```

Note: we plan on making it possible to use Falcon to build itself once we
implement the CMake generator.

## Usage

Falcon is made of a binary "falcond" that acts as the daemon that monitors the
files of your project and builds it. We also provide a python client which you
must use to trigger a build (see ./clients/python/falcon).

