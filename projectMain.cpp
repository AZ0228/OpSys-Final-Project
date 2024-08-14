#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <cmath>
#include <iomanip>
#include <queue>
#include <vector>

struct Process {
    std::string id;
    int arrival_time;
    std::vector<int> cpu_bursts;
    std::vector<int> io_bursts;
    int current_burst_index;
    int total_cpu_time;
    int total_wait_time;
    int total_turnaround_time;
    int done_time;
    bool is_cpu_bound;
    int context_switch;
};

double next_exp(double lambda, int bound){
    double r=drand48();
    double x=-log(r)/lambda;
    if (x>bound)
        return next_exp(lambda, bound);
    return x;
}

void print_queue(std::queue<Process> queue){
    if(queue.empty()){
        std::cout << " empty]" << std::endl;
        return;
    }
    while (!queue.empty()) {
        Process p = queue.front();
        std::cout << " " << p.id;
        queue.pop();
    }
    std::cout << "]" << std::endl;
}

void simulate_fcfs1(std::vector<Process> processes, int tcs, std::ofstream& outfile) {
    bool printAll = true;
    std::queue<Process> ready_queue;  //store Process objects directly
    std::vector<Process> io_queue;  //store Process objects directly
    int current_time = 0;
    bool printInitial = true;
    Process current_process;
    bool has_current_process = false;
    bool contextSwitching = false;

    //check if any processes arrive at time 0
    while (!processes.empty() && processes.front().arrival_time == 0) {
        ready_queue.push(processes.front());
        std::cout << "time 0ms: Process " << processes.front().id << " arrived; added to ready queue [Q";
        print_queue(ready_queue);
        processes.erase(processes.begin());
        printInitial = false;
    }

    if (printInitial) {
        std::cout << "time 0ms: Simulator started for FCFS [Q empty]" << std::endl;
        current_time = processes.front().arrival_time;
    }

    while ((!processes.empty() || !ready_queue.empty() || !io_queue.empty() || has_current_process)) {
        //check if any processes arrive
        for (std::vector<Process>::iterator it = processes.begin(); it != processes.end(); ) {
            if (it->arrival_time == current_time) {
                ready_queue.push(*it);
                std::cout << "time " << current_time << "ms: Process " << it->id << " arrived; added to ready queue [Q";
                print_queue(ready_queue);
                it = processes.erase(it);
            } else {
                ++it;
            }
        }

        //check if any processes are done with IO
        for (std::vector<Process>::iterator it = io_queue.begin(); it != io_queue.end(); ) {
            if (it->arrival_time == current_time) {
                ready_queue.push(*it);
                if (current_time < 10000 || printAll) {
                    std::cout << "time " << current_time << "ms: Process " << it->id << " completed I/O; added to ready queue [Q";
                    print_queue(ready_queue);
                }
                it = io_queue.erase(it);
            } else {
                ++it;
            }
        }

        //check if CPU is idle
        if (!has_current_process && ready_queue.empty()) {
            current_time++;
            continue;
        }

        if(contextSwitching){
            if(current_process.context_switch == 1){
                int burst_time = current_process.cpu_bursts[current_process.current_burst_index];
                if (current_time < 10000 || printAll) {
                    std::cout << "time " << current_time << "ms: Process " << current_process.id << " started using the CPU for " << burst_time << "ms burst [Q";
                    print_queue(ready_queue);
                }
                current_process.done_time = current_time + burst_time;
                contextSwitching = false;
            } else {
                current_process.context_switch--;
                current_time++;
                continue;
            }
        }

        //handle the current process in the CPU
        if (has_current_process) {
            if (current_process.done_time == current_time) {
                if (current_process.current_burst_index < current_process.cpu_bursts.size()-1) {
                    int io_time = current_process.io_bursts[current_process.current_burst_index];
                    current_process.arrival_time = current_time + io_time + tcs / 2;
                    current_process.current_burst_index++;
                    io_queue.push_back(current_process);
                    if (current_time < 10000 || printAll) {
                        std::cout << "time " << current_time << "ms: Process " << current_process.id << " completed a CPU burst; "
                                  << current_process.cpu_bursts.size() - current_process.current_burst_index << " bursts to go [Q";
                        print_queue(ready_queue);
                        std::cout << "time " << current_time << "ms: Process " << current_process.id << " switching out of CPU; blocking on I/O until time "
                                  << current_time + io_time + tcs / 2 << "ms [Q";
                        print_queue(ready_queue);
                    }

                } else {
                    if (current_time < 10000 || printAll || current_process.current_burst_index == current_process.cpu_bursts.size()) {
                        std::cout << "time " << current_time << "ms: Process " << current_process.id << " terminated [Q";
                        print_queue(ready_queue);
                    }
                }
                contextSwitching = true;
                has_current_process = false;
            }
        }

        //process the next process in ready queue
        if ((!has_current_process && !ready_queue.empty())) {
            current_process = ready_queue.front();
            ready_queue.pop();
            has_current_process = true;
            current_process.context_switch = tcs / 2;
            if(contextSwitching){
                current_process.context_switch += tcs / 2;
            } else {
                contextSwitching = true;
            }
        }

        current_time ++;
    }
    std::cout << "time " << current_time + tcs / 2 - 1<< "ms: Simulator ended for FCFS [Q empty]" << std::endl;

}

void simulate_rr(std::vector<Process> processes, int tcs, int tslice, std::ofstream& outfile) {
    bool printAll = true;
    std::queue<Process> ready_queue;
    std::vector<Process> io_queue;
    int current_time = 0;
    bool printInitial = true;
    Process current_process;
    bool has_current_process = false;
    bool contextSwitching = false;
    std::vector<Process> initial_processes(processes);

    //check if any processes arrive at time 0
    while (!processes.empty() && processes.front().arrival_time == 0) {
        ready_queue.push(processes.front());
        std::cout << "time 0ms: Process " << processes.front().id << " arrived; added to ready queue [Q";
        print_queue(ready_queue);
        processes.erase(processes.begin());
        printInitial = false;
    }

    if (printInitial) {
        std::cout << "time 0ms: Simulator started for RR [Q empty]" << std::endl;
        current_time = processes.front().arrival_time;
    }

    while ((!processes.empty() || !ready_queue.empty() || !io_queue.empty() || has_current_process)) {
        //check if any processes arrive
        for (std::vector<Process>::iterator it = processes.begin(); it != processes.end(); ) {
            if (it->arrival_time == current_time) {
                ready_queue.push(*it);
                std::cout << "time " << current_time << "ms: Process " << it->id << " arrived; added to ready queue [Q";
                print_queue(ready_queue);
                it = processes.erase(it);
            } else {
                ++it;
            }
        }

        //check if any processes are done with IO
        for (std::vector<Process>::iterator it = io_queue.begin(); it != io_queue.end(); ) {
            if (it->arrival_time == current_time) {
                ready_queue.push(*it);
                if (current_time < 10000 || printAll) {
                    std::cout << "time " << current_time << "ms: Process " << it->id << " completed I/O; added to ready queue [Q";
                    print_queue(ready_queue);
                }
                it = io_queue.erase(it);
            } else {
                ++it;
            }
        }

        //check if idle
        if (!has_current_process && ready_queue.empty()) {
            current_time++;
            continue;
        }

        if (contextSwitching) {
            if (current_process.context_switch == 1) {
                int burst_time = current_process.cpu_bursts[current_process.current_burst_index];
                if (current_time < 10000 || printAll) {
                    Process initial_process;
                    std::cout << "time " << current_time << "ms: Process " << current_process.id << " started using the CPU for ";
                    bool preempted = false; //keeps track of if the process has been preempted for output thingy
                    for (std::vector<Process>::iterator it = initial_processes.begin(); it != initial_processes.end(); ++it) {
                        if(it->id == current_process.id){
                            if(it->cpu_bursts[current_process.current_burst_index] != current_process.cpu_bursts[current_process.current_burst_index]){
                                preempted = true;
                                initial_process = *it;
                            }
                            break;
                        }
                    }
                    if(preempted){
                        std::cout << "remaining " << burst_time << "ms of " << initial_process.cpu_bursts[current_process.current_burst_index] << "ms burst [Q";
                    } else {
                        std::cout<< burst_time << "ms burst [Q";
                    }
                    print_queue(ready_queue);
                }
                current_process.done_time = current_time + std::min(tslice, burst_time);
                contextSwitching = false;
            } else {
                current_process.context_switch--;
                current_time++;
                continue;
            }
        }

        //current process
        if (has_current_process) {
            if (current_process.done_time == current_time) {
                int burst_time = current_process.cpu_bursts[current_process.current_burst_index];

                if (burst_time > tslice) {
                    current_process.cpu_bursts[current_process.current_burst_index] -= tslice;
                    if(ready_queue.empty()){
                        current_process.done_time = current_time + std::min(tslice, current_process.cpu_bursts[current_process.current_burst_index]);
                        std::cout << "time " << current_time << "ms: Time slice expired; no preemption because ready queue is empty [Q empty]" << std::endl; 
                        current_time ++ ;
                        continue;
                    }
                    if (current_time < 10000 || printAll) {
                        std::cout << "time " << current_time << "ms: Time slice expired; preempting process " << current_process.id << " with " << current_process.cpu_bursts[current_process.current_burst_index] << "ms remaining [Q";
                        print_queue(ready_queue);
                    }
                    ready_queue.push(current_process);
                } else {
                    if (current_process.current_burst_index < current_process.cpu_bursts.size() - 1) {
                        int io_time = current_process.io_bursts[current_process.current_burst_index];
                        current_process.arrival_time = current_time + io_time + tcs / 2;
                        current_process.current_burst_index++;
                        io_queue.push_back(current_process);
                        if (current_time < 10000 || printAll) {
                            std::cout << "time " << current_time << "ms: Process " << current_process.id << " completed a CPU burst; "
                                      << current_process.cpu_bursts.size() - current_process.current_burst_index << " bursts to go [Q";
                            print_queue(ready_queue);
                            std::cout << "time " << current_time << "ms: Process " << current_process.id << " switching out of CPU; blocking on I/O until time "
                                      << current_time + io_time + tcs / 2 << "ms [Q";
                            print_queue(ready_queue);
                        }
                    } else {
                        if (current_time < 10000 || printAll || current_process.current_burst_index == current_process.cpu_bursts.size()) {
                            std::cout << "time " << current_time << "ms: Process " << current_process.id << " terminated [Q";
                            print_queue(ready_queue);
                        }
                    }
                }
                contextSwitching = true;
                has_current_process = false;
            }
        }

        //next process
        if ((!has_current_process && !ready_queue.empty())) {
            current_process = ready_queue.front();
            ready_queue.pop();
            has_current_process = true;
            current_process.context_switch = tcs / 2;
            if (contextSwitching) {
                current_process.context_switch += tcs / 2;
            } else {
                contextSwitching = true;
            }
        }

        current_time++;
    }
    std::cout << "time " << current_time + tcs / 2 - 1 << "ms: Simulator ended for RR [Q Empty]" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc!=9){
        std::cerr << "ERROR: <Incorrect number of arguments>" << std::endl;
        return 1;
    }

    //Read the arguments
    int n=std::atoi(argv[1]);
    int ncpu = std::atoi(argv[2]);
    long int seed = std::atoi(argv[3]);
    double lambda = std::atof(argv[4]);
    int bound = std::atoi(argv[5]);
    //part 2
    int tcs = std::atoi(argv[6]);
    double alpha = std::atof(argv[7]);
    int tslice = std::atoi(argv[8]);

    //Output the arguments
    if (ncpu==1)
        std::cout << "<<< PROJECT PART I\n" << "<<< -- process set (n=" << n << ") with " << ncpu << " CPU-bound process" << std::endl;
    else
        std::cout << "<<< PROJECT PART I\n" << "<<< -- process set (n=" << n << ") with " << ncpu << " CPU-bound processes" << std::endl;
    std::cout << "<<< -- seed=" << seed << "; lambda=" << std::fixed << std::setprecision(6) << lambda << "; bound=" << bound << std::endl;

    //Error checking
    if (n<0){
        std::cerr << "ERROR: <n is negative>" << std::endl;
        return 1;
    }
    if (ncpu<0){
        std::cerr << "ERROR: <ncpu is negative>" << std::endl;
        return 1;
    }
    if (ncpu>n){
        std::cerr << "ERROR: <ncpu is greater than n>" << std::endl;
        return 1;
    }
    if (bound<0){
        std::cerr << "ERROR: <upper bound is negative>" << std::endl;
        return 1;
    }

    //Do stuff
    //change from printing to storing
    srand48(seed);


    int numCPUBoundCPUBurst=0;
    double sumCPUBoundCPUBurst=0;
    int numCPUBoundIOBurst=0;
    double sumCPUBoundIOBurst=0;

    int numIOBoundCPUBurst=0;
    double sumIOBoundCPUBurst=0;
    int numIOBoundIOBurst=0;
    double sumIOBoundIOBurst=0;

    std::vector<Process> processes;
    for (int i = 0; i < n; i++) {
        Process p;
        p.id = std::string(1, 'A' + (i / 10)) + std::to_string(i % 10);
        p.arrival_time = floor(next_exp(lambda, bound));
        int num_bursts = ceil(32 * drand48());
        p.current_burst_index = 0;
        p.total_cpu_time = 0;
        p.total_wait_time = 0;
        p.total_turnaround_time = 0;
        // p.remaining_time = 0;
        p.is_cpu_bound = (i < ncpu);
        if (num_bursts==1)
            std::cout << "CPU-bound process " << p.id << ": arrival time " << p.arrival_time << "ms; " << num_bursts << " CPU burst:" << std::endl;
        else
            std::cout << "CPU-bound process " << p.id << ": arrival time " << p.arrival_time << "ms; " << num_bursts << " CPU bursts:" << std::endl;

        for (int j = 0; j < num_bursts; j++) {
            if (i < ncpu) {
                int burstTime = 4 * ceil(next_exp(lambda, bound));
                p.cpu_bursts.push_back(burstTime);
                if (j < num_bursts - 1) {
                    int ioTime = ceil(next_exp(lambda, bound));
                    p.io_bursts.push_back(ioTime);
                    numCPUBoundIOBurst++;
                    sumCPUBoundIOBurst += ioTime;
                }
                numCPUBoundCPUBurst++;
                sumCPUBoundCPUBurst += burstTime;
            } else {
                int burstTime = ceil(next_exp(lambda, bound));
                p.cpu_bursts.push_back(burstTime);
                if (j < num_bursts - 1) {
                    int ioTime = 8 * ceil(next_exp(lambda, bound));
                    p.io_bursts.push_back(ioTime);
                    numIOBoundCPUBurst++;
                    sumIOBoundIOBurst += ioTime;
                }
                numIOBoundCPUBurst++;
                sumIOBoundCPUBurst += burstTime;
            }
        }
        processes.push_back(p);
    }

    // int numCPUBoundCPUBurst=0;
    // double sumCPUBoundCPUBurst=0;
    // int numCPUBoundIOBurst=0;
    // double sumCPUBoundIOBurst=0;
    // for (int i=0; i<ncpu; i++){
    //     int initial=floor(next_exp(lambda, bound));
    //     int CPUburst=ceil(32*drand48());
    //     std::string name=std::string(1, 'A'+(i/10))+std::to_string(i%10);
    //     if (CPUburst==1)
    //         std::cout << "CPU-bound process " << name << ": arrival time " << initial << "ms; " << CPUburst << " CPU burst:" << std::endl;
    //     else
    //         std::cout << "CPU-bound process " << name << ": arrival time " << initial << "ms; " << CPUburst << " CPU bursts:" << std::endl;
    //     for (int j=0; j<CPUburst; j++){
    //         if (j<CPUburst-1){
    //             int burstTime=4*ceil(next_exp(lambda, bound));
    //             int ioTime=ceil(next_exp(lambda, bound));
    //             numCPUBoundCPUBurst++;
    //             numCPUBoundIOBurst++;
    //             sumCPUBoundIOBurst+=ioTime;
    //             sumCPUBoundCPUBurst+=burstTime;
    //             std::cout << "==> CPU burst " << burstTime << "ms ==> I/O burst " << ioTime << "ms" << std::endl;
    //         }
    //         else{
    //             int burstTime=4*ceil(next_exp(lambda, bound));
    //             numCPUBoundCPUBurst++;
    //             sumCPUBoundCPUBurst+=burstTime;
    //             std::cout << "==> CPU burst " << burstTime << "ms" << std::endl;
    //         }
    //     }
    // }
    // int numIOBoundCPUBurst=0;
    // double sumIOBoundCPUBurst=0;
    // int numIOBoundIOBurst=0;
    // double sumIOBoundIOBurst=0;
    // for (int i=ncpu; i<n; i++){
    //     int initial=floor(next_exp(lambda, bound));
    //     int CPUburst=ceil(32*drand48());
    //     std::string name=std::string(1, 'A'+(i/10))+std::to_string(i%10);
    //     if (CPUburst==1)
    //         std::cout << "I/O-bound process " << name << ": arrival time " << initial << "ms; " << CPUburst << " CPU burst:" << std::endl;
    //     else
    //         std::cout << "I/O-bound process " << name << ": arrival time " << initial << "ms; " << CPUburst << " CPU bursts:" << std::endl;
    //     for (int j=0; j<CPUburst; j++){
    //         if (j<CPUburst-1){
    //             int burstTime=ceil(next_exp(lambda, bound));
    //             int ioTime=8*ceil(next_exp(lambda, bound));
    //             numIOBoundCPUBurst++;
    //             numIOBoundIOBurst++;
    //             sumIOBoundIOBurst+=ioTime;
    //             sumIOBoundCPUBurst+=burstTime;
    //             std::cout << "==> CPU burst " << burstTime << "ms ==> I/O burst " << ioTime << "ms" << std::endl;
    //         }
    //         else{
    //             int burstTime=ceil(next_exp(lambda, bound));
    //             numIOBoundCPUBurst++;
    //             sumIOBoundCPUBurst+=burstTime;
    //             std::cout << "==> CPU burst " << burstTime << "ms" << std::endl;
    //         }
    //     }
    // }


    std::ofstream outfile;
    outfile.open ("simout.txt");
    if (!outfile) {
        std::cerr << "ERROR: <Could not open the file for writing>" << std::endl;
        return 1;
    }




    //Printing stats
    outfile << "-- number of processes: " << n << std::endl;
    outfile << "-- number of CPU-bound processes: " << ncpu << std::endl;
    outfile << "-- number of I/O-bound processes: " << n-ncpu << std::endl;
    if (numCPUBoundCPUBurst==0)
        outfile << "-- CPU-bound average CPU burst time: "  << std::fixed << std::setprecision(3) << "0.000" << " ms" << std::endl;
    else   
        outfile << "-- CPU-bound average CPU burst time: "  << std::fixed << std::setprecision(3) << ceil(1000*sumCPUBoundCPUBurst/numCPUBoundCPUBurst)/1000 << " ms" << std::endl;
    if (numIOBoundCPUBurst==0)
        outfile << "-- I/O-bound average CPU burst time: " << "0.000" << " ms" << std::endl;
    else
        outfile << "-- I/O-bound average CPU burst time: " << ceil(1000*sumIOBoundCPUBurst/numIOBoundCPUBurst)/1000 << " ms" << std::endl;
    if (numCPUBoundCPUBurst+numIOBoundCPUBurst==0)
        outfile << "-- overall average CPU burst time: " << "0.000" << " ms" << std::endl;
    else
        outfile << "-- overall average CPU burst time: " << ceil(1000*(sumCPUBoundCPUBurst+sumIOBoundCPUBurst)/(numCPUBoundCPUBurst+numIOBoundCPUBurst))/1000 << " ms" << std::endl;
    if (numCPUBoundIOBurst==0)
        outfile << "-- CPU-bound average I/O burst time: " << "0.000" << " ms" << std::endl;
    else
        outfile << "-- CPU-bound average I/O burst time: " << ceil(1000*sumCPUBoundIOBurst/numCPUBoundIOBurst)/1000 << " ms" << std::endl;
    if (numIOBoundIOBurst==0)
        outfile << "-- I/O-bound average I/O burst time: " << "0.000" << " ms" << std::endl;
    else        
        outfile << "-- I/O-bound average I/O burst time: " << ceil(1000*sumIOBoundIOBurst/numIOBoundIOBurst)/1000 << " ms" << std::endl;
    if (numCPUBoundIOBurst+numIOBoundIOBurst==0)
        outfile << "-- overall average I/O burst time: " << "0.000" << " ms" << std::endl;
    else
        outfile << "-- overall average I/O burst time: " << ceil(1000*(sumCPUBoundIOBurst+sumIOBoundIOBurst)/(numCPUBoundIOBurst+numIOBoundIOBurst))/1000 << " ms" << std::endl;

    //part 2
    std::cout << "<<< PROJECT PART II\n";
    std::cout << "<<< -- t_cs=" << tcs << "ms; alpha=" << std::fixed << std::setprecision(2) << alpha << "; t_slice=" << tslice << "ms" << std::endl;
    simulate_fcfs1(processes, tcs, outfile);
    std::cout << std::endl;
    simulate_rr(processes, tcs, tslice, outfile);
    outfile.close();
}