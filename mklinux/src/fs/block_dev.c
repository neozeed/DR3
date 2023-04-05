/*
 *  linux/fs/block_dev.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <linux/autoconf.h>

#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/locks.h>
#include <linux/fcntl.h>
#include <linux/mm.h>

#include <asm/segment.h>
#include <asm/system.h>

extern int *blk_size[];
extern int *blksize_size[];

#define MAX_BUF_PER_PAGE (PAGE_SIZE / 512)
#define NBUF 64

int block_write(struct inode * inode, struct file * filp,
	const char * buf, int count)
{
	int blocksize, blocksize_bits, i, buffercount,write_error;
	int block, blocks;
	loff_t offset;
	int chars;
	int written = 0;
	struct buffer_head * bhlist[NBUF];
	unsigned int size;
	kdev_t dev;
	struct buffer_head * bh, *bufferlist[NBUF];
	register char * p;

	write_error = buffercount = 0;
	dev = inode->i_rdev;
	if ( is_read_only( inode->i_rdev ))
		return -EPERM;
	blocksize = BLOCK_SIZE;
	if (blksize_size[MAJOR(dev)] && blksize_size[MAJOR(dev)][MINOR(dev)])
		blocksize = blksize_size[MAJOR(dev)][MINOR(dev)];

	i = blocksize;
	blocksize_bits = 0;
	while(i != 1) {
		blocksize_bits++;
		i >>= 1;
	}

	block = filp->f_pos >> blocksize_bits;
	offset = filp->f_pos & (blocksize-1);

	if (blk_size[MAJOR(dev)])
		size = ((loff_t) blk_size[MAJOR(dev)][MINOR(dev)] << BLOCK_SIZE_BITS) >> blocksize_bits;
	else
		size = INT_MAX;
	while (count>0) {
		if (block >= size)
			return written ? written : -ENOSPC;
		chars = blocksize - offset;
		if (chars > count)
			chars=count;

#if 0
		/* get the buffer head */
		{
			struct buffer_head * (*fn)(kdev_t, int, int) = getblk;
			if (chars != blocksize)
				fn = bread;
			bh = fn(dev, block, blocksize);
		}
#else
		bh = getblk(dev, block, blocksize);

		if (chars != blocksize && !buffer_uptodate(bh)) {
		  if(!filp->f_reada ||
		     !read_ahead[MAJOR(dev)]) {
		    /* We do this to force the read of a single buffer */
		    brelse(bh);
		    bh = bread(dev,block,blocksize);
		  } else {
		    /* Read-ahead before write */
		    blocks = read_ahead[MAJOR(dev)] / (blocksize >> 9) / 2;
		    if (block + blocks > size) blocks = size - block;
		    if (blocks > NBUF) blocks=NBUF;
		    bhlist[0] = bh;
		    for(i=1; i<blocks; i++){
		      bhlist[i] = getblk (dev, block+i, blocksize);
		      if(!bhlist[i]){
			while(i >= 0) brelse(bhlist[i--]);
			return written ? written : -EIO;
		      };
		    };
		    ll_rw_block(READ, blocks, bhlist);
		    for(i=1; i<blocks; i++) brelse(bhlist[i]);
		    wait_on_buffer(bh);
		      
		  };
		};
#endif
		block++;
		if (!bh)
			return written ? written : -EIO;
		p = offset + bh->b_data;
		offset = 0;
		filp->f_pos += chars;
		written += chars;
		count -= chars;
		memcpy_fromfs(p,buf,chars);
		p += chars;
		buf += chars;
		mark_buffer_uptodate(bh, 1);
		mark_buffer_dirty(bh, 0);
		if (filp->f_flags & O_SYNC)
			bufferlist[buffercount++] = bh;
		else
			brelse(bh);
		if (buffercount == NBUF){
			ll_rw_block(WRITE, buffercount, bufferlist);
			for(i=0; i<buffercount; i++){
				wait_on_buffer(bufferlist[i]);
				if (!buffer_uptodate(bufferlist[i]))
					write_error=1;
				brelse(bufferlist[i]);
			}
			buffercount=0;
		}
		if(write_error)
			break;
	}
	if ( buffercount ){
		ll_rw_block(WRITE, buffercount, bufferlist);
		for(i=0; i<buffercount; i++){
			wait_on_buffer(bufferlist[i]);
			if (!buffer_uptodate(bufferlist[i]))
				write_error=1;
			brelse(bufferlist[i]);
		}
	}		
	filp->f_reada = 1;
	if(write_error)
		return -EIO;
	return written;
}

int block_read(struct inode * inode, struct file * filp,
	char * buf, int count)
{
	unsigned int block;
	loff_t offset;
	int blocksize;
	int blocksize_bits, i;
	unsigned int blocks, rblocks, left;
	int bhrequest, uptodate;
	struct buffer_head ** bhb, ** bhe;
	struct buffer_head * buflist[NBUF];
	struct buffer_head * bhreq[NBUF];
	unsigned int chars;
	loff_t size;
	kdev_t dev;
	int read;

	dev = inode->i_rdev;
	blocksize = BLOCK_SIZE;
	if (blksize_size[MAJOR(dev)] && blksize_size[MAJOR(dev)][MINOR(dev)])
		blocksize = blksize_size[MAJOR(dev)][MINOR(dev)];
	i = blocksize;
	blocksize_bits = 0;
	while (i != 1) {
		blocksize_bits++;
		i >>= 1;
	}

	offset = filp->f_pos;
#if	defined(CONFIG_OSFMACH3) && defined(__powerpc__)
	/* XXX to avoid a "might be used uninitialized" warning */
	size = offset;
#endif	/* CONFIG_OSFMACH3 && __powerpc__ */
	if (blk_size[MAJOR(dev)])
		size = (loff_t) blk_size[MAJOR(dev)][MINOR(dev)] << BLOCK_SIZE_BITS;
	else
#ifdef	CONFIG_OSFMACH3
		/* Assume default size is max of long long..
		 * Used to be INT_MAX ..  Can cause NASTY problems
		 * if your device is over 2GB...
		 */
		size =  ~((long long)1<<63);
#else	/* CONFIG_OSFMACH3 */
		size = INT_MAX;
#endif	/* CONFIG_OSFMACH3 */

	if (offset > size)
		left = 0;
	/* size - offset might not fit into left, so check explicitly. */
	else if (size - offset > INT_MAX)
		left = INT_MAX;
	else
		left = size - offset;
	if (left > count)
		left = count;
	if (left <= 0)
		return 0;
	read = 0;
	block = offset >> blocksize_bits;
	offset &= blocksize-1;
	size >>= blocksize_bits;
	rblocks = blocks = (left + offset + blocksize - 1) >> blocksize_bits;
	bhb = bhe = buflist;
	if (filp->f_reada) {
	        if (blocks < read_ahead[MAJOR(dev)] / (blocksize >> 9))
			blocks = read_ahead[MAJOR(dev)] / (blocksize >> 9);
		if (rblocks > blocks)
			blocks = rblocks;
		
	}
	if (block + blocks > size)
		blocks = size - block;

	/* We do this in a two stage process.  We first try to request
	   as many blocks as we can, then we wait for the first one to
	   complete, and then we try to wrap up as many as are actually
	   done.  This routine is rather generic, in that it can be used
	   in a filesystem by substituting the appropriate function in
	   for getblk.

	   This routine is optimized to make maximum use of the various
	   buffers and caches. */

	do {
		bhrequest = 0;
		uptodate = 1;
		while (blocks) {
			--blocks;
			*bhb = getblk(dev, block++, blocksize);
			if (*bhb && !buffer_uptodate(*bhb)) {
				uptodate = 0;
				bhreq[bhrequest++] = *bhb;
			}

			if (++bhb == &buflist[NBUF])
				bhb = buflist;

			/* If the block we have on hand is uptodate, go ahead
			   and complete processing. */
			if (uptodate)
				break;
			if (bhb == bhe)
				break;
		}

		/* Now request them all */
		if (bhrequest) {
			ll_rw_block(READ, bhrequest, bhreq);
		}

		do { /* Finish off all I/O that has actually completed */
			if (*bhe) {
				wait_on_buffer(*bhe);
				if (!buffer_uptodate(*bhe)) {	/* read error? */
				        brelse(*bhe);
					if (++bhe == &buflist[NBUF])
					  bhe = buflist;
					left = 0;
					break;
				}
			}			
			if (left < blocksize - offset)
				chars = left;
			else
				chars = blocksize - offset;
			filp->f_pos += chars;
			left -= chars;
			read += chars;
			if (*bhe) {
				memcpy_tofs(buf,offset+(*bhe)->b_data,chars);
				brelse(*bhe);
				buf += chars;
			} else {
#ifdef	CONFIG_OSFMACH3
				while (chars > 0) {
					extern const char *osfmach3_zero_page;
					unsigned long count;

					if (chars > PAGE_SIZE)
						count = PAGE_SIZE;
					else
						count = chars;
					memcpy_tofs(buf, osfmach3_zero_page,
						    count);
					buf += count;
					chars -= count;
				}
				chars--;
#else	/* CONFIG_OSFMACH3 */
				while (chars-- > 0)
					put_user(0,buf++);
#endif	/* CONFIG_OSFMACH3 */
			}
			offset = 0;
			if (++bhe == &buflist[NBUF])
				bhe = buflist;
		} while (left > 0 && bhe != bhb && (!*bhe || !buffer_locked(*bhe)));
	} while (left > 0);

/* Release the read-ahead blocks */
	while (bhe != bhb) {
		brelse(*bhe);
		if (++bhe == &buflist[NBUF])
			bhe = buflist;
	};
	if (!read)
		return -EIO;
	filp->f_reada = 1;
	return read;
}

int block_fsync(struct inode *inode, struct file *filp)
{
	return fsync_dev (inode->i_rdev);
}
