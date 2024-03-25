# 13th March 2024
I am currently adapting the cuckoo hashing and simple hashing algorithm from https://github.com/encryptogroup/PSI.git.
This github repo is the implementation of Phashing, the first PSI protocol that uses hashing and get good results. 
Progress so far:
- Currently I have only created the class hashing_state in the hashing_util.h file.
- Added utility for generating random numbers, prf, ...
- Added openssl.
- I have also kind of figured out where to put these files in this folder structure.

23/3/2024:
I am trying to do permutation-based cuckoo hashing for the receiver. I am looking at receiver.cpp file, where I put a placeholder there, and will use permutation-based cuckoo hashing in it.

24/3/2024:
We need to study the hashElement function. Basically, we can start hashing the elements and pre-calculate all of its possible addresses, while doing cuckoo hashing. I think this also applies for permutation-based hashing.

What's next:
- First look at the cuckoo.h file, and figure out what is needed to do cuckoo hashing.

Further plan:
- Look at sender.db to see how should we change the code and do permutation-based hashing.