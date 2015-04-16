#ifndef __LIBRVM__
#define __LIBRVM__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <list>
#include <vector>

using namespace std;

#define ALREADYMAPPED 0
#define NOTMAPPED 1

#define BEINGUSED 0
#define NOTBEINGUSED 1

typedef struct _seg_struct
{
	int seg_size;
	void* data;
	char* segname;
	int seg_state;
	int beingused;
	void* undo_log;
	int fh;
} seg_struct;

struct _rvm_struct
{
	char directory[1000];
	vector<seg_struct*> seglist;
	int rvm_numsegs;
	int logfd;	//log file descriptor
};

typedef struct _transzone_struct
{
	seg_struct* parent_seg;
	int offset;
	int tzone_size;
} tzone_struct;

struct _trans_struct
{
	struct _rvm_struct* trans_rvm;
	vector<seg_struct*> seglist;
	int trans_numsegs;
	vector<tzone_struct*> tzones;
	int trans_numzones;
	int transid;
	int trans_state;
};

typedef struct _rvm_struct* rvm_t;
typedef struct _trans_struct* trans_t;

rvm_t rvm_init(const char *directory);
void *rvm_map(rvm_t rvm, const char *segname, int size_to_create);
void rvm_unmap(rvm_t rvm, void *segbase);
void rvm_destroy(rvm_t rvm, const char *segname);
trans_t rvm_begin_trans(rvm_t rvm, int numsegs, void **segbases);
void rvm_about_to_modify(trans_t tid, void *segbase, int offset, int size);
void rvm_commit_trans(trans_t tid);
void rvm_abort_trans(trans_t tid);
void rvm_truncate_log(rvm_t rvm);

#endif
