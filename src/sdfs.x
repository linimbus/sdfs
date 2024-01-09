struct read_req {
	string path<256>;
	int size;
	int offset;
};

typedef struct read_req READ_REQ_T;

struct read_rsp {
	int err;
	int size;
	opaque data<>;
};

typedef struct read_rsp READ_RSP_T;

struct write_req {
	string path<256>;
	int size;
	int offset;
	opaque data<>;
};

typedef struct write_req WRITE_REQ_T;

struct write_rsp {
	int err;
	int size;
};

typedef struct write_rsp WRITE_RSP_T;


struct getattr_req {
	string path<256>;
};

typedef struct getattr_req GETATTR_REQ_T;


struct getattr_rsp {
	int err;
	unsigned long ino;
	unsigned int mode;
	unsigned int uid;
	unsigned int gid;
	unsigned int atime;
	unsigned int mtime;
	unsigned int ctime;
	unsigned int nlink;
	unsigned long size;
};

typedef struct getattr_rsp GETATTR_RSP_T;


struct readdir_req {
	string path<256>;
	int size;
	int offset;
};

typedef struct readdir_req READDIR_REQ_T;


struct readdir_rsp {
	int err;
	int size;
	opaque data<>;
};

typedef struct readdir_rsp READDIR_RSP_T;


struct open_req {
	string path<256>;
	int flag;
};

typedef struct open_req OPEN_REQ_T;


struct access_req {
	string path<256>;
	int mask;
};

typedef struct access_req ACCESS_REQ_T;


struct readlink_req {
	string path<256>;
	int size;
};

typedef struct readlink_req READLINK_REQ_T;

struct readlink_rsp {
	int err;
	opaque data<>;
};

typedef struct readlink_rsp READLINK_RSP_T;


struct mknod_req {
	string path<256>;
	unsigned int mode;
	unsigned int dev;
};

typedef struct mknod_req MKNOD_REQ_T;


struct mkdir_req {
	string path<256>;
	unsigned int mode;
};

typedef struct mkdir_req MKDIR_REQ_T;


struct symlink_req {
	string from<256>;
	string to<256>;
};

typedef struct symlink_req SYMLINK_REQ_T;



struct rename_req {
	string from<256>;
	string to<256>;
};

typedef struct rename_req RENAME_REQ_T;



struct link_req {
	string from<256>;
	string to<256>;
};

typedef struct link_req LINK_REQ_T;





struct chmod_req {
	string path<256>;
	unsigned int mode;
};

typedef struct chmod_req CHMOD_REQ_T;



struct chown_req {
	string path<256>;
	unsigned int uid;
	unsigned int gid;
};

typedef struct chown_req CHOWN_REQ_T;


struct truncate_req {
	string path<256>;
	unsigned int size;
};

typedef struct truncate_req TRUNCATE_REQ_T;




struct statvfs_rsp {
	int err;
	
	unsigned long int bsize;
	unsigned long int frsize;
	unsigned long int blocks;
	unsigned long int bfree;
	unsigned long int bavail;
	unsigned long int files;
	unsigned long int ffree;
	unsigned long int favail;
	unsigned long int fsid;
	unsigned long int flag;
	unsigned long int namemax;
	unsigned int spare[6];
};

typedef struct statvfs_rsp STATVFS_RSP_T;


struct create_req {
	string path<256>;
	unsigned int flag;
	unsigned int mode;
};

typedef struct create_req CREATE_REQ_T;


struct setxattr_req {
	string path<256>;
	string name<256>;
	opaque value<>;
	unsigned int flag;
};

typedef struct setxattr_req SETXATTR_REQ_T;


struct getxattr_req {
	string path<256>;
	string name<256>;
	unsigned int size;
};

typedef struct getxattr_req GETXATTR_REQ_T;


struct getxattr_rsp {
	int err;
	opaque value<>;
};

typedef struct getxattr_rsp GETXATTR_RSP_T;



struct listxattr_req {
	string path<256>;
	unsigned int size;
};

typedef struct listxattr_req LISTXATTR_REQ_T;


struct listxattr_rsp {
	int err;
	opaque value<>;
};

typedef struct listxattr_rsp LISTXATTR_RSP_T;




struct removexattr_req {
	string path<256>;
	string name<256>;
};

typedef struct removexattr_req REMOVEXATTR_REQ_T;

struct fallocate_req {
	string path<256>;
	unsigned int mode;
	unsigned int offset;
	unsigned int length;
};

typedef struct fallocate_req FALLOCATE_REQ_T;


program SDFS_PROG {
	
	version SDFS_V1 {
		
		READ_RSP_T rpc_read(READ_REQ_T) = 1;
		WRITE_RSP_T rpc_write(WRITE_REQ_T) = 2;
		
		int rpc_open(OPEN_REQ_T) = 3;
		int rpc_create(CREATE_REQ_T) = 4;
		
		GETATTR_RSP_T rpc_getattr(GETATTR_REQ_T) = 10;
		READDIR_RSP_T rpc_readdir(READDIR_REQ_T) = 11;
		
		int rpc_access(ACCESS_REQ_T) = 20;
		int rpc_mknod(MKNOD_REQ_T) = 21;
		int rpc_mkdir(MKDIR_REQ_T) = 22;
		int rpc_unlink(string)     = 23;
		int rpc_rmdir(string)      = 24;
		int rpc_symlink(SYMLINK_REQ_T) = 25;
		int rpc_rename(RENAME_REQ_T) = 26;
		int rpc_link(LINK_REQ_T)     = 27;
		int rpc_chmod(CHMOD_REQ_T)   = 28;
		int rpc_chown(CHOWN_REQ_T)   = 29;
		
		int rpc_truncate(TRUNCATE_REQ_T) = 30;
		
		READLINK_RSP_T rpc_readlink(READLINK_REQ_T) = 41;
		STATVFS_RSP_T rpc_statvfs(string) = 42;
		
		int rpc_setxattr(SETXATTR_REQ_T) = 43;
		GETXATTR_RSP_T rpc_getxattr(GETXATTR_REQ_T) = 44;
		LISTXATTR_RSP_T rpc_listxattr(LISTXATTR_REQ_T) = 45;
		
		int rpc_removexattr(REMOVEXATTR_REQ_T) = 46;
		int rpc_fallocate(FALLOCATE_REQ_T) = 47;
	
	} = 0x0001;
} = 0x00010000;


