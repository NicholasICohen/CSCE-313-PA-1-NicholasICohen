/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name:
	UIN:
	Date:
*/
#include "common.h"
#include "FIFORequestChannel.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string>

using namespace std;

string double_three(double x){
	/*string x_s = std::to_string(static_cast<int>(x*1000));
	if(x < 1 && x > 0){
		return "0." + x_s.substr(0,3);
	}
	if(x < 0 && x > -1){
		return "-0." + x_s.substr(0,3);
	}
	return x_s.substr(0,x_s.length()-3) + "." + x_s.substr(x_s.length()-4,3);*/
	string x_s = std::to_string(x);
	for(int i = x_s.length() - 3; i > 0; i--){
		if(x_s.at(i) != '0'){
			if(x_s.at(i) == '.'){
				i--;
			}
			return x_s.substr(0,i+1);
		}
	}
	return x_s;
}

int main (int argc, char *argv[]) {
	int opt;
	int patient_id = 1;
	double time = 0.0;
	int ecg_record = 1;
	int buffer_size = 256;
	string str_buffer_size = "256";
	bool c_flag = false, p_flag = false, t_flag = false, e_flag = false, f_flag = false, m_flag = false;
	string filename = "";
	
	//for(int i = 0; *argv[i] != '\0'; i++){
	//	cout << argv[i] << endl;
	//}
	
	while ((opt = getopt(argc, argv, "p:t:e:f:m:c")) != -1) {
		switch (opt) {
			case 'p':
				//cout << "p" << endl;
				patient_id = atoi (optarg);
				p_flag = true;
				break;
			case 't':
				//cout << "t" << endl;
				time = atof (optarg);
				t_flag = true;
				break;
			case 'e':
				//cout << "e" << endl;
				ecg_record = atoi (optarg);
				e_flag = true;
				break;
			case 'f':
				//cout << "f" << endl;
				filename = optarg;
				f_flag = true;
				break;
			case 'm':
				//cout << "m" << optarg << endl;
				buffer_size = atoi(optarg);
				str_buffer_size = optarg;
				m_flag = false;
				break;
			case 'c':
				c_flag = true;
				break;
		}
	}
	
	int f_pid = fork();
	if(f_pid > 0){
		//cout << "successfull fork: client" << endl;
	}
	else if(f_pid == 0){
		//cout << "successfull fork: server" << endl;
		const char* file = "./server";
		const char* arg1 = "-m";
		const char* arg2 = str_buffer_size.c_str();
		//cout << file << " " << arg1 << " " << arg2 << endl;
		execlp(file, file, arg1, arg2, NULL);
	}
	else{
		cout << "fork error, exiting" << endl;
		return 0;
	}
	
	//cout << patient_id << "\n" << time << "\n" << ecg_record << "\n" << filename << "\n" << buffer_size << "\n" << endl;
	
	//mkfifo("fifo_control1", 0600);
	//mkfifo("fifo_control2", 0600);
	FIFORequestChannel* ctrl_chan = new FIFORequestChannel("control", FIFORequestChannel::CLIENT_SIDE);
	FIFORequestChannel* alt_chan;
	//char* com_buf = new char[MAX_MESSAGE];
	if(c_flag){
		//cout << "starting" << endl;
		char* c_buf = new char[MAX_MESSAGE];
		MESSAGE_TYPE m_newchannel = NEWCHANNEL_MSG;
		//cout << "writing" << endl;
		ctrl_chan->cwrite(&m_newchannel, sizeof(MESSAGE_TYPE));
		string alt_chan_name = "";
		//cout << "reading" << endl;
		ctrl_chan->cread(c_buf, buffer_size);
		for(int i = 0; c_buf[i] != '\0' && i < MAX_MESSAGE; i++){
			alt_chan_name = alt_chan_name + c_buf[i];
		}
		//cout << alt_chan_name << endl;
		//cout << "requesting channel" << endl;
		alt_chan = new FIFORequestChannel(alt_chan_name,FIFORequestChannel::CLIENT_SIDE);
		delete[] c_buf;
	}
	else{
		alt_chan = ctrl_chan;
	}
	
	if(p_flag && e_flag && t_flag){
		char* d_buf = new char[MAX_MESSAGE]; // 256
        	datamsg x(patient_id, time, ecg_record);
		memcpy(d_buf, &x, sizeof(datamsg));
		alt_chan->cwrite(d_buf, sizeof(datamsg)); // question
		double reply;
		alt_chan->cread(&reply, sizeof(double)); //answer
		cout << "For person " << patient_id << ", at time " << time << ", the value of ecg " << ecg_record << " is " << reply << endl;
		delete[] d_buf;
	}
	else if(p_flag){
		char* d_buf = new char[MAX_MESSAGE];
		int d_dump_fd = open("received/x1.csv", O_WRONLY | O_CREAT | O_TRUNC, 0644);
		datamsg x(patient_id, 0.0, 1);
		string write_line = "";
		double d_reading;
		for(int t = 0; t < 1000; t++){
			x.seconds = 0.004 * t;
			write_line = write_line + double_three(x.seconds) + ",";
			x.ecgno = 1;
			memcpy(d_buf, &x, sizeof(datamsg));
			alt_chan->cwrite(d_buf, sizeof(datamsg));
			alt_chan->cread(&d_reading, sizeof(double));
			write_line = write_line + double_three(d_reading) + ",";
			x.ecgno = 2;
			memcpy(d_buf, &x, sizeof(datamsg));
			alt_chan->cwrite(d_buf, sizeof(datamsg));
			alt_chan->cread(&d_reading, sizeof(double));
			write_line = write_line + double_three(d_reading) + "\n";
			write(d_dump_fd, write_line.c_str(), write_line.size());
			write_line = "";
		}
		close(d_dump_fd);
		delete[] d_buf;
	}
	
	if(f_flag){
		char* com_buf = new char[sizeof(filemsg) + filename.length() + 1];
		char* f_buf = new char[buffer_size];
		filemsg x(0, 0);
		//MESSAGE_TYPE m_file = FILE_MSG;
		//memcpy(f_buf, &m_file, sizeof(MESSAGE_TYPE));
		memcpy(com_buf, &x, sizeof(filemsg));
		memcpy(com_buf + sizeof(filemsg), filename.c_str(), filename.length() + 1);
		alt_chan->cwrite(com_buf, sizeof(filemsg) + filename.length() + 1);
		__int64_t filesize;
		alt_chan->cread(&filesize, sizeof(__int64_t));
		//cout << "filesize " << filesize << endl;
		x.length = buffer_size;
		int f_req_fd = open(("received/" + filename).c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
		for(__int64_t offset = 0; offset < filesize; offset += buffer_size){
			if(filesize - offset < buffer_size){
				x.offset = offset;
				x.length = filesize - offset;
				memcpy(com_buf, &x, sizeof(filemsg));
				memcpy(com_buf + sizeof(filemsg), filename.c_str(), filename.length() + 1);
				alt_chan->cwrite(com_buf, sizeof(filemsg) + filename.length() + 1);
				alt_chan->cread(f_buf, filesize-offset);
				write(f_req_fd, f_buf, filesize-offset);
				break;
			}
			x.offset = offset;
			memcpy(com_buf, &x, sizeof(filemsg));
			memcpy(com_buf + sizeof(filemsg), filename.c_str(), filename.length() + 1);
			alt_chan->cwrite(com_buf, sizeof(filemsg) + filename.length() + 1);
			alt_chan->cread(f_buf, buffer_size);
			write(f_req_fd, f_buf, buffer_size);
		}
		close(f_req_fd);
		delete[] com_buf;
		delete[] f_buf;
	}
	if(m_flag){
		
	}
	//FIFORequestChannel chan("control", FIFORequestChannel::CLIENT_SIDE);
	/*
	// example data point request
        
        // sending a non-sense message, you need to change this
	filemsg fm(0, 0);
	string fname = "teslkansdlkjflasjdf.dat";
	
	int len = sizeof(filemsg) + (fname.size() + 1);
	char* buf2 = new char[len];
	memcpy(buf2, &fm, sizeof(filemsg));
	strcpy(buf2 + sizeof(filemsg), fname.c_str());
	chan->cwrite(buf2, len);  // I want the file length;

	delete[] buf2;
	//*/
	// closing the channel    
	//string dump;
	//cin >> dump;
    	MESSAGE_TYPE m_quit = QUIT_MSG;
	//cout << "sending quit" << endl;
	if(c_flag){
		alt_chan->cwrite(&m_quit, sizeof(MESSAGE_TYPE));
	}
    	ctrl_chan->cwrite(&m_quit, sizeof(MESSAGE_TYPE));
	int status;
	waitpid(f_pid, &status, 0);
	delete ctrl_chan;
	if(c_flag){
		delete alt_chan;
	}
}
