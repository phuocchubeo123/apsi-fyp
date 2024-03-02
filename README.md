# Private Set Intersection from Bit Comparison

This repo includes my Final Year Project at Nanyang Technological University. The idea of this PSI protocol is based on [Private Computation On Set Intersection With Sublinear Communication](https://eprint.iacr.org/2022/1137).

This code is not fully releasable yet. I am actively updating the v2 branch, which contains the fastest version of this protocol.

## Dependencies

This repository has a largely similar set of dependencies to microsoft/apsi.
I have so far only test it with vcpkg.

We need:
- g++ compiler.
- vcpkg
- cmake (tested with 3.22)

We can install the following packages with vcpkg:
- Seal
- Kuku
- Flatbuffers
- jsoncpp
- log4cplus
- cppzmq
- tclap

Issues found so far:
- vcpkg cannot detect compiler: please install g++. Gcc may not work.

# What should you look for to modify this repo

## The <tt> cli </tt> folder
The folder contains some custom source code for:
- Generating synthetic data
- Read data from json files
- A program to act as the sender
- A program to act as the receiver

# Currently updating
## Allow hashing for smaller bit length
Since bit length plays a big role in both the multiplicative depth and the number of multiplications, we want to do hashing. The original version of the protocol neglects hashing.

## Allow more flexible bundle size
The current bundle size supported is only the same as the number of bins. We want to support more settings for bundle size and support more experiments.
I plan to divide the bundle sizes into two cases:
- With a small bundle size (need more experiments to confirm, but most likely bundle size <= 4), we support a Branching Program data structure.
- With a large bundle size, we do not need a tree anymore since most likely there is no duplicated note.

# Future works
## Labeled PSI
