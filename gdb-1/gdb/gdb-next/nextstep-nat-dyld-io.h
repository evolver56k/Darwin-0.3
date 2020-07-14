struct objfile *symbol_file_add_bfd_safe
PARAMS ((bfd *abfd, int from_tty, CORE_ADDR addr, 
	 int addrisoffset, int mainline, int mapped, CORE_ADDR mapaddr, int readnow, int user_loaded, int is_solib, const char *prefix));

bfd *symfile_bfd_open_safe
PARAMS ((const char *filename));

bfd *dyld_map_image
PARAMS ((CORE_ADDR addr, CORE_ADDR offset));
