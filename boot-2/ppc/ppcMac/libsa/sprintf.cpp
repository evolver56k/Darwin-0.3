# 1 "sprintf.c"
 




# 1 "libsa.h" 1
 
# 1 "../include/mach-o/loader.h" 1 3



 



 



# 1 "../include/mach/machine.h" 1 3
 
























 























































































 









# 1 "../include/mach/machine/vm_types.h" 1 3



 




# 1 "../include/architecture/ARCH_INCLUDE.h" 1 3
 































# 9 "../include/mach/machine/vm_types.h" 2 3





# 124 "../include/mach/machine.h" 2 3

# 1 "../include/mach/boolean.h" 1 3
 
























 































 









 




# 1 "../include/mach/machine/boolean.h" 1 3



 









# 73 "../include/mach/boolean.h" 2 3





 

















# 125 "../include/mach/machine.h" 2 3


 








struct machine_info {
	integer_t	major_version;	 
	integer_t	minor_version;	 
	integer_t	max_cpus;	 
	integer_t	avail_cpus;	 
	vm_size_t	memory_size;	 
};

typedef struct machine_info	*machine_info_t;
typedef struct machine_info	machine_info_data_t;	 

typedef integer_t	cpu_type_t;
typedef integer_t	cpu_subtype_t;







struct machine_slot {
 integer_t	is_cpu;		 
	cpu_type_t	cpu_type;	 
	cpu_subtype_t	cpu_subtype;	 
 integer_t	running;	 
	integer_t	cpu_ticks[3 ];
	integer_t	clock_freq;	 
};

typedef struct machine_slot	*machine_slot_t;
typedef struct machine_slot	machine_slot_data_t;	 








 


 



 
 
 
 


 
 


 



 

 





 














 


















 





















 










 












 





 








 






 




 






# 12 "../include/mach-o/loader.h" 2 3


 



# 1 "../include/mach/vm_prot.h" 1 3
 
























 


































 










 





typedef int		vm_prot_t;

 









 





 





 








# 18 "../include/mach-o/loader.h" 2 3


 



# 1 "../include/mach/machine/thread_status.h" 1 3



 









# 24 "../include/mach-o/loader.h" 2 3

# 1 "../include/architecture/byte_order.h" 1 3
 


















 
typedef unsigned long NXSwappedFloat;
typedef unsigned long long NXSwappedDouble;





 




enum NXByteOrder {
    NX_UnknownByteOrder,
    NX_LittleEndian,
    NX_BigEndian
};

static __inline__
enum NXByteOrder
NXHostByteOrder(void)
{
    unsigned int	_x;
    
    _x = (NX_BigEndian << 24) | NX_LittleEndian;
        
    return ((enum NXByteOrder)*((unsigned char *)&_x));
}

 





# 274 "../include/architecture/byte_order.h" 3


# 494 "../include/architecture/byte_order.h" 3



# 25 "../include/mach-o/loader.h" 2 3


 


struct mach_header {
	unsigned long	magic;		 
	cpu_type_t	cputype;	 
	cpu_subtype_t	cpusubtype;	 
	unsigned long	filetype;	 
	unsigned long	ncmds;		 
	unsigned long	sizeofcmds;	 
	unsigned long	flags;		 
};

 



 































 














 















struct load_command {
	unsigned long cmd;		 
	unsigned long cmdsize;		 
};

 
















				 

 







union lc_str {
	unsigned long	offset;	 
	char		*ptr;	 
};

 











struct segment_command {
	unsigned long	cmd;		 
	unsigned long	cmdsize;	 
	char		segname[16];	 
	unsigned long	vmaddr;		 
	unsigned long	vmsize;		 
	unsigned long	fileoff;	 
	unsigned long	filesize;	 
	vm_prot_t	maxprot;	 
	vm_prot_t	initprot;	 
	unsigned long	nsects;		 
	unsigned long	flags;		 
};

 










 
























struct section {
	char		sectname[16];	 
	char		segname[16];	 
	unsigned long	addr;		 
	unsigned long	size;		 
	unsigned long	offset;		 
	unsigned long	align;		 
	unsigned long	reloff;		 
	unsigned long	nreloc;		 
	unsigned long	flags;		 
	unsigned long	reserved1;	 
	unsigned long	reserved2;	 
};

 








 






					 
 




















 















 













 


					 
					 




					 

						 

					         
						 



					 

					 

					 












					 
					 
					 
					 
 





struct fvmlib {
	union lc_str	name;		 
	unsigned long	minor_version;	 
	unsigned long	header_addr;	 
};

 





struct fvmlib_command {
	unsigned long	cmd;		 
	unsigned long	cmdsize;	 
	struct fvmlib	fvmlib;		 
};

 








struct dylib {
    union lc_str  name;			 
    unsigned long timestamp;		 
    unsigned long current_version;	 
    unsigned long compatibility_version; 
};

 





struct dylib_command {
	unsigned long	cmd;		 
	unsigned long	cmdsize;	 
	struct dylib	dylib;		 
};

 








struct prebound_dylib_command {
	unsigned long	cmd;		 
	unsigned long	cmdsize;	 
	union lc_str	name;		 
	unsigned long	nmodules;	 
	union lc_str	linked_modules;	 
};

 





struct dylinker_command {
	unsigned long	cmd;		 
	unsigned long	cmdsize;	 
	union lc_str    name;		 
};

 




















struct thread_command {
	unsigned long	cmd;		 
	unsigned long	cmdsize;	 
	 
	 
	 
	 
};

 




struct symtab_command {
	unsigned long	cmd;		 
	unsigned long	cmdsize;	 
	unsigned long	symoff;		 
	unsigned long	nsyms;		 
	unsigned long	stroff;		 
	unsigned long	strsize;	 
};

 





































struct dysymtab_command {
    unsigned long cmd;		 
    unsigned long cmdsize;	 

     














    unsigned long ilocalsym;	 
    unsigned long nlocalsym;	 

    unsigned long iextdefsym;	 
    unsigned long nextdefsym;	 

    unsigned long iundefsym;	 
    unsigned long nundefsym;	 

     







    unsigned long tocoff;	 
    unsigned long ntoc;		 

     








    unsigned long modtaboff;	 
    unsigned long nmodtab;	 

     








    unsigned long extrefsymoff;   
    unsigned long nextrefsyms;	  

     









    unsigned long indirectsymoff;  
    unsigned long nindirectsyms;   

     
























    unsigned long extreloff;	 
    unsigned long nextrel;	 

     




    unsigned long locreloff;	 
    unsigned long nlocrel;	 

};	

 










 
struct dylib_table_of_contents {
    unsigned long symbol_index;	 

    unsigned long module_index;	 

};	

 
struct dylib_module {
    unsigned long module_name;	 

    unsigned long iextdefsym;	 
    unsigned long nextdefsym;	 
    unsigned long irefsym;		 
    unsigned long nrefsym;	 
    unsigned long ilocalsym;	 
    unsigned long nlocalsym;	 

    unsigned long iextrel;	 
    unsigned long nextrel;	 

    unsigned long iinit;	 
    unsigned long ninit;	 

    unsigned long		 
	objc_module_info_addr;   
    unsigned long		 
	objc_module_info_size;	 
};	

 







struct dylib_reference {
    unsigned long isym:24,	 
    		  flags:8;	 
};

 








struct symseg_command {
	unsigned long	cmd;		 
	unsigned long	cmdsize;	 
	unsigned long	offset;		 
	unsigned long	size;		 
};

 





struct ident_command {
	unsigned long cmd;		 
	unsigned long cmdsize;		 
};

 





struct fvmfile_command {
	unsigned long cmd;		 
	unsigned long cmdsize;		 
	union lc_str	name;		 
	unsigned long	header_addr;	 
};


# 2 "libsa.h" 2

# 1 "../include/mach/mach.h" 1 3
 





 














 







# 1 "../include/mach/mach_types.h" 1 3
 
























 















































































































 











# 1 "../include/mach/host_info.h" 1 3
 
























 





















































 











 


typedef integer_t	*host_info_t;		 


typedef integer_t	host_info_data_t[(1024) ];


typedef char	kernel_version_t[(512) ];
 







struct host_basic_info {
	integer_t	max_cpus;	 
	integer_t	avail_cpus;	 
	vm_size_t	memory_size;	 
	cpu_type_t	cpu_type;	 
	cpu_subtype_t	cpu_subtype;	 
};

typedef	struct host_basic_info	host_basic_info_data_t;
typedef struct host_basic_info	*host_basic_info_t;



struct host_sched_info {
	int		min_timeout;	 
	int		min_quantum;	 
};

typedef	struct host_sched_info	host_sched_info_data_t;
typedef struct host_sched_info	*host_sched_info_t;



struct host_load_info {
	integer_t	avenrun[3];	 
	integer_t	mach_factor[3];	 
};

typedef struct host_load_info	host_load_info_data_t;
typedef struct host_load_info	*host_load_info_t;




# 150 "../include/mach/mach_types.h" 2 3



# 1 "../include/mach/memory_object.h" 1 3
 
























 















































 









 




# 1 "../include/mach/port.h" 1 3
 
























 




















































 











# 1 "../include/mach/boolean.h" 1 3
 
























 































 






# 76 "../include/mach/boolean.h" 3


 

















# 91 "../include/mach/port.h" 2 3




typedef natural_t mach_port_t;
typedef mach_port_t *mach_port_array_t;

 














 









typedef natural_t mach_port_right_t;








typedef natural_t mach_port_type_t;
typedef mach_port_type_t *mach_port_type_array_t;









 












 





 

typedef natural_t mach_port_urefs_t;
typedef integer_t mach_port_delta_t;			 

 

typedef natural_t mach_port_seqno_t;		 
typedef unsigned int mach_port_mscount_t;	 
typedef unsigned int mach_port_msgcount_t;	 
typedef unsigned int mach_port_rights_t;	 

typedef struct mach_port_status {
	mach_port_t		mps_pset;	 
	mach_port_seqno_t	mps_seqno;	 
 natural_t mps_mscount;	 
 natural_t mps_qlimit;	 
 natural_t mps_msgcount;	 
 natural_t	mps_sorights;	 
 natural_t		mps_srights;	 
 natural_t		mps_pdrequest;	 
 natural_t		mps_nsrequest;	 
} mach_port_status_t;




 




typedef struct old_mach_port_status {
	mach_port_t		mps_pset;	 
 natural_t mps_mscount;	 
 natural_t mps_qlimit;	 
 natural_t mps_msgcount;	 
 natural_t	mps_sorights;	 
 natural_t		mps_srights;	 
 natural_t		mps_pdrequest;	 
 natural_t		mps_nsrequest;	 
} old_mach_port_status_t;


 

typedef integer_t	port_name_t;		 
typedef port_name_t	port_set_name_t;	 
typedef port_name_t	*port_name_array_t;

typedef integer_t	port_type_t;		 
typedef port_type_t	*port_type_array_t;

	 









typedef	port_name_t	port_t;			 
typedef	port_t		port_rcv_t;		 
typedef	port_t		port_own_t;		 
typedef	port_t		port_all_t;		 
typedef	port_t		*port_array_t;



# 1 "../include/mach/mach_param.h" 1 3
 
























 






























 













 





# 233 "../include/mach/port.h" 2 3



# 89 "../include/mach/memory_object.h" 2 3


typedef	mach_port_t	memory_object_t;
					 
					 
					 
					 

typedef	mach_port_t	memory_object_control_t;
					 
					 

typedef	mach_port_t	memory_object_name_t;
					 
					 

typedef	int		memory_object_copy_strategy_t;
					 

					 

					 

					 
					 

					 
					 
					 

typedef	int		memory_object_return_t;
					 


					 

					 

					 




# 153 "../include/mach/mach_types.h" 2 3


# 1 "../include/mach/processor_info.h" 1 3
 
























 







































 












 


typedef integer_t	*processor_info_t;	 


typedef integer_t	processor_info_data_t[(1024) ];


typedef integer_t	*processor_set_info_t;	 


typedef integer_t	processor_set_info_data_t[(1024) ];

 




struct processor_basic_info {
	cpu_type_t	cpu_type;	 
	cpu_subtype_t	cpu_subtype;	 
 integer_t	running;	 
	integer_t	slot_num;	 
 integer_t	is_master;	 
};

typedef	struct processor_basic_info	processor_basic_info_data_t;
typedef struct processor_basic_info	*processor_basic_info_t;






struct processor_set_basic_info {
	int		processor_count;	 
	int		task_count;		 
	int		thread_count;		 
	int		load_average;		 
	int		mach_factor;		 
};

 




typedef	struct processor_set_basic_info	processor_set_basic_info_data_t;
typedef struct processor_set_basic_info	*processor_set_basic_info_t;





struct processor_set_sched_info {
	int		policies;	 
	int		max_priority;	 
};

typedef	struct processor_set_sched_info	processor_set_sched_info_data_t;
typedef struct processor_set_sched_info	*processor_set_sched_info_t;




# 155 "../include/mach/mach_types.h" 2 3

# 1 "../include/mach/task_info.h" 1 3
 
























 

































 












# 1 "../include/mach/time_value.h" 1 3
 
























 















































 



struct time_value {
	integer_t	seconds;
	integer_t	microseconds;
};
typedef	struct time_value	time_value_t;

 






















 








typedef struct mapped_time_value {
	integer_t seconds;
	integer_t microseconds;
	integer_t check_seconds;
} mapped_time_value_t;


# 73 "../include/mach/task_info.h" 2 3


 


typedef	integer_t	*task_info_t;		 


typedef	integer_t	task_info_data_t[(1024) ];

 




struct task_basic_info {
	integer_t	suspend_count;	 
	integer_t	base_priority;	 
	vm_size_t	virtual_size;	 
	vm_size_t	resident_size;	 
	time_value_t	user_time;	 

	time_value_t	system_time;	 

};

typedef struct task_basic_info		task_basic_info_data_t;
typedef struct task_basic_info		*task_basic_info_t;






struct task_events_info {
	natural_t		faults;		 
	natural_t		zero_fills;	 
	natural_t		reactivations;	 
	natural_t		pageins;	 
	natural_t		cow_faults;	 
	natural_t		messages_sent;	 
	natural_t		messages_received;  
};
typedef struct task_events_info		task_events_info_data_t;
typedef struct task_events_info		*task_events_info_t;






struct task_thread_times_info {
	time_value_t	user_time;	 

	time_value_t	system_time;	 

};

typedef struct task_thread_times_info	task_thread_times_info_data_t;
typedef struct task_thread_times_info	*task_thread_times_info_t;



 









# 156 "../include/mach/mach_types.h" 2 3

# 1 "../include/mach/task_special_ports.h" 1 3
 
























 






























 

















 






















 











# 157 "../include/mach/mach_types.h" 2 3

# 1 "../include/mach/thread_info.h" 1 3
 
























 




































 














# 1 "../include/mach/boolean.h" 1 3
 
























 































 






# 76 "../include/mach/boolean.h" 3


 

















# 78 "../include/mach/thread_info.h" 2 3

# 1 "../include/mach/policy.h" 1 3
 
























 


































 





 










# 79 "../include/mach/thread_info.h" 2 3



 


typedef	integer_t	*thread_info_t;		 


typedef	integer_t	thread_info_data_t[(1024) ];

 




struct thread_basic_info {
	time_value_t	user_time;	 
	time_value_t	system_time;	 
	integer_t	cpu_usage;	 
	integer_t	base_priority;	 
	integer_t	cur_priority;	 
	integer_t	run_state;	 
	integer_t	flags;		 
	integer_t	suspend_count;	 
	integer_t	sleep_time;	 

};

typedef struct thread_basic_info	thread_basic_info_data_t;
typedef struct thread_basic_info	*thread_basic_info_t;



 





 











 







struct thread_sched_info {
	integer_t	policy;		 
	integer_t	data;		 
	integer_t	base_priority;	 
	integer_t	max_priority;    
	integer_t	cur_priority;	 
 integer_t	depressed;	 
	integer_t	depress_priority;  
};

typedef struct thread_sched_info	thread_sched_info_data_t;
typedef struct thread_sched_info	*thread_sched_info_t;




# 158 "../include/mach/mach_types.h" 2 3

# 1 "../include/mach/thread_special_ports.h" 1 3
 
























 






























 
















 
















 











# 159 "../include/mach/mach_types.h" 2 3

# 1 "../include/mach/thread_status.h" 1 3
 
























 



































 










 






 



typedef	natural_t		*thread_state_t;	 


typedef	natural_t	thread_state_data_t[(1024) ];




struct thread_state_flavor {
	int	flavor;			 
	int	count;			 
};


# 160 "../include/mach/mach_types.h" 2 3


# 1 "../include/mach/vm_attributes.h" 1 3
 
























 



















 














 


typedef unsigned int	vm_machine_attribute_t;





 


typedef int		vm_machine_attribute_val_t;










# 162 "../include/mach/mach_types.h" 2 3

# 1 "../include/mach/vm_inherit.h" 1 3
 
























 



























 










 





typedef int		vm_inherit_t;	 

 










# 163 "../include/mach/mach_types.h" 2 3


# 1 "../include/mach/vm_statistics.h" 1 3
 
























 










































 












struct vm_statistics {
	integer_t	pagesize;		 
	integer_t	free_count;		 
	integer_t	active_count;		 
	integer_t	inactive_count;		 
	integer_t	wire_count;		 
	integer_t	zero_fill_count;	 
	integer_t	reactivations;		 
	integer_t	pageins;		 
	integer_t	pageouts;		 
	integer_t	faults;			 
	integer_t	cow_faults;		 
	integer_t	lookups;		 
	integer_t	hits;			 
};

typedef struct vm_statistics	*vm_statistics_t;
typedef struct vm_statistics	vm_statistics_data_t;





 






struct pmap_statistics {
	integer_t		resident_count;	 
	integer_t		wired_count;	 
};

typedef struct pmap_statistics	*pmap_statistics_t;

# 165 "../include/mach/mach_types.h" 2 3


typedef	mach_port_t	task_t;
typedef task_t		*task_array_t;
typedef	task_t		vm_task_t;
typedef task_t		ipc_space_t;
typedef	mach_port_t	thread_t;
typedef	thread_t	*thread_array_t;
typedef mach_port_t	host_t;
typedef mach_port_t	host_priv_t;
typedef mach_port_t	processor_t;
typedef mach_port_t	*processor_array_t;
typedef mach_port_t	processor_set_t;
typedef mach_port_t	processor_set_name_t;
typedef mach_port_t	*processor_set_array_t;
typedef mach_port_t	*processor_set_name_array_t;
 








 



# 1 "../include/mach/std_types.h" 1 3
 
























 


































 









# 1 "../include/mach/boolean.h" 1 3
 
























 































 






# 76 "../include/mach/boolean.h" 3


 

















# 71 "../include/mach/std_types.h" 2 3

# 1 "../include/mach/kern_return.h" 1 3
 
























 


















































 











# 1 "../include/mach/machine/kern_return.h" 1 3



 









# 89 "../include/mach/kern_return.h" 2 3





		 



		 




		 





		 




		 



		 




		 




		 



		 





		 









		 



		 



		 




		 



		 



		 



		 



		 



		 



		 




		 



		 





# 72 "../include/mach/std_types.h" 2 3




typedef	vm_offset_t	pointer_t;
typedef	vm_offset_t	vm_address_t;



# 194 "../include/mach/mach_types.h" 2 3


typedef	unsigned int	vm_region_t;
typedef	vm_region_t	*vm_region_array_t;

typedef	char		vm_page_data_t[4096];


# 30 "../include/mach/mach.h" 2 3

# 1 "../include/mach/thread_switch.h" 1 3
 
























 



































 










# 31 "../include/mach/mach.h" 2 3

# 1 "../include/mach/mach_interface.h" 1 3



 



# 1 "../include/mach/message.h" 1 3
 
























 









































 












 






typedef natural_t mach_msg_timeout_t;

 






 
















































 

























typedef unsigned int mach_msg_bits_t;
typedef	unsigned int mach_msg_size_t;
typedef natural_t mach_msg_seqno_t;
typedef integer_t mach_msg_id_t;

typedef	struct {
    mach_msg_bits_t	msgh_bits;
    mach_msg_size_t	msgh_size;
    mach_port_t		msgh_remote_port;
    mach_port_t		msgh_local_port;
    mach_port_seqno_t	msgh_seqno;
    mach_msg_id_t	msgh_id;
} mach_msg_header_t;

 





 












 
























typedef unsigned int mach_msg_type_name_t;
typedef unsigned int mach_msg_type_size_t;
typedef natural_t  mach_msg_type_number_t;

typedef struct  {
    unsigned int	msgt_name : 8,
			msgt_size : 8,
			msgt_number : 12,
			msgt_inline : 1,
			msgt_longform : 1,
			msgt_deallocate : 1,
			msgt_unused : 1;
} mach_msg_type_t;

typedef	struct	{
    mach_msg_type_t	msgtl_header;
    unsigned short	msgtl_name;
    unsigned short	msgtl_size;
    natural_t		msgtl_number;
} mach_msg_type_long_t;


 




















 










 















 






 















typedef integer_t mach_msg_option_t;



















 










typedef kern_return_t mach_msg_return_t;




		 

		 

		 

		 

		 


		 

		 

		 

		 

		 

		 

		 

		 

		 

		 

		 

		 

		 

		 

		 

		 


		 

		 

		 

		 

		 

		 

		 

		 

		 

		 

		 

		 


extern mach_msg_return_t
mach_msg_trap

   (mach_msg_header_t *msg,
    mach_msg_option_t option,
    mach_msg_size_t send_size,
    mach_msg_size_t rcv_size,
    mach_port_t rcv_name,
    mach_msg_timeout_t timeout,
    mach_port_t notify);
# 452 "../include/mach/message.h" 3


extern mach_msg_return_t
mach_msg

   (mach_msg_header_t *msg,
    mach_msg_option_t option,
    mach_msg_size_t send_size,
    mach_msg_size_t rcv_size,
    mach_port_t rcv_name,
    mach_msg_timeout_t timeout,
    mach_port_t notify);
# 478 "../include/mach/message.h" 3



 

 







typedef	unsigned int	msg_size_t;

typedef	struct {
		unsigned int	msg_unused : 24,
				msg_simple : 8;
		msg_size_t	msg_size;
		integer_t	msg_type;
		port_t		msg_local_port;
		port_t		msg_remote_port;
		integer_t	msg_id;
} msg_header_t;



 











 

















typedef struct  {
	unsigned int	msg_type_name : 8,		 
			msg_type_size : 8,		 
			msg_type_number : 12,		 
			msg_type_inline : 1,		 
			msg_type_longform : 1,		 
			msg_type_deallocate : 1,	 
			msg_type_unused : 1;
} msg_type_t;

typedef	struct	{
	msg_type_t	msg_type_header;
	unsigned short	msg_type_long_name;
	unsigned short	msg_type_long_size;
	natural_t	msg_type_long_number;
} msg_type_long_t;

 






















 












 






 



typedef natural_t	msg_timeout_t;

 






typedef	integer_t		msg_option_t;



























 







typedef	int		msg_return_t;
















 
























 



msg_return_t	msg_send(

	msg_header_t	*header,
	msg_option_t	option,
	msg_timeout_t	timeout);
# 705 "../include/mach/message.h" 3


msg_return_t	msg_receive(

	msg_header_t	*header,
	msg_option_t	option,
	msg_timeout_t	timeout);
# 722 "../include/mach/message.h" 3


msg_return_t	msg_rpc(

	msg_header_t	*header,	 
	msg_option_t	option,
	msg_size_t	rcv_size,
	msg_timeout_t	send_timeout,
	msg_timeout_t	rcv_timeout);
# 744 "../include/mach/message.h" 3


msg_return_t	msg_send_trap(

	msg_header_t	*header,
	msg_option_t	option,
	msg_size_t	send_size,
	msg_timeout_t	timeout);
# 763 "../include/mach/message.h" 3


msg_return_t	msg_receive_trap(

	msg_header_t	*header,
	msg_option_t	option,
	msg_size_t	rcv_size,
	port_name_t	rcv_name,
	msg_timeout_t	timeout);
# 784 "../include/mach/message.h" 3


msg_return_t	msg_rpc_trap(

	msg_header_t	*header,	 
	msg_option_t	option,
	msg_size_t	send_size,
	msg_size_t	rcv_size,
	msg_timeout_t	send_timeout,
	msg_timeout_t	rcv_timeout);
# 808 "../include/mach/message.h" 3



# 8 "../include/mach/mach_interface.h" 2 3









 
extern  kern_return_t task_create (
	task_t target_task,
	boolean_t inherit_memory,
	task_t *child_task);

 
extern  kern_return_t task_terminate (
	task_t target_task);

 
extern  kern_return_t task_threads (
	task_t target_task,
	thread_array_t *thread_list,
	unsigned int *thread_listCnt);

 
extern  kern_return_t thread_terminate (
	thread_t target_thread);

 
extern  kern_return_t vm_allocate (
	vm_task_t target_task,
	vm_address_t *address,
	vm_size_t size,
	boolean_t anywhere);

 
extern  kern_return_t vm_deallocate (
	vm_task_t target_task,
	vm_address_t address,
	vm_size_t size);

 
extern  kern_return_t vm_protect (
	vm_task_t target_task,
	vm_address_t address,
	vm_size_t size,
	boolean_t set_maximum,
	vm_prot_t new_protection);

 
extern  kern_return_t vm_inherit (
	vm_task_t target_task,
	vm_address_t address,
	vm_size_t size,
	vm_inherit_t new_inheritance);

 
extern  kern_return_t vm_read (
	vm_task_t target_task,
	vm_address_t address,
	vm_size_t size,
	pointer_t *data,
	unsigned int *dataCnt);

 
extern  kern_return_t vm_write (
	vm_task_t target_task,
	vm_address_t address,
	pointer_t data,
	unsigned int dataCnt);

 
extern  kern_return_t vm_copy (
	vm_task_t target_task,
	vm_address_t source_address,
	vm_size_t size,
	vm_address_t dest_address);

 
extern  kern_return_t vm_region (
	vm_task_t target_task,
	vm_address_t *address,
	vm_size_t *size,
	vm_prot_t *protection,
	vm_prot_t *max_protection,
	vm_inherit_t *inheritance,
	boolean_t *is_shared,
	memory_object_name_t *object_name,
	vm_offset_t *offset);

 
extern  kern_return_t vm_statistics (
	vm_task_t target_task,
	vm_statistics_data_t *vm_stats);

 
extern  kern_return_t task_by_unix_pid (
	task_t target_task,
	int process_id,
	task_t *result_task);

 
extern  kern_return_t unix_pid (
	task_t target_task,
	int *process_id);

 
extern  kern_return_t netipc_listen (
	port_t request_port,
	int src_addr,
	int dst_addr,
	int src_port,
	int dst_port,
	int protocol,
	port_t ipc_port);

 
extern  kern_return_t netipc_ignore (
	port_t request_port,
	port_t ipc_port);

 
extern  kern_return_t xxx_host_info (
	port_t target_task,
	machine_info_data_t *info);

 
extern  kern_return_t xxx_slot_info (
	task_t target_task,
	int slot,
	machine_slot_data_t *info);

 
extern  kern_return_t xxx_cpu_control (
	task_t target_task,
	int cpu,
	boolean_t running);

 
extern  kern_return_t task_suspend (
	task_t target_task);

 
extern  kern_return_t task_resume (
	task_t target_task);

 
extern  kern_return_t task_get_special_port (
	task_t task,
	int which_port,
	port_t *special_port);

 
extern  kern_return_t task_set_special_port (
	task_t task,
	int which_port,
	port_t special_port);

 
extern  kern_return_t task_info (
	task_t target_task,
	int flavor,
	task_info_t task_info_out,
	unsigned int *task_info_outCnt);

 
extern  kern_return_t thread_create (
	task_t parent_task,
	thread_t *child_thread);

 
extern  kern_return_t thread_suspend (
	thread_t target_thread);

 
extern  kern_return_t thread_resume (
	thread_t target_thread);

 
extern  kern_return_t thread_abort (
	thread_t target_thread);

 
extern  kern_return_t thread_get_state (
	thread_t target_thread,
	int flavor,
	thread_state_t old_state,
	unsigned int *old_stateCnt);

 
extern  kern_return_t thread_set_state (
	thread_t target_thread,
	int flavor,
	thread_state_t new_state,
	unsigned int new_stateCnt);

 
extern  kern_return_t thread_get_special_port (
	thread_t thread,
	int which_port,
	port_t *special_port);

 
extern  kern_return_t thread_set_special_port (
	thread_t thread,
	int which_port,
	port_t special_port);

 
extern  kern_return_t thread_info (
	thread_t target_thread,
	int flavor,
	thread_info_t thread_info_out,
	unsigned int *thread_info_outCnt);

 
extern  kern_return_t port_names (
	task_t task,
	port_name_array_t *port_names_p,
	unsigned int *port_names_pCnt,
	port_type_array_t *port_types,
	unsigned int *port_typesCnt);

 
extern  kern_return_t port_type (
	task_t task,
	port_name_t port_name,
	port_type_t *port_type_p);

 
extern  kern_return_t port_rename (
	task_t task,
	port_name_t old_name,
	port_name_t new_name);

 
extern  kern_return_t port_allocate (
	task_t task,
	port_name_t *port_name);

 
extern  kern_return_t port_deallocate (
	task_t task,
	port_name_t port_name);

 
extern  kern_return_t port_set_backlog (
	task_t task,
	port_name_t port_name,
	int backlog);

 
extern  kern_return_t port_status (
	task_t task,
	port_name_t port_name,
	port_set_name_t *enabled,
	int *num_msgs,
	int *backlog,
	boolean_t *ownership,
	boolean_t *receive_rights);

 
extern  kern_return_t port_set_allocate (
	task_t task,
	port_set_name_t *set_name);

 
extern  kern_return_t port_set_deallocate (
	task_t task,
	port_set_name_t set_name);

 
extern  kern_return_t port_set_add (
	task_t task,
	port_set_name_t set_name,
	port_name_t port_name);

 
extern  kern_return_t port_set_remove (
	task_t task,
	port_name_t port_name);

 
extern  kern_return_t port_set_status (
	task_t task,
	port_set_name_t set_name,
	port_name_array_t *members,
	unsigned int *membersCnt);

 
extern  kern_return_t port_insert_send (
	task_t task,
	port_t my_port,
	port_name_t his_name);

 
extern  kern_return_t port_extract_send (
	task_t task,
	port_name_t his_name,
	port_t *his_port);

 
extern  kern_return_t port_insert_receive (
	task_t task,
	port_all_t my_port,
	port_name_t his_name);

 
extern  kern_return_t port_extract_receive (
	task_t task,
	port_name_t his_name,
	port_all_t *his_port);

 
extern  kern_return_t port_set_backup (
	task_t task,
	port_name_t port_name,
	port_t backup,
	port_t *previous);

 
extern  kern_return_t vm_machine_attribute (
	vm_task_t target_task,
	vm_address_t address,
	vm_size_t size,
	vm_machine_attribute_t attribute,
	vm_machine_attribute_val_t *value);

 
extern  kern_return_t vm_synchronize (
	vm_task_t target_task,
	vm_address_t address,
	vm_size_t size);

 
extern  kern_return_t vm_set_policy (
	vm_task_t target_task,
	vm_address_t address,
	vm_size_t size,
	int policy);

 
extern  kern_return_t vm_deactivate (
	vm_task_t target_task,
	vm_address_t address,
	vm_size_t size,
	int when);


# 32 "../include/mach/mach.h" 2 3

# 1 "../include/mach/mach_host.h" 1 3



 












 
extern  kern_return_t host_processors (
	host_priv_t host_priv,
	processor_array_t *processor_list,
	unsigned int *processor_listCnt);

 
extern  kern_return_t host_info (
	host_t host,
	int flavor,
	host_info_t host_info_out,
	unsigned int *host_info_outCnt);

 
extern  kern_return_t processor_info (
	processor_t processor,
	int flavor,
	host_t *host,
	processor_info_t processor_info_out,
	unsigned int *processor_info_outCnt);

 
extern  kern_return_t processor_start (
	processor_t processor);

 
extern  kern_return_t processor_exit (
	processor_t processor);

 
extern  kern_return_t processor_control (
	processor_t processor,
	processor_info_t processor_cmd,
	unsigned int processor_cmdCnt);

 
extern  kern_return_t processor_set_default (
	host_t host,
	processor_set_name_t *default_set);

 
extern  kern_return_t processor_set_create (
	host_t host,
	processor_set_t *new_set,
	processor_set_name_t *new_name);

 
extern  kern_return_t processor_set_destroy (
	processor_set_t set);

 
extern  kern_return_t processor_set_info (
	processor_set_name_t set_name,
	int flavor,
	host_t *host,
	processor_set_info_t info_out,
	unsigned int *info_outCnt);

 
extern  kern_return_t processor_assign (
	processor_t processor,
	processor_set_t new_set,
	boolean_t wait);

 
extern  kern_return_t processor_get_assignment (
	processor_t processor,
	processor_set_name_t *assigned_set);

 
extern  kern_return_t thread_assign (
	thread_t thread,
	processor_set_t new_set);

 
extern  kern_return_t thread_assign_default (
	thread_t thread);

 
extern  kern_return_t thread_get_assignment (
	thread_t thread,
	processor_set_name_t *assigned_set);

 
extern  kern_return_t task_assign (
	task_t task,
	processor_set_t new_set,
	boolean_t assign_threads);

 
extern  kern_return_t task_assign_default (
	task_t task,
	boolean_t assign_threads);

 
extern  kern_return_t task_get_assignment (
	task_t task,
	processor_set_name_t *assigned_set);

 
extern  kern_return_t host_kernel_version (
	host_t host,
	kernel_version_t kernel_version);

 
extern  kern_return_t thread_priority (
	thread_t thread,
	int priority,
	boolean_t set_max);

 
extern  kern_return_t thread_max_priority (
	thread_t thread,
	processor_set_t processor_set,
	int max_priority);

 
extern  kern_return_t task_priority (
	task_t task,
	int priority,
	boolean_t change_threads);

 
extern  kern_return_t processor_set_max_priority (
	processor_set_t processor_set,
	int max_priority,
	boolean_t change_threads);

 
extern  kern_return_t thread_policy (
	thread_t thread,
	int policy,
	int data);

 
extern  kern_return_t processor_set_policy_enable (
	processor_set_t processor_set,
	int policy);

 
extern  kern_return_t processor_set_policy_disable (
	processor_set_t processor_set,
	int policy,
	boolean_t change_threads);

 
extern  kern_return_t processor_set_tasks (
	processor_set_t processor_set,
	task_array_t *task_list,
	unsigned int *task_listCnt);

 
extern  kern_return_t processor_set_threads (
	processor_set_t processor_set,
	thread_array_t *thread_list,
	unsigned int *thread_listCnt);

 
extern  kern_return_t host_processor_sets (
	host_t host,
	processor_set_name_array_t *processor_sets,
	unsigned int *processor_setsCnt);

 
extern  kern_return_t host_processor_set_priv (
	host_priv_t host_priv,
	processor_set_name_t set_name,
	processor_set_t *set);

 
extern  kern_return_t thread_depress_abort (
	thread_t thread);

 
extern  kern_return_t host_set_time (
	host_priv_t host_priv,
	time_value_t new_time);

 
extern  kern_return_t host_adjust_time (
	host_priv_t host_priv,
	time_value_t new_adjustment,
	time_value_t *old_adjustment);

 
extern  kern_return_t host_get_time (
	host_t host,
	time_value_t *current_time);


# 33 "../include/mach/mach.h" 2 3

# 1 "../include/mach/mach_init.h" 1 3
 





 





























 








 



extern	port_t	task_self_;


extern port_t		thread_reply(void);


extern port_t		task_notify(void);
extern port_t		thread_self(void);

extern	kern_return_t	init_process(void);
extern	boolean_t  	swtch_pri(int pri);
extern	boolean_t  	swtch(void);
extern	kern_return_t	thread_switch(int thread_name, int opt, int opt_time);

extern int		mach_swapon(char *filename, int flags, 
					long lowat, long hiwat);

extern host_t		host_self(void);
extern host_priv_t	host_priv_self(void);



extern void slot_name(cpu_type_t cpu_type, cpu_subtype_t cpu_subtype, 
	char **cpu_name, char **cpu_subname);
 





extern	port_t	bootstrap_port;
extern	port_t	name_server_port;

 



extern	vm_size_t	vm_page_size;





# 34 "../include/mach/mach.h" 2 3



# 3 "libsa.h" 2





extern char *bcopy(char *src, char *dst, int n);

extern void *bsearch(const void *key, const void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *));

extern int bzero(char *b, int length);

extern void *memset(void *s, int c, size_t n);

 






extern int errno;
extern struct segment_command *
  getsegbynamefromheader(struct mach_header *mhp, char *segname);
extern int ptol(char *str);

 






extern int slvprintf(char *buffer, int len, const char *fmt, va_list arg);
extern int sprintf(char *s, const char *format, ...);

extern char *strcat(char *s1, const char *s2);
extern int strcmp(const char *s1, const char *s2);
extern char *strcpy(char *s1, const char *s2);
char *strerror(int errnum);
extern int strncmp(const char *s1, const char *s2, size_t n);
extern char *strncpy(char *s1, const char *s2, size_t n);
extern long strtol(
	const char *nptr,
	char **endptr,
	register int base
);
extern unsigned long strtoul(
	const char *nptr,
	char **endptr,
	register int base
);
extern int atoi(const char *str);

 
extern port_t task_self_;
extern kern_return_t vm_allocate(
	vm_task_t target_task,
	vm_address_t *address,
	vm_size_t size,
	boolean_t anywhere
);
extern kern_return_t vm_deallocate(
	vm_task_t target_task,
	vm_address_t address,
	vm_size_t size
);
extern kern_return_t host_info(
	host_t host,
	int flavor,
	host_info_t host_info,
	unsigned int *host_info_count
);
extern vm_size_t vm_page_size;
extern host_t host_self(void);
extern int getpagesize(void);
extern char *mach_error_string(int errnum);

 
extern void malloc_init(char *start, int size, int nodes);
extern void *malloc(size_t size);
extern void free(void *start);
extern void *realloc(void *ptr, size_t size);

extern void prf(
		const char *fmt,
		va_list ap,
		void (*putfn_p)(),
		void *putfn_arg
);
extern int strncasecmp(const char *s1, const char *s2, size_t n);
# 6 "sprintf.c" 2


struct putc_info {
	char *str;
	char *last_str;
};

static void
sputc(
	int c,
	struct putc_info *pi
)
{
	if (pi->last_str)
		if (pi->str == pi->last_str) {
			*(pi->str) = '\0';
			return;
		}
	*(pi->str)++ = c;
}

 
int sprintf(char *str, const char *fmt, ...)
{
	va_list ap;
	struct putc_info pi;
	
	va_start(ap, fmt);	
	pi.str = str;
	pi.last_str = 0;
	prf(fmt, ap, sputc, &pi);
	*pi.str = '\0';
	va_end(ap);
	return 0;
}

 
int slvprintf(char *str, int len, const char *fmt, va_list ap )
{
	struct putc_info pi;
	pi.str = str;
	pi.last_str = str + len - 1;
	prf(fmt, ap, sputc, &pi);
	*pi.str = '\0';
	return (pi.str - str);
}
