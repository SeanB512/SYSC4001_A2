/**
 *
 * @file interrupts.cpp
 * @author Sasisekhar Govind
 *
 */

#include<interrupts.hpp>

std::tuple<std::string, std::string, int> simulate_trace(std::vector<std::string> trace_file, int time, std::vector<std::string> vectors, std::vector<int> delays, std::vector<external_file> external_files, PCB current, std::vector<PCB> wait_queue) {

    std::string trace;      //!< string to store single line of trace file
    std::string execution = "";  //!< string to accumulate the execution output
    std::string system_status = "";  //!< string to accumulate the system status output
    int current_time = time;

    //parse each line of the input trace file. 'for' loop to keep track of indices.
    for(size_t i = 0; i < trace_file.size(); i++) {
        auto trace = trace_file[i];

        auto [activity, duration_intr, program_name] = parse_trace(trace);

        if(activity == "CPU") { //As per Assignment 1
            execution += std::to_string(current_time) + ", " + std::to_string(duration_intr) + ", CPU Burst\n";
            current_time += duration_intr;
        } else if(activity == "SYSCALL") { //As per Assignment 1
            //1-4)
            auto [intr, time] = intr_boilerplate(current_time, duration_intr, 10, vectors);
            execution += intr;
            current_time = time;

            //5) run ISR (device driver setup)
            execution += std::to_string(current_time) + ", 40, SYSCALL: run the ISR (device driver for device " + std::to_string(duration_intr) + ")\n";
            current_time += 40;

            //6) device performing I/O operation
            execution += std::to_string(current_time) + ", " + std::to_string(delays[duration_intr] - 40) + ", device " + std::to_string(duration_intr) + " performing I/O operation and transferring device data to memory (device busy)\n";
            current_time += delays[duration_intr] - 40;

            //7) return from interrupt
            execution +=  std::to_string(current_time) + ", 1, IRET\n";
            current_time += 1;
            
        } else if(activity == "END_IO") {
            //1-4)
            auto [intr, time] = intr_boilerplate(current_time, duration_intr, 10, vectors);
            current_time = time;
            execution += intr;

            //5) run ISR (device for end of I/O)
            execution += std::to_string(current_time) + ", 40, ENDIO: run the ISR (device driver for device " + std::to_string(duration_intr) + ")\n";
            current_time += 40;

            //6) check device status or mark I/O complete (added this because shown in TA example)
            execution += std::to_string(current_time) + ", " + std::to_string(delays[duration_intr] - 40) + ", device " + std::to_string(duration_intr) + ", check device status and complete operation\n";
            current_time += delays[duration_intr] - 40;

            //7) return from interrupt
            execution +=  std::to_string(current_time) + ", 1, IRET\n";
            current_time += 1;
        } else if(activity == "FORK") {
            //1-4)
            auto [intr, time] = intr_boilerplate(current_time, 2, 10, vectors);
            execution += intr;
            current_time = time;

            ///////////////////////////////////////////////////////////////////////////////////////////
            //Add your FORK output here

            //5) clone PCB
            execution += std::to_string(current_time) + ", " + std::to_string(duration_intr) + ", clone the PCB";
            current_time += duration_intr;

            //6) call scheduler
            execution += std::to_string(current_time) + ", 0, scheduler called";

            //7) return from interrupt
            execution +=  std::to_string(current_time) + ", 1, IRET\n";
            current_time += 1;

            ///////////////////////////////////////////////////////////////////////////////////////////

            //The following loop helps you do 2 things:
            // * Collect the trace of the child (and only the child, skip parent)
            // * Get the index of where the parent is supposed to start executing from
            std::vector<std::string> child_trace;
            bool skip = true;
            bool exec_flag = false;
            int parent_index = 0;

            for(size_t j = i; j < trace_file.size(); j++) {
                auto [_activity, _duration, _pn] = parse_trace(trace_file[j]);
                if(skip && _activity == "IF_CHILD") {
                    skip = false;
                    continue;
                } else if(_activity == "IF_PARENT"){
                    skip = true;
                    parent_index = j;
                    if(exec_flag) {
                        break;
                    }
                } else if(skip && _activity == "ENDIF") {
                    skip = false;
                    continue;
                } else if(!skip && _activity == "EXEC") {
                    skip = true;
                    child_trace.push_back(trace_file[j]);
                    exec_flag = true;
                }

                if(!skip) {
                    child_trace.push_back(trace_file[j]);
                }
            }
            i = parent_index;

            ///////////////////////////////////////////////////////////////////////////////////////////
            //With the child's trace, run the child (HINT: think recursion)

            //allocate memory for fork here
            current = PCB(current.partition_number + 1, current.partition_number, "init", 1, -1);
            allocate_memory(&current);

            //recursion for child:
            auto [ex2, sys2, t2] = simulate_trace(child_trace, current_time, vectors, delays, external_files, current, wait_queue);
            execution += ex2;
            system_status += sys2;
            current_time += t2;

            ///////////////////////////////////////////////////////////////////////////////////////////


        } else if(activity == "EXEC") {
            auto [intr, time] = intr_boilerplate(current_time, 3, 10, vectors);
            current_time = time;
            execution += intr;

            ///////////////////////////////////////////////////////////////////////////////////////////
            //Add your EXEC output here

            //find specific process in external files
            int file_length;
            for(external_file program: external_files){
                if (program.program_name == program_name){
                    file_length = program.size;
                }
            }

            //5) determine program length
            execution += std::to_string(current_time) + ", " + std::to_string(duration_intr) + ", Program is " + std::to_string(file_length) + " Mb large\n";
            current_time += duration_intr;

            //6) load program to memory
            execution += std::to_string(current_time) + ", " + std::to_string(file_length * 15) + ", loading program into memory\n";
            current_time += file_length * 15;

            //7) mark parition
            execution += std::to_string(current_time) + ", 3, marking parition as occupied\n";
            current_time += 3;

            //8) update PCB
            execution += std::to_string(current_time) + ", 6, updating PCB\n";
            current_time += 6;
    
            //9) call scheduler
            execution += std::to_string(current_time) + ", 0, scheduler called";

            //10) return from interrupt
            execution +=  std::to_string(current_time) + ", 1, IRET\n";
            current_time += 1;

            ///////////////////////////////////////////////////////////////////////////////////////////


            std::ifstream exec_trace_file(program_name + ".txt");

            std::vector<std::string> exec_traces;
            std::string exec_trace;
            while(std::getline(exec_trace_file, exec_trace)) {
                exec_traces.push_back(exec_trace);
            }

            ///////////////////////////////////////////////////////////////////////////////////////////
            //With the exec's trace (i.e. trace of external program), run the exec (HINT: think recursion)



            ///////////////////////////////////////////////////////////////////////////////////////////

            break; //Why is this important? (answer in report)

        }
    }

    return {execution, system_status, current_time};
}

int main(int argc, char** argv) {

    //vectors is a C++ std::vector of strings that contain the address of the ISR
    //delays  is a C++ std::vector of ints that contain the delays of each device
    //the index of these elemens is the device number, starting from 0
    //external_files is a C++ std::vector of the struct 'external_file'. Check the struct in 
    //interrupt.hpp to know more.
    auto [vectors, delays, external_files] = parse_args(argc, argv);
    std::ifstream input_file(argv[1]);

    //Just a sanity check to know what files you have
    print_external_files(external_files);

    //Make initial PCB (notice how partition is not assigned yet)
    PCB current(0, -1, "init", 1, -1);
    //Update memory (partition is assigned here, you must implement this function)
    if(!allocate_memory(&current)) {
        std::cerr << "ERROR! Memory allocation failed!" << std::endl;
    }

    std::vector<PCB> wait_queue;

    /******************ADD YOUR VARIABLES HERE*************************/


    /******************************************************************/

    //Converting the trace file into a vector of strings.
    std::vector<std::string> trace_file;
    std::string trace;
    while(std::getline(input_file, trace)) {
        trace_file.push_back(trace);
    }

    auto [execution, system_status, _] = simulate_trace(   trace_file, 
                                            0, 
                                            vectors, 
                                            delays,
                                            external_files, 
                                            current, 
                                            wait_queue);

    input_file.close();

    write_output(execution, "execution.txt");
    write_output(system_status, "system_status.txt");

    return 0;
}
