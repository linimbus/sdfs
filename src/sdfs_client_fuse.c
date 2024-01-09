#include "sdfs.h"

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h>

pthread_mutex_t m_lock = PTHREAD_MUTEX_INITIALIZER;

#define LOCK() (void)pthread_mutex_lock(&m_lock);
#define ULOCK() (void)pthread_mutex_unlock(&m_lock);

#define MAX_OPEN_NUM  1024
#define MAX_FILE_NAME 256
#define FD_OFFSET     10

CLIENT * clnt;

char * g_fd[MAX_OPEN_NUM] = {NULL};

struct sdfs_dir_p {
	char path[MAX_FILE_NAME+1];
	off_t offset;
};

char host[] = "127.0.0.1";

int clnt_debug = 0 ;
int clnt_cnt = 0 ;

#define log(path) \
	if(clnt_debug) \
	{ \
		printf("No.%-3u [%s:%u] %s", clnt_cnt++, __FUNCTION__,__LINE__,path ); \
	}

int sdfs_allocFd(char * path) {
	int i,size;

	log(path);

	size = strlen(path);
	if (size > MAX_FILE_NAME) {
		errno = ENAMETOOLONG;
		log(path);
		return -1;
	}

	for( i = 0 ; i < MAX_OPEN_NUM; i++ ) {
		if( NULL != g_fd[i] ) {
			continue;
		}

		g_fd[i] = (char *)malloc(size+1);
		if ( NULL == g_fd[i] ) {
			errno = ENOMEM;
			log(path);
			return -1;
		}

		strncpy(g_fd[i], path, size);
		g_fd[i][size] = '\0';

		log(path);

		return i + FD_OFFSET;
	}

	errno = EMFILE;
	log(path);

	return -1;
}

char * sdfs_findNameByFd(int fd) {
	int i = fd - FD_OFFSET;

	log("");

	if ( 0 <= i && i < MAX_OPEN_NUM ) {
		log("");
		return g_fd[i];
	}

	log("");

	return NULL;
}

void sdfs_freeFd(int fd) {
	int i = fd - FD_OFFSET;

	log("");

	if ( 0 <= i && i < MAX_OPEN_NUM ) {

		if ( g_fd[i] ) {
			log("");

			free(g_fd[i]);
			g_fd[i] = NULL;
		}
	}
}

int sdfs_getattr(const char * path, struct stat * stbuf ) {
	GETATTR_RSP_T * result;
	GETATTR_REQ_T argp;

	log(path);
	LOCK();

	argp.path = (char *)path;

	result = rpc_getattr_0x0001(&argp, clnt);
	if ( result == NULL ) {
		clnt_perror(clnt, "call failed!");
		ULOCK();
		return -ENOENT;
	}

	if( result->err ) {
		ULOCK();
		return result->err;
	}

	memset(stbuf,0,sizeof(struct stat));

	stbuf->st_mode  = result->mode;
	stbuf->st_nlink = result->nlink;
	stbuf->st_size  = result->size;
	stbuf->st_ino   = result->ino;
	stbuf->st_uid   = result->uid;
	stbuf->st_gid   = result->gid;
	stbuf->st_atime = result->atime;
	stbuf->st_mtime = result->mtime;
	stbuf->st_ctime = result->ctime;

	ULOCK();

	return 0;
}

int sdfs_readdir(const char * path, 
				 void * buf, 
				 fuse_fill_dir_t filler,
				 off_t offset, 
				 struct fuse_file_info * fi )
{
	READDIR_REQ_T argp;
	READDIR_RSP_T * result;

	int res;
	int reds = 0;

	char buf1[MAX_FILE_NAME];

	LOCK();

	argp.path = (char *) path;
	argp.size = -1;
	argp.offset = offset;

	result = rpc_readdir_0x0001( &argp, clnt );
	if ( result == NULL ) {
		clnt_perror(clnt, "call failed!");

		ULOCK();
		return -ENOENT;
	}

	if( result->err ) {
		ULOCK();
		return result->err;
	}
	
	for(;;) {
		sscanf(result->data.data_val + reds , "%s", buf1 );
		filler(buf,buf1,NULL,0);
		reds += strlen(buf1) + 1;

		if (reds >= result->data.data_len ) 
			break;
	}

	ULOCK();

	return 0;
}

int sdfs_open(const char * path, struct fuse_file_info * fi) {
	int * result;
	OPEN_REQ_T arg;
	int fd;
	int res;

	LOCK();

	fd = sdfs_allocFd((char *) path);
	if (-1 == fd) {
		return -errno;
	}

	arg.path = (char *)path;
	arg.flag = fi->flags;

	result = rpc_open_0x0001(&arg, clnt);
	if ( result == NULL ) {
		clnt_perror(clnt, "call failed!");
		sdfs_freeFd( fd);

		ULOCK();
		return -ENOENT;
	}

	if( *result ) {
		sdfs_freeFd( fd);

		ULOCK();
		return *result;
	}

	fi->fh = fd;

	ULOCK();
	return 0;
}

int sdfs_read(const char * path, 
				char * buf, 
				size_t size, 
				off_t offset ,
				struct fuse_file_info *fi)
{
	READ_RSP_T * result;
	READ_REQ_T arg;

	int res;
	size_t size_tmp;
	size_t size_all = 0;
	off_t offset_tmp = 0;

	LOCK();

	do {
		if (size > 4096 ) {
			size_tmp = 4096;
		} else {
			size_tmp = size;
		}

		arg.path = (char *) path;
		arg.size = size_tmp;
		arg.offset = offset;

		result = rpc_read_0x0001( &arg, clnt);
		if ( result == NULL ) {
			clnt_perror(clnt, "call failed!");
			ULOCK();

			if ( size_all > 0 ) {
				return size_all;
			}
		
			return -ENOENT;
		}


		if( result->err ) {
			ULOCK();

			if ( size_all > 0 ) {
				return size_all;
			}

			return result->err;
		}

		size_tmp = result->data.data_len;
		if ( size_tmp == 0 ) {
			break;
		}

		memcpy(buf+offset_tmp, result->data.data_val, size_tmp);

		offset += size_tmp;
		size -= size_tmp;

		size_all += size_tmp;
		offset_tmp += size_tmp;
	} while( size > 0 );

	ULOCK();
	return size_all;
}

int sdfs_write(const char * path, 
				const char * buf, 
				size_t size,
				off_t offset,
				struct fuse_file_info * fi ) 
{
	WRITE_REQ_T arg;
	WRITE_RSP_T * result;

	int res;
	size_t size_tmp;
	size_t size_all = 0 ;
	off_t offset_tmp = 0;

	LOCK();

	do {
		size_tmp = (size > 4096 ) ? 4096: size ;

		arg.path = (char *)path;
		arg.size = size_tmp;
		arg.offset = offset;

		arg.data.data_len = size_tmp;
		arg.data.data_val = (char *) buf + offset_tmp;

		result = rpc_write_0x0001(&arg, clnt);
		if ( result == NULL ) {
			clnt_perror(clnt, "call failed!");
			ULOCK();

			if ( size_all > 0 ) {
				return size_all;
			}
		
			return -ENOENT;
		}

		if( result->err ) {

			ULOCK();

			if ( size_all > 0 ) {
				return size_all;
			}

			return result->err;
		}

		size_tmp = result->size;
		if ( size_tmp == 0 ) {
			break;
		}

		offset += size_tmp;
		size -= size_tmp;

		size_all += size_tmp;
		offset_tmp += size_tmp;

	}while(size > 0 );

	ULOCK();
	return size_all;
}


int sdfs_access(const char * path, int mask  ) {
	ACCESS_REQ_T arg;
	int * result;

	arg.path = (char *) path;
	arg.mask = mask;

	LOCK();

	result = rpc_access_0x0001(&arg , clnt);
	if ( result == NULL ) {
		clnt_perror(clnt, "call failed!");
		ULOCK();
		return -ENOENT;
	}
	ULOCK();
	return *result;
}

int sdfs_readlink(const char *path,char * buf, size_t size) {
	READLINK_RSP_T * result;
	READLINK_REQ_T arg;

	arg.path = (char *)path;
	arg.size = size;

	LOCK();

	result = rpc_readlink_0x0001(&arg,  clnt);
	if ( result == NULL ) {
		clnt_perror(clnt, "call failed!");
		ULOCK();
		return -ENOENT;
	}

	if( result->err ) {
		ULOCK();
		return result->err;
	}

	memcpy(buf, result->data.data_val, result->data.data_len);
	buf[result->data.data_len] = '\0';

	ULOCK();
	return 0;
}

int sdfs_mknod(const char* path, mode_t mode, dev_t rdev ) {

	MKNOD_REQ_T arg;
	int * result;

	arg.path = (char *) path;
	arg.mode = mode;
	arg.dev = rdev;

	LOCK();

	result = rpc_mknod_0x0001(&arg, clnt);
	if ( result == NULL ) {
		clnt_perror(clnt, "call failed!");
		ULOCK();
		return -ENOENT;
	}
	ULOCK();
	return *result;
}

int sdfs_mkdir(const char * path, mode_t mode) {
	MKDIR_REQ_T arg;
	int * result;

	arg.path = (char *)path;
	arg.mode = mode;

	LOCK();

	result = rpc_mkdir_0x0001( &arg, clnt);
	if ( result == NULL ) {
		clnt_perror(clnt, "call failed!");
		ULOCK();
		return -ENOENT;
	}
	ULOCK();
	return *result;
}

int sdfs_symlink(const char * from, const char * to ) {
	SYMLINK_REQ_T arg;
	int * result;

	arg.from = (char *) from;
	arg.to = (char *) to;

	LOCK();

	result = rpc_symlink_0x0001(&arg, clnt);
	if ( result == NULL ) {
		clnt_perror(clnt, "call failed!");
		ULOCK();
		return -ENOENT;
	}
	ULOCK();
	return *result;
}

int sdfs_unlink(const char * path) {
	int * result;
	char * filename = (char *)path;

	LOCK();

	result = rpc_unlink_0x0001(&filename, clnt);
	if ( result == NULL ) {
		clnt_perror(clnt, "call failed!");
		ULOCK();
		return -ENOENT;
	}
	ULOCK();
	return *result;
}

int sdfs_rmdir(const char * path) {
	int * result;
	char * filename = (char *)path;

	LOCK();

	result = rpc_rmdir_0x0001(&filename,  clnt);
	if ( result == NULL ) {
		clnt_perror(clnt, "call failed!");
		ULOCK();
		return -ENOENT;
	}
	ULOCK();
	return *result;
}

int sdfs_rename(const char * from, const char * to) {
	RENAME_REQ_T arg;
	int * result;

	arg.from = (char *) from;
	arg.to = (char *) to;

	LOCK();

	result = rpc_rename_0x0001(&arg, clnt);
	if ( result == NULL ) {
		clnt_perror(clnt, "call failed!");
		ULOCK();
		return -ENOENT;
	}
	ULOCK();
	return *result;	
}

int sdfs_link(const char * from, const char * to) {

	LINK_REQ_T arg;
	int * result;

	arg.from = (char *) from;
	arg.to = (char *) to;

	LOCK();

	result = rpc_link_0x0001(&arg, clnt);
	if ( result == NULL ) {
		clnt_perror(clnt, "call failed!");
		ULOCK();
		return -ENOENT;
	}
	ULOCK();
	return *result;	
}

int sdfs_chmod(const char *path, mode_t mode) {
	CHMOD_REQ_T arg;
	int * result;

	arg.path = (char *) path;
	arg.mode = mode;
	
	LOCK();

	result = rpc_chmod_0x0001(&arg, clnt);
	if ( result == NULL ) {
		clnt_perror(clnt, "call failed!");
		ULOCK();
		return -ENOENT;
	}
	ULOCK();
	return *result;	
}

int sdfs_chown(const char *path, uid_t uid, gid_t gid) {
	CHOWN_REQ_T arg;
	int * result;

	arg.path = (char *)path;
	arg.uid = uid;
	arg.gid = gid;
	
	LOCK();

	result = rpc_chown_0x0001(&arg, clnt);
	if ( result == NULL ) {
		clnt_perror(clnt, "call failed!");
		ULOCK();
		return -ENOENT;
	}
	ULOCK();
	return *result;
}

int sdfs_truncate(const char *path, off_t size ) {
	TRUNCATE_REQ_T arg;
	int * result;

	arg.path = (char *)path;
	arg.size = size;

	LOCK();

	result = rpc_truncate_0x0001(&arg,  clnt);
	if ( result == NULL ) {
		clnt_perror(clnt, "call failed!");
		ULOCK();
		return -ENOENT;
	}

	ULOCK();
	return *result;
}

int sdfs_statfs(const char * path, struct statvfs *stbuf) {

	STATVFS_RSP_T * result;
	char *filename = (char *) path;

	LOCK();

	result = rpc_statvfs_0x0001( &filename, clnt);
	if ( result == NULL ) {
		clnt_perror(clnt, "call failed!");
		ULOCK();
		return -ENOENT;
	}

	if( result->err ) {
		ULOCK();
		return result->err;
	}

	memset(stbuf,0,sizeof(struct statvfs));

	stbuf->f_bsize = result->bsize;
	stbuf->f_frsize = result->frsize;
	stbuf->f_blocks = result->blocks;
	stbuf->f_bfree = result->bfree;
	stbuf->f_bavail = result->bavail;
	stbuf->f_files = result->files;
	stbuf->f_ffree = result->ffree;
	stbuf->f_favail = result->favail;
	stbuf->f_fsid = result->fsid;
	stbuf->f_flag = result->flag;
	stbuf->f_namemax = result->namemax;

	ULOCK();
	return 0;
}

int sdfs_fsync(const char * path, int isdatasync, struct fuse_file_info * fi) {

	log(path);

	//not support

	return 0;
}

int sdfs_create(const char *path, mode_t mode, struct fuse_file_info * fi)
{
	CREATE_REQ_T arg;
	int * result;
	int fd;

	LOCK();

	fd = sdfs_allocFd((char *) path);
	if (-1 == fd) 
	{
		ULOCK();
		return -errno;
	}

	arg.path = (char *)path;
	arg.mode = mode;
	arg.flag = fi->flags;

	result = rpc_create_0x0001(&arg, clnt);
	if ( result == NULL ) {
		clnt_perror(clnt, "call failed!");

		sdfs_freeFd( fd);

		ULOCK();
		return -ENOENT;
	}

	if( *result ) {
		sdfs_freeFd( fd);

		ULOCK();
		return *result;
	}

	fi->fh = fd;

	ULOCK();
	return 0;
}

int sdfs_release( const char * path, struct fuse_file_info *fi ) {
	sdfs_freeFd(fi->fh);
	return 0;
}

#ifdef HAVE_SETXATTR

int sdfs_setxattr(const char *path, 
					const char *name,
					const char *value, 
					size_t size,
					int flags)
{
	SETXATTR_REQ_T arg;
	int * result;

	arg.path = (char *)path;
	arg.name = (char *)name;
	arg.flag = flags;

	arg.value.value_val = value;
	arg.value.value_len = size;

	LOCK();

	result = rpc_setxattr_0x0001(&arg, clnt);
	if ( result == NULL ) {
		clnt_perror(clnt, "call failed!");

		ULOCK();
		return -ENOENT;
	}

	ULOCK();
	return *result;
}

int sdfs_getxattr(const char *path, 
					const char *name,
					char * value,
					size_t size) 
{


}

int sdfs_listxattr(const char *path, char * list, size_t size) 
{

}

int sdfs_removexattr(const char * path, const char * name)
{


}


#endif

int sdfs_opendir(const char * path, struct fuse_file_info * fi ) 
{
	int res;
	int size;
	struct sdfs_dir_p * d;

	READDIR_REQ_T arg;
	READDIR_RSP_T * result;

	size = strlen(path);
	if (size > MAX_FILE_NAME ) 
		return -ENAMETOOLONG;

	arg.path = (char *) path;
	arg.size = 0;
	arg.offset = 0;

	LOCK();

	result = rpc_readdir_0x0001(&arg,  clnt);
	if ( result == NULL ) {
		clnt_perror(clnt, "call failed!");

		ULOCK();
		return -ENOENT;
	}

	if( result->err ) {
		ULOCK();
		return result->err;
	}

	d = (struct sdfs_dir_p *)malloc(sizeof(struct sdfs_dir_p));
	if (NULL == d ) 
	{
		ULOCK();
		return -ENOMEM;
	}

	strncpy(d->path, path, MAX_FILE_NAME + 1);

	d->offset = 0;

	fi->fh = (unsigned long)d;

	ULOCK();
	return 0;
}

int sdfs_releasedir(const char * path, struct fuse_file_info *fi) 
{
	struct sdfs_dir_p * d = (struct sdfs_dir_p *)fi->fh;

	d->offset = 0;
	d->path[0] = '\0';

	free(d);

	return 0;	
}

int sdfs_fgetattr(const char * path,
					struct stat * stbuf,
					struct fuse_file_info * fi ) 
{
	return sdfs_getattr(path, stbuf);
}

int sdfs_ftruncate(const char *path, off_t size , struct fuse_file_info * fi)
{
	return sdfs_truncate( path,  size);
}

int sdfs_utimens(const char * path, const struct timespec ts[2] )
{
	// To be do;
	return 0;
}

int sdfs_flush(const char *path, struct fuse_file_info * fi )
{
	return 0;
}

#ifdef HAVE_POSIX_FALLOCATE

int sdfs_fallocate(const char *path,
					int mode,
					off_t offset,
					off_t length )
{
	FALLOCATE_REQ_T arg;
	int * result;

	arg.path = (char *) path;
	arg.mode = mode;
	arg.offset = offset;
	arg.length = length;

	LOCK();

	result = rpc_fallocate_0x0001(&arg,clnt);
	if ( result == NULL ) {
		clnt_perror(clnt, "call failed!");

		ULOCK();
		return -ENOENT;
	}

	ULOCK();
	return *result;
}

#endif



static struct fuse_operations sdfs_oper = {
	.getattr	= sdfs_getattr,
	.fgetattr	= sdfs_fgetattr,
	.access		= sdfs_access,
	.readlink	= sdfs_readlink,
	.opendir	= sdfs_opendir,
	.readdir	= sdfs_readdir,
	.releasedir	= sdfs_releasedir,
	.mknod		= sdfs_mknod,
	.mkdir		= sdfs_mkdir,
	.symlink	= sdfs_symlink,
	.unlink		= sdfs_unlink,
	.rmdir		= sdfs_rmdir,
	.rename		= sdfs_rename,
	.link		= sdfs_link,
	.chmod		= sdfs_chmod,
	.chown		= sdfs_chown,
	.truncate	= sdfs_truncate,
	.ftruncate	= sdfs_ftruncate,
#ifdef HAVE_UTIMENSAT
	.utimens	= sdfs_utimens,
#endif
	.create		= sdfs_create,
	.open		= sdfs_open,
	.read		= sdfs_read,
	.read_buf	= NULL,
	.write		= sdfs_write,
	.write_buf	= NULL,
	.statfs		= sdfs_statfs,
	.flush		= sdfs_flush,
	.release	= sdfs_release,
	.fsync		= sdfs_fsync,
#ifdef HAVE_POSIX_FALLOCATE
	.fallocate	= sdfs_fallocate,
#endif
#ifdef HAVE_SETXATTR
	.setxattr	= sdfs_setxattr,
	.getxattr	= sdfs_getxattr,
	.listxattr	= sdfs_listxattr,
	.removexattr	= sdfs_removexattr,
#endif
	.lock		= NULL,
	.flock		= NULL,

	.flag_nullpath_ok = 1,
#if HAVE_UTIMENSAT
	.flag_utime_omit_ok = 1,
#endif
};



int main(int argc, char *argv[])
{
	int ret;
	
	clnt = clnt_create(host,SDFS_PROG, SDFS_V1, "udp");
	if (NULL == clnt) 
	{
		clnt_pcreateerror(host);
		return 1;
	}

	ret = fuse_main(argc, argv, &sdfs_oper, NULL);

	clnt_destroy(clnt);
	
	return ret;
}


