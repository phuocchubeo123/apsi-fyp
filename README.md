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

Issues found so far:
- vcpkg cannot detect compiler: please install g++. Gcc may not work.

