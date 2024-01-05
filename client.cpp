/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name:
	UIN: 227007875
	Date: 09/23/2023
*/
#include "common.h"
#include "FIFORequestChannel.h"

#include <sys/wait.h>

using namespace std;


int main (int argc, char *argv[]) {
	int opt;
	int p = -1; // p, t, e set to -1 for determining user spercification
	double t = -1;
	int e = -1;
	int m = MAX_MESSAGE; // -m is buffer capacity, max number of bytes server can send to client and viceversa
	bool newchan = false; // -c for if a new channel request was made
	vector<FIFORequestChannel*> channels; // handling the channel controls
	
	string filename = "";
	while ((opt = getopt(argc, argv, "p:t:e:f:m:c")) != -1) {
		switch (opt) {
			case 'p':
				p = atoi (optarg);
				break;
			case 't':
				t = atof (optarg);
				break;
			case 'e':
				e = atoi (optarg);
				break;
			case 'f':
				filename = optarg;
				break;
			case 'm':
				m = atoi(optarg); // convert to an integer
				break;
			case 'c':
				newchan = true; // request for new channel was made
				break;
		}
	}

	// give arguments for the server
	// server needs './server', '-m', '<val for -m arg>', 'NULL'
	// fork()
	// in the child, run execvp using the server arguments 

	string arg0 = "./server";
	string arg1 = "-m";
	char* arg_list[] = {(char*)arg0.c_str(), (char*)arg1.c_str(), (char*)to_string(m).c_str(), NULL};
	

	//string arg = "-m ";
	//arg += to_string(m);
	
	
	pid_t pid = fork();
	if (pid == 0){
		execvp(arg_list[0], arg_list);
		// execl("./server", "./server", arg.c_str(), NULL);
	}
	

    FIFORequestChannel cont_chan("control", FIFORequestChannel::CLIENT_SIDE);
	channels.push_back(&cont_chan);

	FIFORequestChannel* chan2;

	if (newchan == true){
		MESSAGE_TYPE nc = NEWCHANNEL_MSG; // request byte
    	cont_chan.cwrite(&nc, sizeof(MESSAGE_TYPE)); // request message sent to server
		// receiving the new channel name from the server
		char chan_name[MAX_MESSAGE];
		cont_chan.cread(chan_name, MAX_MESSAGE); // reading the name from the server
		
		cout << chan_name << endl;
		// call FIFORequestChannel w/ name from the server
		// FIFORequestChannel chan2(chan_name, FIFORequestChannel::CLIENT_SIDE);
		chan2 = new FIFORequestChannel(chan_name, FIFORequestChannel::CLIENT_SIDE);

		// push_back new channel into channels
		channels.push_back(chan2);	

	}

	// we want to use the channel that is at the back of the list
	FIFORequestChannel chan = *(channels.back()); // dereferencing to use as object and not pointer

	// example data point request
	// only run this if p, t, and e are specified
	if (p != -1 && t != -1 && e != -1){
		char buf[MAX_MESSAGE]; // 256
		datamsg x(p, t, e); // changed to client request
		
		memcpy(buf, &x, sizeof(datamsg));
		chan.cwrite(buf, sizeof(datamsg)); // question
		double reply;
		chan.cread(&reply, sizeof(double)); //answer
		cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;	
	}

	// else if only p specified, request 1000 data points
	// loop over first 1000 lines
	// send request for ecg1
	// send request for ecg2
	// write line to received/x1.csv
	else if (p != -1 && t == -1 && e == -1){
		
		ofstream myfile;
		myfile.open("/home/zachbeast99/csce-313-programming-assignment-1-cholmcdowell/received/x1.csv"); // path

		double t_temp = 0; // time place holder, will increment by 0.004s

		for (int i = 0; i < 1000; i++){
			char buf[MAX_MESSAGE];
			
			datamsg x(p, t_temp, 1); // for ecg1 at specific time and from user -p

			memcpy(buf, &x, sizeof(datamsg));
			chan.cwrite(buf, sizeof(datamsg)); // question
			double reply;
			chan.cread(&reply, sizeof(double)); //answer
			myfile << t_temp << "," << reply; // put time and ecg1 into x1.csv

			datamsg y(p, t_temp, 2);

			memcpy(buf, &y, sizeof(datamsg));
			chan.cwrite(buf, sizeof(datamsg)); // question
			double reply2;
			chan.cread(&reply2, sizeof(double)); //answer
			
			myfile << "," << reply2 << endl; // put ecg2 into x1.csv
			t_temp += 0.004; // increment time
		}

		
		


		myfile.close();
	}
	


    // sending a non-sense message, you need to change this
	// buf request = size of filemsg + size of filename
	// buf response = size fo buffer capacity
	filemsg fm(0, 0); // asking the server how big file is
	string fname = filename; // filename
	
	int len = sizeof(filemsg) + (fname.size() + 1);
	char* buf2 = new char[len]; // size of file msg + filename + 1 for null terminator
	memcpy(buf2, &fm, sizeof(filemsg)); // copy file msg into the buffer
	strcpy(buf2 + sizeof(filemsg), fname.c_str()); // copy file name into buffer
	chan.cwrite(buf2, len);  // I want the file length; // send buffer to the server

	int64_t filesize = 0; // want to store the file size
	chan.cread(&filesize, sizeof(int64_t)); // recieve the file length

	// char* buf3 = (char*)to_string(m).c_str(); // create buffer of size buff capacity (m)


	// put into loop over the segments in the file filesize/buff capacity
	// buff capacity given by user -m
	// once in loop, create filemsg instance
	cout << &filesize << " " << filesize << endl;
	int64_t buffcap = (int64_t)m;
	cout << "filesize: " << filesize << endl;
	cout << "m: " << buffcap << endl;
	cout << "i: " << ceil((double)filesize/(double)buffcap) << endl;
	int64_t remaining = filesize;
	int64_t offs = 0;
	cout << filename << endl;

	


	ofstream fileObj;
	fileObj.open("/home/zachbeast99/csce-313-programming-assignment-1-cholmcdowell/received/"+filename);
		
	for (double i = ceil((double)filesize/(double)buffcap); i > 0; i--){ // while rem == 0 or for loop till the ceil(filesize/buffcap)
		// filemsg fm(offs, min(remaining,buffcap));
		// cout << "offs: " << offs << " min: " << min(remaining,buffcap) << endl;
		
		filemsg* file_req = (filemsg*)buf2; // reuse buf2
		file_req -> offset = offs; // set offset in the file
		file_req -> length = min(remaining, buffcap); // set length
		
		memcpy(buf2, file_req, sizeof(filemsg)); // copy file msg into the buffer
		strcpy(buf2 + sizeof(filemsg), fname.c_str()); // copy file name into buffer
		chan.cwrite(buf2, len); // send the request
		char* buf3 = new char[min(remaining, buffcap)]; // response buffer
		
		chan.cread(buf3, min(remaining, buffcap)); // cread into buffer
		
		fileObj.write(buf3, min(remaining,buffcap));
		
	
		offs += buffcap;
		remaining -= buffcap;
		
		delete[] buf3;
		
	}
	
	fileObj.close();
	cout << "offs: " << offs << endl;
	cout << "remaining: " << remaining << endl;
	
	delete[] buf2;
	
	// if new channel was created
	if (newchan == true){
		// close channel
		m = QUIT_MSG;
		chan.cwrite(&m, sizeof(MESSAGE_TYPE));
		// delete newly allocated channel
		delete chan2;
	}
	
	
	// closing the channel    
    m = QUIT_MSG; // editted for the earlier processes
    chan.cwrite(&m, sizeof(MESSAGE_TYPE));

	
	
}