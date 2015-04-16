#include "rvm.h"

using namespace std;

void getlogdata(rvm_t rvm, seg_struct* segment)
{
	int offset, size;
	struct stat fs;
	fstream logfile;
	string str;
	size_t index1, index2, index3;
	char* path = (char*) malloc(sizeof(char)*(strlen(rvm->directory)+4));
	//create log path based on rvm directory
	strcpy(path, rvm->directory);
	strcat(path, "/log");
	//use stat to find size of log file
	stat(path, &fs);
	if(fs.st_size == 0)
	{
		//empty log file
		printf("empty logfile\n");
		return;
	}
	//use path to open fstream
	logfile.open(path, ios::binary | ios::in);
	while(logfile.good())
	{
		getline(logfile, str);
		//find the segment name 
		index1 = str.find("Segbase:");
		if(index1 != string::npos) //npos is indication of not finding it
		{
			while( logfile.good() )
			{
				getline(logfile, str);
				index2 = str.find("Offset:");
				index3 = str.find(" ");
				if((index2!=string::npos) &&(index3!=string::npos))
				{
					offset = atoi((str.substr(index2+7, index3-index2)).c_str());
					size = atoi((str.substr(index3+6)).c_str());
					//write changes to memory, seen when segments are mapped
					logfile.read(((char*)segment->data)+offset, size);
					break;
				} 
			}
		}

	}
	logfile.close();
}

rvm_t rvm_init(const char *directory)
//Initialize the library with the specified directory as backing store.
{	
	char path[200];
	struct stat fs;
	//initialize the rvm 
	rvm_t rvm = (rvm_t) malloc(sizeof(rvm_t));
	//check is directory exists or not
	strcpy(rvm->directory, directory);
	//stat fills fs struct with info about directory// 0 success -1 failed
	if(stat(directory, &fs) == -1){
		//directory doesnt exist, create it
		strcpy(path, "mkdir ");
		strcat(path, directory);
		//call terminal instruction to make new directory
		system(path);
		//new directory created!
		//populate rvm structure
		strcpy(path, directory);
		strcat(path, "/log");
		// open log file with read/write priviledges and if not existing, create
		rvm->logfd = open(path, O_RDWR | O_CREAT);
		close(rvm->logfd);
		rvm->rvm_numsegs = 0;	
	}
	else
	{
		//the directory already exists, simply populate the rvm structure
		strcpy(path, directory);
		strcat(path, "/log");
		// open log file with read/write priviledges, already exists
		rvm->logfd = open(path, O_RDWR);
		close(rvm->logfd);
		rvm->rvm_numsegs = 0;
	}
	system("chmod -R 777 *");
	return rvm;
}

void *rvm_map(rvm_t rvm, const char *segname, int size_to_create)
{
	int x, pos, fd;
	struct stat fs;
	char* segmentname = (char*)malloc(sizeof(char)*(strlen(segname)+strlen(rvm->directory)+1));
	strcpy(segmentname, rvm->directory);
	strcat(segmentname, "/");
	strcat(segmentname, segname);
	//check if the segname is already in the list of segments for the rvm
	pos = -1;
	for(x=0;x<rvm->rvm_numsegs;x++){
		if(!strcmp(rvm->seglist[x]->segname, segname))
		{
			pos = x;
		}
	}
	if((pos == -1) || (rvm->seglist[pos]->seg_state == NOTMAPPED))
	{
		//segment is not mapped as yet, we have to map it
		//initialize segment and populate new segment entry
		seg_struct* segment = (seg_struct*) malloc (sizeof(seg_struct));
		segment->segname = (char*) malloc (sizeof(char)*strlen(segmentname));
		strcpy(segment->segname, segmentname);
		segment->seg_size = size_to_create;

		if(stat(segmentname, &fs) == -1)
		{
			fd = open(segmentname, O_WRONLY | O_CREAT | O_EXCL, (mode_t)0600);
			//this stretches the file to match the desired size_to_create
			lseek(fd, size_to_create-1, SEEK_SET);
			//write to end of file to confirm stretch
			write(fd,"",1);
		}
		else
		{
			fd = open(segmentname, O_RDWR);
			stat(segmentname, &fs);
			if(fs.st_size < size_to_create)
			{
				//if existing mapped segment is less than desired size_to_create, then stretch it
				lseek(fd, size_to_create-1, SEEK_SET);
				//write to end of file to confirm stretch
				write(fd, "", 1);
			}
		}

		//populate reset of segment structure
		
		lseek(fd, 0, SEEK_SET);
		segment->fh = fd;
		segment->seg_state = ALREADYMAPPED;
		segment->beingused = NOTBEINGUSED;
		segment->data = malloc(sizeof(char)*size_to_create);
		segment->undo_log = malloc(sizeof(char)*size_to_create);
		//read from file descriptor into segment data - mapping the disk to memory
		read(fd, segment->data, size_to_create);
		//place same data in undo log for now
		memcpy(segment->undo_log, segment->data, size_to_create);

		//check the log for data that already exists for this segment
		getlogdata(rvm, segment);

		//update rvm seg count
		rvm->rvm_numsegs++;

		close(fd);

		return segment->data;
	}
	else
	{
		//segment already mapped
		printf("segment already mapped\n");
		exit(0);
	}
}


void rvm_unmap(rvm_t rvm, void *segbase)
{	
	int x;
	for(x=0; x<rvm->rvm_numsegs; x++)
	{
		if(rvm->seglist[x]->data == segbase)
		{
			rvm->seglist[x]->seg_state = NOTMAPPED;
			return;
		}
	}
	return;
}

void rvm_destroy(rvm_t rvm, const char *segname)
{
	int x = 0;
	int pos = -1;
	char path[1000];
	strcpy(path, rvm->directory);
	strcat(path, "/");
	strcat(path, segname);

	//find position of segment name in rvm
	for(x=0; x<rvm->rvm_numsegs; x++)
	{
		if(!strcmp(segname, rvm->seglist[x]->segname))
		{
			pos = x;
		}
	}

	if(pos != -1)
	{
		//found the segment in the list
		if(rvm->seglist[pos]->seg_state == NOTMAPPED)
		{			
			remove(path);
			return;
		}

		else
		{
			printf("cannot destroy mapped segment\n");
			return;
		}
	}
	else
		//not in segment list
	{
		remove(path);
		return;
	}
}

trans_t rvm_begin_trans(rvm_t rvm, int numsegs, void **segbases)
{
	int pos = -1;
	int i;
	for(i = 0; i<numsegs; i++)
	{
		pos = -1;
		int x = 0;
		for(x=0; x<rvm->rvm_numsegs;x++)
		{
			if(rvm->seglist[x]->data == segbases[i])
			{
				//found the segment - check if already in use
				pos = x;
				if(rvm->seglist[x]->beingused == BEINGUSED)
				{
					//invalide use
					printf("segment already being used\n");
					return (trans_t) -1;
				}
				//set to be used now
				rvm->seglist[x]->beingused = BEINGUSED;
			}
		}
		if(pos == -1)
		{
			printf("segment does not exist\n");
			return (trans_t) -1;
		}
	}

	trans_t trans = (trans_t)malloc(sizeof(trans_t));
	trans->trans_rvm = rvm;
	trans->trans_numsegs = numsegs;
	trans->trans_numzones = 0;
	for(i=0; i<numsegs; i++)
	{
		int x = 0;
		for(x=0; x<rvm->rvm_numsegs;x++)
		{
			if(rvm->seglist[x]->data == segbases[i])
			{ 
				trans->seglist.push_back(rvm->seglist[x]);
			}
		}
	}
	return trans;	
}


void rvm_about_to_modify(trans_t tid, void *segbase, int offset, int size)
{
	tzone_struct* tzone = (tzone_struct*)malloc(sizeof(tzone_struct));
	int pos = -1;
	int x = 0;
	for (x=0;x<tid->trans_numsegs;x++)
	{
		if(tid->seglist[x]->data == segbase){
			//found it
			pos = x;
			tzone->parent_seg = tid->seglist[x];
			tzone->offset = offset;
			tzone->tzone_size = size;
		}
	}
	if(pos == -1){
		printf("segment not found\n");
		return;
	}
	tid->tzones.push_back(tzone);
	tid->trans_numzones++;
	return;
}


void rvm_commit_trans(trans_t tid)
{
	int i;
	char logfile[200];
	strcpy(logfile, tid->trans_rvm->directory);
	strcat(logfile, "/log");

	int lf = open(logfile, O_WRONLY | O_APPEND);
	for(i=0; i<tid->trans_numzones; i++)
	{
		write(lf, "Segbase:", 8);
		write(lf, tid->tzones[i]->parent_seg->segname, strlen(tid->tzones[i]->parent_seg->segname));
		write(lf, "\n", 1);
		char buffer[100];
		sprintf(buffer, "%d", tid->tzones[i]->offset);
		write(lf, "Offset:", 7);
		write(lf, buffer, strlen(buffer));
		write(lf, " ", 1);
		sprintf(buffer, "%d", tid->tzones[i]->tzone_size);
		write(lf, "Size:", 5);
        write(lf, buffer, strlen(buffer));
		write(lf, "\n", 1);
		write(lf, tid->tzones[i]->parent_seg->data + tid->tzones[i]->offset, tid->tzones[i]->tzone_size);
		write(lf, "\n\n", 2);
		memcpy(tid->tzones[i]->parent_seg->undo_log + tid->tzones[i]->offset, tid->tzones[i]->parent_seg->data + tid->tzones[i]->offset, tid->tzones[i]->tzone_size);
		tid->tzones[i]->parent_seg->beingused = NOTBEINGUSED;
	}
	close(lf);
	system("chmod -R 777 *");	
}


void rvm_abort_trans(trans_t tid);
void rvm_truncate_log(rvm_t rvm);
