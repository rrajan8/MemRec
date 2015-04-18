#include "rvm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <vector>
#include <unordered_map>

#define MAPPED 1
#define NOTMAPPED 0

using namespace std;



typedef std::unordered_map<string, seg_struct*> msegmap;

struct _rvm_struct
{
	int id;
	string directory;
	unordered_map<void*, seg_struct*> m_seglist;
	unordered_map<string, seg_struct*> seglist;
	int numsegs;
};



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
	rvm_t rvm;
	unordered_map<void*, seg_struct*> seglist;
	int numsegs;
	//stack<tzone_struct*> undolog;
	//std::deque<tzone_struct*> undolog ;
	std::vector<tzone_struct*> undolog;
	int numtzones;
};



struct _seg_struct
{
	string segname;			//.seg includeds
	void* baseaddr;
	int currsize;
	int size2use;
	trans_t trans_owner;	//pointer to the transaction struct that owns us
};

void rvm_truncate_remap_log(rvm_t rvm, string segname);