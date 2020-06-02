#include <stdlib.h>
#include <iostream>
#include <string>
#include <cstring>
#include <fstream>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/reg.h>
#include <unistd.h>
#include <mysql/mysql.h>
#define MB 1048576

const char* hostname = "172.17.0.1";
const char* user = "root";
const char* pwd ="123";
const char* dbname  = "oj_test";
const char* tablename = "queue_test";
const char* dataname = "data";

const int OJ_AC=1;
const int OJ_WA=2;
const int OJ_SE=3;
const int OJ_RE=4;
const int OJ_TLE=5;
const int OJ_OLE=6;
const int OJ_MLE=7;

const int ALLOW_SYS_CALL_C[] = {0,1,2,3,4,5,8,9,11,12,20,21,59,63,89,158,231,240, SYS_time, SYS_read, SYS_uname, SYS_write
		, SYS_open, SYS_close, SYS_execve, SYS_access, SYS_brk, SYS_munmap, SYS_mprotect, SYS_mmap, SYS_fstat
				, SYS_set_thread_area, 252, SYS_arch_prctl, 0 };
	
bool allowSysCall[512];

int compile(std::string codefile,std::string run_id);
void update_compile_status(std::string run_id,int stat);
void run(std::string dataIn,std::string run_id,int timeLimit,int memLimit);
void savefile(std::string data,std::string data_in);
void judge(std::string run_id,std::string pnum);
void updateRunstatus(int result,std::string run_id);
int checkAnswer(std::string dataOut,std::string outputdata);
void watchRunningStatus(pid_t fpid,const char* errFile,int& result,int& topMem,int& usedTime,int memLimit,int timeLimit);
int getProcStatus(int pid, const char* statusTitle);
int getFileSize(const char* fileName);
void initAllowSysCall();
int main(int argc,char* argv[])
{
		/*set compile xx.cpp xx problemnum*/
		int status = compile(argv[1],argv[2]);
			if(status==-1){
						update_compile_status(argv[2],-1);
							}
				else if(status!=0){
							update_compile_status(argv[2],1);
								}
					else{
								update_compile_status(argv[2],0);
										judge(argv[2],argv[3]);
											}
						return 0;
}
int compile(std::string codefile,std::string run_id)
{
		const char *COMP_CPP[] = {"g++","-Wall","-fno-asm","-lm", "--static", "-std=c++11"
					,codefile.c_str(),"-o",run_id.c_str(),NULL};
			pid_t fpid=fork();
				if(fpid==0){
							execvp(COMP_CPP[0],(char* const*)COMP_CPP);
									exit(0);
										}
					else if(fpid!=-1){
								int status;
										waitpid(fpid,&status,0);
												return status;
													}
						else {
									std::ofstream fout("systemError.log", std::ios_base::out | std::ios_base::app);
											fout << "Error: fork() file when compile.\n";
													fout << "In file " << __FILE__ << " function (" << __FUNCTION__ << ")"
																	<<" , line: " << __LINE__ << "\n";
															fout.close();
																	return -1;
																		}
}
void update_compile_status(std::string run_id,int stat)
{
		MYSQL mysql;
			mysql_init(&mysql);
				std::string status;
					if(stat==0){
								status="running";
									}
						else if(stat==-1){
									status="system error";
										}
							else status="compile error";
								std::cout<<status<<"\n";
									if(!mysql_real_connect(&mysql, hostname, user, pwd, dbname, 0, NULL, 0)) {
											
												printf("when update_compile_status:%s\n", mysql_error(&mysql));
													
													}
										std::string commonds=std::string("update ")+tablename+std::string(" set status='")+status+std::string("' where id='")+run_id+std::string("';");
											mysql_query(&mysql,commonds.c_str());
												mysql_close(&mysql);
}
void judge(std::string run_id,std::string pnum)
{
	initAllowSysCall();
		MYSQL_ROW row;
			MYSQL_FIELD* field=NULL;
				MYSQL_RES *result;
					MYSQL mysql;
						mysql_init(&mysql);
						
							int timeLimit = 1, memLimit = 64;
								
								std::string dataIn = "data.in";
									std::string dataOut ="dataOut.out";
										std::string errOut = "error.out";
											
											if(!mysql_real_connect(&mysql, hostname, user, pwd, dbname, 0, NULL, 0)) {
														printf("when judge:%s\n", mysql_error(&mysql));
															}
												std::string commonds="select * from data where id='"+pnum+"';";
													mysql_query(&mysql,commonds.c_str());
														
														result=mysql_store_result(&mysql);
															row=mysql_fetch_row(result);
																
																int answer = OJ_AC;
																	int maxTime = 0;
																		int topMem = 0;
																			
																			
																			while(row!=NULL){
																						std::string input_data = row[1];
																								std::string output_data = row[2];
																										int test=1;
																												int usedTime=0;
																														
																														savefile(input_data,dataIn);
																																
																																pid_t fpid = fork();
																																		if(fpid==0){
																																						std::cout<<"prepar run!\n";
																																									run(dataIn,run_id,timeLimit,memLimit);
																																												exit(0);
																																														}
																																				else if(fpid!=-1){
																																								watchRunningStatus(fpid,  errOut.c_str(),answer, topMem, usedTime, memLimit, timeLimit);
																																												
																																											if (maxTime < usedTime) maxTime = usedTime;
																																													}
																																						else {
																																										std::ofstream fout("systemError.log", std::ios_base::out | std::ios_base::app);
																																														fout << "# ERR fork in " << __FILE__;
																																																		fout << "function is (" << __FUNCTION__ << ") in line " << __LINE__ << "\n";
																																																						fout.close();
																																																										
																																																										answer = OJ_SE;
																																																												}
																																									
																																								if(answer==OJ_AC){
																																												answer=checkAnswer(dataOut,output_data);
																																															if(answer==OJ_WA){
																																																				std::cout<<"wrong answer in test "<<test<<"\n";
																																																							}
																																																	}
																																										std::cout<<usedTime<<"\n";
																																												std::cout<<timeLimit<<"\n";
																																														test++;
																																																row=mysql_fetch_row(result);
																																																	}
																				mysql_close(&mysql);
																					
																					updateRunstatus(answer,run_id);
																						
}
void savefile(std::string data,std::string data_in)
{
		std::ofstream fout(data_in.c_str());
			fout<<data;
				fout.close();
}
void run(std::string dataIn,std::string run_id,int timeLimit,int memLimit)
{
		freopen(dataIn.c_str(),"r",stdin);
			freopen("dataOut.out","w",stdout);
				freopen("errOut.out", "w", stderr);
					
					ptrace(PTRACE_TRACEME, 0, nullptr, nullptr);
					
						struct rlimit lim;
							lim.rlim_cur = lim.rlim_max = 1;
								setrlimit(RLIMIT_CPU, &lim);
									alarm(0);
										alarm(timeLimit/1000.0);
											
											lim.rlim_cur = lim.rlim_max = 1;
												setrlimit(RLIMIT_NPROC, &lim);
													
													lim.rlim_cur = lim.rlim_max = MB << 6;
														setrlimit(RLIMIT_STACK, &lim);
															
															lim.rlim_cur = memLimit * MB / 2 * 3;
																lim.rlim_max = memLimit * MB;
																	setrlimit(RLIMIT_AS, &lim);
																		
																		run_id="./"+run_id;
																			execl(run_id.c_str(),run_id.c_str(),NULL);
}
void watchRunningStatus(pid_t fpid,const char* errFile,int& result,int& topMem,int& usedTime,int memLimit,int timeLimit)
{
		int tmpMem = 0;
			int status, sig, exitCode;
				struct rusage usage;

					if (topMem == 0)
								topMem = getProcStatus(fpid, "VmRSS:")<<10;
					
						wait(&status);
							if (WIFEXITED(status)) return;
								
								ptrace(PTRACE_SETOPTIONS, fpid, nullptr
														, PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACEEXIT | PTRACE_O_EXITKILL);
									ptrace(PTRACE_SYSCALL, fpid, nullptr, nullptr);
										
										while(true){
													wait4(fpid, &status, __WALL, &usage);
													
															/* update topMem and result */
															tmpMem = getProcStatus(fpid, "VmPeak:") << 10;	
															
																	if (tmpMem > topMem) topMem = tmpMem;
																			if (topMem > (memLimit<<20)) {
																							if (result == OJ_AC) result = OJ_MLE;
																										ptrace(PTRACE_KILL, fpid, nullptr, nullptr);
																										
																													break;
																															}
																			
																					if (WIFEXITED(status)) break;
																							if (getFileSize(errFile)!=0) {
																								/*			std::cout << "RE in line " << __LINE__ << "\n";*/
																								
																											if (result == OJ_AC) result = OJ_RE;
																														ptrace(PTRACE_KILL, fpid, nullptr, nullptr);
																																	break;
																																			}
																									
																									exitCode = WEXITSTATUS(status);
																											
																											if(!(exitCode==0 || exitCode==133 || exitCode==5)){
																															if (result == OJ_AC) {
																																				switch (exitCode) {
																																										case SIGCHLD :
																																																case SIGALRM :
																																																	alarm(0);
																																																						case SIGKILL :
																																																						case SIGXCPU :
																																																							result = OJ_TLE;
																																																													break;
																																																																		case SIGXFSZ :
																																																																			result = OJ_OLE;
																																																																									break;
																																																																														default :
																																																																															result = OJ_RE;
																																																																																			}
																																							}
																																		ptrace(PTRACE_KILL, fpid, nullptr, nullptr);
																																					break;
																																							}
																													
																													if (WIFSIGNALED(status)) {
																																	sig = WTERMSIG(status);
																																	
																																	
																																				if (result == OJ_AC) {
																																									switch (sig) {
																																															case SIGCHLD :
																																																					case SIGALRM :
																																																						alarm(0);
																																																											case SIGKILL :
																																																											case SIGXCPU :
																																																												result = OJ_TLE;
																																																																		break;
																																																																							case SIGXFSZ :
																																																																								result = OJ_OLE;
																																																																														break;
																																																																																			default :
																																																																																				result = OJ_RE;
																																																																																								}
																																												}
																																							ptrace(PTRACE_KILL, fpid, nullptr, nullptr);
																																										break;
																																												}
																															
																															int sysCall = ptrace(PTRACE_PEEKUSER, fpid, ORIG_RAX<<3, nullptr);
																																	/*if (allowSysCall[sysCall]) {
																																		
																																					result = OJ_RE;
																																				std::cout<<"here"<<"\n";																		ptrace(PTRACE_KILL, fpid, nullptr, nullptr);
																																											break;
																																													}
		*/																															
																																			ptrace(PTRACE_SYSCALL, fpid, nullptr, nullptr);
																																				}
											
											if (result == OJ_TLE) usedTime = timeLimit;
												else {
															usedTime += (usage.ru_utime.tv_sec*1000 + usage.ru_utime.tv_usec/1000);
																	usedTime += (usage.ru_stime.tv_sec*1000 + usage.ru_stime.tv_usec/1000);
																	std::cout<<"time:"<<usage.ru_utime.tv_sec*1000<<"\n";																}
												std::cout<<"your program use time:"<<usedTime<<"\n";
												std::cout<<"result:"<<result<<"\n";
}
int checkAnswer(std::string dataOut,std::string outputdata)
{
		std::ifstream fin(dataOut);
			std::string line ;
				std::string user_out="";
					
					while(getline(fin,line)){
								user_out+=line;
									}
						
						fin.close();
							
							int dLen = outputdata.length();
								int uLen = user_out.length();
								
									int result = OJ_AC;
										
										if (uLen!=dLen || outputdata.compare(user_out)!=0) result = OJ_WA;
											
											return result;
}
void updateRunstatus(int result,std::string run_id)
{
		MYSQL mysql;
			mysql_init(&mysql);
				
				if(!mysql_real_connect(&mysql, hostname, user, pwd, dbname, 0, NULL, 0)) {
							printf("when updateRunstatus:%s\n", mysql_error(&mysql));
								}
					std::string status;
						if(result==OJ_AC){
									status="AC";
										}
							else if(result==OJ_WA){
										status="WA";
											}
								std::cout<<status<<"\n";
									std::string commonds=std::string("update queue_test set status")+std::string("='")+status.c_str()+std::string("'where id='")+run_id.c_str()+std::string("';");
										mysql_query(&mysql,commonds.c_str());
											mysql_close(&mysql);
}
int getProcStatus(int pid, const char* statusTitle) {
		char file[64];
			sprintf(file, "/proc/%d/status", pid);
				std::ifstream fin(file);
					std::string line;
					
						int sLen = strlen(statusTitle);
						
							int status = 0;
								while (getline(fin, line)) {
									
											int lLen = line.length();
													if (lLen <= sLen) continue;
													
															bool flag = true;
																	for (int i=0; i<sLen; ++i) {
																					if (line[i] != statusTitle[i]) {
																										flag = false;
																														break;
																																	}
																							}
																	
																			if (flag) {
																							for (int i=sLen; i<lLen; ++i){
																												if (line[i]>='0' && line[i]<='9') status = status*10 + line[i]-'0';
																																else if (status) break;
																																			}
																										break;
																												}
																				}
								
								
									return status;
}
int getFileSize(const char* fileName) {
		struct stat f_stat;
		
			if (stat(fileName, &f_stat) == -1) return 0;
				else return f_stat.st_size;
}
void initAllowSysCall()
{
	memset(allowSysCall, false ,sizeof(allowSysCall));
	for (int i=0; !i || ALLOW_SYS_CALL_C[i]; ++i) {
						allowSysCall[ALLOW_SYS_CALL_C[i]] = true;
						}
}
