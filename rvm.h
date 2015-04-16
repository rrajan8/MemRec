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
#include <stack>
#include <unordered_map>

#define MAPPED 1
#define NOTMAPPED 0

using namespace std;


typedef struct _seg_struct
{
	char[100] segname;
	void* baseaddr;
	int currsize;
	int size2use;
	trans_t trans_owner;	//pointer to the transaction struct that owns us
	int fh;					//file handle
} seg_struct;

struct _rvm_struct
{
	int id;
	char directory[1000];
	unordered_map<void*, seg_struct*> seglist;
	int numsegs;
};

typedef struct _rvm_struct* rvm_t;

typedef struct _tzone_struct
{
	seg_struct* parentseg;
	int offset;
	int size;
	void* memdata;
} tzone_struct;

struct _trans_struct
{
	int id;
	rvm_t* rvm;
	unordered_map<void*, seg_struct*> seglist;
	int numsegs;
	stack<tzone_struct*> undolog;
	int numtzones;
};

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
