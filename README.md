## A fast and simple duplicate file finder, written in C.

The architecture follows:

1. Recursively directory walk through the provided root.
   - Custom `walk_dir` in [`tree.c`](./tree.c), `lstat` based (skips symlinks).
2. Group all the files by their sizes.
3. Find partial hash of initial and middle chunks.
   - Uses [xxHash](https://xxhash.com/) for fast, non-crypto fingerprinting.
4. Verify hash hit files with `memcmp`.

### TODO
- [x] perform memcmp verification.
- [ ] restructure [idup.c](./idup.c)
- [ ] depth cap in [tree.c](./tree.c)
- [ ] better results UI

