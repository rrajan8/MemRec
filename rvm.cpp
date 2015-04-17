#include "rvm.h"
#include <string>
#include <sstream>
#include <iostream>
using namespace std;

int rvm_id = 0;
int trans_id = 0;

rvm_t rvm_init(const char *directory)
//Initialize the library with the specified directory as backing store.
{	
	char path[200];
	struct stat fs;
	
	stringstream filenames;

	//initialize the rvm 
	rvm_t rvm = new struct _rvm_struct;
	rvm->id = rvm_id++;
	rvm->numsegs = 0;
	//check is directory exists or not
	
	string valstr(directory);
	
	rvm->directory = valstr;
	
	
	filenames.clear();
	//stat fills fs struct with info about directory// 0 success -1 failed
	if(stat(directory, &fs) == -1){
		//directory doesnt exist, create it
		
		strcpy(path, "mkdir ");
		
		strcat(path, directory);
	
		//call terminal instruction to make new directory
		system(path);
		
		//new directory created!
		//populate rvm structure
		// this is a new directory so there are no segments
	}
	else
	{
		//the directory already exists, simply populate the rvm structure
		
		string location;
		
		strcpy(path, directory);
		
		filenames << path;
		filenames >> location;
		
		string command = "/bin/ls ";
		command.append(location);
		command.append("/*.seg");
		
		int numsegs = 0;
		
		FILE *proc = popen(command.c_str(),"r");
		char buf[300];
		filenames.clear();
		while ( !feof(proc) && fgets(buf,sizeof(buf),proc) )
		{
			string temp;
			seg_struct* seg = new seg_struct;
			
			filenames <<  buf;
			filenames >> temp;
			
			size_t i = temp.rfind("/", temp.length());
			seg->segname = temp.substr(i+1,temp.length()-1);
			 
			seg->currsize = -1;
			seg->size2use = -1;
			seg->trans_owner = NULL;
			seg->baseaddr = NULL;
			rvm->seglist[seg->segname] = seg;
			
			temp.append(".log");
			ofstream myfile;
  			myfile.open (temp, ofstream::in);
  			myfile.close();
			
			rvm->numsegs++;
		}
	}
	system("chmod -R 777 *"); //will have to fix this later
	return rvm;
}

void *rvm_map(rvm_t rvm, const char *segname, int size_to_create)
{
	cout << "lol2" << endl;
	stringstream ss;
	string seg_name;
	ss << segname;
	ss >> seg_name;
	seg_name.append(".seg");
	cout << "lol1" << endl;
	if(rvm->seglist.find(seg_name) == rvm->seglist.end())
	{
		//createing metadata for the segment
		seg_struct* seg = new seg_struct;
		seg->segname = seg_name;
		seg->currsize = -1;
		seg->size2use = -1;
		seg->trans_owner = NULL;
		seg->baseaddr = NULL;
		rvm->seglist[seg->segname] = seg;
		rvm->numsegs++;
		
		//create a new segment file
		
		string directory = rvm->directory;
		
		directory.append("/");
	
		directory.append(seg_name);
		cout << directory << endl;
		ofstream myfile;
  		myfile.open (directory, ofstream::out);
  		myfile.close();
  	
  		//create a new log file
		directory.append(".log");
  		myfile.open (directory, ofstream::out);
  		myfile.close();
	}
	
	
	if(rvm->seglist[seg_name]->trans_owner != NULL)
	{
		cout << "cannot map: segment undergoing transaction" << endl;
	}
	
	if(rvm->seglist[seg_name]->baseaddr == NULL) //unmapped segment
	{
		
		rvm->seglist[seg_name]->baseaddr = (void*)malloc(size_to_create);
		rvm->seglist[seg_name]->size2use = size_to_create;
		cout << "lol0989etartAdsfddfsad" << endl;
		rvm->seglist[seg_name]->currsize = size_to_create;
		
		rvm->m_seglist[rvm->seglist[seg_name]->baseaddr] = rvm->seglist[seg_name];
		cout << "lol0989etartAdsad" << endl;
	}
	else
	{
		cout << "segment is already mapped" << endl;
		if(rvm->seglist[seg_name]->currsize >= size_to_create)
		{
			rvm->seglist[seg_name]->size2use = 	size_to_create;
		}
		else
		{
			void* temp = (void*) malloc(size_to_create);
			memcpy ( temp, rvm->seglist[seg_name]->baseaddr, rvm->seglist[seg_name]->currsize);
			free(rvm->seglist[seg_name]->baseaddr);
			rvm->seglist[seg_name]->baseaddr = temp;
		}
	}
	
	rvm_truncate_remap_log(rvm, seg_name);
	cout << "lol0989etarsadtAdsad" << endl;
	return rvm->seglist[seg_name]->baseaddr;
}


void rvm_unmap(rvm_t rvm, void *segbase)
{
	if(rvm->m_seglist[segbase]->trans_owner != NULL){
		cout << "cannot unmap: segment is in a transaction" << endl;
		return;
	}
	rvm->m_seglist[segbase]->currsize = -1;
	rvm->m_seglist[segbase]->size2use = -1;
	free(rvm->m_seglist[segbase]->baseaddr);
	rvm->m_seglist[segbase]->baseaddr = NULL;
	rvm->m_seglist.erase(segbase);
	return;
}

void rvm_destroy(rvm_t rvm, const char *segname)
{
	string seg_name = segname;
	seg_name.append(".seg");
	if(rvm->seglist[seg_name]->baseaddr != NULL )
	{
		cout << "cannot destroy: is mapped" << endl;
		return;
	}
	string del("rm ");
	string directory = rvm->directory;
	directory.append("/");
	directory.append(seg_name);
	del.append(directory);
	system(del.c_str());
	del.append("log");
	system(del.c_str());
	
	rvm->seglist.erase(seg_name);
	rvm->numsegs--;
}

trans_t rvm_begin_trans(rvm_t rvm, int numsegs, void **segbases)
{
	trans_t trans = new _trans_struct;
	trans->id = trans_id++;
	trans->rvm = rvm;
	trans->numsegs = numsegs;
	trans->numtzones = 0;
	
	for(int i = 0; i<numsegs; i++)
	{
		if(rvm->m_seglist.find(segbases[i]) == rvm->m_seglist.end() || rvm->m_seglist[segbases[i]]->trans_owner != NULL)
		{
			trans->seglist.clear();
			delete trans;
			return (trans_t)-1;
		}
		
		trans->seglist[segbases[i]] = rvm->m_seglist[segbases[i]];
		trans->seglist[segbases[i]]->trans_owner = trans;
		
		
	}

	
	return trans;	
}


void rvm_about_to_modify(trans_t tid, void *segbase, int offset, int size)
{
	
	if(tid->seglist.find(segbase) == tid->seglist.end())
	{
		cout << "error: segment not part of transaction id " << tid->id << endl;
		return;
	}
	
	tzone_struct* logger = new tzone_struct;
	logger->parentseg = tid->seglist[segbase];
	logger->offset = offset;
	logger->size = size;
	logger->memdata = malloc(size);
	
	tid->undolog.push_back(logger);
	tid->numtzones++;
	
}


void rvm_commit_trans(trans_t tid)
{
	//modifies the logfile of each segment affected by transaction
	//use the undolog to get the offsets and size of memdata to commit to logfile
	// segmment-(baseaddr + offset) -> start point
	// write (size) bytes from there to log
	// do not use the unlog's data from the tzones
	
	int offset, size;
	void* segaddr;
	tzone_struct* tzone;
	
	//traverse the undolog to find segbases, offsets and sizes
	//get the undolog in the transaction to be committed
	//traverse logfile from oldest to newest entry
	for (int i = 0; i < tid->numtzones; i++)
	{
		tzone = tid->undolog[i];
		//for each tzone, get the offset and size and parentseg base address
		segaddr = tzone->parentseg->baseaddr;
		offset = tzone->offset;
		size = tzone->size;
		//now we got to commit the memdata using the baseadd + offset + size to the logfile
		//get the parent_segment's logfile
		string path;
		path = tid->rvm->directory;
		//strcpy(path, tid->rvm->directory.c_str());
		path.append("/");
		//strcat(path, "/");
		path.append(tzone->parentseg->segname);
		path.append(".log");
		//strcat(path, tzone->parentseg->segname.c_str());
		//strcat(path, ".log");
		//now we have the pseg.log file to commit in to
		std::fstream fs;
		fs.open (path, std::fstream::in | std::fstream::out | std::fstream::app | std::fstream::binary);
		//read/write/append/binary mode
		// offset <|||||> size <|||||> /n data (size) >
		fs <<  offset << "|||||" << size << "|||||" << endl ;
		//put memdata followed by tab for delimiter
		fs.write(((char*)segaddr+offset), size);
		fs << endl;
		fs.close();
	}
	//finished committing the transactions, the segments will have no more transownwer
	for(int i = 0; i < tid->numtzones; i++)
	{
		tzone = tid->undolog[i];
		//set the manager of all parentsegs to NULL
		tzone->parentseg->trans_owner = NULL;
	}
	
}


void rvm_abort_trans(trans_t tid)
{
	//aborting - copy data from the undo logs back into the memdata
	// copy memdata from zone->data tp memdata
	
	int offset, size;
	void* segaddr;
	tzone_struct* rtzone;
	
	//traverse tzones most recent to latest - from bottom to top
	//std::vector<tzone_struct>::reverse_iterator rtzone = tid->undolog.rbegin();
	for(int i = tid->numtzones-1; i >= 0; i--)
	{
		//copy from tzone->data to parent_seg->memdata - undoing transactions in reverse
		//memcpy (Dest. src. size)
		rtzone = tid->undolog[i];
		segaddr = rtzone->parentseg->baseaddr;
		offset = rtzone->offset;
		size = rtzone->size;
		memcpy((char*)segaddr+offset, rtzone->memdata, size);	
	}
	//finished aborting the transactions, the segments will have no more transownwer
	for(int i = 0; i < tid->numtzones; i++)
	{
		rtzone = tid->undolog[i];
		//set the manager of all parentsegs to NULL
		rtzone->parentseg->trans_owner = NULL;
	}
	
}


void rvm_truncate_log(rvm_t rvm)
{
	// go through all the logs of each segment and commit the data to the segment file
	string segname;
	seg_struct* segment;
	//iterate through all the logs/segment files
	for(msegmap::iterator it = rvm->seglist.begin(); it != rvm->seglist.end(); ++it)
	{
		//each of these segments have log files
		segname = it->first;	//get the key - base address memdata
		segment = it->second;	///get the segment
		//now we can get the name of the segment to  create the path of file
		string segpath;
		segpath = rvm->directory;
		//strcpy(path, tid->rvm->directory.c_str());
		segpath.append("/");
		//strcat(path, "/");
		segpath.append(segment->segname); 
		//segpath.append(".seg");
		string logpath;
		logpath = rvm->directory;
		//strcpy(path, tid->rvm->directory.c_str());
		logpath.append("/");
		//strcat(path, "/");
		logpath.append(segment->segname);
		logpath.append(".log");
		//now we have both file paths - log and seg 

		struct stat filestat;
		//check if logfile is empty, then nothing to do
		stat(logpath.c_str(), &filestat);
		if(filestat.st_size == 0){
			printf("logfile is empty - nothing to truncate - in trunc\n");
			//go to next logfile
			continue;
		}
		
		//create 2 buffers - one to hold the entire segment file - one to hold the data in log to copy over
		//lets create the buffer for the segment file	
		fstream fs;
		fs.open (segpath, std::fstream::in | std::fstream::out | std::fstream::binary);
		fs.seekg(0, fs.end);
		int seglength = fs.tellg();
		fs.seekg(0, fs.beg);
		char* segbuff = new char[seglength];
		fs.read(segbuff, seglength);
		fs.close();
		//now we find the data portion of the log file with corresponding size and offset
		
		//get data from log file into buffer - line by line
		char* line = NULL;
		size_t len = 0;
		ssize_t rd;
		FILE* logfp = fopen(logpath.c_str(), "r");
	    rd = getline(&line, &len, logfp);
	    while(rd != -1)
	    {
	    	//iterate line by line
	    	//sort through line based on delimiter
	    	int offset = atoi(strtok(line, "|||||"));
	    	int size = atoi(strtok(NULL, "|||||"));
	    	//fp at end of line - grab the data from logfile now
	    	//read from the logfile (@fp + size) into the segmentbuffer(+offset)
	    	fread((segbuff + offset), 1, size, logfp);
	    	//now segbuffer has the data from log - move to next line of log
	    	free(line);
	    	line = NULL;
	    	rd = getline(&line, &len, logfp);
	    }
	    //segbuff has all truncated data from logfile
	    //create new segment logfile using segbuffer
	    FILE* segfp = fopen(segpath.c_str(), "w");
	    fwrite(segbuff, 1, seglength, segfp);
	    fclose(segfp);
	    fclose(logfp);
	    //log file completely truncated, move on to next segment in directory
	}
}

void rvm_truncate_remap_log(rvm_t rvm, string segname)
{
	//now we can get the name of the segment to  create the path of file
	string segpath;
	segpath = rvm->directory;
	//strcpy(path, tid->rvm->directory.c_str());
	segpath.append("/");
	//strcat(path, "/");
	segpath.append(segname); 
	//segpath.append(".seg");
	string logpath;
	logpath = rvm->directory;
	//strcpy(path, tid->rvm->directory.c_str());
	logpath.append("/");
	//strcat(path, "/");
	logpath.append(segname);
	logpath.append(".log");
	//now we have both file paths - log and seg 
	
	struct stat filestat;
	//check if logfile is empty, then nothing to do
	stat(logpath.c_str(), &filestat);
	if(filestat.st_size == 0){
		printf("logfile is empty - nothing to truncate - in trunc_remap\n");
		return;
	}

	//create 2 buffers - one to hold the entire segment file - one to hold the data in log to copy over
	//lets create the buffer for the segment file	
	fstream fs;
	fs.open (segpath, std::fstream::in | std::fstream::out | std::fstream::binary);
	fs.seekg(0, fs.end);
	int seglength = fs.tellg();
	fs.seekg(0, fs.beg);
	char* segbuff = new char[seglength];
	fs.read(segbuff, seglength);
	fs.close();
	//now we find the data portion of the log file with corresponding size and offset
	
	//get data from log file into buffer - line by line
	char* line = NULL;
	size_t len = 0;
	ssize_t rd;
	FILE* logfp = fopen(logpath.c_str(), "r");
    rd = getline(&line, &len, logfp);
    while(rd != -1)
    {
    	//iterate line by line
    	//sort through line based on delimiter
    	int offset = atoi(strtok(line, "|||||"));
    	int size = atoi(strtok(NULL, "|||||"));
    	//fp at end of line - grab the data from logfile now
    	//read from the logfile (@fp + size) into the segmentbuffer(+offset)
    	fread((segbuff + offset), 1, size, logfp);
    	//now segbuffer has the data from log - move to next line of log
    	free(line);
    	line = NULL;
    	rd = getline(&line, &len, logfp);
    }
    //segbuff has all truncated data from logfile
    //create new segment logfile using segbuffer
    FILE* segfp = fopen(segpath.c_str(), "w");
    fwrite(segbuff, 1, seglength, segfp);
    fclose(segfp);
    fclose(logfp);
    //log file completely truncated, move on to next segment in directory
}
