# Homework 11

## Creating a Problem

The inode of the new file was not written out but the directory entry pointing to the inode was. If you use `gdb` to examine the contents of the log about to be committed, you'll discover that the log is about to write out blocks to two sectors: 4 (inode) and 34 (directory entry). The code we added to `commit()` overwrites the destination sector of the inode from sector 4 to sector 0. Sector 0 is the boot sector so that inode will never be read again. However, I do wonder how we're able to boot xv6 after this panic given that this write should corrupt the boot sector. Maybe during `make` the boot sector gets overwritten. In any case, after startup, when we attempt to `cat a`, the filesystem locates the directory entry for `a`, which points to an unallocated inode, whose type is set to 0, causing a panic in `ilock()`.

## Solving the Problem

`cat a` now succeeds but shows an empty file. That's because `install_trans()` fixed the dangling directory entry by writing out the file inode (inum 24 to sector 4). This is possible because in `commit()` we never cleared the log header so the log contents remained just like we left them before the crash. The file is empty because we crashed in `sys_open()` and never got the chance to write out any data blocks.

## Streamlining Commit

The buffer cache must not evict sector 3 prior to commit because if it did, an incoming read for sector 3 would get a cache miss, forcing a read from disk which would return a previous version of sector 3.

The `install_trans()` function has been edited to include the optimization of writing to disk directly from the cache. However, note that this optimization can only be applied when `install_trans()` is called from `commit()` and not from `recover_from_log()`. That is because in the latter case, the cache is empty.
